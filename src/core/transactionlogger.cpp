#include "core/transactionlogger.h"
#include <iostream>
#include <algorithm>
 
namespace Nimonspoli {
 
void TransactionLogger::log(int turn, const std::string& username,
                            const std::string& action, const std::string& detail) {
    entries_.push_back({turn, username, action, detail});
}
 
void TransactionLogger::print(int n) const {
    int start = (n == 0) ? 0 : std::max(0, (int)entries_.size() - n);
    if (n == 0)
        std::cout << "=== Log Transaksi Penuh ===\n";
    else
        std::cout << "=== Log Transaksi (" << n << " Terakhir) ===\n";
 
    for (int i = start; i < (int)entries_.size(); ++i) {
        const auto& e = entries_[i];
        std::cout << "[Turn " << e.turn << "] "
                  << e.username << " | "
                  << e.action   << " | "
                  << e.detail   << "\n";
    }
}
 
} // namespace Nimonspoli