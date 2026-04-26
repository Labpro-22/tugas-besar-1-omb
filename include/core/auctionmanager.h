#pragma once
#include <vector>
#include "core/player.h"
#include "core/property.h"
#include "core/game.h"

namespace Nimonspoli {

class AuctionManager {
public:
    AuctionManager(Property& prop, const std::vector<Player*>& order, int passesNeeded, Game& game);

    Player* currentPlayer() const;
    void processBid(int amount);
    void processPass();

    bool isFinished() const { return finished_; }
    bool isFailed() const { return failed_; }
    Player* winner() const { return winner_; }
    int highBid() const { return highBid_; }
    Property& property() const { return prop_; }

    void finalizeAuction(Game& game);
    void resetForRetry();

private:
    void checkState();

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

} // namespace Nimonspoli