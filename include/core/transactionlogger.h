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
    void log(int turn, const std::string& username,
             const std::string& action, const std::string& detail);
    void print(int n = 0) const;

    const std::vector<LogEntry>& entries() const { return entries_; }
    void loadEntries(const std::vector<LogEntry>& entries) { entries_ = entries; }

private:
    std::vector<LogEntry> entries_;
};

} 