#pragma once
#include <string>
#include <vector>

namespace Nimonspoli {

class LogEntry {
public:
    int         turn;
    std::string username;
    std::string action;   // "DADU", "BELI", "SEWA", "KARTU" …
    std::string detail;
};

class TransactionLogger {
public:
    static void log(int turn, const std::string& username,
             const std::string& action, const std::string& detail);
    static void print(int n = 0);

    static const std::vector<LogEntry>& entries() { return entries_; }
    static void loadEntries(const std::vector<LogEntry>& entries) { entries_ = entries; }

private:
    static std::vector<LogEntry> entries_;
};

} 