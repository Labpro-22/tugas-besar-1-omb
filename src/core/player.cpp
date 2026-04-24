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

void Player::useSkillCard(int handIndex, Game& game) {
    if (hasRolled_) throw std::logic_error("Kartu kemampuan hanya bisa digunakan SEBELUM melempar dadu.");
    if (usedCardThisTurn_) throw std::logic_error("Penggunaan kartu dibatasi maksimal 1 kali dalam 1 giliran.");
    if (handIndex < 0 || handIndex >= handSize()) throw std::out_of_range("Indeks kartu tidak valid.");
    
    SkillCard* card = hand_[handIndex];
    removeFromHand(handIndex);
    
    card->use(*this, game);
    
    setUsedCard(true);
    game.skillDeck().discard(card);
    game.logger().log(game.currentTurn(), username_, "KARTU", "Pakai " + card->description());
}

void Player::dropSkillCard(int handIndex, Game& game) {
    if (handIndex < 0 || handIndex >= handSize()) throw std::out_of_range("Indeks kartu tidak valid.");
    
    SkillCard* card = hand_[handIndex];
    removeFromHand(handIndex);
    game.skillDeck().discard(card);
    game.logger().log(game.currentTurn(), username_, "DROP_KARTU", "Membuang " + card->description());
}


Property* Player::getOwnedProperty(const std::string& code) const {
    for (auto* prop : properties_) {
        if (prop->code() == code) return prop;
    }
    return nullptr; 
}

void Player::buildProperty(const std::string& code, Game& game) {
    Property* prop = getOwnedProperty(code);
    if (!prop) throw std::invalid_argument("Properti tidak ditemukan atau bukan milikmu.");
    if (prop->type() != PropertyType::STREET) throw std::invalid_argument("Hanya properti Street yang bisa dibangun.");
    
    static_cast<Street*>(prop)->buildHouseOrHotel(*this, game);
}

void Player::mortgageProperty(const std::string& code, Game& game) {
    Property* prop = getOwnedProperty(code);
    if (!prop) throw std::invalid_argument("Properti tidak ditemukan atau bukan milikmu.");
    
    prop->performMortgage(*this, game);
}

void Player::redeemProperty(const std::string& code, Game& game) {
    Property* prop = getOwnedProperty(code);
    if (!prop) throw std::invalid_argument("Properti tidak ditemukan atau bukan milikmu.");
    
    prop->performRedeem(*this, game);
}

void Player::applyFestival(const std::string& code, Game& game) {
    Property* prop = getOwnedProperty(code);
    if (!prop) throw std::invalid_argument("Properti tidak ditemukan atau bukan milikmu.");
    if (prop->type() != PropertyType::STREET) throw std::invalid_argument("Hanya properti Street yang dapat mengadakan festival.");
    
    static_cast<Street*>(prop)->applyFestivalBoost(*this, game);
}
}