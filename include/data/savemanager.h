#pragma once
#include <string>
#include "core/game.h"
using namespace std;
namespace Nimonspoli {

class SaveManager {
public:
    static void save(const Game& game, const string& path);
    static bool load(Game& game, const string& path);

private:
    static string serializePlayers(const Game& game);
    static string serializeProperties(const Game& game);
    static string serializeDeck(const Game& game);
    static string serializeLog(const Game& game);

    static void deserializePlayers(Game& game, istream& in);
    static void deserializeProperties(Game& game, istream& in);
    static void deserializeDeck(Game& game, istream& in);
    static void deserializeLog(Game& game, istream& in);
};

} 