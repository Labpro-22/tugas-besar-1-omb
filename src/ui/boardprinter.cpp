#include "ui/boardprinter.h"
#include "core/propertytypes.h"
#include "core/tiles.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <array>
#include <vector>

using namespace std;

namespace Nimonspoli {

static string fit10(const string& s) {
    if ((int)s.size() >= 10) return s.substr(0, 10);
    return s + string(10 - (int)s.size(), ' ');
}

const char* BoardPrinter::groupColor(ColorGroup cg) const {
    switch (cg) {
        case ColorGroup::COKLAT:     return Color::COKLAT;
        case ColorGroup::BIRU_MUDA:  return Color::BIRU_MUDA;
        case ColorGroup::MERAH_MUDA: return Color::MERAH_MUDA;
        case ColorGroup::ORANGE:     return Color::ORANGE_C;
        case ColorGroup::MERAH:      return Color::MERAH;
        case ColorGroup::KUNING:     return Color::KUNING;
        case ColorGroup::HIJAU:      return Color::HIJAU;
        case ColorGroup::BIRU_TUA:   return Color::BIRU_TUA;
        default:                     return Color::DEFAULT_C;
    }
}

string BoardPrinter::buildingStr(int level) const {
    if (level == 0) return "";
    if (level == Street::HOTEL) return "*";
    return string(level, '^');
}

string BoardPrinter::ownerStr(const Property* prop) const {
    if (!prop || prop->isBank()) return "";
    const auto& players = game_.players();
    for (int i = 0; i < (int)players.size(); ++i) {
        if (players[i].get() == prop->owner())
            return "P" + to_string(i + 1);
    }
    return "??";
}

string BoardPrinter::playerBadges(int tileIndex) const {
    string out;
    const auto& players = game_.players();
    for (int i = 0; i < (int)players.size(); ++i) {
        const auto& p = players[i];
        if (p->position() == tileIndex && !p->isBankrupt()) {
            string badge = "(" + to_string(i + 1) + ")";
            if (p->isJailed()) badge = "IN:" + to_string(i + 1);
            out += badge;
        }
    }
    return out;
}

string BoardPrinter::colorCode(int idx) const {
    auto* tile = game_.board().getTile(idx);
    if (tile->type() == TileType::PROPERTY) {
        auto* pt = static_cast<const PropertyTile*>(tile);
        auto* prop = pt->property();
        if (prop->type() == PropertyType::STREET) {
            auto* s = static_cast<const Street*>(prop);
            return groupColor(s->colorGroup());
        }
        if (prop->type() == PropertyType::UTILITY) return Color::UTIL;
    }
    return Color::DEFAULT_C;
}

string BoardPrinter::colorTag(int idx) const {
    auto* tile = game_.board().getTile(idx);
    if (tile->type() == TileType::PROPERTY) {
        auto* pt = static_cast<const PropertyTile*>(tile);
        auto* prop = pt->property();
        if (prop->type() == PropertyType::UTILITY) return "AB";
        if (prop->type() == PropertyType::STREET) {
            auto* s = static_cast<const Street*>(prop);
            switch (s->colorGroup()) {
                case ColorGroup::COKLAT: return "CK";
                case ColorGroup::BIRU_MUDA: return "BM";
                case ColorGroup::MERAH_MUDA: return "PK";
                case ColorGroup::ORANGE: return "OR";
                case ColorGroup::MERAH: return "MR";
                case ColorGroup::KUNING: return "KN";
                case ColorGroup::HIJAU: return "HJ";
                case ColorGroup::BIRU_TUA: return "BT";
                default: return "DF";
            }
        }
    }
    return "DF";
}

array<string,2> BoardPrinter::tileLines(int idx) const {
    auto* tile = game_.board().getTile(idx);
    string line1 = "[" + colorTag(idx) + "] " + tile->code();
    string line2;

    if (tile->type() == TileType::PROPERTY) {
        auto* pt = static_cast<const PropertyTile*>(tile);
        auto* prop = pt->property();
        string owner = ownerStr(prop);
        if (!owner.empty()) {
            line2 += owner;
            if (prop->type() == PropertyType::STREET) {
                auto* s = static_cast<const Street*>(prop);
                string b = buildingStr(s->buildingLevel());
                if (!b.empty()) line2 += " " + b;
            }
            if (prop->isMortgaged()) line2 += " [M]";
        }
    }

    if (tile->type() == TileType::JAIL) {
        string jailed, visiting;
        const auto& players = game_.players();
        for (int i = 0; i < (int)players.size(); ++i) {
            const auto& p = players[i];
            if (p->position() == idx && !p->isBankrupt()) {
                if (p->isJailed()) {
                    if (!jailed.empty()) jailed += ",";
                    jailed += to_string(i + 1);
                } else {
                    if (!visiting.empty()) visiting += ",";
                    visiting += to_string(i + 1);
                }
            }
        }
        line2.clear();
        if (!jailed.empty()) line2 += "IN:" + jailed;
        if (!visiting.empty()) {
            if (!line2.empty()) line2 += " ";
            line2 += "V:" + visiting;
        }
    } else {
        string badges = playerBadges(idx);
        if (!badges.empty()) {
            if (!line2.empty()) line2 += " ";
            line2 += badges;
        }
    }

    return {line1, line2};
}

string BoardPrinter::tileCell(int idx) const {
    auto* tile  = game_.board().getTile(idx);
    string code   = tile->code();
    string owner, bldg, badges;

    if (tile->type() == TileType::PROPERTY) {
        auto* pt   = static_cast<const PropertyTile*>(tile);
        auto* prop = pt->property();
        owner  = ownerStr(prop);
        if (prop->type() == PropertyType::STREET) {
            auto* s = static_cast<const Street*>(prop);
            bldg = buildingStr(s->buildingLevel());
        }
        if (prop->isMortgaged()) owner += "[M]";
    }

    if (tile->type() == TileType::JAIL) {
        string jailed, visiting;
        for (auto& p : game_.players()) {
            int pi = &p - &game_.players()[0];
            if (p->position() == idx) {
                if (p->isJailed()) {
                    if (!jailed.empty()) jailed += ",";
                    jailed += to_string(pi + 1);
                } else {
                    if (!visiting.empty()) visiting += ",";
                    visiting += to_string(pi + 1);
                }
            }
        }
        string cell = code;
        if (!jailed.empty())  cell += " IN:" + jailed;
        if (!visiting.empty()) cell += " V:" + visiting;
        return cell;
    }

    badges = playerBadges(idx);
    string content = code;
    if (!owner.empty())  content += " " + owner;
    if (!bldg.empty())   content += " " + bldg;
    if (!badges.empty()) content += " " + badges;
    return content;
}

static void printHRule(int cols = 11) {
    cout << "+";
    for (int i = 0; i < cols; ++i) cout << "----------+";
    cout << "\n";
}

void BoardPrinter::printTopRow() const {
    printHRule();
    cout << "|";
    for (int i = 20; i <= 30; ++i) {
        auto lines = tileLines(i);
        string col = colorCode(i);
        cout << col << fit10(lines[0]) << Color::RESET << "|";
    }
    cout << "\n|";
    for (int i = 20; i <= 30; ++i) {
        auto lines = tileLines(i);
        string col = colorCode(i);
        cout << col << fit10(lines[1]) << Color::RESET << "|";
    }
    cout << "\n";
    printHRule();
}

void BoardPrinter::printBottomRow() const {
    printHRule();
    cout << "|";
    for (int i = 10; i >= 0; --i) {
        auto lines = tileLines(i);
        string col = colorCode(i);
        cout << col << fit10(lines[0]) << Color::RESET << "|";
    }
    cout << "\n|";
    for (int i = 10; i >= 0; --i) {
        auto lines = tileLines(i);
        string col = colorCode(i);
        cout << col << fit10(lines[1]) << Color::RESET << "|";
    }
    cout << "\n";
    printHRule();
}

void BoardPrinter::printSideRows() const {
    const int centerWidth = 98;
    vector<string> centerContent(26, string(centerWidth, ' '));

    auto putText = [&](int lineIdx, const string& text, bool centered) {
        if (lineIdx < 0 || lineIdx >= 26) return;
        string res = string(centerWidth, ' ');
        if (centered) {
            int padding = (centerWidth - (int)text.length()) / 2;
            if (padding < 0) padding = 0;
            for (size_t i = 0; i < text.length() && i + padding < centerWidth; ++i) {
                res[padding + i] = text[i];
            }
        } else {
            int offset = 28; 
            for (size_t i = 0; i < text.length() && i + offset < centerWidth; ++i) {
                res[offset + i] = text[i];
            }
        }
        centerContent[lineIdx] = res;
    };

    // Row 1 & 2 content
    putText(3, "==================================================", true);
    putText(4, "||                  NIMONSPOLI                  ||", true);
    putText(5, "==================================================", true);

    string turn_str = "TURN " + to_string(game_.currentTurn());
    if (game_.maxTurn() >= 1) turn_str += " / " + to_string(game_.maxTurn());
    else turn_str += " (unlimited)";
    putText(7, turn_str, true);

    // Row 3 Legend Separator
    putText(9, "--------------------------------------------------", true);
    
    // Row 4+ Legend Block
    putText(10, "LEGENDA KEPEMILIKAN & STATUS", false);
    putText(11, "P1-P4 : Properti milik Pemain 1-4", false);
    putText(12, "^     : Rumah Level 1", false);
    putText(13, "^^    : Rumah Level 2", false);
    putText(14, "^^^   : Rumah Level 3", false);
    putText(15, "* : Hotel (Maksimal)", false);
    putText(16, "(1)-(4): Bidak (IN=Tahanan, V=Mampir)", false);

    putText(18, "KODE WARNA:", false);
    putText(19, "[CK]=Coklat    [MR]=Merah", false);
    putText(20, "[BM]=Biru Muda [KN]=Kuning", false);
    putText(21, "[PK]=Pink      [HJ]=Hijau", false);
    putText(22, "[OR]=Orange    [BT]=Biru Tua", false);
    putText(23, "[DF]=Aksi      [AB]=Utilitas", false);

    for (int row = 0; row < 9; ++row) {
        int leftIdx  = 19 - row;
        int rightIdx = 31 + row;
        auto l = tileLines(leftIdx);
        auto r = tileLines(rightIdx);
        string lc = colorCode(leftIdx);
        string rc = colorCode(rightIdx);

        // Line 1
        cout << "|" << lc << fit10(l[0]) << Color::RESET << "|";
        cout << centerContent[row * 3];
        cout << "|" << rc << fit10(r[0]) << Color::RESET << "|\n";

        // Line 2
        cout << "|" << lc << fit10(l[1]) << Color::RESET << "|";
        cout << centerContent[row * 3 + 1];
        cout << "|" << rc << fit10(r[1]) << Color::RESET << "|\n";

        // Side Rule separator (kecuali pada petak paling bawah)
        if (row < 8) {
            cout << "+----------+";
            cout << centerContent[row * 3 + 2];
            cout << "+----------+\n";
        }
    }
}

void BoardPrinter::printBoard() const {
    cout << "\n";
    printTopRow();
    printSideRows();
    printBottomRow();
    cout << "\n";
}

void BoardPrinter::printDeed(const string& code) const {
    Property* prop = game_.board().getProperty(code);
    if (!prop) {
        cout << "Petak \"" << code << "\" tidak ditemukan atau bukan properti.\n";
        return;
    }

    const char* col = Color::DEFAULT_C;
    if (prop->type() == PropertyType::STREET) {
        auto* s = static_cast<Street*>(prop);
        col = groupColor(s->colorGroup());
    }

    string titleStr = "[" + colorGroupToString(prop->type() == PropertyType::STREET ? static_cast<Street*>(prop)->colorGroup() : ColorGroup::DEFAULT) + "] " + prop->name() + " (" + prop->code() + ")";
    
    int innerWidth = 30;
    int padding = (innerWidth - (int)titleStr.length()) / 2;
    if (padding < 0) padding = 0;
    
    string centeredTitle = string(padding, ' ') + titleStr;
    if (centeredTitle.length() < innerWidth) {
        centeredTitle += string(innerWidth - centeredTitle.length(), ' ');
    }

    cout << col
         << "+================================+\n"
         << "|       AKTA KEPEMILIKAN         |\n"
         << "| " << centeredTitle << " |\n"
         << "+================================+\n"
         << Color::RESET;

    auto row = [](const string& label, const string& val) {
        cout << "| " << left << setw(17) << label << " : " << left << setw(10) << val << " |\n";
    };

    row("Harga Beli",  "M" + to_string(prop->buyPrice()));
    row("Nilai Gadai", "M" + to_string(prop->mortgageValue()));
    cout << "+--------------------------------+\n";

    if (prop->type() == PropertyType::STREET) {
        auto* s = static_cast<Street*>(prop);
        row("Sewa (unimproved)", "M" + to_string(s->calcRent()));
        row("Harga Rumah",  "M" + to_string(s->houseUpgradeCost()));
        row("Harga Hotel",  "M" + to_string(s->hotelUpgradeCost()));
        if (s->festival().isActive()) {
            cout << "+--------------------------------+\n";
            row("Festival x" + to_string(s->festival().multiplier()),
                to_string(s->festival().duration()) + " turn");
        }
    } else if (prop->type() == PropertyType::RAILROAD) {
        row("Sewa (1 RR)", "M25");
        row("Sewa (2 RR)", "M50");
        row("Sewa (3 RR)", "M100");
        row("Sewa (4 RR)", "M200");
    } else if (prop->type() == PropertyType::UTILITY) {
        row("Sewa (1 util)", "4 x dadu");
        row("Sewa (2 util)", "10 x dadu");
    }

    cout << "+--------------------------------+\n";
    string statusStr;
    if (prop->isBank())       statusStr = "BANK";
    else if (prop->isMortgaged()) statusStr = "MORTGAGED [M]";
    else statusStr = "OWNED (" + prop->owner()->username() + ")";
    
    string statusLine = "Status : " + statusStr;
    if (statusLine.length() > innerWidth) {
        statusLine = statusLine.substr(0, innerWidth - 3) + "...";
    }
    
    cout << "| " << left << setw(innerWidth) << statusLine << " |\n";
    cout << "+================================+\n";
}

void BoardPrinter::printProperties() const {
    const Player& player = game_.currentPlayer();
    if (player.properties().empty()) {
        cout << "Kamu belum memiliki properti apapun.\n";
        return;
    }

    cout << "=== Properti Milik: " << player.username() << " ===\n";

    map<string, vector<Property*>> grouped;
    for (auto* prop : player.properties()) {
        string group;
        if (prop->type() == PropertyType::STREET) group = colorGroupToString(static_cast<Street*>(prop)->colorGroup());
        else if (prop->type() == PropertyType::RAILROAD) group = "STASIUN";
        else group = "UTILITAS";
        grouped[group].push_back(prop);
    }

    int total = 0;
    for (auto& [group, props] : grouped) {
        cout << "[" << group << "]\n";
        for (auto* prop : props) {
            string status = prop->isMortgaged() ? " MORTGAGED [M]" : " OWNED";
            string bldg;
            if (prop->type() == PropertyType::STREET) {
                auto* s = static_cast<Street*>(prop);
                int lvl = s->buildingLevel();
                if (lvl == Street::HOTEL) bldg = " Hotel";
                else if (lvl > 0) bldg = " " + to_string(lvl) + " rumah";
            }
            cout << "  - " << prop->name() << " (" << prop->code()
                      << ")" << bldg << " M" << prop->buyPrice()
                      << status << "\n";
            total += prop->buyPrice();
        }
    }
    cout << "Total kekayaan properti: M" << total << "\n";
}

void BoardPrinter::printTurnHeader() const {
    const Player& p = game_.currentPlayer();
    cout << "\n============================\n"
              << "Giliran: " << p.username()
              << "  |  Saldo: M" << p.balance()
              << "  |  Turn " << game_.currentTurn();
    if (game_.maxTurn() >= 1) cout << "/" << game_.maxTurn();
    cout << "\n============================\n";
}

void BoardPrinter::printWinner() const {
    auto ws = game_.winners();
    auto active = game_.activePlayers();

    cout << "\n╔══════════════════════════════╗\n";
    if (active.size() == 1) {
        cout << "║   PERMAINAN SELESAI          ║\n"
             << "║   (Semua kecuali satu bangkrut)║\n";
    } else {
        cout << "║   PERMAINAN SELESAI          ║\n"
             << "║   (Batas giliran tercapai)   ║\n";
        cout << "╠══════════════════════════════╣\n";
        cout << "║  Rekap pemain:               ║\n";
        for (auto* p : active) {
            cout << "║  " << left << setw(12) << p->username()
                      << " M" << setw(6) << p->balance()
                      << " props:" << p->properties().size()
                      << "            ║\n";
        }
    }

    cout << "╠══════════════════════════════╣\n";
    if (ws.size() == 1) {
        cout << "║  PEMENANG: " << left << setw(18) << ws[0]->username() << "║\n";
    } else {
        cout << "║  SERI! Pemenang:             ║\n";
        for (auto* w : ws) cout << "║    - " << left << setw(24) << w->username() << "║\n";
    }
    cout << "╚══════════════════════════════╝\n";
}

} // namespace Nimonspoli