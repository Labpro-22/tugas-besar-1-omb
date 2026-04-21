#pragma once
#include "core/player.h"
using n
namespace Nimonspoli {

// Dari sini semua transaksi biar gampang dilog, itu aja sebenarnya
class Bank {
public:
    void pay(Player& player, int amount) {player += amount;}
    void collect(Player& player, int amount) {player -= amount;}
    void transfer(Player& from, Player& to, int amount) {
        from -= amount;
        to   += amount;
    }
};

} 