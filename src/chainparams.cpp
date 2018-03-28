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
    const char* pszTimestamp = "Facebook data scandal opens new era in global privacy enforcement";
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
        consensus.CoinbaseLock = 0;
        consensus.ForceSegwit = false;

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
        consensus.ForceSegwit = true;

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
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;
        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000"); // need change to the value of the longest chain

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x033e277a5f7c88317a748f5bf353580f12088c41ccb8da36484aca49114636fd");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
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
            1522157467,
            uint256S("0x0000000000000000000000000000000000000000000000000000000000000075"),
            ParseHex("011da54a86d7976776ab51adcfd776406edb9dabe408fe9f63ca5b6f4fede87489f937127712e6de8fd50380296c0d9b04def585106d4970bf44466e035db40e961faeea44e297578543c2a2f486f65be57e920d0343c2d41fca97fb57d6b38aeaf481a18ec73e7cb00bda6258d9c3bb619f5a1b7e415c550f35653ac94e18af359492117c20916ed6ba924d1a56b6ee3fc84d79cd6e7f47ea01c36f09e8a3f6e30e1eee163ec839028469bbecc0c8a08fc9e321a91b89b514ee7cb5374a768c0d6593db29b769c56b075ff12a3d90fa71c2335887f166d85b39a905a37230e8058b61fcdda1275c8dcb0be19b456b508c8ade8bf5c65313e0ff77ba0b39c8f3826299095514d1f3a5a2925f7564bd5eda37b752ad6f4ff78aadcb9645366ba0f199a77894900d730d54528b0211a80d8252dcf238b9dff356ae2f287aa4b8bf4b53b678e78352f274c1a260961aa995047db4976f70675bacdf12a500ce5479c621fe6e07074b0c10923b18b1fb40d1d7e7266c1997b3b3a5a805f57d450aa80673b6963566de7ee269a35a3b827c08c91bc81206e5065cf4a4f8167705cf84a87cfc7d0dd84220c387371131fa32ea33e03c7160d431e125381c1aae0ce73b3f4560e50d225413664f45b9c3b50f909ea965191e272939b3f5daffa5edbadd1efd6a4fe3a6e29399c2f971f587ecef4c63739b43be18f509a303badd0cf8e6c7bdf685edd63e078773de0c482d38e2a057d01adfc0d774064f4b9151f96c3eb1913450bd713ececb6b1740b8fdca78db2e8b8dddedcc58d664b74562db61481d3828f469a49b2b4a1b91580fd1b96137a00d7d9f1f83aba7a3f3a906d70c4b03182a324b9bb3f5a1df45639231ecc88960b21edbe1243042e647d1d4e5b0e7f2604c32ad8a232e7a71cc258d4997f9dc37512334a809be471d1b7af41d6f8502d752bb9fa192fb2dd4b3fb72c00c590e678d2bfd0408549b30cbd035c161b5defa77c4aa210b3504a3147b491683a293bda71cf21962b91196507fbbfbfa18acdc0522503d43b28031aea8b88a60bff20d24763c42068e5d988735123ce9b1e662ef46cced772ccd46665d52095dfebb4b3017934ce0c9ea3e697b167b414067bb37b699cbe7c3d9f0eaf4493eecfdbf135d45d404a1a99fc03f7108f681b1d65441bd45f74a8107e48f171e8dbdfdca90732d776fb116136c5a8060751be6b0d3de8f7f9d2bb7991060aac75db5fb2c321a602edbf6eddcabba98aa4228de3b0ea7d0556b8732021297f7dcb5f50519d6e8a9673db201967fbee70815a18fbf1a11eb86c0589199eb9312e5489b88e508dc80a0aad333e0afbf80f723bbe818dfbd912ed20a2919519c071050aa2330c4cd30a62cc1225a49f813f9b649cff65c75cdefe98babf826d2eadf9cb976044892434f458eef99b13578d8ed3a72b13795a6353037db2b852f79b9ab49b484ffb932c5fd6e9425d9108e7ef346498be7402a31e8d81cf4b1bdbe163b13119020e5ae6204d3ee1be1c353b5bcb6de09bc42c8057046b70941ac01904be563cd6ecfb17e82cd7b711381a8f130107287140892254179ec671b1eb90b5e0a6007e27d1a4c5718cd388a88d3bd2e3312f31c103be5f380d35d964f4c533673fd632b26a51d5b0bff0a42d91976c79064a6c8910ce4618f6e510255b6d70d846f5fe00f2287e3aca1ac5ca213f8b970de49941d8936b49be4d605a321a7875ce6a895f6719a8b881e7933c8ccb58069b67de898cb72c9762c87bd373a0ed0ff93c80478ac47da029ad560f1b1478c0bd314276cceac43940e938eb90394ff3b03a97f54709f96102464d0ec063b4f95b05502a0c039419a89bb065437ee4423fc9a6fd9a84077600ef4cb8f0c43b89340"),
            0x2007ffff, 1, 25 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S("0x033e277a5f7c88317a748f5bf353580f12088c41ccb8da36484aca49114636fd"));
        assert(genesis.hashMerkleRoot == uint256S("0xe89cf9aeaa7b90a0c6695946bf23a41447c4ade44a63e6fa40481e35957e69aa"));

