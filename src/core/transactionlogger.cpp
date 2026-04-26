#include "core/transactionlogger.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
 
namespace Nimonspoli {
 
std::vector<LogEntry> TransactionLogger::entries_;

void TransactionLogger::log(int turn, const std::string& username,
                            const std::string& action, const std::string& detail) {
    entries_.push_back({turn, username, action, detail});
}
 
void TransactionLogger::print(int n) {
    int start = (n == 0) ? 0 : std::max(0, (int)entries_.size() - n);
    if (n == 0)
        std::cout << "=== Log Transaksi Penuh ===\n";
    else
        std::cout << "=== Log Transaksi (" << n << " Terakhir) ===\n";
 
    for (int i = start; i < (int)entries_.size(); ++i) {
    const auto& e = entries_[i];
    std::cout << "[Turn " << std::setw(3) << e.turn << "] "
              << std::left << std::setw(12) << e.username << " | "
              << std::left << std::setw(18) << e.action   << " | "
              << e.detail   << "\n";
    }
}
 
} 