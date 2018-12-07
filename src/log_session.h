// Copyright (c) 2018 FA Enterprise system
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef LOG_SESSION_H
#define LOG_SESSION_H
#include <fstream>
#include <sstream>
#include <deque>
#include <mutex>

// Example use:
// LogSession::evmLog() << "Debug: data I want to log. " << LogSession::endL;
// Your data will be cached in a queue (with a size limit) and flushed to a file.
// If no filename is specified, only the ram cache is used.
// Each call of operator<< locks a mutex, so excessive use will penalize performance
// (far less than the actual disk I/O if you are also writing the file).
class LogSession {
public:
    static std::string currentNetName;
    static std::string regtestName;
    unsigned maximumLinesToStore;
    std::string name; //<- session name, for example, debug or evm.
    std::string fileName; //<- reserved
    std::fstream file;
    std::deque<std::string> lines;
    int64_t numberOfDeletedLines;
    std::string currentLine;
    std::mutex lock;

    LogSession();
    static LogSession& evmLog(); //<- use to fetch a global logfile. This is a function to avoid the "static initialization order fiasco".
    static LogSession& debugLog(); //<- use to fetch a global logfile.
    enum specialSymbols{
        endL, //<- use log << LogSession::endL to end the line and flush the log file.
        //not used at the moment but reserved for future use:
        red, blue, yellow, green, purple, cyan, normalColor, orange
    };

    LogSession& operator<<(LogSession::specialSymbols input);

    void initialize();
    template <typename anyInputType>
    LogSession& operator<<(const anyInputType& input) {
        std::lock_guard<std::mutex> theGuard(this->lock);
        //Don't log anything if not on regtest.
        if (this->currentNetName != this->regtestName) {
            return *this;
        }
        std::stringstream out;
        out << input;
        this->currentLine += out.str();
        return *this;
    }
    std::string ToStringStatistics();
};
#endif
