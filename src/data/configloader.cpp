#include "data/configloader.h"
#include "core/board.h"
#include "core/tiles.h"
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
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line);

        int id; string code, name, jenis, warna;
        ss >> id >> code >> name >> jenis >> warna;

        if (jenis == "STREET") {
            int buyPrice, mortgageValue, houseUpg, hotelUpg;
            ss >> buyPrice >> mortgageValue >> houseUpg >> hotelUpg;
            array<int,6> rents{};
            for (int& r : rents) ss >> r;

            ColorGroup cg = stringToColorGroup(warna);
            props.push_back(make_unique<Street>(
                code, name, cg, buyPrice, mortgageValue,
                houseUpg, hotelUpg, rents));

        } else if (jenis == "RAILROAD") {
            int mortgageValue;
            ss >> mortgageValue;
            props.push_back(make_unique<Railroad>(
                code, name, mortgageValue, rrCfg));

        } else if (jenis == "UTILITY") {
            int mortgageValue;
            ss >> mortgageValue;
            props.push_back(make_unique<Utility>(
                code, name, mortgageValue, utilCfg));
        }
    }
    return props;
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

    // Helper lambda 
    auto propTile = [&](int idx, const string& code) {
        Property* p = board->getProperty(code);
        if (!p) throw runtime_error("Property not found: " + code);
        return make_unique<PropertyTile>(idx, code, p->name(), p);
    };

    board->addTile(make_unique<GoTile>(0, special.goSalary));
    board->addTile(propTile(1, "GRT"));
    board->addTile(make_unique<CommunityTile>(2));
    board->addTile(propTile(3, "TSK"));
    board->addTile(make_unique<TaxTile>(4, "PPH", "Pajak Penghasilan", TaxType::PPH, tax.pphFlat, tax.pphPercent));
    board->addTile(propTile(5, "GBR"));
    board->addTile(propTile(6, "BGR"));
    board->addTile(make_unique<FestivalTile>(7));
    board->addTile(propTile(8, "DPK"));
    board->addTile(propTile(9, "BKS"));
    board->addTile(make_unique<JailTile>(10, special.jailFine));
    board->addTile(propTile(11, "MGL"));
    board->addTile(propTile(12, "PLN"));
    board->addTile(propTile(13, "SOL"));
    board->addTile(propTile(14, "YOG"));
    board->addTile(propTile(15, "STB"));
    board->addTile(propTile(16, "MAL"));
    board->addTile(make_unique<CommunityTile>(17));
    board->addTile(propTile(18, "SMG"));
    board->addTile(propTile(19, "SBY"));
    board->addTile(make_unique<FreeParkingTile>(20));
    board->addTile(propTile(21, "MKS"));
    board->addTile(make_unique<ChanceTile>(22));
    board->addTile(propTile(23, "BLP"));
    board->addTile(propTile(24, "MND"));
    board->addTile(propTile(25, "TUG"));
    board->addTile(propTile(26, "PLB"));
    board->addTile(propTile(27, "PKB"));
    board->addTile(propTile(28, "PAM"));
    board->addTile(propTile(29, "MED"));
    board->addTile(make_unique<GoToJailTile>(30));
    board->addTile(propTile(31, "BDG"));
    board->addTile(propTile(32, "DEN"));
    board->addTile(make_unique<FestivalTile>(33));
    board->addTile(propTile(34, "MTR"));
    board->addTile(propTile(35, "GUB"));
    board->addTile(make_unique<ChanceTile>(36));
    board->addTile(propTile(37, "JKT"));
    board->addTile(make_unique<TaxTile>(38, "PBM", "Pajak Barang Mewah", TaxType::PBM, tax.pbmFlat, 0.0f));
    board->addTile(propTile(39, "IKN"));
    return board;
}

ColorGroup ConfigLoader::parseColor(const string& s) const {
    return stringToColorGroup(s);
}

} 