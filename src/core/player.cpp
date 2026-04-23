#include "core/player.h"
#include "core/game.h"
#include "core/property.h"
#include "core/propertytypes.h"
#include "core/tiles.h"
#include <string>

namespace Nimonspoli {

int Player::netWorth() const {
    int total = balance_;
    for (const auto* prop : properties_) {
        total += prop->buyPrice();
        if (prop->type() == PropertyType::STREET) {
            const auto* s = static_cast<const Street*>(prop);
            int lvl = s->buildingLevel();
            if (lvl == Street::HOTEL)
                total += s->hotelUpgradeCost();
            else
                total += lvl * s->houseUpgradeCost();
        }
    }
    return total;
}

int Player::maxLiquidation() const {
    int potential = balance_;
    for (const auto* prop : properties_) {
        if (prop->isMortgaged()) {
            continue;
        }
        potential += prop->liquidationValue();
    }
    return potential;
}

void Player::goToJail(Game& game) {
    int jailIdx = 10;
    for (int i = 0; i < game.board().size(); ++i) {
        if (game.board().getTile(i)->type() == TileType::JAIL) { jailIdx = i; break; }
    }
    setPosition(jailIdx);
    setStatus(PlayerStatus::JAILED);
    resetJailTurns();
    game.dice().resetDoubleCount();
    game.logger().log(game.currentTurn(), username_, "PENJARA", "Masuk penjara");
}

void Player::handleJailTurn(Game& game) {
    if (jailTurns_ >= 3) {
        if (!canAfford(game.specialConfig().jailFine)) { 
            game.handleBankruptcy(*this, nullptr, game.specialConfig().jailFine); 
            return; 
        }
        game.bank().collect(*this, game.specialConfig().jailFine);
        setStatus(PlayerStatus::ACTIVE);
        resetJailTurns();
        game.logger().log(game.currentTurn(), username_, "KELUAR_PENJARA", "Bayar denda M" + std::to_string(game.specialConfig().jailFine) + " (wajib)");
        game.cmdRollDice();
        return;
    }
    
    auto [d1, d2] = game.dice().roll();
    if (game.callbacks().onDiceRolled) game.callbacks().onDiceRolled(d1, d2);
    
    game.logger().log(game.currentTurn(), username_, "DADU_PENJARA", "Lempar: " + std::to_string(d1) + "+" + std::to_string(d2));
    
    if (game.dice().isDouble()) {
        setStatus(PlayerStatus::ACTIVE);
        resetJailTurns();
        game.dice().resetDoubleCount();
        game.logger().log(game.currentTurn(), username_, "KELUAR_PENJARA", "Dadu double!");
        setHasRolled(true);
        game.movePlayer(*this, d1 + d2);
        game.processLanding(*this, position_, d1 + d2);
    } else {
        incrementJailTurns();
        game.logger().log(game.currentTurn(), username_, "PENJARA", "Gagal keluar (percobaan " + std::to_string(jailTurns_) + "/3)");
        setHasRolled(true);
    }
}

void Player::receiveGoSalary(Game& game) {
    game.bank().pay(*this, game.specialConfig().goSalary);
    game.logger().log(game.currentTurn(), username_, "GAJI", "Melewati/berhenti di GO, menerima M" + std::to_string(game.specialConfig().goSalary));
}

void Player::declareBankruptcy(Player* creditor, int required, Game& game) {
    game.logger().log(game.currentTurn(), username_, "BANGKRUT", creditor ? "Kreditor: " + creditor->username() : "Kreditor: Bank");
    
    // Trigger likuidasi HANYA jika pemain punya aset yang bisa dilikuidasi
    if (game.callbacks().onLiquidation && !properties_.empty()) {
        game.callbacks().onLiquidation(*this, required, creditor);
        if (!isBankrupt()) return; // Terselamatkan oleh likuidasi
    }
    
    setStatus(PlayerStatus::BANKRUPT);
    std::vector<Property*> assets = properties_;
    
    if (creditor) {
        for (auto* prop : assets) {
            prop->setOwner(creditor);
            creditor->addProperty(prop);
        }
        game.refreshPropertyCounts(this);
        game.refreshPropertyCounts(creditor);
        
        creditor->operator+=(balance_);
        balance_ = 0;
        
        game.logger().log(game.currentTurn(), username_, "BANGKRUT", "Semua aset dialihkan ke " + creditor->username());
    } else {
        for (auto* prop : assets) {
            if (prop->type() == PropertyType::STREET) {
                // Cast aman karena kita tahu ini Street
                static_cast<Street*>(prop)->demolishAll();
            }
            prop->setOwner(nullptr);
            prop->setStatus(PropertyStatus::BANK);
            game.handleAuction(*prop);
        }
        game.refreshPropertyCounts(this);
        game.bank().collect(*this, balance_);
        game.logger().log(game.currentTurn(), username_, "BANGKRUT", "Semua properti dikembalikan ke Bank dan dilelang");
    }
    
    clearProperties();
}
}