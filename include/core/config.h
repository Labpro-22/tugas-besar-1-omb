#pragma once
#include <vector>
#include <map>
 
namespace Nimonspoli {
 
struct TaxConfig {
    int   pphFlat       = 150;
    float pphPercent    = 0.10f;
    int   pbmFlat       = 200;
};
 
struct SpecialConfig {
    int goSalary  = 200;
    int jailFine  = 50;
};
 
struct MiscConfig {
    int maxTurn      = 15;
    int startBalance = 1000;
};
 
struct RailroadConfig {
    // index = jumlah railroad yang dimiliki (dari 1, bukan 0), value = rent
    std::map<int,int> rentTable;  // e.g. {1->25, 2->50, 3->100, 4->200}
};
 
struct UtilityConfig {
    // index = jumlah utility yang dimiliki (sama dari 1, bukan 0), value = multiplier
    std::map<int,int> multiplierTable; // e.g. {1->4, 2->10}
};
 
}