#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "core/types.h"
using namespace std;

namespace Nimonspoli {

class Property;
class SkillCard;
class Game;

class Player {
public:
    static constexpr int MAX_HAND_SIZE = 3;

    explicit Player(const string& username, int startBalance)
        : username_(username), balance_(startBalance) {}

    // Overload: Intinya buat duitnya, +- nambah kurang balance, <> buat ngebandingin networth
    Player& operator+=(int amount) {
        balance_ += amount;
        return *this;
    }
    Player& operator-=(int amount) {
        balance_ -= amount;
        return *this;
    }

    bool operator<(const Player& other) const {
        return this->netWorth() < other.netWorth();
    }
    bool operator>(const Player& other) const { return other < *this; }
    const std::string& username() const { return username_; }

    int  balance()     const { return balance_; }
    bool canAfford(int amount) const { return balance_ >= amount; }

    int  position()    const { return position_; }
    void setPosition(int pos) { position_ = pos; }

    PlayerStatus status()      const { return status_; }
    void setStatus(PlayerStatus s)   { status_ = s; }
    bool isActive()   const { return status_ == PlayerStatus::ACTIVE; }
    bool isJailed()   const { return status_ == PlayerStatus::JAILED; }

    bool isBankrupt() const { return status_ == PlayerStatus::BANKRUPT; }
    int  jailTurns()  const { return jailTurns_; }
    void setJailTurns(int n)    { jailTurns_ = n; }
    void incrementJailTurns()   { ++jailTurns_; }
    void resetJailTurns()       { jailTurns_ = 0; }

    void addProperty(Property* p)    { properties_.push_back(p); }
    void removeProperty(Property* p) {
        auto it = find(properties_.begin(), properties_.end(), p);
        if (it != properties_.end()) properties_.erase(it);
    }
    void clearProperties()                         { properties_.clear(); }
    const vector<Property*>& properties() const { return properties_; }
    vector<Property*>&       properties()       { return properties_; }

    bool handFull()    const { return (int)hand_.size() >= MAX_HAND_SIZE; }
    int  handSize()    const { return (int)hand_.size(); }
    void addToHand(SkillCard* card) { hand_.push_back(card); }
    void removeFromHand(int index) {
        if (index < 0 || index >= (int)hand_.size())
            throw std::out_of_range("Invalid hand index.");
        hand_.erase(hand_.begin() + index);
    }
    const vector<SkillCard*>& hand() const { return hand_; }

    bool  isShielded()       const { return shielded_; }
    void  setShielded(bool v)      { shielded_ = v; }

    bool  hasDiscount()      const { return discountPct_ > 0; }
    int   discountPct()      const { return discountPct_; }
    void  setDiscountPct(int pct)  { discountPct_ = pct; }
    void  clearDiscount()          { discountPct_ = 0; }

    bool  usedCardThisTurn()   const { return usedCardThisTurn_; }
    void  setUsedCard(bool v)        { usedCardThisTurn_ = v; }
    bool  hasRolled()          const { return hasRolled_; }
    void  setHasRolled(bool v)       { hasRolled_ = v; }

    void resetTurnState() {
        usedCardThisTurn_ = false;
        hasRolled_        = false;
        shielded_         = false;
        clearDiscount();
    }

    int netWorth() const;
    int maxLiquidation() const;
    // Tambahkan di area public
    void useSkillCard(int handIndex, Game& game);
    void dropSkillCard(int handIndex, Game& game);
    // Delegated logic dari Game
    void goToJail(Game& game);
    void handleJailTurn(Game& game);
    void receiveGoSalary(Game& game);
    void declareBankruptcy(Player* creditor, int required, Game& game);
    Property* getOwnedProperty(const std::string& code) const;
    void buildProperty(const std::string& code, Game& game);
    void mortgageProperty(const std::string& code, Game& game);
    void redeemProperty(const std::string& code, Game& game);
    void applyFestival(const std::string& code, Game& game);
private:
    string          username_;
    int                  balance_          = 0;
    int                  position_         = 0;   // tile index (0-based)
    PlayerStatus         status_           = PlayerStatus::ACTIVE;
    int                  jailTurns_        = 0;

    vector<Property*>  properties_;
    vector<SkillCard*> hand_;          

    bool  shielded_          = false;
    int   discountPct_       = 0;
    bool  usedCardThisTurn_  = false;
    bool  hasRolled_         = false;
};

}