#include "ui/boardprinter.h"
#include "core/propertytypes.h"
#include "core/tiles.h"
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;
namespace Nimonspoli {

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
        string col = colorCode(i);
        string cell = tileCell(i);
        cout << col << setw(10) << left
                  << cell.substr(0, 10) << Color::RESET << "|";
    }
    cout << "\n";
    printHRule();
}

void BoardPrinter::printBottomRow() const {
    printHRule();
    cout << "|";
    for (int i = 10; i >= 0; --i) {
        string col = colorCode(i);
        string cell = tileCell(i);
        cout << col << setw(10) << left
                  << cell.substr(0, 10) << Color::RESET << "|";
    }
    cout << "\n";
    printHRule();
}

void BoardPrinter::printSideRows() const {
    vector<string> rightCells, leftCells;
    for (int i = 31; i <= 39; ++i) rightCells.push_back(tileCell(i));
    for (int i = 19; i >= 11; --i) leftCells.push_back(tileCell(i));
    vector<string> center = {
        "",
        " ================================== ",
        " ||       NIMONSPOLI             || ",
        " ================================== ",
        "",
        "  TURN " + to_string(game_.currentTurn()) +
            (game_.maxTurn() >= 1 ? " / " + to_string(game_.maxTurn()) : " (unlimited)"),
        "",
        " ---------------------------------- ",
        " LEGENDA KEPEMILIKAN & STATUS       ",
        " P1-P4 : milik Pemain 1-4           ",
        " ^  : Rumah Lv1  ^^ : Lv2          ",
        " ^^^: Rumah Lv3   * : Hotel         ",
        " (1)-(4): Bidak pemain              ",
        " [M]: Digadaikan                    ",
        " ---------------------------------- ",
        " WARNA: [CK]Coklat [BM]Biru Muda   ",
        "  [PK]Pink   [OR]Orange [MR]Merah   ",
        "  [KN]Kuning [HJ]Hijau  [BT]Biru Tua",
        "  [AB]Utilitas [DF]Aksi             ",
        "",
    };

    while ((int)center.size() < 9) center.push_back("");
    if ((int)center.size() > 9) center.resize(9);

    for (int row = 0; row < 9; ++row) {
        int leftIdx  = 19 - row;
        int rightIdx = 31 + row;
        string lc = colorCode(leftIdx);
        string rc = colorCode(rightIdx);

        string lCell = leftCells[row].substr(0, 10);
        string rCell = rightCells[row].substr(0, 10);
        int lpad = 10 - (int)lCell.size();
        int rpad = 10 - (int)rCell.size();

        cout << "|" << lc << lCell << string(lpad, ' ') << Color::RESET;
        string cLine = center[row];
        if ((int)cLine.size() < 97) cLine += string(97 - cLine.size(), ' ');
        cout << " " << cLine.substr(0, 97) << " ";
        cout << "|" << rc << rCell << string(rpad, ' ') << Color::RESET << "|\n";
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

    cout << col
              << "+================================+\n"
              << "|       AKTA KEPEMILIKAN         |\n"
              << "| " << left << setw(30)
              << ("[" + colorGroupToString(prop->type() == PropertyType::STREET ? static_cast<Street*>(prop)->colorGroup() : ColorGroup::DEFAULT) + "] " + prop->name() + " (" + prop->code() + ")")
              << " |\n"
              << "+================================+\n"
              << Color::RESET;

    auto row = [](const string& label, const string& val) {
        cout << "| " << left << setw(20) << label << right << setw(9) << val << " |\n";
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
            row("Festival aktif x" + to_string(s->festival().multiplier()),
                to_string(s->festival().duration()) + " giliran");
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
    row("Status", statusStr);
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