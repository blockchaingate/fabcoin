// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "amount.h"
#include "chain.h"
#include "chainparams.h"
#include "coins.h"
#include "./wallet/wallet.h"
#include "consensus/consensus.h"
#include "consensus/tx_verify.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "crypto/equihash.h"
#include "hash.h"
#include "validation.h"
#include "net.h"
#include "policy/feerate.h"
#include "policy/policy.h"
#include "pow.h"
#include "primitives/transaction.h"
#include "script/standard.h"
#include "timedata.h"
#include "txmempool.h"
#include "util.h"
#include "utilmoneystr.h"
#include "validationinterface.h"

#include <algorithm>
#include <queue>
#include <utility>

#ifdef ENABLE_GPU
#include "libgpusolver/libgpusolver.h"
#endif

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

//////////////////////////////////////////////////////////////////////////////
//
// FabcoinMiner
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest fee rate of a transaction combined with all
// its ancestors.

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockWeight = 0;

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks)
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams);

    return nNewTime - nOldTime;
}

BlockAssembler::Options::Options() {
    blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
}

BlockAssembler::BlockAssembler(const CChainParams& params, const Options& options) : chainparams(params)
{
    blockMinFeeRate = options.blockMinFeeRate;
    // Limit weight to between 4K and MAX_BLOCK_WEIGHT-4K for sanity:
    nBlockMaxWeight = std::max<size_t>(4000, std::min<size_t>(MAX_BLOCK_WEIGHT - 4000, options.nBlockMaxWeight));
}

static BlockAssembler::Options DefaultOptions(const CChainParams& params)
{
    // Block resource limits
    // If neither -blockmaxsize or -blockmaxweight is given, limit to DEFAULT_BLOCK_MAX_*
    // If only one is given, only restrict the specified resource.
    // If both are given, restrict both.
    BlockAssembler::Options options;
    options.nBlockMaxWeight = gArgs.GetArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    if (gArgs.IsArgSet("-blockmintxfee")) {
        CAmount n = 0;
        ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n);
        options.blockMinFeeRate = CFeeRate(n);
    } else {
        options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    }
    return options;
}

BlockAssembler::BlockAssembler(const CChainParams& params) : BlockAssembler(params, DefaultOptions(params)) {}

void BlockAssembler::resetBlock()
{
    inBlock.clear();

    // Reserve space for coinbase tx
    nBlockWeight = 4000;
    nBlockSigOpsCost = 400;
    fIncludeWitness = false;

    // These counters do not include coinbase tx
    nBlockTx = 0;
    nFees = 0;
}

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn, bool fMineWitnessTx)
{
    int64_t nTimeStart = GetTimeMicros();

    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if(!pblocktemplate.get())
        return nullptr;
    pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

    LOCK2(cs_main, mempool.cs);
    CBlockIndex* pindexPrev = chainActive.Tip();
    nHeight = pindexPrev->nHeight + 1;

    pblock->nVersion = ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        pblock->nVersion = gArgs.GetArg("-blockversion", pblock->nVersion);

    pblock->nTime = GetAdjustedTime();
    const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                       ? nMedianTimePast
                       : pblock->GetBlockTime();

    // Decide whether to include witness transactions
    // This is only needed in case the witness softfork activation is reverted
    // (which would require a very deep reorganization) or when
    // -promiscuousmempoolflags is used.
    // TODO: replace this with a call to main to assess validity of a mempool
    // transaction (which in most cases can be a no-op).
    fIncludeWitness = IsWitnessEnabled(pindexPrev, chainparams.GetConsensus()) && fMineWitnessTx;

    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;
    addPackageTxs(nPackagesSelected, nDescendantsUpdated);

    int64_t nTime1 = GetTimeMicros();

    nLastBlockTx = nBlockTx;
    nLastBlockWeight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
    coinbaseTx.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));
    pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus());
    pblocktemplate->vTxFees[0] = -nFees;

    const Consensus::Params& params = chainparams.GetConsensus();
    int ser_flags = (nHeight < params.FABHeight) ? SERIALIZE_BLOCK_LEGACY : 0;
    uint64_t nSerializeSize = GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION | ser_flags);
    LogPrintf("CreateNewBlock(): total size: %u block weight: %u txs: %u fees: %ld sigops %d\n",
              nSerializeSize, GetBlockWeight(*pblock, chainparams.GetConsensus()), nBlockTx, nFees, nBlockSigOpsCost);

    arith_uint256 nonce;
    if (nHeight >= params.FABHeight) {
        // Randomise nonce for new block foramt.
        nonce = UintToArith256(GetRandHash());
        // Clear the top and bottom 16 bits (for local use as thread flags and counters)
        nonce >>= 128;
        nonce <<= 32;
    }

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    pblock->nHeight        = pindexPrev->nHeight + 1;
    memset(pblock->nReserved, 0, sizeof(pblock->nReserved));
    UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus());
    pblock->nNonce         = ArithToUint256(nonce);
    pblock->nSolution.clear();
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);

    CValidationState state;
    if (!TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
        throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
    }
    int64_t nTime2 = GetTimeMicros();

    LogPrint(BCLog::BENCH, "CreateNewBlock() packages: %.2fms (%d packages, %d updated descendants), validity: %.2fms (total %.2fms)\n", 0.001 * (nTime1 - nTimeStart), nPackagesSelected, nDescendantsUpdated, 0.001 * (nTime2 - nTime1), 0.001 * (nTime2 - nTimeStart));

    return std::move(pblocktemplate);
}

