#include "core/player.h"
#include "core/property.h"
#include "core/propertytypes.h"

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

}