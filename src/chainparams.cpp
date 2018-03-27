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

#include "consensus/params.h"
#include "arith_uint256.h"
#include "pow.h"

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
    genesis.hashStateRoot = uint256(h256Touint(dev::h256("e965ffd002cd6ad0e2dc402b8044de833e06b23127ea8c3d80aec91410771495"))); // fasc
    genesis.hashUTXORoot = uint256(h256Touint(dev::sha3(dev::rlp("")))); // fasc
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


static CBlock CreateGenesisBlockTestnet(uint32_t nTime, const uint256& nNonce, const std::vector<unsigned char>& nSolution, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Trump fires Rex Tillerson, selects Mike Pompeo as new Secretary of State";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nSolution, nBits, nVersion, genesisReward);
}

static CBlock CreateGenesisBlock_legacy(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)

{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = ArithToUint256(arith_uint256(nNonce));
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.nHeight  = 0;
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashStateRoot = uint256(h256Touint(dev::h256("e965ffd002cd6ad0e2dc402b8044de833e06b23127ea8c3d80aec91410771495"))); // fasc
    genesis.hashUTXORoot = uint256(h256Touint(dev::sha3(dev::rlp("")))); // fasc

    return genesis;
}

static CBlock CreateGenesisBlock_legacy(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock_legacy(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
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

class CBitMainParams : public CChainParams {
public:
    CBitMainParams() {
        strNetworkID = "bitmain";
        consensus.nSubsidyHalvingInterval = 1680000;
        consensus.FABHeight = -1;
        consensus.BIP34Height = 227931;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.BIP65Height = 388381; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 363725; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xd9;
        nDefaultPort = 8333;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock_legacy(1231006505, 2083236893, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        checkpointData = {
            {
                { 11111, uint256S("0x0000000069e244f73d78e8fd29ba2fd2ed618bd6fa2ee92559f542fdb26e7c1d")},
                { 33333, uint256S("0x000000002dd5588a74784eaa7ab0507a18ad16a236e7b1ce69f00d7ddfb5d0a6")},
                { 74000, uint256S("0x0000000000573993a3c9e41ce34471c079dcf5f52a0e824a81e7f953b8661a20")},
                {105000, uint256S("0x00000000000291ce28027faea320c8d2b054b2e0fe44a773f3eefb151d6bdc97")},
                {134444, uint256S("0x00000000000005b12ffd4cd315cd34ffd4a594f430ac814c91184a0d42d2b0fe")},
                {168000, uint256S("0x000000000000099e61ea72015e79632f216fe6cb33d7899acb35b75c8303b763")},
                {193000, uint256S("0x000000000000059f452a5f7340de6682a977387c17010ff6e6c3bd83ca8b1317")},
                {210000, uint256S("0x000000000000048b95347e83192f69cf0366076336c639f9b7228e9ba171342e")},
                {216116, uint256S("0x00000000000001b4f4b433e81ee46494af945cf96014816a4e2370f11b23df4e")},
                {225430, uint256S("0x00000000000001c108384350f74090433e7fcf79a606b8e797f065b130575932")},
                {250000, uint256S("0x000000000000003887df1f29024b06fc2200b55f8af8f35453d7be294df2d214")},
                {279000, uint256S("0x0000000000000001ae8c72a0b0c301f67e3afca10e819efa9041e458e9bd7e40")},
                {295000, uint256S("0x00000000000000004d9b4ef50f0f9d686fd69db2e03af35a100370c64632a983")},
            }
        };

 chainTxData = ChainTxData{
            // Data as of block 0000000000000000002d6cca6761c99b3c2e936f9a0e304b7c7651a993f461de (height 506081).
            1516903077, // * UNIX timestamp of last known number of transactions
            295363220,  // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            3.5         // * estimated number of transactions per second after that timestamp
        };
    }
};


const arith_uint256 maxUint = UintToArith256(uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
 
        consensus.nSubsidyHalvingInterval = 3360000;
        consensus.FABHeight = 0;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835");
        consensus.BIP65Height = 0; 
        consensus.BIP66Height = 0; 
        consensus.CoinbaseLock = 80000;

        consensus.powLimit = uint256S("07ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimitLegacy = uint256S("0003ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

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
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1479168000; // November 15th, 2016.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1510704000; // November 15th, 2017.

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000"); // need change to the value of the longest chain

        // By default assume that the signatures in ancestors of this block are valid.
        const std::string genesisHashString =  "0x4a0f561d97c693e7cb8b2022405371e55d9caca010b52cbf41258076f57a9e9a";
        consensus.defaultAssumeValid = uint256S(genesisHashString);

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        //pchMessageStart[0] = 0xf9;
        //pchMessageStart[1] = 0xbe;
        //pchMessageStart[2] = 0xb4;
        //pchMessageStart[3] = 0xd9;
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xbf;
        pchMessageStart[3] = 0xab;
        nDefaultPort = 8665;
        nPruneAfterHeight = 100000;
        
        const size_t N = 200, K = 9;
        BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
        nEquihashN = N;
        nEquihashK = K;

        // 1517433514 2018.1.31
        genesis = CreateGenesisBlock(
            1517433514,
            uint256S("0x0000000000000000000000000000000000000000000000000000000000000001"),
            ParseHex("0115e6ccd7186a8338625a9544d5e7eed224b7e2a20f4468d14884065919e1b43bdb49b0da7d66d5cbc111ad7387e445b42275aa6202b22480d887b115ec7d1ef8cd565312e92cff6773f84aa355cf951fbfcf3109c734e942087a396cca364671d9c2c609bd90e35b139c25f1e7673535a80eb6473748779dd3a9ff1d762a53be02529fd29d5704d7a7e564970e233532b014362e3edc61758e8de70ba47d5c797705822e5030540177a4531d0d257f1c9e8189437da6787901c4504a3b33ba38236391315c2e95649ef7c6730e8cf8ed9a04898cc0dc8c24f73cbe92c4a473ea6f7e49ff5395159843173ed88f31d45dd1d67a2e2abec2531ea7c7150aba38ab071fda76ab68858a63d5865604dee4112ba02aaa0b55f5638a3ff35064e82bb3bc40df1b2918fed2568be471d793e7195d21dd7a76c13718a7772a3a271f078d85af41e9e6440f5066268956f4cb8b01457f6a18d15ca39f03605ddfdf05001f28f5ea980381e0c528d19f78ae9cd7393474577a1b0f373565224516a5f5d5c1c5e939e6dcd658e307749d1dc93f46537cc0f51501add8e5e8458153ca9a8c15ff06ac08e31cdd73c25a8b1d9a95dd4b4115a5d2debb075414946301f2adf35fc1af7225eb7a41e36de9bf22950c797c0892c86b59166434bc08bfd20230e3b800ce24d41bed3c0e6315de262396cdcf037d0577bd418c0737e3605fe62bad418f68a187d7d49629fd3a42ef0d7fb551fcd27d59016883b84e7ad1e55d2796c64c2771930e6d4a0b94bc3222a1b5c28a027c1b7c250635badb529daff49df03d5558e95b687e4636bbf6da0954e91218084184468ae3fab4319ff1fb38f1a7615be9d46faf2f4673e60bb9668bf9181aa2803a6b7619b01dc4e3659a29887fc50c3dae943d6bf03f8b3522ec2170009f0293998d330670ab51d9169d295b4901ebb6c9d6b19573bcd2b90debebc5be5f7b35e6761b7541a88c4a83ece46021eaad37a454bff9f77ef807dd4650ae0ab638d9a62160ce22da7ee52617ef840ac96e6088db38b924b384c99c2d262a7ecf7cb59604773db4235854a706b365a5b15d00b25794dbe95508a01eae59039d1a35c10578ab46b642b1c49fec0610b12fa10df2fce5afb285367f56540dbec7966d6a21cd9270ec2028e7f550154e6d7cd64d5b0254b3df0426d3d6c0b77ecdd20fa1317b62d864d1e94ecd96242986f4c4ea77a5b33617dea272a85b5acddceb493286efe0435fef691fbf646921b1a7118dea38440c514c760020e5618791c3d5746bb2f7feba285ef05909636be062c3cddb0a8893dddd4e2fbd6ef9141528378afba14a6af1ed6771f768017c048e5f60ba8b6b0dd1253271a7e127ccdfc772d4c0455a69ec1db21635deedbba1cdb20bda90b43bab761af11ec4ef45660254d2ade67024b9cd8ed27e1964f5eeb9ac362de41d149a74b490c44089979852255ed85308a9bf70440aa2526055c9ad2ebf19913a32e16349802b6dadec0dd6fac96d8944806899d6fc7ebf5cf273fc7431100472a74c35597115d1578558f5e148c77cdebfa3112b8c7d1b57103ee7f305c705b2de9c59f3949c6ea808639ba23c0c79ef318720c3147612ed5e5f52555c0b31fe41e8cf2e2686ff31e993ae8df8fc5a7c58d605eb95fdbb28f669e5730141cd19803e48ff7a941511d8a1f1ce21548dac55712dcf0b56110bd10d816621e2d959cb1ac933844bd591aec163ab1aeffe1e9635ea7a9d3e8e1a52912a9525443168530b9fb8a05b086d0f08b80c65c1cb3da0e0e7c841ed5fd8751f0e1d5194296713987ef471e35b1dbd4235092e9605c5124a7cf55650830494243136752c1838b14e56f654216d72771c2f8567d40bb75290fb577a6253bcb2a4"),
            0x2007ffff, 1, 25 * COIN );
/*
    int iii = 0;
    auto nNonce  = uint256();
    while (iii < (1<<20) ) {
    iii++;
    nNonce = ArithToUint256(UintToArith256(nNonce) + 1);
        genesis = CreateGenesisBlock(
            1517433514,
             nNonce, 
            ParseHex("0115e6ccd7186a8338625a9544d5e7eed224b7e2a20f4468d14884065919e1b43bdb49b0da7d66d5cbc111ad7387e445b42275aa6202b22480d887b115ec7d1ef8cd565312e92cff6773f84aa355cf951fbfcf3109c734e942087a396cca364671d9c2c609bd90e35b139c25f1e7673535a80eb6473748779dd3a9ff1d762a53be02529fd29d5704d7a7e564970e233532b014362e3edc61758e8de70ba47d5c797705822e5030540177a4531d0d257f1c9e8189437da6787901c4504a3b33ba38236391315c2e95649ef7c6730e8cf8ed9a04898cc0dc8c24f73cbe92c4a473ea6f7e49ff5395159843173ed88f31d45dd1d67a2e2abec2531ea7c7150aba38ab071fda76ab68858a63d5865604dee4112ba02aaa0b55f5638a3ff35064e82bb3bc40df1b2918fed2568be471d793e7195d21dd7a76c13718a7772a3a271f078d85af41e9e6440f5066268956f4cb8b01457f6a18d15ca39f03605ddfdf05001f28f5ea980381e0c528d19f78ae9cd7393474577a1b0f373565224516a5f5d5c1c5e939e6dcd658e307749d1dc93f46537cc0f51501add8e5e8458153ca9a8c15ff06ac08e31cdd73c25a8b1d9a95dd4b4115a5d2debb075414946301f2adf35fc1af7225eb7a41e36de9bf22950c797c0892c86b59166434bc08bfd20230e3b800ce24d41bed3c0e6315de262396cdcf037d0577bd418c0737e3605fe62bad418f68a187d7d49629fd3a42ef0d7fb551fcd27d59016883b84e7ad1e55d2796c64c2771930e6d4a0b94bc3222a1b5c28a027c1b7c250635badb529daff49df03d5558e95b687e4636bbf6da0954e91218084184468ae3fab4319ff1fb38f1a7615be9d46faf2f4673e60bb9668bf9181aa2803a6b7619b01dc4e3659a29887fc50c3dae943d6bf03f8b3522ec2170009f0293998d330670ab51d9169d295b4901ebb6c9d6b19573bcd2b90debebc5be5f7b35e6761b7541a88c4a83ece46021eaad37a454bff9f77ef807dd4650ae0ab638d9a62160ce22da7ee52617ef840ac96e6088db38b924b384c99c2d262a7ecf7cb59604773db4235854a706b365a5b15d00b25794dbe95508a01eae59039d1a35c10578ab46b642b1c49fec0610b12fa10df2fce5afb285367f56540dbec7966d6a21cd9270ec2028e7f550154e6d7cd64d5b0254b3df0426d3d6c0b77ecdd20fa1317b62d864d1e94ecd96242986f4c4ea77a5b33617dea272a85b5acddceb493286efe0435fef691fbf646921b1a7118dea38440c514c760020e5618791c3d5746bb2f7feba285ef05909636be062c3cddb0a8893dddd4e2fbd6ef9141528378afba14a6af1ed6771f768017c048e5f60ba8b6b0dd1253271a7e127ccdfc772d4c0455a69ec1db21635deedbba1cdb20bda90b43bab761af11ec4ef45660254d2ade67024b9cd8ed27e1964f5eeb9ac362de41d149a74b490c44089979852255ed85308a9bf70440aa2526055c9ad2ebf19913a32e16349802b6dadec0dd6fac96d8944806899d6fc7ebf5cf273fc7431100472a74c35597115d1578558f5e148c77cdebfa3112b8c7d1b57103ee7f305c705b2de9c59f3949c6ea808639ba23c0c79ef318720c3147612ed5e5f52555c0b31fe41e8cf2e2686ff31e993ae8df8fc5a7c58d605eb95fdbb28f669e5730141cd19803e48ff7a941511d8a1f1ce21548dac55712dcf0b56110bd10d816621e2d959cb1ac933844bd591aec163ab1aeffe1e9635ea7a9d3e8e1a52912a9525443168530b9fb8a05b086d0f08b80c65c1cb3da0e0e7c841ed5fd8751f0e1d5194296713987ef471e35b1dbd4235092e9605c5124a7cf55650830494243136752c1838b14e56f654216d72771c2f8567d40bb75290fb577a6253bcb2a4"),
            0x2007ffff, 1, 25 * COIN );
     bool postfork = true;
     const Consensus::Params param = consensus;
     if (CheckProofOfWork(genesis.GetHash(consensus), 0x207fffff,  postfork, param) ) break;
     }

*/
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S(genesisHashString));
        assert(genesis.hashMerkleRoot == uint256S("0xafcaf52027ac3c032eda00c018bde8996dfec7523d88d028f0fd188a35b01b06"));

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

        checkpointData = (CCheckpointData) {
            {
                {0, uint256S(genesisHashString)},
            }
        };

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

        consensus.nSubsidyHalvingInterval = 3360000;
        consensus.FABHeight = 0;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835");
        consensus.BIP65Height = 0; 
        consensus.BIP66Height = 0; 
        consensus.CoinbaseLock = 0;

        consensus.powLimit = uint256S("07ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimitLegacy = uint256S("0003ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
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

        // By default assume that the signatures in ancestors of this block are valid.
        const std::string genesisHashString =  "0x39676a2ad6c2395538966b03049c18592f440dc346b1589513161222a0118b5b";
        consensus.defaultAssumeValid = uint256S(genesisHashString);

        //pchMessageStart[0] = 0x0b;
        //pchMessageStart[1] = 0x11;
        //pchMessageStart[2] = 0x09;
        //pchMessageStart[3] = 0x07;
        pchMessageStart[0] = 0x0f;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x0a;
        pchMessageStart[3] = 0x0b;
        nDefaultPort = 18665;
        nPruneAfterHeight = 1000;

        const size_t N = 200, K = 9;
        BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
        nEquihashN = N;
        nEquihashK = K;

        // 1517433514 2018.1.31
        genesis = CreateGenesisBlockTestnet(
            1521040319,
            uint256S("0x0000000000000000000000000000000000000000000000000000000000000001"),
            ParseHex("009251b3fd34dca9ffe4c81443cd04fe0f0ad2560e258fd3c26174f5afd75c5320d5a47789cbdc141c820aabed9e8ba58a0d61935596edf756b6e15bb98f40269811c6859070075c08f3e15fcbb732ef27fe1c08060a0e584aaa94b35fc3c06e732ebe88e37e913a171278bd615f1ad084dc9e37ed825dd8d7c2539e4dbc06fb37d58686855edd1a71adbcf2c1e1b64015fd02313d46a18df05b7dcface3218c1fd70cf06f73dbf104f1c3da795fc099ab50822d5f680a03cf3a1f41ac09f8cb21de2230256abcf498394ccf3e37b516ccdc0dbbaeb383a6334f417e91a426e818d8c4063ac67b1c972c65bbd6f067e11e01ceff65e2d9c4359de5480f1ac0ce5ccc8acbb6ad93b140443ffd95b7960aca2e12b60d998f53bd405484c815b5beb946704dad001de4436136142b4bd2c5223be9c46741a6b41178301e93239a8b206d37a569c3970f3bf83e38103822f500fbacceda20b3a7920860c6fba7cf4de457f77bc320837eef38257b938d52c71af0639ef65ac77b614c198b3c2886ebc13d7b45f26064fd6738ceb5fd66173a423c5c118f7b3d5fe635f5e2fcc1727a1cdda4e2052af2cf82c8979b40e0c330f26939cde6fd7ce30a1a6ceaa47d1fb86fef604827c74e2542db203eee842d832a68afe7fd93e02223a2dbcc39aa1cfb1ad14e6476f65c0ff6947fe48e873bbce66a7e996a5d96d001311995b151895d269027026cec61dbe3527fcfaa47d2f50924531ca975e59a0e0b66bbef43039fe8c30897fa11d68f0103ab7af49f287b38e2c487fa65580d0875e82ee10489a52e79eb4a60033e8c6c58c1b003570da4a2ccdca4f95a53f9cf5051b59963134bc12220e1804251eff3fcdf83a058fab0d9df72d1d80307d59940b804f7afbde2a0e657a9b886e79b7e45da157085f5335684a70a6265dcd1d55bafa2361e430a03168b8ddf91714341c3023ebd4c241d75af6ef9be0544a3d2879e2ced8ddfc25c94ca8734f6d914f18e1bbc145494913e5105efc2f908df66fd404f3dda0c1cc7ee21fa1088437ba201e32934b296236cb9d476094540d0ba6bb493693d10a110cc04a79c42dfb5e9177307f9061b0fc3e6fcc8c8b0c82e0a7e12f5683d215ed70c7b5c0755baf5658615c64615bde033f8343173af54ee2255a39f2be85716e086bf64ce9f25d403b729336450b7553b3076fbdd434cfded797d429e042c3d913ec5e95582de70c5eb5483850e594ba4462c8de198ef551a6753c503f67d21dc391032118547307b4ee6b158e244ef84b39e2ddd2fbd2d0f0d7ee51536cff189d817ab07d7b34396e5dbe4f4cd0eca6c24bf1f16aacd9338bf6b42ac0d7f7969d97f9184382daab32450b3dda9c2a8653114e2423687753ea7702e866775f2a00db9422bf7552f4c515f3456ff8de30451110943c74f9f482c66278ed9c019a7d7cfa66c2b2eb5cd1f78664bdc8433b1dc378dfd51762dbc76144fcc92e096f26abefa01f267fc9f46c0691af32e18ba10cc49ad6ec39e1f6451a5b47755a226ee05450fe3b898728c2282ee6581c0f0f041da976097e0df1511ac376fdebdefa75c5271216d80317c8431738114ad59324b5e6101e77a1203ea2b5364eebcecd998213dbb6baa1b6dc9bd8f035717e54a91c9e9f512f2079834f471adfca5c8e903a296b4968e7523dbcafa671fe42e7eaab9477f43f8d0b67ef93abd461966061cc2b32c0e0a352f030d15004c31ce6dd2ebb0e0f6273d093d39a6d199fbcdb41647a536662b1376bedc1018bad9be86f72ed461c1b952765055378b69d22132c36ab3e6a75e5f697483faa47a10357f5a4dee561033b740b0c5d0a26653b596c7d094c6db5ed742861458c4ce6acb32ec8167b58460dd52e6abfbdcbcd2"),
            0x2007ffff, 1, 25 * COIN );

/*

    int iii = 0;
    auto nNonce  = uint256();
    while (iii < (1<<20) ) {
    iii++;
    nNonce = ArithToUint256(UintToArith256(nNonce) + 1);
        genesis = CreateGenesisBlockTestnet(
            1521040319,
            nNonce, 
            ParseHex("009251b3fd34dca9ffe4c81443cd04fe0f0ad2560e258fd3c26174f5afd75c5320d5a47789cbdc141c820aabed9e8ba58a0d61935596edf756b6e15bb98f40269811c6859070075c08f3e15fcbb732ef27fe1c08060a0e584aaa94b35fc3c06e732ebe88e37e913a171278bd615f1ad084dc9e37ed825dd8d7c2539e4dbc06fb37d58686855edd1a71adbcf2c1e1b64015fd02313d46a18df05b7dcface3218c1fd70cf06f73dbf104f1c3da795fc099ab50822d5f680a03cf3a1f41ac09f8cb21de2230256abcf498394ccf3e37b516ccdc0dbbaeb383a6334f417e91a426e818d8c4063ac67b1c972c65bbd6f067e11e01ceff65e2d9c4359de5480f1ac0ce5ccc8acbb6ad93b140443ffd95b7960aca2e12b60d998f53bd405484c815b5beb946704dad001de4436136142b4bd2c5223be9c46741a6b41178301e93239a8b206d37a569c3970f3bf83e38103822f500fbacceda20b3a7920860c6fba7cf4de457f77bc320837eef38257b938d52c71af0639ef65ac77b614c198b3c2886ebc13d7b45f26064fd6738ceb5fd66173a423c5c118f7b3d5fe635f5e2fcc1727a1cdda4e2052af2cf82c8979b40e0c330f26939cde6fd7ce30a1a6ceaa47d1fb86fef604827c74e2542db203eee842d832a68afe7fd93e02223a2dbcc39aa1cfb1ad14e6476f65c0ff6947fe48e873bbce66a7e996a5d96d001311995b151895d269027026cec61dbe3527fcfaa47d2f50924531ca975e59a0e0b66bbef43039fe8c30897fa11d68f0103ab7af49f287b38e2c487fa65580d0875e82ee10489a52e79eb4a60033e8c6c58c1b003570da4a2ccdca4f95a53f9cf5051b59963134bc12220e1804251eff3fcdf83a058fab0d9df72d1d80307d59940b804f7afbde2a0e657a9b886e79b7e45da157085f5335684a70a6265dcd1d55bafa2361e430a03168b8ddf91714341c3023ebd4c241d75af6ef9be0544a3d2879e2ced8ddfc25c94ca8734f6d914f18e1bbc145494913e5105efc2f908df66fd404f3dda0c1cc7ee21fa1088437ba201e32934b296236cb9d476094540d0ba6bb493693d10a110cc04a79c42dfb5e9177307f9061b0fc3e6fcc8c8b0c82e0a7e12f5683d215ed70c7b5c0755baf5658615c64615bde033f8343173af54ee2255a39f2be85716e086bf64ce9f25d403b729336450b7553b3076fbdd434cfded797d429e042c3d913ec5e95582de70c5eb5483850e594ba4462c8de198ef551a6753c503f67d21dc391032118547307b4ee6b158e244ef84b39e2ddd2fbd2d0f0d7ee51536cff189d817ab07d7b34396e5dbe4f4cd0eca6c24bf1f16aacd9338bf6b42ac0d7f7969d97f9184382daab32450b3dda9c2a8653114e2423687753ea7702e866775f2a00db9422bf7552f4c515f3456ff8de30451110943c74f9f482c66278ed9c019a7d7cfa66c2b2eb5cd1f78664bdc8433b1dc378dfd51762dbc76144fcc92e096f26abefa01f267fc9f46c0691af32e18ba10cc49ad6ec39e1f6451a5b47755a226ee05450fe3b898728c2282ee6581c0f0f041da976097e0df1511ac376fdebdefa75c5271216d80317c8431738114ad59324b5e6101e77a1203ea2b5364eebcecd998213dbb6baa1b6dc9bd8f035717e54a91c9e9f512f2079834f471adfca5c8e903a296b4968e7523dbcafa671fe42e7eaab9477f43f8d0b67ef93abd461966061cc2b32c0e0a352f030d15004c31ce6dd2ebb0e0f6273d093d39a6d199fbcdb41647a536662b1376bedc1018bad9be86f72ed461c1b952765055378b69d22132c36ab3e6a75e5f697483faa47a10357f5a4dee561033b740b0c5d0a26653b596c7d094c6db5ed742861458c4ce6acb32ec8167b58460dd52e6abfbdcbcd2"),
            0x2007ffff, 1, 25 * COIN );
     bool postfork = true;
     const Consensus::Params param = consensus;
     if (CheckProofOfWork(genesis.GetHash(consensus), 0x207fffff,  postfork, param) ) break;
     }
*/


        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S(genesisHashString));
        assert(genesis.hashMerkleRoot == uint256S("0x3725088af50d5bfa636f5c051887e35b4a117a7c2a46944897e6e91efbe24eb5"));

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

        checkpointData = (CCheckpointData) {
            {
                {0, uint256S(genesisHashString)},
            }
        };

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
        consensus.FABHeight = 3000;
        consensus.CoinbaseLock = 0;

        consensus.nPowAveragingWindow = 17;
        consensus.nPowMaxAdjustDown = 32;
        consensus.nPowMaxAdjustUp = 16;

        consensus.nPowTargetTimespan = 1.75 * 24 * 60 * 60; // 1.75 days
        consensus.nPowTargetSpacing = 1.25 * 60; // 75 seconds
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimitLegacy = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
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

        // By default assume that the signatures in ancestors of this block are valid.
        //const std::string genesisHashString =  "0xe738ccb74b988c39f0a03938d46e7c831199381d89291c3a12d83ffd5835876b";
        const std::string genesisHashString =  "0x073fe2eaeff656e348bc84dbd7e8ee24be8f6daf60757260cd998b27cfcdf62a";
        consensus.defaultAssumeValid = uint256S(genesisHashString);

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
        // jyan genesis = CreateGenesisBlock_legacy(1296688602, 2, 0x207fffff, 1, 50 * COIN);
        genesis = CreateGenesisBlock(
            1517433514,
            uint256S("0x0000000000000000000000000000000000000000000000000000000000000001"),
            ParseHex("02e95dd630c7a59cd3256a2e7fd385e30dbf14aad14f6585c7abf63c2b4ff418c786bf70"),
            0x207fffff, 1, 50 * COIN );
/*
	int iii = 0;
	auto nNonce  = uint256();
	while (iii < (1<<20) ) {
	iii++;
	nNonce = ArithToUint256(UintToArith256(nNonce) + 1);
        genesis = CreateGenesisBlock(
            1517433514,
	    nNonce,
            ParseHex("02e95dd630c7a59cd3256a2e7fd385e30dbf14aad14f6585c7abf63c2b4ff418c786bf70"),
            0x207fffff, 1, 50 * COIN );
	 bool postfork = true;
	 const Consensus::Params param = consensus;
	 if (CheckProofOfWork(genesis.GetHash(consensus), 0x207fffff,  postfork, param) ) break;
	 }

*/
        //jyan consensus.hashGenesisBlock = genesis.GetHash();
        consensus.hashGenesisBlock = genesis.GetHash(consensus);


        assert(consensus.hashGenesisBlock == uint256S(genesisHashString));
        assert(genesis.hashMerkleRoot == uint256S("0x83acaf917b80757baa79d9635c35f0b09c7cab3c30f213d62419cc3630bc6960"));


        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = false; //jyan
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = (CCheckpointData) {
            {
                {0, uint256S(genesisHashString)},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
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

const CChainParams &GetParams() {
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
    else if (chain == CBaseChainParams::BITMAIN)
        return std::unique_ptr<CChainParams>(new CBitMainParams());
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

