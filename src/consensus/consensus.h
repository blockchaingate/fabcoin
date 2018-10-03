// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FABCOIN_CONSENSUS_CONSENSUS_H
#define FABCOIN_CONSENSUS_CONSENSUS_H

#include <stdlib.h>
#include <stdint.h>

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
extern unsigned int dgpMaxBlockSerSize;
extern unsigned int dgpMaxBlockWeight;
extern unsigned int dgpMaxBlockBaseSize;
extern unsigned int dgpMaxBlockSize; // fasc
extern int64_t dgpMaxBlockSigOps;
extern unsigned int dgpMaxProtoMsgLength;
extern unsigned int dgpMaxTxSigOps;

//static const int MAX_TRANSACTION_BASE_SIZE = 1000000;
static const int MAX_TRANSACTION_BASE_SIZE = 2000000;

//static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 4000000;
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 8000000;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
//static const unsigned int MAX_BLOCK_WEIGHT = 4000000;
static const unsigned int MAX_BLOCK_WEIGHT = 8000000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const int64_t MAX_BLOCK_SIGOPS_COST = 80000;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 800;

static const int INITIAL_BLOCK_REWARD = 25;
static const int INITIAL_BLOCK_REWARD_REGTEST = 20000;

static const int WITNESS_SCALE_FACTOR = 4;

static const size_t MIN_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 60; // 60 is the lower bound for the size of a valid serialized CTransaction
static const size_t MIN_SERIALIZABLE_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 10; // 10 is the lower bound for the size of a serialized CTransaction

/** Flags for nSequence and nLockTime locks */
enum {
    /* Interpret sequence numbers as relative lock-time constraints. */
    LOCKTIME_VERIFY_SEQUENCE = (1 << 0),

    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

void updateBlockSizeParams(unsigned int newBlockSize);
#endif // FABCOIN_CONSENSUS_CONSENSUS_H
