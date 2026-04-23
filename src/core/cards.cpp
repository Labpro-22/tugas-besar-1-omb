#include "core/cards.h"
#include "core/game.h"
#include "core/player.h"
#include "core/board.h"
#include "core/propertytypes.h"
#include <stdexcept>
#include <iostream>

using namespace std;

namespace Nimonspoli {

ChanceCard::ChanceCard(ChanceEffect effect) : Card(""), effect_(effect) {
    switch (effect) {
        case ChanceEffect::GO_NEAREST_STATION:
            description_ = "Pergi ke stasiun terdekat."; break;
        case ChanceEffect::MOVE_BACK_3:
            description_ = "Mundur 3 petak."; break;
        case ChanceEffect::GO_TO_JAIL:
            description_ = "Masuk Penjara."; break;
    }
}

void ChanceCard::execute(Player& player, Game& game) {
    switch (effect_) {
        case ChanceEffect::GO_NEAREST_STATION: {
            int oldPos = player.position();
            int nearest = game.board().nearestRailroad(oldPos);
            int steps = (nearest - oldPos + game.board().size()) % game.board().size();
            game.movePlayer(player, steps, true);
            game.board().getTile(nearest)->onLanded(player, game);
            break;
        }
        case ChanceEffect::MOVE_BACK_3: {
            int newPos = (player.position() - 3 + game.board().size()) % game.board().size();
            game.teleportPlayer(player, newPos);
            game.board().getTile(newPos)->onLanded(player, game);
            break;
        }
        case ChanceEffect::GO_TO_JAIL:
            game.sendToJail(player);
            break;
    }
}

CommunityCard::CommunityCard(CommunityEffect effect) : Card(""), effect_(effect) {
    switch (effect) {
        case CommunityEffect::BIRTHDAY:
            description_ = "Ini adalah hari ulang tahun Anda. Dapatkan M100 dari setiap pemain."; break;
        case CommunityEffect::DOCTOR:
            description_ = "Biaya dokter. Bayar M700."; break;
        case CommunityEffect::ELECTION:
            description_ = "Anda mau nyaleg. Bayar M200 kepada setiap pemain."; break;
    }
}

void CommunityCard::execute(Player& player, Game& game) {
    auto active = game.activePlayers();
    switch (effect_) {
        case CommunityEffect::BIRTHDAY: {
            for (auto* other : active) {
                if (other == &player) continue;
                if (!other->canAfford(100)) {
                    game.handleBankruptcy(*other, &player, 100);
                } else {
                    game.bank().transfer(*other, player, 100);
                    game.logger().log(game.currentTurn(), other->username(),
                                      "DANA_UMUM", "Bayar M100 ulang tahun ke " + player.username());
                }
            }
            break;
        }
        case CommunityEffect::DOCTOR: {
            if (!player.canAfford(700)) {
                game.handleBankruptcy(player, nullptr, 700);
            } else {
                game.bank().collect(player, 700);
                game.logger().log(game.currentTurn(), player.username(),
                                  "DANA_UMUM", "Biaya dokter M700");
            }
            break;
        }
        case CommunityEffect::ELECTION: {
            for (auto* other : active) {
                if (other == &player) continue;
                if (!player.canAfford(200)) {
                    game.handleBankruptcy(player, other, 200);
                    return;
                }
                game.bank().transfer(player, *other, 200);
                game.logger().log(game.currentTurn(), player.username(),
                                  "DANA_UMUM", "Bayar M200 nyaleg ke " + other->username());
            }
            break;
        }
    }
}

void MoveCard::use(Player& player, Game& game) {
    game.movePlayer(player, value_);
    game.board().getTile(player.position())->onLanded(player, game);
}

void DiscountCard::use(Player& player, Game& /*game*/) {
    player.setDiscountPct(value_);
    // Discount is consumed when the player buys something (handled in GameCLI)
}

void ShieldCard::use(Player& player, Game& /*game*/) {
    player.setShielded(true);
}

void TeleportCard::use(Player& player, Game& game) {
    int target = -1;
    if (game.callbacks().onTeleport)
        target = game.callbacks().onTeleport(player);
    if (target >= 0 && target < game.board().size()) {
        game.teleportPlayer(player, target);
        game.board().getTile(target)->onLanded(player, game);
    }
}

void LassoCard::use(Player& player, Game& game) {
    if (game.callbacks().onLasso) {
        Player* target = game.callbacks().onLasso(player);
        if (target) game.applyLasso(player, *target);
    }
}

void DemolitionCard::use(Player& player, Game& game) {
    if (game.callbacks().onDemolition) {
        string code = game.callbacks().onDemolition(player);
        if (!code.empty()) {
            try { game.applyDemolition(player, code); }
            catch (const exception& e) {
                cerr << "[DemolitionCard] " << e.what() << "\n";
            }
        }
    }
}

void Game::applyDemolition(Player& caster, const string& targetCode) {
    Property* prop = board_->getProperty(targetCode);
    if (!prop || prop->type() != PropertyType::STREET)
        throw invalid_argument("Properti tidak valid.");
    if (prop->owner() == &caster)
        throw invalid_argument("Tidak bisa menghancurkan properti sendiri.");
    if (!prop->isOwned())
        throw invalid_argument("Properti tidak dimiliki siapapun.");

    auto* street = static_cast<Street*>(prop);
    if (street->buildingLevel() == 0)
        throw logic_error("Properti tidak memiliki bangunan.");

    int refund = 0;
    if (street->hasHotel()) {
        refund = street->hotelUpgradeCost() / 2;
    } else {
        refund = street->houseUpgradeCost() / 2;
    }
    int lvl = street->buildingLevel() - 1;
    street->setBuildingLevel(lvl);

    bank_.pay(*prop->owner(), refund);
    logger_.log(currentTurn_, caster.username(), "KARTU",
                "DemolitionCard: " + prop->name() +
                " dikurangi 1 level (refund M" + to_string(refund) +
                " ke " + prop->owner()->username() + ")");
}

}