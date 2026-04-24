#pragma once
#include <string>
#include <memory>
#include "core/game.h"
#include "ui/boardprinter.h"

namespace Nimonspoli {

class GameCLI {
public:
    explicit GameCLI(Game& game);
    void run();

private:
    void registerCallbacks();
    void showMainMenu();
    void setupNewGame();
    void setupLoadGame();
    void gameLoop();
    void processTurn();
    void dispatch(const string& line);

    void cmdCetakPapan();
    void cmdLemparDadu();
    void cmdAturDadu(const string& args);
    void cmdCetakAkta(const string& args);
    void cmdCetakProperti();
    void cmdGadai();
    void cmdTebus();
    void cmdBangun();
    void cmdSimpan(const string& args);
    void cmdMuat(const string& args);
    void cmdCetakLog(const string& args);
    void cmdGunakanKemampuan();

    bool        promptBuyStreet(Property& prop);
    void        runAuction(Property& prop);
    void        promptPPH();
    void        promptFestival();
    void        promptLiquidation(int required, Player* creditor);
    void        promptDropCard(Player& player);
    int         promptTeleportIndex();
    Player*     promptLassoTarget(Player& caster);
    string promptDemolitionCode(Player& caster);

    // Data
    string prompt(const string& msg) const;
    int         promptInt(const string& msg, int lo, int hi) const;
    bool        promptYN(const string& msg) const;
    void        printHelp() const;
    void        printError(const string& msg) const;

    Game&        game_;
    BoardPrinter printer_;
    bool         turnOver_ = false;
};

}