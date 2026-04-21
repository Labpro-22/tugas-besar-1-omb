#pragma once
#include <string>
#include "core/types.h"

namespace Nimonspoli {

class Player; 

class Property {
public:
    Property(const std::string& code, const std::string& name,
             PropertyType type, int buyPrice, int mortgageValue)
        : code_(code), name_(name), type_(type),
          buyPrice_(buyPrice), mortgageValue_(mortgageValue) {}

    virtual ~Property() = default;
    virtual int calcRent(int diceTotal = 0) const = 0;
    const std::string& code()          const { return code_; }
    const std::string& name()          const { return name_; }
    PropertyType       type()          const { return type_; }
    PropertyStatus     status()        const { return status_; }
    int                buyPrice()      const { return buyPrice_; }
    int                mortgageValue() const { return mortgageValue_; }
    Player*            owner()         const { return owner_; }

    void setOwner(Player* p)           { owner_ = p; }
    void setStatus(PropertyStatus s)   { status_ = s; }

    bool isOwned()      const { return status_ == PropertyStatus::OWNED; }
    bool isMortgaged()  const { return status_ == PropertyStatus::MORTGAGED; }
    bool isBank()       const { return status_ == PropertyStatus::BANK; }

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

}