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
    //txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

    txNew.vin[0].scriptSig = CScript() << 00 << 488804799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

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

const arith_uint256 maxUint = UintToArith256(uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.strNetworkID = "main";
 
        consensus.nSubsidyHalvingInterval = 3360000;
        consensus.FABHeight = 0;
        consensus.ContractHeight = 200000;
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
        consensus.defaultAssumeValid = uint256S("0x03292eeaf1835cffc791e15b05f558adc080f85b3c54e351c1010a2699d2fc48");

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
            1522249768,
            uint256S("0x000000000000000000000000000000000000000000000000000000000000007f"),
            ParseHex("001895e4c29529f904cf86613cdbddc709a5deb8122360c532ab93dd776a4ad816af47c4cb62ab3ba8840dc4a99373a991f1a56bb25983cab978f4bd3518671872b44ed606b884e8fa94c439f7a9a9be273c929d0116f7dd750b18474139c1d3eb2bb0d1ea467c95120d5ecf91b223f9c9d02f7306076851be1ab0bb4dd40d7a44aa45180cc2caba1302a725fc8e332df520d35f61e429bba2373398448695d43c40dad5459bbae8016aaa0a7259a4f7cd5d938a6bf3d52166c8945a3620775e1ca7cbceab00265791e6eb55db14cadb28f104113f346b8f8a53897d15a73678d8fa94421da59831a0ea3c331ed1150d3c891ed67b0a2287e0fde72d09a1dc008989428f954918932162245ad49d17a52b4c02db629d582b3ef66f77963de0780a57af3b661a0a158b1790e25395bb7a2116405564c1216eac80e7296f364edd574551ddff52e8bb198b4f3fe61c144b034a8ecd959856f3e9ad223c371655e95a08bc56be121eb9e4ab88c8598c71b2214394d02a127f14806119129eff36d2d206b199744d90ad7b966a6a7c0fdc319ca381f5964061c5b6a64ba5d6d3af09fc1871ea0fbe753e34147f32f0a0e1f9acbb5d4e8713f82c8a15432b76039a7d2fb3b511df46bfb65aa1acb63b911b5da25878e8e20d5b2ad23fcc7f04b349175ad1ff1cd46725116d2f2ded4f23dd02ba5e31258ba9868104c657133058db5ccf53f1dfdfd40d48cc6847891c070d8beb06a0835fdf161962837263eaf83e79a2c405cd4233cc964d159e0fc2aad4ed62111dfc2bd5252763d3a719b4237fcb865a56ecd2cc836cb45deb76072dafa5bfc2f6c32837251e90d34e35624258705d2fc1eede3017b3bf7f34c3597021cf69ee3ddfe70509df35aff0207bf7a9aff44d6ea3574e61ce37c3c02ff992db7f904ffb2424f43d18528f1e169875baf001e78645d1e93549fca822cce874b9bd72de14bcc908b83be8436307b56bcc37766051672f0a465fb6fd0b01cab3a3149ad19b7ab11d1968111db04c78124e57e33c73305796ccce1376b697cb9b55cd455a07cd01f5da47bf41a8e23ef8c026da68b308f5054df66a2efce3f12f4ed1354c72e6954ef09addb7b953eec90e91ce6ee4ced42339741357e859ed667bb01de632229ef738bed0434d946ba2745551cc6ca824fbd4b906843ff45be9c2a9ee8e1127405d0665c6090ee5092a46d42429e7b6696dd6937d65697edea8c9b82a9617edc717abcf5bdfb047f45a7b4ab509b06155215926855fcf825b14b51a63a322aa7556691799b672ff150f1fda6d280f35d657b257a6c04eb674779937606e24c64294a2c35d8b9738bc2d753b0b3f595f1455196c2c8ca11bd7bd86d523d5f4441656acc79a31b02db3e3936e58aa84cfe3463baf32cf09dc8839fee402f11c46091accc1786340f67d4e49c480eb99c5f218a5d31cf395f4312aaef38a26ed219df655d088200c5a51be2e55603dc04d54b68de3d2f26ac0742f6553578db6e3e4982d37297564e97af665fdd638bf0d05f4c17a4fc8d3abcf7ab329a7a553a92daf38a522383c77a2b2cfd56d6d06a89a6a575fc73ba67d342e1149dcea061564b92295c4080fe525b60a1b19ba8d442815321817a7c57cdb47767ebe3a425054d492a005d7ac6922cd615678c9c4a05b2698a61dd0fb17c1775a4e2675ac5011b762b8c3175cab6a8e8679cffb0a5a466e679126c3ce54d259d94a9cf6d2b73b99031aef95081ed2a602c008f499c3d95a1de1dd3f4aba0b5dbb6dac950f75a455b1e68d237569965a8cdd380ceaa32e8b9792f3c0df736c25382be58360f511f517ca90fbbce898a198c964b4cace26ead3d6bce7f427ddd73f6fa84c05674426404ce13daa228a563d50"),
            0x2007ffff, 1, 25 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S("0x03292eeaf1835cffc791e15b05f558adc080f85b3c54e351c1010a2699d2fc48"));
        assert(genesis.hashMerkleRoot == uint256S("0xe89cf9aeaa7b90a0c6695946bf23a41447c4ade44a63e6fa40481e35957e69aa"));

        // Note that of those with the service bits flag, most only support a subset of possible options
        vSeeds.emplace_back("dnsseed.fabnetwork.info", true);  // only supports x1, x5, x9, and xd
        vSeeds.emplace_back("dnsseed.fabcoin.club", true);  // only supports x1, x5, x9, and xd
        vSeeds.emplace_back("dnsseed1.fabcoin.club", true);  // only supports x1, x5, x9, and xd
        vSeeds.emplace_back("dnsseed.fabcoin.biz", true);  // only supports x1, x5, x9, and xd

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
                { 0,    uint256S("0x03292eeaf1835cffc791e15b05f558adc080f85b3c54e351c1010a2699d2fc48")},            
                { 1388, uint256S("0x000048a31f2269d3dbc2a9c70d39eeeb155f435f00f65a0681cedc558f05ecd2")},            
                { 1398, uint256S("0x00017f30bd3c67005fb37d7c9c3d31df571699710126ae23137bddaee9d4ae70")},            
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
        consensus.strNetworkID = "test";

        consensus.nSubsidyHalvingInterval = 3360000;
        consensus.FABHeight = 0;
        consensus.ContractHeight = 200000 ;
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

        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testnet-seed.fabnetwork.info", true); 

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

