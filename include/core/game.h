#pragma once
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "core/board.h"
#include "core/player.h"
#include "core/dice.h"
#include "core/bank.h"
#include "core/cards.h"
#include "core/carddeck.h"
#include "core/transactionlogger.h"
#include "core/config.h"
using namespace std;
namespace Nimonspoli {

class AuctionManager;  // forward
class GameCLI;         // forward
class SaveManager;     // forward

// UI callbacks buat CLI
class GameCallbacks {
public:
    function<bool(Property&)>              onOfferPurchase; //Prompt player mau beli apa engga, true = iya, false = ga
    function<void(Property&)>              onAuction;
    function<void(Player&)>                onTaxPPH; // Prompt player milih bayar pake apa
    function<void(Player&)>                onFestival; // Prompt player milih properti buat festival
    function<void(Player&, int, Player*)>  onLiquidation; // prompt si debtor buat liquidkan asset sebanyak amount ke creditor (debtor, amount, creditor)
    function<int(Player&)>                 onTeleport;    // Prompt player buat milih teleport ke mana waktu dapat kartunya
    function<Player*(Player&)>             onLasso; //Prompt player milih siapa yg mau di lasso
    function<string(Player&)>         onDemolition; //Prompt player milih properti yg mau didemo
    function<void(Player&)>                onDropCard; //Prompt player milih kartu yg mau didrop waktu overflow
    function<void(Property&)>              onAutoPurchase;  //Prompt player langsung mendapatkan properti tanpa membeli (stasiun)
    function<void(int, int)>               onDiceRolled; // 
};

class Game {
public:
    Game(TaxConfig tax, SpecialConfig special, MiscConfig misc,
         RailroadConfig rrCfg, UtilityConfig utilCfg);

    // Masukin UI Callbacks di yg di atas
    void setCallbacks(GameCallbacks cb) { cb_ = move(cb); }
    const GameCallbacks& callbacks()  const { return cb_; }

    // Setup
    void addPlayer(const string& username);
    void setBoard(unique_ptr<Board> board);
    void randomizeTurnOrder();
    void initCardDecks();    

    // Main game loop
    void runTurn();          

    // Commands
    void cmdRollDice();
    void cmdSetDice(int d1, int d2);
    void cmdBuild(const string& code);
    void cmdMortgage(const string& code);
    void cmdRedeem(const string& code);
    void cmdPrintDeed(const string& code) const;
    void cmdPrintProperties() const;
    void cmdSave(const string& path) const;
    void cmdLoad(const string& path);
    void cmdPrintLog(int n = 0) const;
    void cmdUseSkillCard(int handIndex);
    void cmdDropCard(int handIndex);
    void cmdLiquidateSell(const string& code);

    // Query utk gamestate
    bool           isOver()           const;
    Player*        winner()           const;   // nullptr klo >1/blom ad
    vector<Player*> winners()    const;   // buat tie-break

    Player&        currentPlayer();
    const Player&  currentPlayer()    const;
    int            currentTurn()      const { return currentTurn_; }
    int            maxTurn()          const { return misc_.maxTurn; }
    Board&         board()                  { return *board_; }
    const Board&   board()            const { return *board_; }
    Bank&          bank()                   { return bank_; }
    Dice&          dice()                   { return dice_; }

    CardDeck<ChanceCard>&    chanceDeck()         { return chanceDeck_; }
    CardDeck<CommunityCard>& communityDeck()       { return communityDeck_; }
    CardDeck<SkillCard>&     skillDeck()           { return skillDeck_; }
    const CardDeck<SkillCard>& skillDeck()   const { return skillDeck_; }

    const vector<unique_ptr<Player>>& players() const { return players_; }

    const TaxConfig&      taxConfig()     const { return tax_; }
    const SpecialConfig&  specialConfig() const { return special_; }
    const MiscConfig&     miscConfig()    const { return misc_; }
    const RailroadConfig& rrConfig()      const { return rrCfg_; }
    const UtilityConfig&  utilConfig()    const { return utilCfg_; }

    // Mechanics
    void movePlayer(Player& player, int steps, bool collectGoSalary = true);
    void teleportPlayer(Player& player, int targetIndex);
    void sendToJail(Player& player);

    void handleRentPayment(Player& payer, Property& prop, int diceTotal);
    void handleTaxPPH(Player& player);
    void resolveTaxPPHChoice(Player& player, bool usePercentage);
    void handleTaxPBM(Player& player);
    void handleFestival(Player& player);
    void handleBankruptcy(Player& debtor, Player* creditor, int required = 0);  // nullptr = bank
    void handleAuction(Property& prop);

    void applyFestival(Player& player, const string& code);
    void finishAuction(Player& winner, Property& prop, int finalBid);
   
    int  lastDiceTotal() const { return lastDiceTotal_; }
    bool lastJailWasTripleDouble() const { return lastJailByTripleDouble_; }

    // Distribute  1 skill card ke semua player
    void distributeSkillCard(Player& player);

    // Advance ke turn selanjutnya
    void advanceTurn();

    // Check and eliminate bankrupt players
    vector<Player*> activePlayers() const;

    // Called after any ownership transfer to keep monopoly/RR/util counts current
    void refreshPropertyCounts(Player* player);
    void handlePropertyPurchase(Player& player, Property& prop);
    void processLanding(Player& player, int tileIndex, int diceTotal);

private:
    friend class SaveManager;

    // helper Internal
    void handleJailTurn(Player& player);
    bool tryLiquidate(Player& player, int required);
    void liquidateForBankruptcy(Player& debtor, Player* creditor);
    void awardGoSalary(Player& player);

    // Data point
    unique_ptr<Board>                   board_;
    vector<unique_ptr<Player>>     players_;
    vector<int>                         turnOrder_;  // isiny index players_
    int                                      turnOrderIdx_ = 0;
    int                                      currentTurn_  = 1;
    int                                      lastDiceTotal_ = 0;
    bool                                     lastJailByTripleDouble_ = false;

    Dice               dice_;
    Bank               bank_;

    CardDeck<ChanceCard>    chanceDeck_;
    CardDeck<CommunityCard> communityDeck_;
    CardDeck<SkillCard>     skillDeck_;

    TaxConfig      tax_;
    SpecialConfig  special_;
    MiscConfig     misc_;
    RailroadConfig rrCfg_;
    UtilityConfig  utilCfg_;
    GameCallbacks  cb_;
};

}