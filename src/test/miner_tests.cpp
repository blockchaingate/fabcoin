// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "coins.h"
#include "consensus/consensus.h"
#include "consensus/merkle.h"
#include "consensus/tx_verify.h"
#include "consensus/validation.h"
#include "validation.h"
#include "miner.h"
#include "policy/policy.h"
#include "pubkey.h"
#include "script/standard.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"

#include "test/test_fabcoin.h"

#include <memory>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(miner_tests, TestingSetup)

static CFeeRate blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);

static BlockAssembler AssemblerForTest(const CChainParams& params) {
    BlockAssembler::Options options;

    options.nBlockMaxWeight = MAX_BLOCK_WEIGHT;
    options.nBlockMaxSize = MAX_BLOCK_SERIALIZED_SIZE;
    options.blockMinFeeRate = blockMinFeeRate;
    return BlockAssembler(params, options);
}

static
struct {
    unsigned char extranonce;
    const char *nonce_hex;
    const char *solution_hex;
} blockinfo[] = {
{ 4, "0000000000000000000000000000000000000000000000000000000000006f8e", "00ad453045e66e23f21bb988aaf8155b85fc1f18115238448a8d852ebf369ce705621dc4" }, 
{ 2, "0000000000000000000000000000000000000000000000000000000000001fd7", "0c2dd014b54ce5af3a112e049601a39ccba015f5ce9c23bc00faf91b50d7bee354850da5" }, 
{ 1, "0000000000000000000000000000000000000000000000000000000000001dff", "04f4d336013535772c10f5d6bf053d5d6d8508b4b93d859421a1d217dcf9fd03bd51f4ff" }, 
{ 1, "0000000000000000000000000000000000000000000000000000000000009602", "0175c355c1afc8777111244c5f36ec11f7ff3c435c0e99f53e89ee4033977a94c42b29a0" }, 

{ 2, "0000000000000000000000000000000000000000000000000000000000000274", "06c50c8ad5d361b51c1d22d08c63c4ce53e2073b983b3133590cff1f628c1c826ed192d3" },
{ 2, "000000000000000000000000000000000000000000000000000000000000035f", "012bc79db3a571d9dd060e213c7a2d1b25e60a7828fd62560147982269533a94b331e54c" }, 

{ 1, "0000000000000000000000000000000000000000000000000000000000002886", "00e3d779e2139d7fe212d1d5d9a31d4afdef264b1b1ca3452120cb6cbd2297d857c2b3c8" }, 
{ 2, "0000000000000000000000000000000000000000000000000000000000001505", "00b5eef92191ecf5dc1ee14deb64eac1775b0390521fa4c465d1ea29da6dfd5367de21fe" }, 
{ 2, "00000000000000000000000000000000000000000000000000000000000042d0", "007488f9f1fe96452401a1ca25e33628d4bc0d1f239a6466c5a8e4302fce9cf794be1bbc" }, 


    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {3, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {3, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {5, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {5, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {5, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {5, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {6, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {5, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {5, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {5, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {0, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {5, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {1, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" }, 
    {2, "000000000000000000000000000000000000000000000000000000000000415c", "03ee1bf1d4ed2237a808e0d17ad20a10bedd1dd6ca5e8231b4e9db4150e6ffb6bf59e5ed" },
};

CBlockIndex CreateBlockIndex(int nHeight)
{
    CBlockIndex index;
    index.nHeight = nHeight;
    index.pprev = chainActive.Tip();
    return index;
}

bool TestSequenceLocks(const CTransaction &tx, int flags)
{
    LOCK(mempool.cs);
    return CheckSequenceLocks(tx, flags);
}

#if 0

BOOST_AUTO_TEST_CASE(creategenesisblock_validity)
{
    //creategenesisblock();
    std::cerr << "creategenesisblock_validity mainnet" << std::endl;
    /* main */
    creategenesisblock( 1517433514, 0x1f03ffff );

    std::cerr << "creategenesisblock_validity testnet" << std::endl;
    /* testnet */
    creategenesisblock( 1517433514, 0x1f03ffff );

    std::cerr << "creategenesisblock_validity regtest" << std::endl;
    /* regtest */
    creategenesisblock( 1517433514, 0x2007ffff );

    BOOST_CHECK(true);
}

#endif

#if 0
BOOST_AUTO_TEST_CASE(creategenesisblock_main_validity)
{
    std::cerr << "creategenesisblock_main_validity " << std::endl;
    // Note that by default, these tests run with size accounting enabled.
    auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    CChainParams& chainparams = *chainParams;

    CBlock pblock = chainparams.GenesisBlock(); 
    memset(pblock.nReserved, 0, sizeof(pblock.nReserved));

    pblock.nNonce = uint256();
    pblock.nSolution.clear();

    Scan_nNonce_nSolution ( & pblock , chainparams.EquihashN(), chainparams.EquihashK() );
    std::cerr << pblock.ToString() << std::endl;

    BOOST_CHECK(true);
}
#endif

#if 0
BOOST_AUTO_TEST_CASE(creategenesisblock_testnet_validity)
{
    std::cerr << " creategenesisblock_testnet_validity  " << std::endl;
    // Note that by default, these tests run with size accounting enabled.
    auto chainParams = CreateChainParams(CBaseChainParams::TESTNET);

    CChainParams& chainparams = *chainParams;

    CBlock pblock = chainparams.GenesisBlock(); 
    memset(pblock.nReserved, 0, sizeof(pblock.nReserved));

    pblock.nNonce = uint256();;
    pblock.nSolution.clear();

    Scan_nNonce_nSolution ( & pblock , chainparams.EquihashN(), chainparams.EquihashK() );

    std::cerr << pblock.ToString() << std::endl;

    BOOST_CHECK(true);
}
#endif


// Test suite for ancestor feerate transaction selection.
// Implemented as an additional function, rather than a separate test case,
// to allow reusing the blockchain created in CreateNewBlock_validity.
void TestPackageSelection(const CChainParams& chainparams, CScript scriptPubKey, std::vector<CTransactionRef>& txFirst)
{
    // Test the ancestor feerate transaction selection.
    TestMemPoolEntryHelper entry;

    // Test that a medium fee transaction will be selected after a higher fee
    // rate package with a low fee rate parent.
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 5000000000LL - 1000;
    // This tx has a low fee: 1000 satoshis
    uint256 hashParentTx = tx.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a medium fee: 10000 satoshis
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = 5000000000LL - 10000;
    uint256 hashMediumFeeTx = tx.GetHash();
    mempool.addUnchecked(hashMediumFeeTx, entry.Fee(10000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a high fee, but depends on the first transaction
    tx.vin[0].prevout.hash = hashParentTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000; // 50k satoshi fee
    uint256 hashHighFeeTx = tx.GetHash();
    mempool.addUnchecked(hashHighFeeTx, entry.Fee(50000).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));

    std::unique_ptr<CBlockTemplate> pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[1]->GetHash() == hashParentTx);
    BOOST_CHECK(pblocktemplate->block.vtx[2]->GetHash() == hashHighFeeTx);
    BOOST_CHECK(pblocktemplate->block.vtx[3]->GetHash() == hashMediumFeeTx);

    // Test that a package below the block min tx fee doesn't get included
    tx.vin[0].prevout.hash = hashHighFeeTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000; // 0 fee
    uint256 hashFreeTx = tx.GetHash();
    mempool.addUnchecked(hashFreeTx, entry.Fee(0).FromTx(tx));
    size_t freeTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);

    // Calculate a fee on child transaction that will put the package just
    // below the block min tx fee (assuming 1 child tx of the same size).
    CAmount feeToUse = blockMinFeeRate.GetFee(2*freeTxSize) - 1;

    tx.vin[0].prevout.hash = hashFreeTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000 - feeToUse;
    uint256 hashLowFeeTx = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx, entry.Fee(feeToUse).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    // Verify that the free tx and the low fee tx didn't get selected
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeTx);
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashLowFeeTx);
    }

    // Test that packages above the min relay fee do get included, even if one
    // of the transactions is below the min relay fee
    // Remove the low fee transaction and replace with a higher fee transaction
    mempool.removeRecursive(tx);
    tx.vout[0].nValue -= 2; // Now we should be just over the min relay fee
    hashLowFeeTx = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx, entry.Fee(feeToUse+2).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[4]->GetHash() == hashFreeTx);
    BOOST_CHECK(pblocktemplate->block.vtx[5]->GetHash() == hashLowFeeTx);

    // Test that transaction selection properly updates ancestor fee
    // calculations as ancestor transactions get included in a block.
    // Add a 0-fee transaction that has 2 outputs.
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vout.resize(2);
    tx.vout[0].nValue = 5000000000LL - 100000000;
    tx.vout[1].nValue = 100000000; // 1FAB output
    uint256 hashFreeTx2 = tx.GetHash();
    mempool.addUnchecked(hashFreeTx2, entry.Fee(0).SpendsCoinbase(true).FromTx(tx));

    // This tx can't be mined by itself
    tx.vin[0].prevout.hash = hashFreeTx2;
    tx.vout.resize(1);
    feeToUse = blockMinFeeRate.GetFee(freeTxSize);
    tx.vout[0].nValue = 5000000000LL - 100000000 - feeToUse;
    uint256 hashLowFeeTx2 = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx2, entry.Fee(feeToUse).SpendsCoinbase(false).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);

    // Verify that this tx isn't selected.
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeTx2);
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashLowFeeTx2);
    }

    // This tx will be mineable, and should cause hashLowFeeTx2 to be selected
    // as well.
    tx.vin[0].prevout.n = 1;
    tx.vout[0].nValue = 100000000 - 10000; // 10k satoshi fee
    mempool.addUnchecked(tx.GetHash(), entry.Fee(10000).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[8]->GetHash() == hashLowFeeTx2);
}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(CreateNewBlock_validity)
{
    // Note that by default, these tests run with size accounting enabled.
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const CChainParams& chainparams = *chainParams;
    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    std::unique_ptr<CBlockTemplate> pblocktemplate;
    CMutableTransaction tx,tx2;
    CScript script;
    uint256 hash;
    TestMemPoolEntryHelper entry;
    entry.nFee = 11;
    entry.nHeight = 11;

    LOCK(cs_main);
    fCheckpointsEnabled = false;

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));

    // We can't make transactions until we have inputs
    // Therefore, load 100 blocks :)
    int baseheight = 0;
    std::vector<CTransactionRef> txFirst;
    CBlock *pblock = &pblocktemplate->block; // pointer for convenience
    pblock->nTime = chainActive.Tip()->GetMedianTimePast()+1;
    for (unsigned int i = 0; i < sizeof(blockinfo)/sizeof(*blockinfo); ++i)
    {
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = 4;

        pblock->nTime +=10;
        CMutableTransaction txCoinbase(*pblock->vtx[0]);
        txCoinbase.nVersion = 1;
        txCoinbase.vin[0].scriptSig = CScript();

        txCoinbase.vin[0].scriptSig.push_back(pblock->nHeight );
        txCoinbase.vin[0].scriptSig.push_back(blockinfo[i].extranonce);
        txCoinbase.vout.resize(1); // --todo-- Ignore the (optional) segwit commitment added by CreateNewBlock (as the hardcoded nonces don't account for this)
        txCoinbase.vout[0].scriptPubKey = CScript();
        pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
        if (txFirst.size() == 0)
            baseheight = chainActive.Height();
        if (txFirst.size() < 4)
            txFirst.push_back(pblock->vtx[0]);
        pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

        memset(pblock->nReserved, 0, sizeof(pblock->nReserved));
        pblock->nNonce    = uint256S(blockinfo[i].nonce_hex);
        pblock->nSolution = ParseHex(blockinfo[i].solution_hex);

        std::cerr << "pblock" << pblock->ToString() << std::endl;

        std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);

        bool ProcessNewBlock_flag = ProcessNewBlock(chainparams, shared_pblock, true, nullptr) ;

#if 0
        if ( !ProcessNewBlock_flag ) {
            std::cerr << "before scan pblock" << pblock->ToString() << std::endl;

            memset(pblock->nReserved, 0, sizeof(pblock->nReserved));
            pblock->nNonce = uint256();
            pblock->nSolution.clear();

            Scan_nNonce_nSolution (  pblock , chainparams.EquihashN(), chainparams.EquihashK() );
            std::cerr << "after scan pblock" << pblock->ToString() << std::endl;


            std::cerr << " saved : { " << + blockinfo[i].extranonce << ", " << "\"" << blockinfo[i].nonce_hex << "\", \"" << blockinfo[i].solution_hex << "\" }, "  << std::endl;
            std::cerr << "scaned : { " << + blockinfo[i].extranonce << ", " << "\"" << pblock->nNonce.GetHex() << "\", \"" << HexStr(pblock->nSolution.begin(), pblock->nSolution.end()) << "\" }, "  << std::endl;
        }
        else {
            std::cerr << " saved : { " << + blockinfo[i].extranonce << ", " << "\"" << blockinfo[i].nonce_hex << "\", \"" << blockinfo[i].solution_hex << "\" }, "  << std::endl;
            std::cerr << "passed : { " << + blockinfo[i].extranonce << ", " << "\"" << pblock->nNonce.GetHex() << "\", \"" << HexStr(pblock->nSolution.begin(), pblock->nSolution.end()) << "\" }, "  << std::endl;
        }
#endif

        BOOST_CHECK( ProcessNewBlock_flag );

        // for next loop
        pblock->hashPrevBlock = pblock->GetHash();
        pblock->nHeight ++;
    }

    // Just to make sure we can still make simple blocks
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));

    const CAmount BLOCKSUBSIDY = 50*COIN;
    const CAmount LOWFEE = CENT;
    const CAmount HIGHFEE = COIN;
    const CAmount HIGHERFEE = 4*COIN;

    // block sigops > limit: 1000 CHECKMULTISIG + 1
    tx.vin.resize(1);
    // NOTE: OP_NOP is used to force 20 SigOps for the CHECKMULTISIG
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_0 << OP_0 << OP_NOP << OP_CHECKMULTISIG << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = i == 0; // only first tx spends coinbase
        // If we don't set the # of sig ops in the CTxMemPoolEntry, template creation fails
        mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK_THROW(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    mempool.clear();

    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = i == 0; // only first tx spends coinbase
        // If we do set the # of sig ops in the CTxMemPoolEntry, template creation passes
        mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).SigOpsCost(80).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    mempool.clear();

    // block size > limit
    tx.vin[0].scriptSig = CScript();
    // 18 * (520char + DROP) + OP_1 = 9433 bytes
    std::vector<unsigned char> vchData(520);
    for (unsigned int i = 0; i < 18; ++i)
        tx.vin[0].scriptSig << vchData << OP_DROP;
    tx.vin[0].scriptSig << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 128; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = i == 0; // only first tx spends coinbase
        mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    mempool.clear();

    // orphan in mempool, template creation fails
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).FromTx(tx));
    BOOST_CHECK_THROW(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    mempool.clear();

    // child with higher feerate than parent
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vin[0].prevout.hash = hash;
    tx.vin.resize(2);
    tx.vin[1].scriptSig = CScript() << OP_1;
    tx.vin[1].prevout.hash = txFirst[0]->GetHash();
    tx.vin[1].prevout.n = 0;
    tx.vout[0].nValue = tx.vout[0].nValue+BLOCKSUBSIDY-HIGHERFEE; //First txn output + fresh coinbase - new txn fee
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHERFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    mempool.clear();

    // coinbase in mempool, template creation fails
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_1;
    tx.vout[0].nValue = 0;
    hash = tx.GetHash();
    // give it a fee so it'll get mined
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    BOOST_CHECK_THROW(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    mempool.clear();

    // invalid (pre-p2sh) txn in mempool, template creation fails
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = BLOCKSUBSIDY-LOWFEE;
    script = CScript() << OP_0;
    tx.vout[0].scriptPubKey = GetScriptForDestination(CScriptID(script));
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vin[0].prevout.hash = hash;
    tx.vin[0].scriptSig = CScript() << std::vector<unsigned char>(script.begin(), script.end());
    tx.vout[0].nValue -= LOWFEE;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    BOOST_CHECK_THROW(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    mempool.clear();

    // double spend txn pair in mempool, template creation fails
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vout[0].scriptPubKey = CScript() << OP_2;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK_THROW(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    mempool.clear();

    // subsidy changing
    int nHeight = chainActive.Height();
    // Create an actual 1679999-long block chain (without valid blocks).
    while (chainActive.Tip()->nHeight < 1679999) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(InsecureRand256());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        chainActive.SetTip(next);
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    // Extend to a 1680000-long block chain.
    while (chainActive.Tip()->nHeight < 1680000) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(InsecureRand256());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        chainActive.SetTip(next);
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    // Delete the dummy blocks again.
    while (chainActive.Tip()->nHeight > nHeight) {
        CBlockIndex* del = chainActive.Tip();
        chainActive.SetTip(del->pprev);
        pcoinsTip->SetBestBlock(del->pprev->GetBlockHash());
        delete del->phashBlock;
        delete del;
    }

    // non-final txs in mempool
    SetMockTime(chainActive.Tip()->GetMedianTimePast()+1);
    int flags = LOCKTIME_VERIFY_SEQUENCE|LOCKTIME_MEDIAN_TIME_PAST;
    // height map
    std::vector<int> prevheights;

    // relative height locked
    tx.nVersion = 2;
    tx.vin.resize(1);
    prevheights.resize(1);
    tx.vin[0].prevout.hash = txFirst[0]->GetHash(); // only 1 transaction
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].nSequence = chainActive.Tip()->nHeight + 1; // txFirst[0] is the 2nd block
    prevheights[0] = baseheight + 1;
    tx.vout.resize(1);
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    tx.nLockTime = 0;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    BOOST_CHECK(SequenceLocks(tx, flags, &prevheights, CreateBlockIndex(chainActive.Tip()->nHeight + 2))); // Sequence locks pass on 2nd block

    // relative time locked
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | (((chainActive.Tip()->GetMedianTimePast()+1-chainActive[1]->GetMedianTimePast()) >> CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) + 1); // txFirst[1] is the 3rd block
    prevheights[0] = baseheight + 2;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail

    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime += 512; //Trick the MedianTimePast
    BOOST_CHECK(SequenceLocks(tx, flags, &prevheights, CreateBlockIndex(chainActive.Tip()->nHeight + 1))); // Sequence locks pass 512 seconds later
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime -= 512; //undo tricked MTP

    // absolute height locked
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL - 1;
    prevheights[0] = baseheight + 3;
    tx.nLockTime = chainActive.Tip()->nHeight + 1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTx(tx, flags)); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 2, chainActive.Tip()->GetMedianTimePast())); // Locktime passes on 2nd block

    // absolute time locked
    tx.vin[0].prevout.hash = txFirst[3]->GetHash();
    tx.nLockTime = chainActive.Tip()->GetMedianTimePast();
    prevheights.resize(1);
    prevheights[0] = baseheight + 4;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTx(tx, flags)); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 2, chainActive.Tip()->GetMedianTimePast() + 1)); // Locktime passes 1 second later

    // mempool-dependent transactions (not added)
    tx.vin[0].prevout.hash = hash;
    prevheights[0] = chainActive.Tip()->nHeight + 1;
    tx.nLockTime = 0;
    tx.vin[0].nSequence = 0;
    BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    tx.vin[0].nSequence = 1;
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG;
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | 1;
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));

    // None of the of the absolute height/time locked tx should have made
    // it into the template because we still check IsFinalTx in CreateNewBlock,
    // but relative locked txs will if inconsistently added to mempool.
    // For now these will still generate a valid template until BIP68 soft fork
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3);
    // However if we advance height by 1 and time by 512, all of them should be mined
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime += 512; //Trick the MedianTimePast
    chainActive.Tip()->nHeight++;
    SetMockTime(chainActive.Tip()->GetMedianTimePast() + 1);

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 5);

    chainActive.Tip()->nHeight--;
    SetMockTime(0);
    mempool.clear();

    TestPackageSelection(chainparams, scriptPubKey, txFirst);

    fCheckpointsEnabled = true;
}

BOOST_AUTO_TEST_SUITE_END()