//        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

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
        consensus.strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)

        consensus.BIP34Height = 0; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests) // activate for fabcoin
        consensus.BIP34Hash = uint256S("0x0e28ba5df35c1ac0893b7d37db241a6f4afac5498da89369067427dc369f9df3");
        consensus.BIP65Height = 0; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 0; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        consensus.nPowTargetTimespan = 1.75 * 24 * 60 * 60; // 1.75 days
        consensus.nPowTargetSpacing = 1.25 * 60; // 75 seconds
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimitLegacy = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;

        consensus.FABHeight = 1000000;
        consensus.ContractHeight = 0;
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
        consensus.defaultAssumeValid = uint256S("0x0e28ba5df35c1ac0893b7d37db241a6f4afac5498da89369067427dc369f9df3");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 38665;
        nPruneAfterHeight = 1000;
      
        const size_t N = 48, K = 5;
        BOOST_STATIC_ASSERT(equihash_parameters_acceptable(N, K));
        nEquihashN = N;
        nEquihashK = K;

/*
for (int i=0;i<20;i++) {
        genesis = CreateGenesisBlock_legacy(1296688602, i, 0x207fffff, 5, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        std::cout << consensus.hashGenesisBlock.GetHex() << std::endl; 
}
*/

        genesis = CreateGenesisBlock_legacy(1296688602, 12, 0x207fffff, 5, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash(consensus);
        assert(consensus.hashGenesisBlock == uint256S("0x0e28ba5df35c1ac0893b7d37db241a6f4afac5498da89369067427dc369f9df3"));
        assert(genesis.hashMerkleRoot == uint256S("0x487e51f691fd29b1ee5c7fc3341eb4f91b86f4a8eace12d259f89f70af558ee1"));

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
                {0, uint256S("0e28ba5df35c1ac0893b7d37db241a6f4afac5498da89369067427dc369f9df3")},

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
 * Regression network parameters overwrites for unit testing
 */
class CUnitTestParams : public CRegTestParams
{
public:
    CUnitTestParams()
    {
        strNetworkID = "unittest";
        consensus.strNetworkID = "unittest";

        // Activate the the BIPs for regtest as in Fabcoin
        consensus.nSubsidyHalvingInterval = 850;
        consensus.nRuleChangeActivationThreshold = 633; //  258 75% for testchains
        consensus.nMinerConfirmationWindow = 844; // Faster than normal for regtest (144 instead of 2016)

        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.FABHeight = 24000;
        consensus.CoinbaseLock = 0;
        consensus.ForceSegwit = false;
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
    else if (chain == CBaseChainParams::UNITTEST)
        return std::unique_ptr<CChainParams>(new CUnitTestParams());

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
