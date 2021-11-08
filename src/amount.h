// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Fabcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FABCOIN_AMOUNT_H
#define FABCOIN_AMOUNT_H

#include <serialize.h>

#include <stdlib.h>
#include <string>

/** Amount in liu (Can be negative) */
typedef int64_t CAmount;

static const CAmount COIN = 100000000;
static const CAmount CENT = 1000000;

extern const std::string CURRENCY_UNIT;

/** No amount larger than this (in liu) is valid.*/
static const CAmount MAX_MONEY = (42000000 + 32000000) * COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

#endif //  FABCOIN_AMOUNT_H
