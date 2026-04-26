#include "core/cards.h"
#include "core/game.h"
#include "core/player.h"
#include "core/board.h"
#include "core/propertytypes.h"
#include <stdexcept>
#include <iostream>

using namespace std;

namespace Nimonspoli {

ChanceGoNearestStation::ChanceGoNearestStation() 
    : ChanceCard("Pergi ke stasiun terdekat.") {}

void ChanceGoNearestStation::execute(Player& player, Game& game) {
    int oldPos = player.position();
    int nearest = game.board().nearestRailroad(oldPos);
    int steps = (nearest - oldPos + game.board().size()) % game.board().size();
    std::cout << "Bidak dipindahkan ke stasiun terdekat: "
              << game.board().getTile(nearest)->name() << ".\n";
    game.movePlayer(player, steps, true);
    game.board().getTile(nearest)->onLanded(player, game);
}

ChanceMoveBack3::ChanceMoveBack3() 
    : ChanceCard("Mundur 3 petak.") {}

void ChanceMoveBack3::execute(Player& player, Game& game) {
    int newPos = (player.position() - 3 + game.board().size()) % game.board().size();
    std::cout << "Bidak mundur 3 petak ke: " << game.board().getTile(newPos)->name() << ".\n";
    game.teleportPlayer(player, newPos);
    game.board().getTile(newPos)->onLanded(player, game);
}

ChanceGoToJail::ChanceGoToJail() 
    : ChanceCard("Masuk Penjara.") {}

void ChanceGoToJail::execute(Player& player, Game& game) {
    std::cout << "Bidak dipindahkan ke Penjara!\n";
    game.sendToJail(player);
}

CommunityBirthday::CommunityBirthday() 
    : CommunityCard("Ini adalah hari ulang tahun Anda. Dapatkan M100 dari setiap pemain.") {}

void CommunityBirthday::execute(Player& player, Game& game) {
    auto active = game.activePlayers();
    int received = 0;
    for (auto* other : active) {
        if (other == &player) continue;
        if (!other->canAfford(100)) {
            game.handleBankruptcy(*other, &player, 100);
        } else {
            game.bank().transfer(*other, player, 100);
            std::cout << other->username() << " membayar M100 kepada " << player.username() << ".\n";
            TransactionLogger::log(game.currentTurn(), other->username(),
                              "DANA_UMUM", "Bayar M100 ulang tahun ke " + player.username());
            received += 100;
        }
    }
    std::cout << player.username() << " menerima total M" << received
              << " hadiah ulang tahun. Saldo: M" << player.balance() << "\n";
}

CommunityDoctor::CommunityDoctor() 
    : CommunityCard("Biaya dokter. Bayar M700.") {}

void CommunityDoctor::execute(Player& player, Game& game) {
    if (!player.canAfford(700)) {
        std::cout << "Kamu tidak mampu membayar biaya dokter! (M700)\n"
                  << "Uang kamu saat ini: M" << player.balance() << "\n";
        game.handleBankruptcy(player, nullptr, 700);
    } else {
        int before = player.balance();
        game.bank().collect(player, 700);
        std::cout << "Kamu membayar M700 ke Bank. Saldo: M" << before << " -> M" << player.balance() << "\n";
        TransactionLogger::log(game.currentTurn(), player.username(),
                          "DANA_UMUM", "Biaya dokter M700");
    }
}

CommunityElection::CommunityElection() 
    : CommunityCard("Anda mau nyaleg. Bayar M200 kepada setiap pemain.") {}

void CommunityElection::execute(Player& player, Game& game) {
    auto active = game.activePlayers();
    int paid = 0;
    for (auto* other : active) {
        if (other == &player) continue;
        if (!player.canAfford(200)) {
            std::cout << "Kamu tidak mampu membayar M200 kepada " << other->username() << "!\n"
                      << "Uang kamu saat ini: M" << player.balance() << "\n";
            game.handleBankruptcy(player, other, 200);
            return;
        }
        game.bank().transfer(player, *other, 200);
        std::cout << player.username() << " membayar M200 kepada " << other->username() << ".\n";
        TransactionLogger::log(game.currentTurn(), player.username(),
                          "DANA_UMUM", "Bayar M200 nyaleg ke " + other->username());
        paid += 200;
    }
    std::cout << player.username() << " total membayar M" << paid
              << " untuk nyaleg. Saldo: M" << player.balance() << "\n";
}

void MoveCard::use(Player& player, Game& game) {
    std::cout << "MoveCard diaktifkan! Maju " << value_ << " petak.\n";
    game.movePlayer(player, value_);
    game.board().getTile(player.position())->onLanded(player, game);
}

void DiscountCard::use(Player& player, Game& ) {
    player.setDiscountPct(value_);
    std::cout << "DiscountCard aktif! Diskon " << value_ << "% untuk pembelian properti berikutnya.\n";
}

void ShieldCard::use(Player& player, Game&) {
    player.setShielded(true);
    std::cout << "ShieldCard diaktifkan! Kamu kebal terhadap tagihan atau sanksi selama giliran ini.\n";
}

void FreeJailCard::use(Player&, Game& ) {
    std::cout << "FreeJailCard hanya bisa digunakan saat berada di penjara.\n";
}

void TeleportCard::use(Player& player, Game& game) {
    int target = -1;
    if (game.callbacks().onTeleport)
        target = game.callbacks().onTeleport(player);
    if (target >= 0 && target < game.board().size()) {
        std::cout << "TeleportCard! Pindah ke " << game.board().getTile(target)->name() << ".\n";
        game.teleportPlayer(player, target);
        game.board().getTile(target)->onLanded(player, game);
    }
}

void LassoCard::use(Player& player, Game& game) {
    if (game.callbacks().onLasso) {
        Player* target = game.callbacks().onLasso(player);
        if (target) {
            int dest = player.position();
            target->setPosition(dest);
            std::cout << "LassoCard! " << target->username()
                      << " ditarik ke " << game.board().getTile(dest)->name()
                      << " (" << game.board().getTile(dest)->code() << ").\n";
            TransactionLogger::log(game.currentTurn(), player.username(), "KARTU", "LassoCard: " + target->username() + " ditarik ke " + game.board().getTile(dest)->name());
            game.board().getTile(dest)->onLanded(*target, game);
        }
    }
}

void DemolitionCard::use(Player& player, Game& game) {
    if (game.callbacks().onDemolition) {
        string code = game.callbacks().onDemolition(player);
        if (!code.empty()) {
            try { 
                Property* prop = game.board().getProperty(code);
                if (!prop || prop->type() != PropertyType::STREET)
                    throw invalid_argument("Properti tidak valid.");
                if (prop->owner() == &player)
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

                game.bank().pay(*prop->owner(), refund);
                std::cout << "DemolitionCard! " << prop->name()
                          << " dikurangi 1 level bangunan. "
                          << "Refund M" << refund
                          << " diberikan ke " << prop->owner()->username() << ".\n";
                TransactionLogger::log(game.currentTurn(), player.username(), "KARTU",
                            "DemolitionCard: " + prop->name() +
                            " dikurangi 1 level (refund M" + to_string(refund) +
                            " ke " + prop->owner()->username() + ")");
            } catch (const exception& e) {
                cerr << "[DemolitionCard] " << e.what() << "\n";
            }
        }
    }
}

}