#if 0
        // Note that of those with the service bits flag, most only support a subset of possible options
        vSeeds.emplace_back("seed.fabexplorer.info", true); 
#else
        vFixedSeeds.clear();
        vSeeds.clear();
#endif

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = (CCheckpointData) {
            {
                { 0, uint256S("0x033e277a5f7c88317a748f5bf353580f12088c41ccb8da36484aca49114636fd")},            }
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
        consensus.ForceSegwit = false;

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
        consensus.defaultAssumeValid = uint256S("0x0500238931fa06c38381611e9244d9523926d6dc501664de27d1bff4e22b9afa"); 

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
            uint256S("0x0000000000000000000000000000000000000000000000000000000000000060"),
            ParseHex("009251b3fd34dca9ffe4c81443cd04fe0f0ad2560e258fd3c26174f5afd75c5320d5a47789cbdc141c820aabed9e8ba58a0d61935596edf756b6e15bb98f40269811c6859070075c08f3e15fcbb732ef27fe1c08060a0e584aaa94b35fc3c06e732ebe88e37e913a171278bd615f1ad084dc9e37ed825dd8d7c2539e4dbc06fb37d58686855edd1a71adbcf2c1e1b64015fd02313d46a18df05b7dcface3218c1fd70cf06f73dbf104f1c3da795fc099ab50822d5f680a03cf3a1f41ac09f8cb21de2230256abcf498394ccf3e37b516ccdc0dbbaeb383a6334f417e91a426e818d8c4063ac67b1c972c65bbd6f067e11e01ceff65e2d9c4359de5480f1ac0ce5ccc8acbb6ad93b140443ffd95b7960aca2e12b60d998f53bd405484c815b5beb946704dad001de4436136142b4bd2c5223be9c46741a6b41178301e93239a8b206d37a569c3970f3bf83e38103822f500fbacceda20b3a7920860c6fba7cf4de457f77bc320837eef38257b938d52c71af0639ef65ac77b614c198b3c2886ebc13d7b45f26064fd6738ceb5fd66173a423c5c118f7b3d5fe635f5e2fcc1727a1cdda4e2052af2cf82c8979b40e0c330f26939cde6fd7ce30a1a6ceaa47d1fb86fef604827c74e2542db203eee842d832a68afe7fd93e02223a2dbcc39aa1cfb1ad14e6476f65c0ff6947fe48e873bbce66a7e996a5d96d001311995b151895d269027026cec61dbe3527fcfaa47d2f50924531ca975e59a0e0b66bbef43039fe8c30897fa11d68f0103ab7af49f287b38e2c487fa65580d0875e82ee10489a52e79eb4a60033e8c6c58c1b003570da4a2ccdca4f95a53f9cf5051b59963134bc12220e1804251eff3fcdf83a058fab0d9df72d1d80307d59940b804f7afbde2a0e657a9b886e79b7e45da157085f5335684a70a6265dcd1d55bafa2361e430a03168b8ddf91714341c3023ebd4c241d75af6ef9be0544a3d2879e2ced8ddfc25c94ca8734f6d914f18e1bbc145494913e5105efc2f908df66fd404f3dda0c1cc7ee21fa1088437ba201e32934b296236cb9d476094540d0ba6bb493693d10a110cc04a79c42dfb5e9177307f9061b0fc3e6fcc8c8b0c82e0a7e12f5683d215ed70c7b5c0755baf5658615c64615bde033f8343173af54ee2255a39f2be85716e086bf64ce9f25d403b729336450b7553b3076fbdd434cfded797d429e042c3d913ec5e95582de70c5eb5483850e594ba4462c8de198ef551a6753c503f67d21dc391032118547307b4ee6b158e244ef84b39e2ddd2fbd2d0f0d7ee51536cff189d817ab07d7b34396e5dbe4f4cd0eca6c24bf1f16aacd9338bf6b42ac0d7f7969d97f9184382daab32450b3dda9c2a8653114e2423687753ea7702e866775f2a00db9422bf7552f4c515f3456ff8de30451110943c74f9f482c66278ed9c019a7d7cfa66c2b2eb5cd1f78664bdc8433b1dc378dfd51762dbc76144fcc92e096f26abefa01f267fc9f46c0691af32e18ba10cc49ad6ec39e1f6451a5b47755a226ee05450fe3b898728c2282ee6581c0f0f041da976097e0df1511ac376fdebdefa75c5271216d80317c8431738114ad59324b5e6101e77a1203ea2b5364eebcecd998213dbb6baa1b6dc9bd8f035717e54a91c9e9f512f2079834f471adfca5c8e903a296b4968e7523dbcafa671fe42e7eaab9477f43f8d0b67ef93abd461966061cc2b32c0e0a352f030d15004c31ce6dd2ebb0e0f6273d093d39a6d199fbcdb41647a536662b1376bedc1018bad9be86f72ed461c1b952765055378b69d22132c36ab3e6a75e5f697483faa47a10357f5a4dee561033b740b0c5d0a26653b596c7d094c6db5ed742861458c4ce6acb32ec8167b58460dd52e6abfbdcbcd2"),
            0x2007ffff, 1, 25 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S("0x0500238931fa06c38381611e9244d9523926d6dc501664de27d1bff4e22b9afa"));
        assert(genesis.hashMerkleRoot == uint256S("0x3725088af50d5bfa636f5c051887e35b4a117a7c2a46944897e6e91efbe24eb5"));

        vFixedSeeds.clear();
        vSeeds.clear();
#if 0
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testnet-seed.fabexplorer.info", true); 
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
                {0, uint256S("0x0500238931fa06c38381611e9244d9523926d6dc501664de27d1bff4e22b9afa")},
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
        consensus.nSubsidyHalvingInterval = 850;

        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.FABHeight = 24000;
        consensus.CoinbaseLock = 0;
        consensus.ForceSegwit = false;

        consensus.nPowAveragingWindow = 17;
        consensus.nPowMaxAdjustDown = 32;
        consensus.nPowMaxAdjustUp = 16;

        consensus.nPowTargetTimespan = 1.75 * 24 * 60 * 60; // 1.75 days
        consensus.nPowTargetSpacing = 1.25 * 60; // 75 seconds
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimitLegacy = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;

        consensus.nRuleChangeActivationThreshold = 633; //  258 75% for testchains
        consensus.nMinerConfirmationWindow = 844; // Faster than normal for regtest (144 instead of 2016)

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
        consensus.defaultAssumeValid = uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206");

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

        genesis = CreateGenesisBlock_legacy(1296688602, 2, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));


        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = (CCheckpointData) {
            {
                {0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")},

            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

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
