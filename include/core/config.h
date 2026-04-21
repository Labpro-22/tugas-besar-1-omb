#pragma once
#include <vector>
#include <map>
 
namespace Nimonspoli {
 
class TaxConfig {
public:
    int   pphFlat       = 150;
    float pphPercent    = 0.10f;
    int   pbmFlat       = 200;
};
 
class SpecialConfig {
public:
    int goSalary  = 200;
    int jailFine  = 50;
};
 
class MiscConfig {
public:
    int maxTurn      = 15;
    int startBalance = 1000;
};
 
class RailroadConfig {
public:
    // index = jumlah railroad yang dimiliki (dari 1, bukan 0), value = rent
    std::map<int,int> rentTable;  // e.g. {1->25, 2->50, 3->100, 4->200}
};
 
class UtilityConfig {
public:
    // index = jumlah utility yang dimiliki (sama dari 1, bukan 0), value = multiplier
    std::map<int,int> multiplierTable; // e.g. {1->4, 2->10}
};
 
}