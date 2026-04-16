#pragma once
#include <string>
#include "core/Types.h"

namespace Nimonspoli {

class Player; // forward declaration

class Property {
public:
    Property(const std::string& code, const std::string& name,
             PropertyType type, int buyPrice, int mortgageValue)
        : code_(code), name_(name), type_(type),
          buyPrice_(buyPrice), mortgageValue_(mortgageValue) {}

    virtual ~Property() = default;

    // Core polymorphic method — each subclass computes rent differently
    // diceTotal is only used by Utility; ignored by Street and Railroad
    virtual int calcRent(int diceTotal = 0) const = 0;

    // ── Getters ─────────────────────────────────────────────────────────────
    const std::string& code()          const { return code_; }
    const std::string& name()          const { return name_; }
    PropertyType       type()          const { return type_; }
    PropertyStatus     status()        const { return status_; }
    int                buyPrice()      const { return buyPrice_; }
    int                mortgageValue() const { return mortgageValue_; }
    Player*            owner()         const { return owner_; }

    // ── Ownership ────────────────────────────────────────────────────────────
    void setOwner(Player* p)           { owner_ = p; }
    void setStatus(PropertyStatus s)   { status_ = s; }

    bool isOwned()      const { return status_ == PropertyStatus::OWNED; }
    bool isMortgaged()  const { return status_ == PropertyStatus::MORTGAGED; }
    bool isBank()       const { return status_ == PropertyStatus::BANK; }

    // Total liquidation value (overridden by Street to include buildings)
    virtual int liquidationValue() const { return buyPrice_; }

protected:
    std::string    code_;
    std::string    name_;
    PropertyType   type_;
    PropertyStatus status_  = PropertyStatus::BANK;
    int            buyPrice_;
    int            mortgageValue_;
    Player*        owner_   = nullptr;
};

} // namespace Nimonspoli