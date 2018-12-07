// Copyright (c) 2018 FA Enterprise system
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/server.h"

#include "crypto/sha3.h"
#include "../aggregate_schnorr_signature.h"
#include "utilstrencodings.h"
#include <primitives/transaction.h>
#include <core_io.h>
#include <coins.h>
#include <txmempool.h>
#include <validation.h>
#include <log_session.h>

#include <univalue.h>
#include <iostream>
#include <mutex>


UniValue getlogfile(const JSONRPCRequest& request) {
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getlogfile ( logfilenameString )\n"
            "\nFetches log files for the current session. Available in -regtest only."
            "\nTo be documented further.\n"
        );
    UniValue result(UniValue::VOBJ);
    std::stringstream errorStream;
    if (!request.params[0].isStr()) {
        errorStream << "Expected string input, got: " << request.params[0].write() << ". ";
        result.pushKV("error", errorStream.str());
        return result;
    }
    std::string requestedLog = request.params[0].get_str();
    LogSession* theLog = nullptr;
    if (requestedLog == LogSession::debugLog().name) {
        theLog = & LogSession::debugLog();
    } else if (requestedLog == LogSession::evmLog().name) {
        theLog = & LogSession::evmLog();
    }
    if (theLog == nullptr) {
        errorStream << "I do not recognize the log name: " << requestedLog;
        result.pushKV("error", errorStream.str());
        return result;
    }
    std::lock_guard<std::mutex> theGuard(theLog->lock);
    UniValue logLines;
    logLines.setArray();
    for (unsigned i = 0; i < theLog->lines.size(); i ++) {
        logLines.push_back(theLog->lines[i]);
    }
    result.pushKV("logLines", logLines);
    result.pushKV("resultHTML",  theLog->ToStringStatistics());
    result.pushKV("requestedLog", request.params[0]);
    return result;
}