void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        // Only test txs not already in the block
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        }
        else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOpsCost)
{
    // TODO: switch to weight-based accounting for packages instead of vsize-based accounting.
    if (nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= nBlockMaxWeight)
        return false;
    if (nBlockSigOpsCost + packageSigOpsCost >= MAX_BLOCK_SIGOPS_COST)
        return false;
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
// - premature witness (in case segwit transactions are added to mempool before
//   segwit activation)
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package)
{
    for (const CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, nLockTimeCutoff))
            return false;
        if (!fIncludeWitness && it->GetTx().HasWitness())
            return false;
    }
    return true;
}

void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    pblock->vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    bool fPrintPriority = gArgs.GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        LogPrintf("fee %s txid %s\n",
                  CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
                  iter->GetTx().GetHash().ToString());
    }
}

int BlockAssembler::UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,
        indexed_modified_transaction_set &mapModifiedTx)
{
    int nDescendantsUpdated = 0;
    for (const CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
        for (CTxMemPool::txiter desc : descendants) {
            if (alreadyAdded.count(desc))
                continue;
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.nModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCostWithAncestors -= it->GetSigOpCost();
                mapModifiedTx.insert(modEntry);
            } else {
                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
    return nDescendantsUpdated;
}

// Skip entries in mapTx that are already in a block or are present
// in mapModifiedTx (which implies that the mapTx ancestor state is
// stale due to ancestor inclusion in the block)
// Also skip transactions that we've already failed to add. This can happen if
// we consider a transaction in mapModifiedTx and it fails: we can then
// potentially consider it again while walking mapTx.  It's currently
// guaranteed to fail again, but as a belt-and-suspenders check we put it in
// failedTx and avoid re-evaluation, since the re-evaluation would be using
// cached size/sigops/fee values that are not actually correct.
bool BlockAssembler::SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx)
{
    assert (it != mempool.mapTx.end());
    return mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it);
}

void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, CTxMemPool::txiter entry, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    // Sort package by ancestor count
    // If a transaction A depends on transaction B, then A's ancestor count
    // must be greater than B's.  So this is sufficient to validly order the
    // transactions for block inclusion.
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}

