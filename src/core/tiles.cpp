#include "core/tiles.h"
#include "core/game.h"
#include "core/player.h"
#include "core/propertytypes.h"
#include <iostream>
#include <sstream>

namespace Nimonspoli {

std::string PropertyTile::summary() const {
    std::ostringstream oss;
    oss << property_->name() << " (" << property_->code() << ")";
    if (property_->isOwned() && property_->owner())
        oss << " [" << property_->owner()->username() << "]";
    else if (property_->isMortgaged())
        oss << " [GADAI]";
    else
        oss << " [BANK]";
    return oss.str();
}

void PropertyTile::onLanded(Player& player, Game& game) {
    if (property_->isBank()) {
        game.handlePropertyPurchase(player, *property_);
    } else if (property_->isOwned()) {
        if (property_->owner() == &player) {
            std::cout << "Properti ini milikmu sendiri. Tidak ada sewa.\n";
        } else {
            game.handleRentPayment(player, *property_, game.lastDiceTotal());
        }
    } else if (property_->isMortgaged()) {
        std::cout << "Kamu mendarat di " << property_->name()
                  << " (" << property_->code() << "), milik "
                  << (property_->owner() ? property_->owner()->username() : "?")
                  << ".\nProperti ini sedang digadaikan [M]. Tidak ada sewa yang dikenakan.\n";
    }
}

void GoTile::onLanded(Player& player, Game& game) {
    // Salary sudah otomatis diterima di Player::move() saat melewati/berhenti di GO.
    // Di sini hanya tampilkan pesan konfirmasi — tidak bayar ulang.
    std::cout << "Kamu berhenti tepat di Petak Mulai (GO)! Gaji M" << salary_
              << " sudah diterima. Saldo: M" << player.balance() << "\n";
    (void)game;
}

void JailTile::onLanded(Player& player, Game& game) {
    std::cout << "Kamu hanya mampir di Penjara. Tidak ada penalti — selamat jalan!\n";
    TransactionLogger::log(game.currentTurn(), player.username(),
                      "MAMPIR", "Hanya mampir di Penjara");
}

void GoToJailTile::onLanded(Player& player, Game& game) {
    std::cout << "Kamu mendarat di Petak Pergi ke Penjara!\n"
              << "Bidak dipindahkan ke Penjara — kamu menjadi tahanan! Giliran berakhir.\n";
    game.sendToJail(player);
}

void TaxTile::onLanded(Player& player, Game& game) {
    if (taxType_ == TaxType::PPH) {
        game.handleTaxPPH(player);
    } else {
        std::cout << "Kamu mendarat di Pajak Barang Mewah (PBM)!\n";
        game.handleTaxPBM(player);
    }
}

void FestivalTile::onLanded(Player& player, Game& game) {
    std::cout << "Kamu mendarat di petak Festival!\n";
    game.handleFestival(player);
}

void ChanceTile::onLanded(Player& player, Game& game) {
    std::cout << "Kamu mendarat di Petak Kesempatan!\nMengambil kartu...\n";
    ChanceCard* card = game.chanceDeck().draw();
    std::cout << "Kartu: \"" << card->description() << "\"\n";
    TransactionLogger::log(game.currentTurn(), player.username(),
                      "KESEMPATAN", card->description());
    card->execute(player, game);
    game.chanceDeck().discard(card);
}

void CommunityTile::onLanded(Player& player, Game& game) {
    std::cout << "Kamu mendarat di Petak Dana Umum!\nMengambil kartu...\n";
    CommunityCard* card = game.communityDeck().draw();
    std::cout << "Kartu: \"" << card->description() << "\"\n";
    TransactionLogger::log(game.currentTurn(), player.username(),
                      "DANA_UMUM", card->description());
    card->execute(player, game);
    game.communityDeck().discard(card);
}

}