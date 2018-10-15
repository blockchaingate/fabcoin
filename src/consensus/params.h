// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FABCOIN_CONSENSUS_PARAMS_H
#define FABCOIN_CONSENSUS_PARAMS_H

#include <uint256.h>
#include <map>
#include <string>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    std::string strNetworkID;

    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /** Block height at which BIP65 becomes active */
    int BIP65Height;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
    /** Block height at which Fabcoin Equihash hard fork becomes active */
    uint32_t FABHeight;
    /** Block height at which LWMA becomes active */
    uint32_t LWMAHeight;
    /** Block height at which Fabcoin Smart Contract hard fork becomes active */
    uint32_t ContractHeight;
    /** Block height at which EquihashFAB (184,7) becomes active */
    uint32_t EquihashFABHeight;
    /** Limit BITCOIN_MAX_FUTURE_BLOCK_TIME **/
    int64_t MaxFutureBlockTime;
    /** Block height before which the coinbase subsidy will be locked for the same period */
    int CoinbaseLock;
    /** whether segwit is active */
    bool ForceSegwit;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];

    /** Proof of work parameters */
    uint256 powLimit;
    uint256 powLimitLegacy;

    const uint256& PowLimit(bool postfork) const { return postfork ? powLimit : powLimitLegacy; }
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetTimespan;
    int64_t nPowTargetSpacing;

    int64_t DifficultyAdjustmentInterval(uint32_t nheight=0) const { return nPowTargetTimespan / (nheight<EquihashFABHeight?nPowTargetSpacing:2*nPowTargetSpacing); }
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;
    
    int nFixUTXOCacheHFHeight; //fasc

    // Params for Zawy's LWMA difficulty adjustment algorithm.
    int64_t nZawyLwmaAveragingWindow;
    bool bZawyLwmaSolvetimeLimitation;
    uint8_t MaxBlockInterval;

    //Digishield logic for difficulty adjustment
    int64_t nDigishieldPowAveragingWindow;
    int64_t nDigishieldPowMaxAdjustDown;
    int64_t nDigishieldPowMaxAdjustUp;
    int64_t DigishieldAveragingWindowTimespan(uint32_t nheight=0) const { return nDigishieldPowAveragingWindow * (nheight<EquihashFABHeight?nPowTargetSpacing:2*nPowTargetSpacing); }
    int64_t DigishieldMinActualTimespan(uint32_t nheight=0) const { return (DigishieldAveragingWindowTimespan(nheight) * (100 - nDigishieldPowMaxAdjustUp  )) / 100; }
    int64_t DigishieldMaxActualTimespan(uint32_t nheight=0) const { return (DigishieldAveragingWindowTimespan(nheight) * (100 + nDigishieldPowMaxAdjustDown)) / 100; }

};
} // namespace Consensus

#endif // FABCOIN_CONSENSUS_PARAMS_H