// This transaction selection algorithm orders the mempool based
// on feerate of a transaction including all unconfirmed ancestors.
// Since we don't remove transactions from the mempool as we select them
// for block inclusion, we need an alternate method of updating the feerate
// of a transaction with its not-yet-selected ancestors as we go.
// This is accomplished by walking the in-mempool descendants of selected
// transactions and storing a temporary modified state in mapModifiedTxs.
// Each time through the loop, we compare the best transaction in
// mapModifiedTxs with the next transaction in the mempool to decide what
// transaction package to work on next.
void BlockAssembler::addPackageTxs(int &nPackagesSelected, int &nDescendantsUpdated)
{
    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
    indexed_modified_transaction_set mapModifiedTx;
    // Keep track of entries that failed inclusion, to avoid duplicate work
    CTxMemPool::setEntries failedTx;

    // Start by adding all descendants of previously added txs to mapModifiedTx
    // and modifying them for their already included ancestors
    UpdatePackagesForAdded(inBlock, mapModifiedTx);

    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;

    // Limit the number of attempts to add transactions to the block when it is
    // close to full; this is just a simple heuristic to finish quickly if the
    // mempool has a lot of entries.
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    int64_t nConsecutiveFailed = 0;

    while (mi != mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty())
    {
        // First try to find a new transaction in mapTx to evaluate.
        if (mi != mempool.mapTx.get<ancestor_score>().end() &&
                SkipMapTxEntry(mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx)) {
            ++mi;
            continue;
        }

        // Now that mi is not stale, determine which transaction to evaluate:
        // the next entry from mapTx, or the best from mapModifiedTx?
        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();
        if (mi == mempool.mapTx.get<ancestor_score>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                    CompareModifiedEntry()(*modit, CTxMemPoolModifiedEntry(iter))) {
                // The best entry in mapModifiedTx has higher score
                // than the one from mapTx.
                // Switch which transaction (package) to consider
                iter = modit->iter;
                fUsingModified = true;
            } else {
                // Either no entry in mapModifiedTx, or it's worse than mapTx.
                // Increment mi for the next loop iteration.
                ++mi;
            }
        }

        // We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
        // contain anything that is inBlock.
        assert(!inBlock.count(iter));

        uint64_t packageSize = iter->GetSizeWithAncestors();
        CAmount packageFees = iter->GetModFeesWithAncestors();
        int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
        if (fUsingModified) {
            packageSize = modit->nSizeWithAncestors;
            packageFees = modit->nModFeesWithAncestors;
            packageSigOpsCost = modit->nSigOpCostWithAncestors;
        }

        if (packageFees < blockMinFeeRate.GetFee(packageSize)) {
            // Everything else we might consider has a lower fee rate
            return;
        }

        if (!TestPackage(packageSize, packageSigOpsCost)) {
            if (fUsingModified) {
                // Since we always look at the best entry in mapModifiedTx,
                // we must erase failed entries so that we can consider the
                // next best entry on the next loop iteration
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }

            ++nConsecutiveFailed;

            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight >
                    nBlockMaxWeight - 4000) {
                // Give up if we're close to full and haven't succeeded in a while
                break;
            }
            continue;
        }

        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

        onlyUnconfirmed(ancestors);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!TestPackageTransactions(ancestors)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }

        // This transaction will make it in; reset the failed counter.
        nConsecutiveFailed = 0;

        // Package can be added. Sort the entries in a valid order.
        std::vector<CTxMemPool::txiter> sortedEntries;
        SortForBlock(ancestors, iter, sortedEntries);

        for (size_t i=0; i<sortedEntries.size(); ++i) {
            AddToBlock(sortedEntries[i]);
            // Erase from the modified set, if present
            mapModifiedTx.erase(sortedEntries[i]);
        }

        ++nPackagesSelected;

        // Update transactions that depend on each of these
        nDescendantsUpdated += UpdatePackagesForAdded(ancestors, mapModifiedTx);
    }
}

void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

#ifdef ENABLE_WALLET
//////////////////////////////////////////////////////////////////////////////
//
// Internal miner
//
#include "net.h"
//
// ScanHash scans nonces looking for a hash with at least some zero bits.
// The nonce is usually preserved between calls, but periodically or if the
// nonce is 0xffff0000 or above, the block is rebuilt and nNonce starts over at
// zero.
//

static bool ProcessBlockFound(const CBlock* pblock, const CChainParams& chainparams)
{
    LogPrintf("%s\n", pblock->ToString());
    LogPrintf("generated %s\n", FormatMoney(pblock->vtx[0]->vout[0].nValue));

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("FabcoinMiner: generated block is stale");
    }

    // Inform about the new block
    GetMainSignals().BlockFound(pblock->GetHash());

    // Process this block the same as if we had received it from another node
    bool fNewBlock = false;
    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
    if (!ProcessNewBlock(chainparams, shared_pblock, true, &fNewBlock))
        return error("FabcoinMiner: ProcessNewBlock, block not accepted");

    return true;
}

