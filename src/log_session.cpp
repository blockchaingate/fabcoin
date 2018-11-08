// Copyright (c) 2018 FA Enterprise system
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "log_session.h"
#include <util.h>

std::string LogSession::currentNetName = "";
std::string LogSession::regtestName = "regtest";

LogSession& LogSession::debugLog()
{
    static LogSession result;
    result.name = "debug";
    result.fileName = (GetDataDir() / "debug_session.log").string();
    result.initialize();
    return result;
}

LogSession& LogSession::evmLog()
{
    static LogSession result;
    result.name = "myEvm";
    result.fileName = (GetDataDir() / "myEvm.log").string();
    result.initialize();
    return result;
}

void LogSession::initialize()
{
    if (this->currentNetName != this->regtestName) {
        return;
    }
    if (this->fileName == "") {
        return;
    }
    this->file.open(this->fileName, std::fstream::in | std::fstream::out | std::fstream::trunc);
    if (!this->file.is_open()) {
        std::cout << "Fatal error: failed to open log file: " << this->fileName << std::endl;
    }
}

LogSession::LogSession()
{
    this->maximumLinesToStore = 10000;
    this->numberOfDeletedLines = 0;
}

std::string LogSession::ToStringStatistics() {
    std::stringstream out;
    out << "Displaying " << this->lines.size() << " lines. ";
    if (this->numberOfDeletedLines > 0)
        out << "In addition " << this->numberOfDeletedLines << " have been pruned (total lines: "
            << this->lines.size() + this->numberOfDeletedLines << "). ";
    out << "Log name: " << this->name << ". ";
    if (this->fileName == "") {
        out << "The lines are not logged in a file. ";
    } else {
        out << "The lines are logged in file: " << this->fileName;
    }
    return out.str();
}

LogSession& LogSession::operator<<(specialSymbols input) {
    //Don't log anything if not on regtest.
    if (this->currentNetName != this->regtestName) {
        return *this;
    }
    if (input == LogSession::endL) {
        std::lock_guard<std::mutex> theGuard(this->lock);
        this->lines.push_back(this->currentLine);
        if (this->lines.size() > this->maximumLinesToStore) {
            this->lines.pop_front();
            this->numberOfDeletedLines ++;
        }
        this->currentLine = "";
        if (this->fileName != "") {
            this->file.flush();
        }
    }
    return *this;
}
