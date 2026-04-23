#include "core/propertytypes.h"
#include "core/game.h"
#include "core/player.h"
#include <stdexcept>
#include <string>
#include <algorithm>
#include <climits>

namespace Nimonspoli {

void Property::handlePurchase(Player& player, Game& game) {
    if (type_ == PropertyType::RAILROAD || type_ == PropertyType::UTILITY) {
        setOwner(&player);
        setStatus(PropertyStatus::OWNED);
        player.addProperty(this);
        game.refreshPropertyCounts(&player);
        game.logger().log(game.currentTurn(), player.username(), type_ == PropertyType::RAILROAD ? "RAILROAD" : "UTILITY", name_ + " kini milik " + player.username() + " (otomatis)");
        return;
    }
    bool wantsBuy = player.canAfford(buyPrice_) && game.callbacks().onOfferPurchase && game.callbacks().onOfferPurchase(*this);
    if (!wantsBuy) { this->handleAuction(game); return; }
    int price = buyPrice_;
    if (player.hasDiscount()) {
        price = price * (100 - player.discountPct()) / 100;
        player.clearDiscount();
        game.logger().log(game.currentTurn(), player.username(), "DISKON", name_ + " diskon -> M" + std::to_string(price));
    }
    game.bank().collect(player, price);
    setOwner(&player);
    setStatus(PropertyStatus::OWNED);
    player.addProperty(this);
    game.refreshPropertyCounts(&player);
    game.logger().log(game.currentTurn(), player.username(), "BELI", "Beli " + name_ + " seharga M" + std::to_string(price));
}

void Property::handleRent(Player& payer, int diceTotal, Game& game) {
    if (isMortgaged()) return;
    if (owner_ == &payer) return;
    int rent = calcRent(diceTotal);
    if (rent <= 0) return;
    game.logger().log(game.currentTurn(), payer.username(), "SEWA", "Bayar M" + std::to_string(rent) + " ke " + owner_->username() + " (" + name_ + ")");
    if (!payer.canAfford(rent)) { game.handleBankruptcy(payer, owner_); return; }
    game.bank().transfer(payer, *owner_, rent);
}

void Property::handleAuction(Game& game) {
    game.logger().log(game.currentTurn(), "SISTEM", "LELANG", "Lelang dimulai: " + name_);
}

void Property::finishAuction(Player& winner, int finalBid, Game& game) {
    if (finalBid > 0) game.bank().collect(winner, finalBid);
    setOwner(&winner);
    setStatus(PropertyStatus::OWNED);
    winner.addProperty(this);
    game.refreshPropertyCounts(&winner);
    game.logger().log(game.currentTurn(), winner.username(), "LELANG", "Menang lelang " + name_ + " seharga M" + std::to_string(finalBid));
}

void Property::performMortgage(Player& player, Game& game) {
    if (isMortgaged()) throw std::logic_error("Properti sudah digadaikan.");
    game.bank().pay(player, mortgageValue());
    setStatus(PropertyStatus::MORTGAGED);
    game.logger().log(game.currentTurn(), player.username(), "GADAI", name_ + " digadaikan, menerima M" + std::to_string(mortgageValue()));
}

void Property::performRedeem(Player& player, Game& game) {
    if (!isMortgaged()) throw std::logic_error("Properti tidak sedang digadaikan.");
    int cost = buyPrice();
    if (!player.canAfford(cost)) throw std::runtime_error("Uang tidak cukup untuk menebus. Harga tebus: M" + std::to_string(cost));
    game.bank().collect(player, cost);
    setStatus(PropertyStatus::OWNED);
    game.logger().log(game.currentTurn(), player.username(), "TEBUS", name_ + " ditebus, bayar M" + std::to_string(cost));
}

void Street::performMortgage(Player& player, Game& game) {
    if (isMortgaged()) throw std::logic_error("Properti sudah digadaikan.");
    auto group = game.board().colorGroupStreets(colorGroup_);
    bool hasBldg = false;
    for (auto* s : group) if (s->buildingLevel() > 0) { hasBldg = true; break; }
    if (hasBldg) {
        for (auto* s : group) {
            int refund = s->demolishAll();
            if (refund > 0) {
                game.bank().pay(player, refund);
                game.logger().log(game.currentTurn(), player.username(), "JUAL_BANGUNAN", s->name() + " -> refund M" + std::to_string(refund));
            }
        }
    }
    Property::performMortgage(player, game);
}

void Street::buildHouseOrHotel(Player& player, Game& game) {
    if (!hasMonopoly()) throw std::logic_error("Kamu belum memonopoli color group ini.");
    if (hasHotel()) throw std::logic_error("Properti sudah memiliki hotel (maksimal).");
    auto groupStreets = game.board().colorGroupStreets(colorGroup_);
    int minLevel = INT_MAX;
    for (auto* s : groupStreets) minLevel = std::min(minLevel, s->buildingLevel());
    if (buildingLevel_ > minLevel) throw std::logic_error("Pemerataan bangunan tidak terpenuhi.");
    int cost = nextBuildCost();
    if (!player.canAfford(cost)) throw std::runtime_error("Uang tidak cukup untuk membangun.");
    game.bank().collect(player, cost);
    addBuilding();
    std::string label = hasHotel() ? "Hotel" : std::to_string(buildingLevel_) + " rumah";
    game.logger().log(game.currentTurn(), player.username(), "BANGUN", name_ + " -> " + label + " (biaya M" + std::to_string(cost) + ")");
}

void Street::applyFestivalBoost(Player& player, Game& game) {
    festival_.boost();
    game.logger().log(game.currentTurn(), player.username(), "FESTIVAL", name_ + ": sewa x" + std::to_string(festival_.multiplier()) + " selama 3 giliran");
}

} 