void static FabcoinMiner(const CChainParams& chainparams, GPUConfig conf)
{
    static const int nInnerLoopCount = 0x10000;
    int nCounter = 0;

    if(conf.useGPU)
        LogPrintf("FabcoinMiner started on GPU device: %u\n", conf.currentDevice);
    else
        LogPrintf("FabcoinMiner started on CPU \n");

    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("fabcoin-miner");
  
    unsigned int nExtraNonce = 0;
    std::shared_ptr<CReserveScript> coinbaseScript;
    if( ::vpwallets.size() > 0 )
    {    
        GetMainSignals().ScriptForMining(coinbaseScript);
    }

    unsigned int n = chainparams.EquihashN();
    unsigned int k = chainparams.EquihashK();

    uint8_t * header = NULL;
#ifdef ENABLE_GPU
    GPUSolver * g_solver = NULL;
    if(conf.useGPU) 
    {
        g_solver = new GPUSolver(conf.currentPlatform, conf.currentDevice);
        LogPrint(BCLog::POW, "Using Equihash solver GPU with n = %u, k = %u\n", n, k);
        header = (uint8_t *) calloc(CBlockHeader::HEADER_SIZE, sizeof(uint8_t));
    }
#endif

    std::mutex m_cs;
    bool cancelSolver = false;
    //    boost::signals2::connection c = uiInterface.NotifyBlockTip.connect(
    //        [&m_cs, &cancelSolver](const uint256& hashNewTip) mutable {
    //            std::lock_guard<std::mutex> lock{m_cs};
    //            cancelSolver = true;
    //    }
    //    );

    try {
        // Throw an error if no script was provided.  This can happen
        // due to some internal error but also if the keypool is empty.
        // In the latter case, already the pointer is NULL.
        if (!coinbaseScript || coinbaseScript->reserveScript.empty())
            throw std::runtime_error("No coinbase script available (mining requires a wallet)");

        while (true) {
            if (chainparams.MiningRequiresPeers()) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                do {
                    unsigned int nNodeCount = g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL);
                    if ( nNodeCount && !IsInitialBlockDownload())
                        break;
                    MilliSleep(1000);
                } while (true);
            }

            //
            // Create new block
            //
            unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            CBlockIndex* pindexPrev = chainActive.Tip();

            std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateNewBlock(coinbaseScript->reserveScript));
            if (!pblocktemplate.get())
            {
                LogPrintf("Error in FabcoinMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread\n");
                return;
            }
            CBlock *pblock = &pblocktemplate->block;
            IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

            LogPrintf("Running FabcoinMiner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
                ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

            //
            // Search
            //
            int64_t nStart = GetTime();
            arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);
            uint256 hash;

            nCounter = 0;
            if (conf.useGPU)
               LogPrint(BCLog::POW, "Equihash solver in GPU (%u, %u) with nNonce = %s hashTarget=%s\n", conf.currentPlatform, conf.currentDevice, pblock->nNonce.ToString(), hashTarget.GetHex());
            else LogPrint(BCLog::POW, "Equihash solver in CPU with nNonce = %s hashTarget=%s\n", pblock->nNonce.ToString(), hashTarget.GetHex());
  

            while (true) 
            {
                // I = the block header minus nonce and solution.
                CEquihashInput I{*pblock};
                CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                ss << I;

                // Hash state
                crypto_generichash_blake2b_state state;
                if(conf.useGPU)
                {
#ifdef ENABLE_GPU
                    memcpy(header, &ss[0], ss.size());
#endif
                }
                else
                {
                    EhInitialiseState(n, k, state);
                    // H(I||...
                    crypto_generichash_blake2b_update(&state, (unsigned char*)&ss[0], ss.size());
                }

                // H(I||V||...
                crypto_generichash_blake2b_state curr_state;

                if(conf.useGPU)
                {
#ifdef ENABLE_GPU
                    for (size_t i = 0; i < FABCOIN_NONCE_LEN; ++i)
                        header[108 + i] = pblock->nNonce.begin()[i];
#endif
                }
                else
                {
                    curr_state = state;
                    crypto_generichash_blake2b_update(&curr_state,pblock->nNonce.begin(),pblock->nNonce.size());
                }                

                // (x_1, x_2, ...) = A(I, V, n, k)
                //LogPrint(BCLog::POW, "Running Equihash solver in %u %u %u with nNonce = %s\n", conf.currentPlatform, conf.currentDevice, pblock->nNonce.ToString());

                std::function<bool(std::vector<unsigned char>)> validBlock =
                    [&pblock, &hashTarget, &m_cs, &cancelSolver, &chainparams](std::vector<unsigned char> soln) 
                {
                    // Write the solution to the hash and compute the result.
                    //LogPrint(BCLog::POW, "- Checking solution against target\n");
                    pblock->nSolution = soln;

                    if (UintToArith256(pblock->GetHash()) > hashTarget) 
                    {
                        return false;
                    }

                    // Found a solution
                    SetThreadPriority(THREAD_PRIORITY_NORMAL);
                    LogPrintf("FabcoinMiner:\n");
                    LogPrintf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", pblock->GetHash().GetHex(), hashTarget.GetHex());
                    LogPrintf("Block ------------------ \n%s\n ----------------", pblock->ToString());

                    if (ProcessBlockFound(pblock, chainparams)) 
                    {
                        // Ignore chain updates caused by us
                        std::lock_guard<std::mutex> lock{m_cs};
                        cancelSolver = false;
                    }
                    SetThreadPriority(THREAD_PRIORITY_LOWEST);

                    // In regression test mode, stop mining after a block is found.
                    if (chainparams.MineBlocksOnDemand()) {
                        // Increment here because throwing skips the call below
                        throw boost::thread_interrupted();
                    }
                    return true;
                };
            
#ifdef ENABLE_GPU
                std::function<bool(GPUSolverCancelCheck)> cancelledGPU = [&m_cs, &cancelSolver](GPUSolverCancelCheck pos) {
                    std::lock_guard<std::mutex> lock{m_cs};
                    return cancelSolver;
                };
#endif
                std::function<bool(EhSolverCancelCheck)> cancelled = [&m_cs, &cancelSolver](EhSolverCancelCheck pos) {
                    std::lock_guard<std::mutex> lock{m_cs};
                    return cancelSolver;
                };

                try {
                    if(!conf.useGPU) 
                    {
                        // If we find a valid block, we rebuild
                        bool found = EhOptimisedSolve(n, k, curr_state, validBlock, cancelled);
                        if (found) {
                            break;
                        }
                    }
                    else 
                    {
#ifdef ENABLE_GPU
                        bool found = g_solver->run(n, k, header, CBlockHeader::HEADER_SIZE, pblock->nNonce, validBlock, cancelledGPU, curr_state);
                        if (found)
                            break;
#endif
                    }
                } catch (EhSolverCancelledException&) {
                    LogPrint(BCLog::POW, "Equihash solver cancelled\n");
                    std::lock_guard<std::mutex> lock{m_cs};
                    cancelSolver = false;
                }

                // Check for stop or if block needs to be rebuilt
                boost::this_thread::interruption_point();
                // Regtest mode doesn't require peers
                unsigned int nNodeCount = g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL);
                if (nNodeCount == 0 && chainparams.MiningRequiresPeers())
                    break;
                if ( nCounter == nInnerLoopCount )
                    break;
                if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                    break;
                if (pindexPrev != chainActive.Tip())
                    break;

                //LogPrint(BCLog::POW, "solver... nNonce = %s -> Hash = %s \n", pblock->nNonce.ToString(), pblock->GetHash().GetHex());
                // Update nNonce and nTime
                pblock->nNonce = ArithToUint256(UintToArith256(pblock->nNonce) + 1);
                ++nCounter;

                // Update nTime every few seconds
                if (UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev) < 0)
                    break; // Recreate the block if the clock has run backwards,
                // so that we can use the correct time.
                if (chainparams.GetConsensus().fPowAllowMinDifficultyBlocks)
                {
                    // Changing pblock->nTime can change work required on testnet:
                    hashTarget.SetCompact(pblock->nBits);
                }
            }
        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("FabcoinMiner terminated\n");
