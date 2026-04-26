#include "core/auctionmanager.h"
#include <string>

namespace Nimonspoli {

AuctionManager::AuctionManager(Property& prop, const std::vector<Player*>& order, int passesNeeded, Game& game)
    : prop_(prop), order_(order), passesNeeded_(passesNeeded), game_(game) {}

Player* AuctionManager::currentPlayer() const {
    return order_[idx_ % order_.size()];
}

void AuctionManager::processBid(int amount) {
    highBid_ = amount;
    winner_ = currentPlayer();
    passCount_ = 0;
    TransactionLogger::log(game_.currentTurn(), winner_->username(), "LELANG", "BID M" + std::to_string(amount));
    checkState();
    if (!finished_) ++idx_;
}

void AuctionManager::processPass() {
    ++passCount_;
    TransactionLogger::log(game_.currentTurn(), currentPlayer()->username(), "LELANG", "PASS");
    checkState();
    if (!finished_ && !failed_) ++idx_;
}

void AuctionManager::finalizeAuction(Game& game) {
    if (winner_) {
        game.finishAuction(*winner_, prop_, highBid_);
    }
}

void AuctionManager::resetForRetry() {
    idx_ = 0;
    passCount_ = 0;
    failed_ = false;
}

void AuctionManager::checkState() {
    if (passCount_ >= passesNeeded_ && winner_ != nullptr) {
        finished_ = true;
    } else if (passCount_ >= (int)order_.size() && winner_ == nullptr) {
        failed_ = true;
    }
}

} // namespace Nimonspoli