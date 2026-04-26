#include "data/configloader.h"
#include "core/board.h"
#include "core/tiles.h"
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <array>

namespace Nimonspoli {

ConfigLoader::ConfigLoader(const string& configDir) : configDir_(configDir) {}

string ConfigLoader::filePath(const string& filename) const {
    return configDir_ + "/" + filename;
}
TaxConfig ConfigLoader::loadTaxConfig() const {
    ifstream f(filePath("tax.txt"));
    if (!f) throw runtime_error("Cannot open tax.txt");

    TaxConfig cfg;
    string line;
    getline(f, line); // skip header
    f >> cfg.pphFlat >> cfg.pphPercent >> cfg.pbmFlat;
    cfg.pphPercent /= 100.0f; 
    return cfg;
}

SpecialConfig ConfigLoader::loadSpecialConfig() const {
    ifstream f(filePath("special.txt"));
    if (!f) throw runtime_error("Cannot open special.txt");

    SpecialConfig cfg;
    string line;
    getline(f, line); // skip header
    f >> cfg.goSalary >> cfg.jailFine;
    return cfg;
}

MiscConfig ConfigLoader::loadMiscConfig() const {
    ifstream f(filePath("misc.txt"));
    if (!f) throw runtime_error("Cannot open misc.txt");

    MiscConfig cfg;
    string line;
    getline(f, line); // skip header
    f >> cfg.maxTurn >> cfg.startBalance;
    return cfg;
}

RailroadConfig ConfigLoader::loadRailroadConfig() const {
    ifstream f(filePath("railroad.txt"));
    if (!f) throw runtime_error("Cannot open railroad.txt");

    RailroadConfig cfg;
    string line;
    getline(f, line); // skip header
    int count, rent;
    while (f >> count >> rent)
        cfg.rentTable[count] = rent;
    return cfg;
}

UtilityConfig ConfigLoader::loadUtilityConfig() const {
    ifstream f(filePath("utility.txt"));
    if (!f) throw runtime_error("Cannot open utility.txt");

    UtilityConfig cfg;
    string line;
    getline(f, line); // skip header
    int count, mult;
    while (f >> count >> mult)
        cfg.multiplierTable[count] = mult;
    return cfg;
}

vector<unique_ptr<Property>> ConfigLoader::loadProperties(
    const RailroadConfig& rrCfg,
    const UtilityConfig&  utilCfg) const
{
    ifstream f(filePath("property.txt"));
    if (!f) throw runtime_error("Cannot open property.txt");

    vector<unique_ptr<Property>> props;
    string line;
    getline(f, line); // skip header line

    while (getline(f, line)) {
        while (!line.empty() && isspace(line.back())) line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line);

        int id; string code, name, jenis, warna;
        if (!(ss >> id >> code >> name >> jenis >> warna)) {
            throw runtime_error("Malformed property row: " + line);
        }

        if (jenis == "STREET") {
            int buyPrice, mortgageValue, houseUpg, hotelUpg;
            if (!(ss >> buyPrice >> mortgageValue >> houseUpg >> hotelUpg)) {
                throw runtime_error("Malformed STREET row: " + line);
            }
            array<int,6> rents{};
            vector<string> rentTokens;
            string tok;
            while (ss >> tok) rentTokens.push_back(tok);

            auto parseInt = [&](const string &s) {
                try { return stoi(s); }
                catch (...) { throw runtime_error("Malformed STREET rent token: " + s + " in " + line); }
            };

            if (rentTokens.size() == 6) {
                for (int i = 0; i < 6; ++i) rents[i] = parseInt(rentTokens[i]);
            } else {
                int ell = -1;
                for (size_t i = 0; i < rentTokens.size(); ++i) {
                    if (rentTokens[i] == "..." || rentTokens[i] == "…") { ell = static_cast<int>(i); break; }
                }
                if (ell == -1) {
                    throw runtime_error("Malformed STREET rent table: " + line);
                }
                vector<string> leftToks, rightToks;
                for (int i = 0; i < ell; ++i) leftToks.push_back(rentTokens[i]);
                for (size_t i = ell + 1; i < rentTokens.size(); ++i) rightToks.push_back(rentTokens[i]);

                if (leftToks.empty() || rightToks.empty()) {
                    throw runtime_error("Malformed STREET rent table (ellipsis must have numbers on both sides): " + line);
                }

                for (size_t i = 0; i < leftToks.size(); ++i) {
                    if (i >= 6) throw runtime_error("Too many rent tokens: " + line);
                    rents[static_cast<int>(i)] = parseInt(leftToks[i]);
                }
                for (size_t i = 0; i < rightToks.size(); ++i) {
                    if (i >= 6) throw runtime_error("Too many rent tokens: " + line);
                    rents[6 - static_cast<int>(rightToks.size()) + static_cast<int>(i)] = parseInt(rightToks[i]);
                }

                int idx = 0;
                while (idx < 6 && rents[idx] == 0) ++idx;
                if (idx == 6) throw runtime_error("No rent anchors found: " + line);
                int cur = idx;
                while (cur < 6) {
                    int next = cur + 1;
                    while (next < 6 && rents[next] == 0) ++next;
                    if (next >= 6) break;
                    int leftVal = rents[cur];
                    int rightVal = rents[next];
                    int span = next - cur;
                    for (int k = 1; k < span; ++k) {
                        double t = static_cast<double>(k) / span;
                        rents[cur + k] = static_cast<int>(round(leftVal + (rightVal - leftVal) * t));
                    }
                    cur = next;
                }

                for (int i = 0; i < 6; ++i) {
                    if (rents[i] == 0) throw runtime_error("Malformed STREET rent table after expansion: " + line);
                }
            }

            ColorGroup cg = stringToColorGroup(warna);
            props.push_back(make_unique<Street>(
                code, name, cg, buyPrice, mortgageValue,
                houseUpg, hotelUpg, rents));

        } else if (jenis == "RAILROAD") {
            int mortgageValue;
            if (!(ss >> mortgageValue)) {
                throw runtime_error("Malformed RAILROAD row: " + line);
            }
            props.push_back(make_unique<Railroad>(
                code, name, mortgageValue, rrCfg));

        } else if (jenis == "UTILITY") {
            int mortgageValue;
            if (!(ss >> mortgageValue)) {
                throw runtime_error("Malformed UTILITY row: " + line);
            }
            props.push_back(make_unique<Utility>(
                code, name, mortgageValue, utilCfg));
        } else {
            throw runtime_error("Unknown property type: " + jenis);
        }
    }
    return props;
}

