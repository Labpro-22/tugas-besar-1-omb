#include "core/board.h"
#include "core/player.h"
#include <stdexcept>
#include <algorithm>
using namespace std;

namespace Nimonspoli {

void Board::addTile(unique_ptr<Tile> tile) {
    if (static_cast<int>(tiles_.size()) >= BOARD_SIZE)
        throw out_of_range("Board overflow: too many tiles");
    tiles_.push_back(move(tile));
}

void Board::addProperty(unique_ptr<Property> prop) {
    string code = prop->code();
    if (properties_.count(code))
        throw invalid_argument("Duplicate property code: " + code);
    properties_[code] = move(prop);
}

Tile* Board::getTile(int index) const {
    if (index < 0 || index >= (int)tiles_.size())
        throw out_of_range("Tile index out of range: " + to_string(index));
    return tiles_[index].get();
}

Property* Board::getProperty(const string& code) const {
    auto it = properties_.find(code);
    if (it == properties_.end()) return nullptr;
    return it->second.get();
}

int Board::nearestRailroad(int currentPos) const {
    int n = size();
    for (int step = 1; step < n; ++step) {
        int idx = (currentPos + step) % n;
        if (tiles_[idx]->type() == TileType::PROPERTY) {
            auto* pt = static_cast<PropertyTile*>(tiles_[idx].get());
            if (pt->property()->type() == PropertyType::RAILROAD)
                return idx;
        }
    }
    return currentPos; // fallback (Ga tau kapan keluar kasusnya klo ga ada)
}

vector<Street*> Board::colorGroupStreets(ColorGroup cg) const {
    vector<Street*> result;
    for (auto& [code, prop] : properties_) {
        if (prop->type() == PropertyType::STREET) {
            auto* s = static_cast<Street*>(prop.get());
            if (s->colorGroup() == cg) result.push_back(s);
        }
    }
    return result;
}

vector<Railroad*> Board::railroadsOf(Player* player) const {
    vector<Railroad*> result;
    for (auto& [code, prop] : properties_) {
        if (prop->type() == PropertyType::RAILROAD && prop->owner() == player)
            result.push_back(static_cast<Railroad*>(prop.get()));
    }
    return result;
}

vector<Utility*> Board::utilitiesOf(Player* player) const {
    vector<Utility*> result;
    for (auto& [code, prop] : properties_) {
        if (prop->type() == PropertyType::UTILITY && prop->owner() == player)
            result.push_back(static_cast<Utility*>(prop.get()));
    }
    return result;
}

void Board::recalcMonopoly(Player* player) {
    map<ColorGroup, int> owned, total;
    for (auto& [code, prop] : properties_) {
        if (prop->type() != PropertyType::STREET) continue;
        auto* s = static_cast<Street*>(prop.get());
        total[s->colorGroup()]++;
        if (prop->owner() == player) owned[s->colorGroup()]++;
    }
    for (auto& [cg, cnt] : total) {
        bool mono = (owned.count(cg) && owned[cg] == cnt);
        for (auto* s : colorGroupStreets(cg)) {
            if (s->owner() == player) {
                s->setMonopoly(mono);
            }
        }
    }
}

void Board::recalcRailroadCount(Player* player) {
    auto rrs = railroadsOf(player);
    int cnt = static_cast<int>(rrs.size());
    for (auto* rr : rrs) rr->setOwnerRailroadCount(cnt);
}

void Board::recalcUtilityCount(Player* player) {
    auto utils = utilitiesOf(player);
    int cnt = static_cast<int>(utils.size());
    for (auto* u : utils) u->setOwnerUtilityCount(cnt);
}

void Board::tickFestivals(Player* player) {
    for (auto& [code, prop] : properties_) {
        if (prop->type() == PropertyType::STREET && prop->owner() == player) {
            auto* s = static_cast<Street*>(prop.get());
            s->festival().tick();
        }
    }
}

vector<Player*> Board::auctionOrder(Player* trigger, const vector<Player*>& activePlayers) const {
    size_t trigIdx = 0;
    for (size_t i = 0; i < activePlayers.size(); ++i) {
        if (activePlayers[i] == trigger) { trigIdx = i; break; }
    }
    vector<Player*> order;
    for (size_t i = 1; i < activePlayers.size(); ++i)
        order.push_back(activePlayers[(trigIdx + i) % activePlayers.size()]);
    if (order.empty()) order = activePlayers;
    return order;
}

int Board::auctionPassesNeeded(const vector<Player*>& auctionParticipants) const {
    int needed = static_cast<int>(auctionParticipants.size()) - 1;
    return (needed < 0) ? 0 : needed;
}
int Board::findTileIndex(TileType type) const {
    for (int i = 0; i < static_cast<int>(tiles_.size()); ++i)
        if (tiles_[i]->type() == type) return i;
    return -1;
}
string Board::tileDescription(int index) const {
    if (index < 0 || index >= static_cast<int>(tiles_.size())) return "???";
    return tiles_[index]->summary();
}

} 