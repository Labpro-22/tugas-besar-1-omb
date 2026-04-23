#pragma once
#include "core/property.h"
#include "core/festivaleffect.h"
#include "core/config.h"
#include <array>
#include <stdexcept>
 
namespace Nimonspoli {
class Street : public Property {
public:
    static constexpr int MAX_HOUSES = 4;
    static constexpr int HOTEL      = 5;
 
    Street(const std::string& code, const std::string& name,
           ColorGroup colorGroup,
           int buyPrice, int mortgageValue,
           int houseUpgradeCost, int hotelUpgradeCost,
           const std::array<int,6>& rentTable)
        : Property(code, name, PropertyType::STREET, buyPrice, mortgageValue),
          colorGroup_(colorGroup),
          houseUpgradeCost_(houseUpgradeCost),
          hotelUpgradeCost_(hotelUpgradeCost),
          rentTable_(rentTable) {}
 
    // Rent
    int calcRent(int diceTotal = 0) const override {
        int base = rentTable_[buildingLevel_]; 
        if (buildingLevel_ == 0 && monopoly_) base *= 2;
        return base * festival_.multiplier();
    }
 
    // Build management
    int  buildingLevel()     const { return buildingLevel_; }
    bool hasHotel()          const { return buildingLevel_ == HOTEL; }
    bool canBuildHouse()     const { return buildingLevel_ < MAX_HOUSES && monopoly_; }
    bool canUpgradeToHotel() const { return buildingLevel_ == MAX_HOUSES && monopoly_; }
 
    // Cost untuk +1 rumah/hotel
    int nextBuildCost() const {
        return (buildingLevel_ < MAX_HOUSES) ? houseUpgradeCost_ : hotelUpgradeCost_;
    }
 
    // Jual semua (refund = 1/2 harga beli)
    int demolishAll() {
        int refund = 0;
        if (buildingLevel_ == HOTEL) {
            refund = hotelUpgradeCost_ / 2;
        } else {
            refund = (buildingLevel_ * houseUpgradeCost_) / 2;
        }
        buildingLevel_ = 0;
        return refund;
    }
 
    void addBuilding() {
        if (buildingLevel_ >= HOTEL)
            throw std::logic_error("Cannot build further — already has hotel.");
        ++buildingLevel_;
    }

    void performMortgage(Player& player, Game& game) override;
    void buildHouseOrHotel(Player& player, Game& game);
    void applyFestivalBoost(Player& player, Game& game);
 
    FestivalEffect&       festival()       { return festival_; }
    const FestivalEffect& festival() const { return festival_; }
 
    void setMonopoly(bool v) { monopoly_ = v; }
    bool hasMonopoly()  const { return monopoly_; }
 
    ColorGroup colorGroup() const { return colorGroup_; }
    int houseUpgradeCost()  const { return houseUpgradeCost_; }
    int hotelUpgradeCost()  const { return hotelUpgradeCost_; }
 
    int liquidationValue() const override {
        int buildingValue = 0;
        if (buildingLevel_ == HOTEL)
            buildingValue = hotelUpgradeCost_ / 2;
        else
            buildingValue = (buildingLevel_ * houseUpgradeCost_) / 2;
        return buyPrice_ + buildingValue;
    }
 
    void setBuildingLevel(int lvl) { buildingLevel_ = lvl; }
 
private:
    ColorGroup         colorGroup_;
    int                houseUpgradeCost_;
    int                hotelUpgradeCost_;
    std::array<int,6>  rentTable_;   
    int                buildingLevel_ = 0;
    bool               monopoly_      = false;
    FestivalEffect     festival_;
};
 
 
class Railroad : public Property {
public:
    Railroad(const std::string& code, const std::string& name,
             int mortgageValue, const RailroadConfig& cfg)
        : Property(code, name, PropertyType::RAILROAD, 0, mortgageValue),
          cfg_(cfg) {}
 
    void setOwnerRailroadCount(int n) { ownerRRCount_ = n; }
 
    int calcRent(int diceTotal = 0) const override {
        auto it = cfg_.rentTable.find(ownerRRCount_);
        return (it != cfg_.rentTable.end()) ? it->second : 0;
    }
 
private:
    const RailroadConfig& cfg_;
    int ownerRRCount_ = 1;
};
 
class Utility : public Property {
public:
    Utility(const std::string& code, const std::string& name,
            int mortgageValue, const UtilityConfig& cfg)
        : Property(code, name, PropertyType::UTILITY, 0, mortgageValue),
          cfg_(cfg) {}
 
    void setOwnerUtilityCount(int n) { ownerUtilCount_ = n; }
 
    int calcRent(int diceTotal = 0) const override {
        auto it = cfg_.multiplierTable.find(ownerUtilCount_);
        int mult = (it != cfg_.multiplierTable.end()) ? it->second : 4;
        return diceTotal * mult;
    }
 
private:
    const UtilityConfig& cfg_;
    int ownerUtilCount_ = 1;
};
 
}