vector<ConfigLoader::AksiEntry> ConfigLoader::loadAksiTiles() const {
    ifstream f(filePath("aksi.txt"));
    if (!f) throw runtime_error("Cannot open aksi.txt");

    vector<AksiEntry> entries;
    string line;
    getline(f, line);
    getline(f, line);

    while (getline(f, line)) {
        while (!line.empty() && isspace(static_cast<unsigned char>(line.back()))) line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line);

        int id; string code, name, jenis, warna;
        if (!(ss >> id >> code >> name >> jenis >> warna)) {
            throw runtime_error("Malformed aksi row: " + line);
        }
        for (auto& c : name) if (c == '_') c = ' ';
        entries.push_back({id - 1, code, name, jenis});
    }
    return entries;
}

unique_ptr<Board> ConfigLoader::buildBoard(
    vector<unique_ptr<Property>>& properties,
    const SpecialConfig& special,
    const TaxConfig& tax) const
{
    auto board = make_unique<Board>();

    for (auto& p : properties)
        board->addProperty(move(p));
    properties.clear();

    auto aksiList = loadAksiTiles();
    map<int, AksiEntry> aksiByIdx;
    for (auto& a : aksiList) aksiByIdx[a.idx] = a;

    auto makeAksiTile = [&](const AksiEntry& a) -> unique_ptr<Tile> {
        if (a.code == "GO")  return make_unique<GoTile>(a.idx, special.goSalary);
        if (a.code == "PEN") return make_unique<JailTile>(a.idx);
        if (a.code == "BBP") return make_unique<FreeParkingTile>(a.idx);
        if (a.code == "PPJ") return make_unique<GoToJailTile>(a.idx);
        if (a.code == "DNU") return make_unique<CommunityTile>(a.idx);
        if (a.code == "KSP") return make_unique<ChanceTile>(a.idx);
        if (a.code == "FES") return make_unique<FestivalTile>(a.idx);
        if (a.code == "PPH") return make_unique<TaxTile>(a.idx, "PPH", a.name, TaxType::PPH, tax.pphFlat, tax.pphPercent);
        if (a.code == "PBM") return make_unique<TaxTile>(a.idx, "PBM", a.name, TaxType::PBM, tax.pbmFlat, 0.0f);
        throw runtime_error("Unknown aksi tile code: " + a.code);
    };

    ifstream pf(filePath("property.txt"));
    if (!pf) throw runtime_error("Cannot open property.txt");
    string pline;
    getline(pf, pline);
    map<int, string> codeByIdx;
    while (getline(pf, pline)) {
        while (!pline.empty() && isspace(static_cast<unsigned char>(pline.back()))) pline.pop_back();
        if (pline.empty() || pline[0] == '#') continue;
        istringstream pss(pline);
        int id; string code;
        if (!(pss >> id >> code)) continue;
        codeByIdx[id - 1] = code;
    }

    for (int idx = 0; idx < Board::BOARD_SIZE; ++idx) {
        auto itA = aksiByIdx.find(idx);
        if (itA != aksiByIdx.end()) {
            board->addTile(makeAksiTile(itA->second));
            continue;
        }
        auto itP = codeByIdx.find(idx);
        if (itP == codeByIdx.end()) {
            throw runtime_error("Tile index " + to_string(idx) + " tidak ada di aksi.txt maupun property.txt");
        }
        Property* p = board->getProperty(itP->second);
        if (!p) throw runtime_error("Property not found: " + itP->second);
        board->addTile(make_unique<PropertyTile>(idx, p->code(), p->name(), p));
    }

    if (board->size() != Board::BOARD_SIZE) {
        throw runtime_error("Invalid board size: " + to_string(board->size()));
    }
    return board;
}

ColorGroup ConfigLoader::parseColor(const string& s) const {
    return stringToColorGroup(s);
}

}