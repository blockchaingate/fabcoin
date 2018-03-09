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
        consensus.powLimit = uint256S("07ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

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
        consensus.defaultAssumeValid = uint256S("0x06c40d7f829cd71181b079f33f25141971d8320bfe31ed45f3ecd87cb65bfd0f");

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
            uint256S("0x000000000000000000000000000000000000000000000000000000000000005e"),
            ParseHex("0115e6ccd7186a8338625a9544d5e7eed224b7e2a20f4468d14884065919e1b43bdb49b0da7d66d5cbc111ad7387e445b42275aa6202b22480d887b115ec7d1ef8cd565312e92cff6773f84aa355cf951fbfcf3109c734e942087a396cca364671d9c2c609bd90e35b139c25f1e7673535a80eb6473748779dd3a9ff1d762a53be02529fd29d5704d7a7e564970e233532b014362e3edc61758e8de70ba47d5c797705822e5030540177a4531d0d257f1c9e8189437da6787901c4504a3b33ba38236391315c2e95649ef7c6730e8cf8ed9a04898cc0dc8c24f73cbe92c4a473ea6f7e49ff5395159843173ed88f31d45dd1d67a2e2abec2531ea7c7150aba38ab071fda76ab68858a63d5865604dee4112ba02aaa0b55f5638a3ff35064e82bb3bc40df1b2918fed2568be471d793e7195d21dd7a76c13718a7772a3a271f078d85af41e9e6440f5066268956f4cb8b01457f6a18d15ca39f03605ddfdf05001f28f5ea980381e0c528d19f78ae9cd7393474577a1b0f373565224516a5f5d5c1c5e939e6dcd658e307749d1dc93f46537cc0f51501add8e5e8458153ca9a8c15ff06ac08e31cdd73c25a8b1d9a95dd4b4115a5d2debb075414946301f2adf35fc1af7225eb7a41e36de9bf22950c797c0892c86b59166434bc08bfd20230e3b800ce24d41bed3c0e6315de262396cdcf037d0577bd418c0737e3605fe62bad418f68a187d7d49629fd3a42ef0d7fb551fcd27d59016883b84e7ad1e55d2796c64c2771930e6d4a0b94bc3222a1b5c28a027c1b7c250635badb529daff49df03d5558e95b687e4636bbf6da0954e91218084184468ae3fab4319ff1fb38f1a7615be9d46faf2f4673e60bb9668bf9181aa2803a6b7619b01dc4e3659a29887fc50c3dae943d6bf03f8b3522ec2170009f0293998d330670ab51d9169d295b4901ebb6c9d6b19573bcd2b90debebc5be5f7b35e6761b7541a88c4a83ece46021eaad37a454bff9f77ef807dd4650ae0ab638d9a62160ce22da7ee52617ef840ac96e6088db38b924b384c99c2d262a7ecf7cb59604773db4235854a706b365a5b15d00b25794dbe95508a01eae59039d1a35c10578ab46b642b1c49fec0610b12fa10df2fce5afb285367f56540dbec7966d6a21cd9270ec2028e7f550154e6d7cd64d5b0254b3df0426d3d6c0b77ecdd20fa1317b62d864d1e94ecd96242986f4c4ea77a5b33617dea272a85b5acddceb493286efe0435fef691fbf646921b1a7118dea38440c514c760020e5618791c3d5746bb2f7feba285ef05909636be062c3cddb0a8893dddd4e2fbd6ef9141528378afba14a6af1ed6771f768017c048e5f60ba8b6b0dd1253271a7e127ccdfc772d4c0455a69ec1db21635deedbba1cdb20bda90b43bab761af11ec4ef45660254d2ade67024b9cd8ed27e1964f5eeb9ac362de41d149a74b490c44089979852255ed85308a9bf70440aa2526055c9ad2ebf19913a32e16349802b6dadec0dd6fac96d8944806899d6fc7ebf5cf273fc7431100472a74c35597115d1578558f5e148c77cdebfa3112b8c7d1b57103ee7f305c705b2de9c59f3949c6ea808639ba23c0c79ef318720c3147612ed5e5f52555c0b31fe41e8cf2e2686ff31e993ae8df8fc5a7c58d605eb95fdbb28f669e5730141cd19803e48ff7a941511d8a1f1ce21548dac55712dcf0b56110bd10d816621e2d959cb1ac933844bd591aec163ab1aeffe1e9635ea7a9d3e8e1a52912a9525443168530b9fb8a05b086d0f08b80c65c1cb3da0e0e7c841ed5fd8751f0e1d5194296713987ef471e35b1dbd4235092e9605c5124a7cf55650830494243136752c1838b14e56f654216d72771c2f8567d40bb75290fb577a6253bcb2a4"),
            0x2007ffff, 1, 25 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x06c40d7f829cd71181b079f33f25141971d8320bfe31ed45f3ecd87cb65bfd0f"));
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
                { 0, uint256S("0x06c40d7f829cd71181b079f33f25141971d8320bfe31ed45f3ecd87cb65bfd0f")},            }
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
        consensus.nSubsidyHalvingInterval = 1680000; // fabcoin halving every 4 years
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0001cfb309df094182806bf71c66fd4d2d986ff2a309d211db602fc9a7db1835");
        consensus.BIP65Height = 0; 
        consensus.BIP66Height = 0; 
        consensus.powLimit = uint256S("07ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
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
        consensus.defaultAssumeValid = uint256S("0x06c40d7f829cd71181b079f33f25141971d8320bfe31ed45f3ecd87cb65bfd0f"); 

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
            uint256S("0x000000000000000000000000000000000000000000000000000000000000005e"),
            ParseHex("0115e6ccd7186a8338625a9544d5e7eed224b7e2a20f4468d14884065919e1b43bdb49b0da7d66d5cbc111ad7387e445b42275aa6202b22480d887b115ec7d1ef8cd565312e92cff6773f84aa355cf951fbfcf3109c734e942087a396cca364671d9c2c609bd90e35b139c25f1e7673535a80eb6473748779dd3a9ff1d762a53be02529fd29d5704d7a7e564970e233532b014362e3edc61758e8de70ba47d5c797705822e5030540177a4531d0d257f1c9e8189437da6787901c4504a3b33ba38236391315c2e95649ef7c6730e8cf8ed9a04898cc0dc8c24f73cbe92c4a473ea6f7e49ff5395159843173ed88f31d45dd1d67a2e2abec2531ea7c7150aba38ab071fda76ab68858a63d5865604dee4112ba02aaa0b55f5638a3ff35064e82bb3bc40df1b2918fed2568be471d793e7195d21dd7a76c13718a7772a3a271f078d85af41e9e6440f5066268956f4cb8b01457f6a18d15ca39f03605ddfdf05001f28f5ea980381e0c528d19f78ae9cd7393474577a1b0f373565224516a5f5d5c1c5e939e6dcd658e307749d1dc93f46537cc0f51501add8e5e8458153ca9a8c15ff06ac08e31cdd73c25a8b1d9a95dd4b4115a5d2debb075414946301f2adf35fc1af7225eb7a41e36de9bf22950c797c0892c86b59166434bc08bfd20230e3b800ce24d41bed3c0e6315de262396cdcf037d0577bd418c0737e3605fe62bad418f68a187d7d49629fd3a42ef0d7fb551fcd27d59016883b84e7ad1e55d2796c64c2771930e6d4a0b94bc3222a1b5c28a027c1b7c250635badb529daff49df03d5558e95b687e4636bbf6da0954e91218084184468ae3fab4319ff1fb38f1a7615be9d46faf2f4673e60bb9668bf9181aa2803a6b7619b01dc4e3659a29887fc50c3dae943d6bf03f8b3522ec2170009f0293998d330670ab51d9169d295b4901ebb6c9d6b19573bcd2b90debebc5be5f7b35e6761b7541a88c4a83ece46021eaad37a454bff9f77ef807dd4650ae0ab638d9a62160ce22da7ee52617ef840ac96e6088db38b924b384c99c2d262a7ecf7cb59604773db4235854a706b365a5b15d00b25794dbe95508a01eae59039d1a35c10578ab46b642b1c49fec0610b12fa10df2fce5afb285367f56540dbec7966d6a21cd9270ec2028e7f550154e6d7cd64d5b0254b3df0426d3d6c0b77ecdd20fa1317b62d864d1e94ecd96242986f4c4ea77a5b33617dea272a85b5acddceb493286efe0435fef691fbf646921b1a7118dea38440c514c760020e5618791c3d5746bb2f7feba285ef05909636be062c3cddb0a8893dddd4e2fbd6ef9141528378afba14a6af1ed6771f768017c048e5f60ba8b6b0dd1253271a7e127ccdfc772d4c0455a69ec1db21635deedbba1cdb20bda90b43bab761af11ec4ef45660254d2ade67024b9cd8ed27e1964f5eeb9ac362de41d149a74b490c44089979852255ed85308a9bf70440aa2526055c9ad2ebf19913a32e16349802b6dadec0dd6fac96d8944806899d6fc7ebf5cf273fc7431100472a74c35597115d1578558f5e148c77cdebfa3112b8c7d1b57103ee7f305c705b2de9c59f3949c6ea808639ba23c0c79ef318720c3147612ed5e5f52555c0b31fe41e8cf2e2686ff31e993ae8df8fc5a7c58d605eb95fdbb28f669e5730141cd19803e48ff7a941511d8a1f1ce21548dac55712dcf0b56110bd10d816621e2d959cb1ac933844bd591aec163ab1aeffe1e9635ea7a9d3e8e1a52912a9525443168530b9fb8a05b086d0f08b80c65c1cb3da0e0e7c841ed5fd8751f0e1d5194296713987ef471e35b1dbd4235092e9605c5124a7cf55650830494243136752c1838b14e56f654216d72771c2f8567d40bb75290fb577a6253bcb2a4"),
            0x2007ffff, 1, 25 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x06c40d7f829cd71181b079f33f25141971d8320bfe31ed45f3ecd87cb65bfd0f"));
        assert(genesis.hashMerkleRoot == uint256S("0xafcaf52027ac3c032eda00c018bde8996dfec7523d88d028f0fd188a35b01b06"));

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
                {0, uint256S("0x06c40d7f829cd71181b079f33f25141971d8320bfe31ed45f3ecd87cb65bfd0f")},
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
            uint256S("0x000000000000000000000000000000000000000000000000000000000000003b"),
            ParseHex("057067f9f08ebd59ef0854d3cd130e290dee1bf0d3eed28d7141a12b47112f35cbde8386"),
            0x2007ffff, 1, 25 * COIN );
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x023dac02cc4987c6bc5b8f7d37fc8cbd8381ec75a8d16a3eb9e0228b645915a4"));
        assert(genesis.hashMerkleRoot == uint256S("afcaf52027ac3c032eda00c018bde8996dfec7523d88d028f0fd188a35b01b06"));

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00d0afe208ed97ccf503adbda469e7feddc0eebae96b67ea54a13792873e3c4d");

        checkpointData = (CCheckpointData) {
            {
               {0, uint256S("0x023dac02cc4987c6bc5b8f7d37fc8cbd8381ec75a8d16a3eb9e0228b645915a4")},
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

