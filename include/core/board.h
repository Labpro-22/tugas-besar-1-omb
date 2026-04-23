#pragma once
#include <vector>
#include <map>
#include <string>
#include <memory>
#include "core/tiles.h"
#include "core/property.h"
#include "core/propertytypes.h"
using namespace std;
namespace Nimonspoli {

class Player;

class Board {
public:
    static constexpr int BOARD_SIZE = 40;

    void addTile(unique_ptr<Tile> tile);
    void addProperty(unique_ptr<Property> prop);

    Tile*     getTile(int index) const;
    Property* getProperty(const string& code) const;
    int       size() const { return static_cast<int>(tiles_.size()); }

    const vector<unique_ptr<Tile>>&     tiles()      const { return tiles_; }
    const map<string, unique_ptr<Property>>& properties() const { return properties_; }

    int advance(int currentPos, int steps) const {
        return (currentPos + steps) % size();
    }

    int nearestRailroad(int currentPos) const;

    // Call setiap perubahan owner
    void recalcMonopoly(Player* player);
    void recalcRailroadCount(Player* player);
    void recalcUtilityCount(Player* player);

    vector<Street*> colorGroupStreets(ColorGroup cg) const;
    vector<Railroad*> railroadsOf(Player* player) const;
    vector<Utility*> utilitiesOf(Player* player) const;

    void tickFestivals(Player* player);

    vector<Player*> auctionOrder(Player* trigger, const vector<Player*>& activePlayers) const;

    int auctionPassesNeeded(const vector<Player*>& auctionParticipants) const;
    int findTileIndex(TileType type) const;
    
    string tileDescription(int index) const;

private:
    vector<unique_ptr<Tile>>            tiles_;
    map<string, unique_ptr<Property>>   properties_;
};

} 