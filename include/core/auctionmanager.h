#pragma once
#include <vector>
#include "core/player.h"
#include "core/property.h"
#include "core/game.h"

namespace Nimonspoli {

class AuctionManager {
public:
    AuctionManager(Property& prop, const std::vector<Player*>& order, int passesNeeded, Game& game)
        : prop_(prop), order_(order), passesNeeded_(passesNeeded), game_(game) {}

    Player* currentPlayer() const {
        return order_[idx_ % order_.size()];
    }

    void processBid(int amount) {
        highBid_ = amount;
        winner_ = currentPlayer();
        passCount_ = 0;
        TransactionLogger::log(game_.currentTurn(), winner_->username(), "LELANG", "BID M" + std::to_string(amount));
        checkState();
        if (!finished_) ++idx_;
    }

    void processPass() {
        ++passCount_;
        TransactionLogger::log(game_.currentTurn(), currentPlayer()->username(), "LELANG", "PASS");
        checkState();
        if (!finished_ && !failed_) ++idx_;
    }

    bool isFinished() const { return finished_; }
    bool isFailed() const { return failed_; }
    
    Player* winner() const { return winner_; }
    int highBid() const { return highBid_; }
    Property& property() const { return prop_; }

    void finalizeAuction(Game& game) {
        if (winner_) {
            game.finishAuction(*winner_, prop_, highBid_);
        }
    }

    void resetForRetry() {
        idx_ = 0;
        passCount_ = 0;
        failed_ = false;
    }

private:
    void checkState() {
        if (passCount_ >= passesNeeded_ && winner_ != nullptr) {
            finished_ = true;
        } else if (passCount_ >= (int)order_.size() && winner_ == nullptr) {
            failed_ = true;
        }
    }

    Property& prop_;
    Game& game_;
    std::vector<Player*> order_;
    int passesNeeded_;
    
    int highBid_ = 0;
    Player* winner_ = nullptr;
    int passCount_ = 0;
    size_t idx_ = 0;
    
    bool finished_ = false;
    bool failed_ = false;
};

}
