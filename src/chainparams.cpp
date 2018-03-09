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
    // todo - height is not in genesis 
    //txNew.vin[0].scriptSig = CScript() << 00 << 520617983 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vin[0].scriptSig = CScript() <<  520617983 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

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
 
        consensus.nSubsidyHalvingInterval = 1680000;
        consensus.FABHeight = 0;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835");
        consensus.BIP65Height = 0; 
        consensus.BIP66Height = 0; 
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
        consensus.defaultAssumeValid = uint256S("0x00dfd158881d90583835516f775ae2e6c49b62eb39fccb251e787c85ee84b994");

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
        
        const size_t N = 200, K = 9;
        BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
        nEquihashN = N;
        nEquihashK = K;

        // 1517433514 2018.1.31
        genesis = CreateGenesisBlock(
            1517433514,
            uint256S("0x0000000000000000000000000000000000000000000000000000000000000014"),
            ParseHex("0108353b75c2f99d7f22e07eecdb85e6207c3312172a5cdaeb6ed5a681e463195605628f22ed949953460c6fcc244b04a0328ee982e148c456c22e86dd7a951cac6564bc3412afd4ae944777d408316a675e8761018d18911ca1728f86ad511dc948dba6d8c616e757086fc5bf5b039e534b2fd2977ed7f1b1a9bef7454321ec33de9fd98adae53b340a826391a63c479ea524386a3fed2610b07a980465dc01c2fa73200bff2ccb06da08384bf59e43f93df0cf106d1f1380bcbffb0e0d54627bdf3e1323f9c581b7aef82bd593075f88ba07b544214e229d9963554a01ff6f3db32c88dadabf1258b937db62a86124cb929a57c50151048eeec0bb1430b0a4afd08c8f933a464f5144b286533cde5401236a8765fe0f4699dd0cb6cb034fb795c310ae3e8b1b16c18a3657d987dbac72449a27d3d9e49ada9fdc2da2fdda94da8ccfcd57b52034c0aff9ceac7dcb0f032fcab5c49fce135121a0836bb5bc6072e4adb707046d6d50639406b92a04b06ce87ec22ad6bc789a541a8d9c0c37ed322d8853a630cfb60ebef7a33d42e32cda66801f1c01d7b1f5180e02c0f2c281717a57031926146012ed8139b3ec8439b1e4c836fb201b93ea2278dd9a9aa98b9f9510648f20b51e354e4d8feaad20e5d7b6c3f8c7c9edc5c300e3acb8d23875d1e7a433f40efe4551062d1dfb84de69b3620f33d4dfb010048db2b9675c7b5f811ef31a741fdbbecb49babed00acd770ba043fcc0855815872fb1988e29ac37bebe0b7bb5f20665ad4b6eec822de3fe2e9e0ba83d7f4f2b8d3dd4fe25b41d4231d4fa31e3c6ddff63de124a0575b6b4df1a5311896255a898ea84fd7292769ea4123f0a2ef85d65bceea0e162b59c18d4e4379154460fc28d30f2cf4d1f3761384c00dbf6d699d05bebd92005038271cc367892a66788406e02fdfaf7b8f6480229894f9c09a27272b0a386c55abe8e4ecd5a04f50f095d2f5a87a9a77fbeb36627d9280136d07570530529c7866956e6238959a800246cf986f6e4b897960be5f439ce60a3cda1266ab57cdb7cbeff5f5e0ce7043450967d634b4da8ffe0ca0ce7ed6ceafe312aa22900868b458db6d29216090e684d34c34a92bf92a10e67cbc4ab03bbf683f342d69a76cabd3ae26b15832ae15496954c02e9f7175417feaaeb129993bf6ff506f77cc1ced0b6a8dfd706d9ba6d061ac31b5ce36c15f35ad1ec0a03987dcf9657d8ffb087312a1bce882c3c7177302e15b58ac2e7e6b7e6fcd2360c3a772c592d03749018eeed9bdbf63c3b5c11cad5f1b853460821072dbf04ddc6d4e9543038a7c185a8263e34fa090161dda89c4b99600cd43d7c614eed5e1a5552f20c2055b9d725df6f89a31233e99905b92d35eb723519c26e27d95f2eb511ab32f757983bfe1e0b932770033b6b574e0d51896e7f016a721c6dc89286e987ef0889d2d31deaac558b2d2783e75c9a567c0f7573d457ac5de5cb189f71a735d6396d7cd8cded56d9f57e7e31ffcb3666ee05fc91cd018774b81b40ca1c09e70f005a1a2b8e30a0c84cc2f0cdb77f30dc6d87357d13ad8389562f26e3c86b7431bb3b11b6fad4d86a99318034395ca981794d1695575933530dc26730eaf3797a1c1548a2f471b5efe7afadd1494b32d1ba2799041ad7ed4890d8b4f69675df68ccdc7a4f5ebf3d6553f79e9146de2bad16339764eeece7aa362999ff460a343c9d6b59a1eaf95a80d0c09684a5886ddf273945dc8ae892eaba19db38580d22e09b8f3b8cbd396d07f4d5e46fb8b633fe2b83467571893dd3f5b62ebf10782d316754bbd8d16bb6621a44e5aa0431bd4a1d1cfc3effcad28243a052a210ca7f9461560d3997012d57239c5321668b3aa56486e15b73be6bf75363ce"),
            0x2007ffff, 1, 50 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S("0x00dfd158881d90583835516f775ae2e6c49b62eb39fccb251e787c85ee84b994"));
        assert(genesis.hashMerkleRoot == uint256S("0x7b4992742919dc326f297b549df082cf4b4c88f2a52f1987a06826e600d4479e"));

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
                { 0, uint256S("0x00dfd158881d90583835516f775ae2e6c49b62eb39fccb251e787c85ee84b994")},            }
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

        consensus.nSubsidyHalvingInterval = 1680000;
        consensus.FABHeight = 0;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835");
        consensus.BIP65Height = 0; 
        consensus.BIP66Height = 0; 
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
        consensus.defaultAssumeValid = uint256S("0x00dfd158881d90583835516f775ae2e6c49b62eb39fccb251e787c85ee84b994"); 

        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        nDefaultPort = 18665;
        nPruneAfterHeight = 1000;

        const size_t N = 200, K = 9;
        BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
        nEquihashN = N;
        nEquihashK = K;

        // 1517433514 2018.1.31
        genesis = CreateGenesisBlock(
            1517433514,
            uint256S("0x0000000000000000000000000000000000000000000000000000000000000014"),
            ParseHex("0108353b75c2f99d7f22e07eecdb85e6207c3312172a5cdaeb6ed5a681e463195605628f22ed949953460c6fcc244b04a0328ee982e148c456c22e86dd7a951cac6564bc3412afd4ae944777d408316a675e8761018d18911ca1728f86ad511dc948dba6d8c616e757086fc5bf5b039e534b2fd2977ed7f1b1a9bef7454321ec33de9fd98adae53b340a826391a63c479ea524386a3fed2610b07a980465dc01c2fa73200bff2ccb06da08384bf59e43f93df0cf106d1f1380bcbffb0e0d54627bdf3e1323f9c581b7aef82bd593075f88ba07b544214e229d9963554a01ff6f3db32c88dadabf1258b937db62a86124cb929a57c50151048eeec0bb1430b0a4afd08c8f933a464f5144b286533cde5401236a8765fe0f4699dd0cb6cb034fb795c310ae3e8b1b16c18a3657d987dbac72449a27d3d9e49ada9fdc2da2fdda94da8ccfcd57b52034c0aff9ceac7dcb0f032fcab5c49fce135121a0836bb5bc6072e4adb707046d6d50639406b92a04b06ce87ec22ad6bc789a541a8d9c0c37ed322d8853a630cfb60ebef7a33d42e32cda66801f1c01d7b1f5180e02c0f2c281717a57031926146012ed8139b3ec8439b1e4c836fb201b93ea2278dd9a9aa98b9f9510648f20b51e354e4d8feaad20e5d7b6c3f8c7c9edc5c300e3acb8d23875d1e7a433f40efe4551062d1dfb84de69b3620f33d4dfb010048db2b9675c7b5f811ef31a741fdbbecb49babed00acd770ba043fcc0855815872fb1988e29ac37bebe0b7bb5f20665ad4b6eec822de3fe2e9e0ba83d7f4f2b8d3dd4fe25b41d4231d4fa31e3c6ddff63de124a0575b6b4df1a5311896255a898ea84fd7292769ea4123f0a2ef85d65bceea0e162b59c18d4e4379154460fc28d30f2cf4d1f3761384c00dbf6d699d05bebd92005038271cc367892a66788406e02fdfaf7b8f6480229894f9c09a27272b0a386c55abe8e4ecd5a04f50f095d2f5a87a9a77fbeb36627d9280136d07570530529c7866956e6238959a800246cf986f6e4b897960be5f439ce60a3cda1266ab57cdb7cbeff5f5e0ce7043450967d634b4da8ffe0ca0ce7ed6ceafe312aa22900868b458db6d29216090e684d34c34a92bf92a10e67cbc4ab03bbf683f342d69a76cabd3ae26b15832ae15496954c02e9f7175417feaaeb129993bf6ff506f77cc1ced0b6a8dfd706d9ba6d061ac31b5ce36c15f35ad1ec0a03987dcf9657d8ffb087312a1bce882c3c7177302e15b58ac2e7e6b7e6fcd2360c3a772c592d03749018eeed9bdbf63c3b5c11cad5f1b853460821072dbf04ddc6d4e9543038a7c185a8263e34fa090161dda89c4b99600cd43d7c614eed5e1a5552f20c2055b9d725df6f89a31233e99905b92d35eb723519c26e27d95f2eb511ab32f757983bfe1e0b932770033b6b574e0d51896e7f016a721c6dc89286e987ef0889d2d31deaac558b2d2783e75c9a567c0f7573d457ac5de5cb189f71a735d6396d7cd8cded56d9f57e7e31ffcb3666ee05fc91cd018774b81b40ca1c09e70f005a1a2b8e30a0c84cc2f0cdb77f30dc6d87357d13ad8389562f26e3c86b7431bb3b11b6fad4d86a99318034395ca981794d1695575933530dc26730eaf3797a1c1548a2f471b5efe7afadd1494b32d1ba2799041ad7ed4890d8b4f69675df68ccdc7a4f5ebf3d6553f79e9146de2bad16339764eeece7aa362999ff460a343c9d6b59a1eaf95a80d0c09684a5886ddf273945dc8ae892eaba19db38580d22e09b8f3b8cbd396d07f4d5e46fb8b633fe2b83467571893dd3f5b62ebf10782d316754bbd8d16bb6621a44e5aa0431bd4a1d1cfc3effcad28243a052a210ca7f9461560d3997012d57239c5321668b3aa56486e15b73be6bf75363ce"),
            0x2007ffff, 1, 50 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S("0x00dfd158881d90583835516f775ae2e6c49b62eb39fccb251e787c85ee84b994"));
        assert(genesis.hashMerkleRoot == uint256S("0x7b4992742919dc326f297b549df082cf4b4c88f2a52f1987a06826e600d4479e"));

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
                {0, uint256S("0x00dfd158881d90583835516f775ae2e6c49b62eb39fccb251e787c85ee84b994")},
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
        consensus.defaultAssumeValid = uint256S("0x04afce71be4bfe0e3558ecaedd57ae986cf700ad90777fa599fb131ce3a4960a");

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