#ifdef ENABLE_GPU
        if(conf.useGPU)
            delete g_solver;
        free(header);
#endif
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("FabcoinMiner runtime error: %s\n", e.what());
#ifdef ENABLE_GPU
        if(conf.useGPU)
            delete g_solver;
        free(header);
#endif
        return;
    }

#ifdef ENABLE_GPU
    if(conf.useGPU)
        delete g_solver;
    free(header);
#endif

//    c.disconnect();
}


void GenerateFabcoins(bool fGenerate, int nThreads, const CChainParams& chainparams)
{
    static boost::thread_group* minerThreads = NULL;

    if (nThreads < 0) 
        nThreads = GetNumCores();
    
    if (minerThreads != NULL)
    {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (nThreads == 0 || !fGenerate)
        return;
}

void GenerateFabcoins(bool fGenerate, int nThreads, const CChainParams& chainparams, GPUConfig conf)
{
    static boost::thread_group* minerThreads = NULL;

    if (nThreads < 0)
        nThreads = GetNumCores();

    if (minerThreads != NULL)
    {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (nThreads == 0 || !fGenerate)
        return;

    minerThreads = new boost::thread_group();

    // If using GPU
    if(conf.useGPU) {
#ifdef ENABLE_GPU
        conf.currentPlatform = 0;
        conf.currentDevice = conf.selGPU;

        std::vector<cl::Platform> platforms = cl_gpuminer::getPlatforms();

        // use all available GPUs
        if(conf.allGPU) {

            int devicesFound = 0;
            unsigned numPlatforms = platforms.size();

            for(unsigned platform = 0; platform < numPlatforms; ++platform) {

                std::vector<cl::Device> devices = cl_gpuminer::getDevices(platforms, platform);
                unsigned noDevices = devices.size();
                devicesFound += noDevices;
                for(unsigned device = 0; device < noDevices; ++device) {

                    conf.currentPlatform = platform;
                    conf.currentDevice = device;

                    cl_ulong result;
                    devices[device].getInfo(CL_DEVICE_GLOBAL_MEM_SIZE, &result);

                    int maxThreads = nThreads;
                    if (!conf.forceGenProcLimit) {
                        if (result > 7500000000) {
                            maxThreads = std::min(4, nThreads);
                        } else if (result > 5500000000) {
                            maxThreads = std::min(3, nThreads);
                        } else if (result > 3500000000) {
                            maxThreads = std::min(2, nThreads);
                        } else {
                            maxThreads = std::min(1, nThreads);
                        }
                    }

                    for (int i = 0; i < maxThreads; i++){
                        LogPrintf("FabcoinMiner GPU platform=%d device=%d thread=%d!\n", conf.currentPlatform, conf.currentDevice, i);
                        minerThreads->create_thread(boost::bind(&FabcoinMiner, boost::cref(chainparams), conf));
                    }

                }
            }

            if (devicesFound <= 0) {
                LogPrintf("FabcoinMiner ERROR, No OpenCL devices found!\n");
            }

        } 
        else
        {

            // mine on specified GPU device
            std::vector<cl::Device> devices = cl_gpuminer::getDevices(platforms, conf.currentPlatform);

            if (devices.size() > conf.currentDevice) 
            {

                cl_ulong result;
                devices[conf.currentDevice].getInfo(CL_DEVICE_GLOBAL_MEM_SIZE, &result);

                int maxThreads = nThreads;
                if (!conf.forceGenProcLimit) {
                    if (result > 7500000000) {
                        maxThreads = std::min(4, nThreads);
                    } else if (result > 5500000000) {
                        maxThreads = std::min(3, nThreads);
                    } else if (result > 3500000000) {
                        maxThreads = std::min(2, nThreads);
                    } else {
                        maxThreads = std::min(1, nThreads);
                    }
                }

                for (int i = 0; i < maxThreads; i++) {
                    minerThreads->create_thread(boost::bind(&FabcoinMiner, boost::cref(chainparams), conf));
                }
            } 
            else 
            {
                LogPrintf("FabcoinMiner ERROR, No OpenCL devices found!\n");
            }
        }
#endif
    }
    else
    {
        for (int i = 0; i < nThreads; i++){
            LogPrintf("FabcoinMiner CPU, thread=%d!\n",  i);
            minerThreads->create_thread(boost::bind(&FabcoinMiner, boost::cref(chainparams), conf));    
        }
    }
}


void Scan_nNonce_nSolution(CBlock *pblock, unsigned int n, unsigned int k ) 
{
    bool cancelSolver = false;
    uint64_t nMaxTries = 0;

    //
    // Search
    //
    arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);
    uint256 hash;

    while (true) {
        nMaxTries++;
        // Hash state
        crypto_generichash_blake2b_state state;
        EhInitialiseState(n, k, state);

        // I = the block header minus nonce and solution.
        CEquihashInput I{*pblock};
        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
        ss << I;

        // H(I||...
        crypto_generichash_blake2b_update(&state, (unsigned char*) &ss[0], ss.size());

        // H(I||V||...
        crypto_generichash_blake2b_state curr_state;
        curr_state = state;
        crypto_generichash_blake2b_update(&curr_state, pblock->nNonce.begin(), pblock->nNonce.size());

        // (x_1, x_2, ...) = A(I, V, n, k)
        //  LogPrint(BCLog::POW, "Running Equihash solver \"%s\" with nNonce = %s\n",
        //      solver, pblock->nNonce.ToString());

        std::function<bool(std::vector<unsigned char>) > validBlock =
                [&pblock, &hashTarget, &cancelSolver ](std::vector<unsigned char> soln) {
                    // Write the solution to the hash and compute the result.
                    LogPrint(BCLog::POW, "- Checking solution against target\n");
                    pblock->nSolution = soln;

                    if (UintToArith256(pblock->GetHash()) > hashTarget) {
                        return false;
                    }

                    // Found a solution
                    SetThreadPriority(THREAD_PRIORITY_NORMAL);
                    LogPrintf("Scan_nNonce:\n");
                    LogPrintf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", pblock->GetHash().GetHex(), hashTarget.GetHex());

                    return true;
                };

        std::function<bool(EhSolverCancelCheck) > cancelled = [ &cancelSolver](EhSolverCancelCheck pos) {
            return cancelSolver;
        };

        try {
            // If we find a valid block, we rebuild
            bool found = EhOptimisedSolve(n, k, curr_state, validBlock, cancelled);
            if (found) {
                pblock->ToString();
 
                break;
            }
        } catch (EhSolverCancelledException&) {
            LogPrint(BCLog::POW, "Equihash solver cancelled\n");
            cancelSolver = false;
        }

        // Update nNonce and nTime
        pblock->nNonce = ArithToUint256(UintToArith256(pblock->nNonce) + 1);
    }
}


void creategenesisblock ( uint32_t nTime, uint32_t nBits )
{
    const char* pszTimestamp = "Fabcoin1ff5c8707d920ee573f5f1d43e559dfa3e4cb3f97786e8cb1685c991786b2";
    const CScript genesisOutputScript = CScript() << ParseHex("0322fdc78866c654c11da2fac29f47b2936f2c75a569155017893607b9386a4861") << OP_CHECKSIG;

    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);

    txNew.vin[0].scriptSig = CScript() << 00 << 520617983 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

    txNew.vout[0].nValue = 25 * COIN;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock pblock;
    //pblock.nTime = GetAdjustedTime();
    pblock.nTime = nTime;
    pblock.nBits = nBits;
    pblock.nNonce     = uint256();
    //pblock.nSolution  = uint256();
    pblock.nVersion = 4;
    pblock.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    pblock.hashPrevBlock.SetNull();
    pblock.nHeight  = 0;
    pblock.hashMerkleRoot = BlockMerkleRoot(pblock);

    const size_t N = 48, K = 5;
    Scan_nNonce_nSolution ( & pblock, N, K );

    std::cerr << pblock.ToString() << std::endl;
}
#endif // ENABLE_WALLET

