#pragma once
#include <string>

namespace Nimonspoli {

// Player
enum class PlayerStatus { ACTIVE, JAILED, BANKRUPT };

// Property
enum class PropertyStatus { BANK, OWNED, MORTGAGED };
enum class PropertyType   { STREET, RAILROAD, UTILITY };

// Tilez 
enum class TileType {
    PROPERTY,       
    GO,
    JAIL,
    FREE_PARKING,
    GO_TO_JAIL,
    CHANCE,
    COMMUNITY,
    FESTIVAL,
    TAX_PPH,
    TAX_PBM
};

// Card
enum class SkillCardType {
    MOVE, DISCOUNT, SHIELD, TELEPORT, LASSO, DEMOLITION
};

enum class ChanceEffect {
    GO_NEAREST_STATION, MOVE_BACK_3, GO_TO_JAIL
};

enum class CommunityEffect {
    BIRTHDAY, DOCTOR, ELECTION
};

// Tax
enum class TaxType { PPH, PBM };

// ColorGroup
enum class ColorGroup {
    COKLAT, BIRU_MUDA, MERAH_MUDA, ORANGE,
    MERAH, KUNING, HIJAU, BIRU_TUA,
    DEFAULT   // railroad / utility / action tiles
};

inline std::string colorGroupToString(ColorGroup cg) {
    switch (cg) {
    case ColorGroup::COKLAT:     return "COKLAT";
    case ColorGroup::BIRU_MUDA:  return "BIRU_MUDA";
    case ColorGroup::BIRU_TUA:   return "BIRU_TUA";
    case ColorGroup::ORANGE:     return "ORANGE";
    case ColorGroup::MERAH:      return "MERAH";
    case ColorGroup::MERAH_MUDA: return "MERAH_MUDA";
    case ColorGroup::KUNING:     return "KUNING";
    case ColorGroup::HIJAU:      return "HIJAU";
    default:                     return "DEFAULT";
    }
}

inline ColorGroup stringToColorGroup(const std::string& s) {
    if (s == "COKLAT")     return ColorGroup::COKLAT;
    if (s == "BIRU_MUDA")  return ColorGroup::BIRU_MUDA;
    if (s == "BIRU_TUA")   return ColorGroup::BIRU_TUA;

    if (s == "ORANGE")     return ColorGroup::ORANGE;
    if (s == "MERAH_MUDA") return ColorGroup::MERAH_MUDA;
    if (s == "MERAH")      return ColorGroup::MERAH;
    if (s == "KUNING")     return ColorGroup::KUNING;
    if (s == "HIJAU")      return ColorGroup::HIJAU;

    return ColorGroup::DEFAULT;
}

} // namespace Nimonspoli