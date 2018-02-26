// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

// For equihash_parameters_acceptable.
#include "crypto/equihash.h"
#include "net.h"
#include "validation.h"

#define equihash_parameters_acceptable(N, K) \
    ((CBlockHeader::HEADER_SIZE + equihash_solution_size(N, K))*MAX_HEADERS_RESULTS < \
    MAX_PROTOCOL_MESSAGE_LENGTH-1000)

#include <assert.h>
#include "chainparamsseeds.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, const uint256& nNonce, const std::vector<unsigned char>& nSolution, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 00 << 520617983 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nSolution = nSolution;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));

    genesis.hashPrevBlock.SetNull();
    genesis.nHeight  = 0;
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);

    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *     CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *     vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, const uint256& nNonce, const std::vector<unsigned char>& nSolution, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Fabcoin1ff5c8707d920ee573f5f1d43e559dfa3e4cb3f97786e8cb1685c991786b2";
    const CScript genesisOutputScript = CScript() << ParseHex("0322fdc78866c654c11da2fac29f47b2936f2c75a569155017893607b9386a4861") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nSolution, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

const arith_uint256 maxUint = UintToArith256(uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
 
        consensus.nSubsidyHalvingInterval = 1680000;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835");
        consensus.BIP65Height = 0; 
        consensus.BIP66Height = 0; 
        consensus.powLimit = uint256S("0003ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        consensus.nPowAveragingWindow = 17;
        assert(maxUint/UintToArith256(consensus.powLimit) >= consensus.nPowAveragingWindow);

        consensus.nPowMaxAdjustDown = 32;
        consensus.nPowMaxAdjustUp = 16;

        consensus.nPowTargetTimespan = 1.75 * 24 * 60 * 60; // 1.75 days
        consensus.nPowTargetSpacing = 1.25 * 60; // 75 seconds

        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;

        consensus.nMinerConfirmationWindow = consensus.nPowTargetTimespan / consensus.nPowTargetSpacing; // 2016
        consensus.nRuleChangeActivationThreshold = consensus.nMinerConfirmationWindow * 0.95 + 1; // 95% of 2016

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000"); // need change to the value of the longest chain


        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xd9;
        nDefaultPort = 8665;
        nPruneAfterHeight = 100000;
        
        const size_t N = 48, K = 5;
        BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
        nEquihashN = N;
        nEquihashK = K;

        // 1517433514 2018.1.31
        genesis = CreateGenesisBlock(
            1517433514,
            uint256S("0x0000000000000000000000000000000000000000000000000000000000001e8f"),
            ParseHex("069804c9a4fe5d5ae829d9d11b02b5cd69e412ff2217744eae333315e3e53f524c5e41ce"),
            0x1f03ffff, 4, 50 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835"));
        assert(genesis.hashMerkleRoot     == uint256S("83acaf917b80757baa79d9635c35f0b09c7cab3c30f213d62419cc3630bc6960"));

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835");

        checkpointData = (CCheckpointData) {
            {
                { 0, uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835")},            }
        };


#if 0
        // Note that of those with the service bits flag, most only support a subset of possible options
        vSeeds.emplace_back("seed.fabcoin.sipa.be", true); // Pieter Wuille, only supports x1, x5, x9, and xd
        vSeeds.emplace_back("dnsseed.bluematt.me", true); // Matt Corallo, only supports x9
        vSeeds.emplace_back("dnsseed.fabcoin.dashjr.org", false); // Luke Dashjr
        vSeeds.emplace_back("seed.fabcoinstats.com", true); // Christian Decker, supports x1 - xf
        vSeeds.emplace_back("seed.fabcoin.jonasschnelli.ch", true); // Jonas Schnelli, only supports x1, x5, x9, and xd
        vSeeds.emplace_back("seed.btc.petertodd.org", true); // Peter Todd, only supports x1, x5, x9, and xd
#else
        vFixedSeeds.clear();
        vSeeds.clear();
#endif

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));


        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        chainTxData = ChainTxData{
            0,
            0,
            0
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 1680000; // fabcoin halving every 4 years
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835");
        consensus.BIP65Height = 0; 
        consensus.BIP66Height = 0; 
        consensus.powLimit = uint256S("0003ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        consensus.nPowAveragingWindow = 17;
        assert(maxUint/UintToArith256(consensus.powLimit) >= consensus.nPowAveragingWindow);
        consensus.nPowMaxAdjustDown = 32; // 32% adjustment down
        consensus.nPowMaxAdjustUp = 16; // 16% adjustment up

        consensus.nPowTargetTimespan = 1.75 * 24 * 60 * 60; // 1.75 days, for SHA256 mining only
        consensus.nPowTargetSpacing = 1.25 * 60; // 75 seconds

        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008


        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");


        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        nDefaultPort = 18665;
        nPruneAfterHeight = 1000;

        const size_t N = 48, K = 5;
        BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
        nEquihashN = N;
        nEquihashK = K;

        // 1517433514 2018.1.31
        genesis = CreateGenesisBlock(
            1517433514,
            uint256S("0x0000000000000000000000000000000000000000000000000000000000001e8f"),
            ParseHex("069804c9a4fe5d5ae829d9d11b02b5cd69e412ff2217744eae333315e3e53f524c5e41ce"),
            0x1f03ffff, 4, 50 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835"));
        assert(genesis.hashMerkleRoot == uint256S("83acaf917b80757baa79d9635c35f0b09c7cab3c30f213d62419cc3630bc6960"));

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835"); 

        checkpointData = (CCheckpointData) {
            {
                {0, uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835")},
            }
        };

        vFixedSeeds.clear();
        vSeeds.clear();
#if 0
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testnet-seed.bitcoin.jonasschnelli.ch", true);
        vSeeds.emplace_back("seed.tbtc.petertodd.org", true);
        vSeeds.emplace_back("testnet-seed.bluematt.me", false);
#endif

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        chainTxData = ChainTxData{
            0,
            0,
            0
        };

    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;

        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)

        consensus.nPowAveragingWindow = 17;
        consensus.nPowMaxAdjustDown = 32;
        consensus.nPowMaxAdjustUp = 16;

        consensus.nPowTargetTimespan = 1.75 * 24 * 60 * 60; // 1.75 days
        consensus.nPowTargetSpacing = 1.25 * 60; // 75 seconds
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;

        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18666;
        nPruneAfterHeight = 1000;
      
        const size_t N = 48, K = 5;
        BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
        nEquihashN = N;
        nEquihashK = K;

        // 1517433514 2018.1.31
        genesis = CreateGenesisBlock(
            1517433514,
            uint256S("0x00000000000000000000000000000000000000000000000000000000000000a5"),
            ParseHex("082205565143429ba30b6a934b6277759b7a25e6ddf2d4f65aa7f1286b5a9e940d8dd7a8"),
            0x2007ffff, 4, 50 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x00d0afe208ed97ccf503adbda469e7feddc0eebae96b67ea54a13792873e3c4d"));
        assert(genesis.hashMerkleRoot == uint256S("83acaf917b80757baa79d9635c35f0b09c7cab3c30f213d62419cc3630bc6960"));

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00d0afe208ed97ccf503adbda469e7feddc0eebae96b67ea54a13792873e3c4d");

        checkpointData = (CCheckpointData) {
            {
               {0, uint256S("0x00d0afe208ed97ccf503adbda469e7feddc0eebae96b67ea54a13792873e3c4d")},
            }
        };

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;


        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}

