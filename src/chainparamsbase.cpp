// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <tinyformat.h>
#include <util.h>

#include <assert.h>

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::REGTEST = "regtest";
const std::string CBaseChainParams::REGTESTWITHNET = "regtestwithnet";
const std::string CBaseChainParams::UNITTEST = "unittest";
std::string CBaseChainParams::kanbanId = "";
std::vector<unsigned char> CBaseChainParams::kanbanIdBytes;

void AppendParamsHelpMessages(std::string& strUsage, bool debugHelp)
{
    strUsage += HelpMessageGroup(_("Chain selection options:"));
    strUsage += HelpMessageOpt("-testnet", _("Use the test chain"));
    if (debugHelp) {
        strUsage += HelpMessageOpt("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                                   "This is intended for regression testing tools and app development.");
    }
}

/**
 * Main network
 */
class CBaseMainParams : public CBaseChainParams
{
public:
    CBaseMainParams()
    {
        nRPCPort = 8667;
    }
};

/**
 * Testnet (v3)
 */
class CBaseTestNetParams : public CBaseChainParams
{
public:
    CBaseTestNetParams()
    {
        nRPCPort = 18667;
        strDataDir = "testnet3";
    }
};

/*
 * Regression test
 */
class CBaseRegTestWithNetParams : public CBaseChainParams
{
public:
    CBaseRegTestWithNetParams()
    {
        nRPCPort = 40667;
        strDataDir = "regtestwithnet";
    }
};

/*
 * Regression test
 */
class CBaseRegTestParams : public CBaseChainParams
{
public:
    CBaseRegTestParams()
    {
        nRPCPort = 38667;
        strDataDir = "regtest";
    }
};

static std::unique_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CBaseChainParams>(new CBaseMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CBaseChainParams>(new CBaseTestNetParams());
    else if (chain == CBaseChainParams::REGTESTWITHNET)
        return std::unique_ptr<CBaseChainParams>(new CBaseRegTestWithNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CBaseChainParams>(new CBaseRegTestParams());
    else if (chain == CBaseChainParams::UNITTEST)
        return std::unique_ptr<CBaseChainParams>(new CBaseRegTestParams());
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectBaseParams(const std::string& chain)
{
    globalChainBaseParams = CreateBaseChainParams(chain);
}

std::string ChainNameFromCommandLine()
{
    bool fRegTest = gArgs.GetBoolArg("-regtest", false);
    bool fRegTestWithNet = gArgs.GetBoolArg("-regtestwithnet", false);
    bool fTestNet = gArgs.GetBoolArg("-testnet", false);
    int numNets = ((int) fRegTest) + ((int) fRegTestWithNet) + ((int) fTestNet);
    if (numNets > 1)
        throw std::runtime_error("Too many networks selected(-regtest, -regtestwithnet, -testnet).");
    if (fRegTest)
        return CBaseChainParams::REGTEST;
    if (fRegTestWithNet)
        return CBaseChainParams::REGTESTWITHNET;
    if (fTestNet)
        return CBaseChainParams::TESTNET;
    return CBaseChainParams::MAIN;
}
