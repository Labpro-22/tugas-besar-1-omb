#pragma once
#include <string>
#include <vector>
#include <memory>
#include "core/config.h"
#include "core/propertytypes.h"
using namespace std;
namespace Nimonspoli {

class Board;

class ConfigLoader {
public:
    explicit ConfigLoader(const string& configDir);

    // Tiap method baca satu file, return data
    vector<unique_ptr<Property>> loadProperties(
        const RailroadConfig& rrCfg,
        const UtilityConfig&  utilCfg) const;

    RailroadConfig loadRailroadConfig() const;
    UtilityConfig  loadUtilityConfig()  const;
    TaxConfig      loadTaxConfig()      const;
    SpecialConfig  loadSpecialConfig()  const;
    MiscConfig     loadMiscConfig()     const;

    unique_ptr<Board> buildBoard(
        vector<unique_ptr<Property>>& properties,
        const SpecialConfig& special,
        const TaxConfig&     tax) const;

private:
    string configDir_;
    string filePath(const string& filename) const;
    ColorGroup  parseColor(const string& s)       const;

    struct AksiEntry {
        int idx;          
        string code;      
        string name;      
        string jenis;     
    };
    vector<AksiEntry> loadAksiTiles() const;
};

} 