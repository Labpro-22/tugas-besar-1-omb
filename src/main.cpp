#include <iostream>
#include <stdexcept>
#include "data/configloader.h"
#include "core/game.h"
#include "ui/gameCLI.h"

using namespace Nimonspoli;

int main() {
    try {
        ConfigLoader loader("config");

        // Load 
        auto tax     = loader.loadTaxConfig();
        auto special = loader.loadSpecialConfig();
        auto misc    = loader.loadMiscConfig();
        auto rrCfg   = loader.loadRailroadConfig();
        auto utilCfg = loader.loadUtilityConfig();
        Nimonspoli::Game game(tax, special, misc, rrCfg, utilCfg);
        auto properties = loader.loadProperties(rrCfg, utilCfg);
        auto board      = loader.buildBoard(properties, special, tax);
        game.setBoard(std::move(board));

        // Hand off ke CLI
        Nimonspoli::GameCLI cli(game);
        cli.run();

    } catch (const exception& e) {
        cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }

    return 0;
}