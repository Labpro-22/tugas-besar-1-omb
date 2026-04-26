#include "core/game.h"
#include "core/propertytypes.h"
#include "core/tiles.h"
#include <algorithm>
#include <random>
#include <stdexcept>
#include <iostream>
#include <climits>
#include <chrono>
using namespace std;
namespace Nimonspoli {
Game::Game(TaxConfig tax, SpecialConfig special, MiscConfig misc,
           RailroadConfig rrCfg, UtilityConfig utilCfg)
    : tax_(tax), special_(special), misc_(misc),
      rrCfg_(rrCfg), utilCfg_(utilCfg) {}


void Game::addPlayer(const string& username) {
    players_.push_back(make_unique<Player>(username, misc_.startBalance));
    turnOrder_.push_back(static_cast<int>(players_.size()) - 1);
}


void Game::setBoard(unique_ptr<Board> board) {
    board_ = move(board);
}

void Game::randomizeTurnOrder() {
    unsigned seed = static_cast<unsigned>(chrono::system_clock::now().time_since_epoch().count());
    mt19937 rng(seed);
    shuffle(turnOrder_.begin(), turnOrder_.end(), rng);
}


void Game::initCardDecks() {
    chanceDeck_.addCard(make_unique<ChanceGoNearestStation>());
    chanceDeck_.addCard(make_unique<ChanceMoveBack3>());
    chanceDeck_.addCard(make_unique<ChanceGoToJail>());
    chanceDeck_.shuffle();
    
    communityDeck_.addCard(make_unique<CommunityBirthday>());
    communityDeck_.addCard(make_unique<CommunityDoctor>());
    communityDeck_.addCard(make_unique<CommunityElection>());
    communityDeck_.shuffle();
    
    mt19937 rng(random_device{}());
    uniform_int_distribution<> stepDist(1, 6);
    uniform_int_distribution<> pctDist(10, 50);
    for (int i = 0; i < 4; ++i) skillDeck_.addCard(make_unique<MoveCard>(stepDist(rng)));
    for (int i = 0; i < 3; ++i) skillDeck_.addCard(make_unique<DiscountCard>(pctDist(rng)));
    for (int i = 0; i < 2; ++i) skillDeck_.addCard(make_unique<ShieldCard>());
    for (int i = 0; i < 2; ++i) skillDeck_.addCard(make_unique<TeleportCard>());
    for (int i = 0; i < 2; ++i) skillDeck_.addCard(make_unique<LassoCard>());
    for (int i = 0; i < 2; ++i) skillDeck_.addCard(make_unique<DemolitionCard>());
    skillDeck_.shuffle();
}

Player& Game::currentPlayer() {
    return *players_[turnOrder_[turnOrderIdx_]];
}

const Player& Game::currentPlayer() const {
    return *players_[turnOrder_[turnOrderIdx_]];
}

vector<Player*> Game::activePlayers() const {
    vector<Player*> out;
    for (auto& p : players_) if (p->isActive() || p->isJailed()) out.push_back(p.get());
    return out;
}

bool Game::isOver() const {
    auto active = activePlayers();
    if (active.size() <= 1) return true;
    if (misc_.maxTurn < 1) return active.size() == 1;
    return currentTurn_ > misc_.maxTurn;
}

vector<Player*> Game::winners() const {
    auto active = activePlayers();
    if (active.empty()) return {};
    if (active.size() == 1) return active;
    sort(active.begin(), active.end(), [](Player* a, Player* b){ return *b < *a; });
    int topWorth = active[0]->netWorth();
    vector<Player*> tied;
    for (auto* p : active) if (p->netWorth() == topWorth) tied.push_back(p);
    if (tied.size() == 1) return tied;
    int maxProps = 0;
    for (auto* p : tied) maxProps = max(maxProps, (int)p->properties().size());
    vector<Player*> tied2;
    for (auto* p : tied) if ((int)p->properties().size() == maxProps) tied2.push_back(p);
    if (tied2.size() == 1) return tied2;
    int maxCards = 0;
    for (auto* p : tied2) maxCards = max(maxCards, p->handSize());
    vector<Player*> tied3;
    for (auto* p : tied2) if (p->handSize() == maxCards) tied3.push_back(p);
    return tied3;
}

Player* Game::winner() const {
    auto w = winners();
    return (w.size() == 1) ? w[0] : nullptr;
}

void Game::runTurn() {
    Player& player = currentPlayer();
    if (!player.isActive() && !player.isJailed()) {
        advanceTurn();
        return;
    }
    player.resetTurnState();
    if (dice_.doubleCount() == 0) {
        distributeSkillCard(player);
    }
}


void Game::advanceTurn() {
    board_->tickFestivals(&currentPlayer());
    int n = static_cast<int>(turnOrder_.size());
    int attempts = 0;
    do {
        turnOrderIdx_ = (turnOrderIdx_ + 1) % n;
        ++attempts;
        if (attempts > n) break;
    } while (!players_[turnOrder_[turnOrderIdx_]]->isActive() && !players_[turnOrder_[turnOrderIdx_]]->isJailed());

    if (turnOrderIdx_ == 0) {
        ++currentTurn_;
    }
}

void Game::distributeSkillCard(Player& player) {
    SkillCard* card = nullptr;
    try {
        card = skillDeck_.draw();
    } catch (const runtime_error&) {
        return;
    }
    // Tambahkan kartu ke tangan dulu (sementara bisa melebihi MAX)
    player.addToHand(card);
    // Jika tangan melampaui batas, minta pemain drop 1 kartu via callback
    if (player.handSize() > Player::MAX_HAND_SIZE) {
        if (cb_.onDropCard) cb_.onDropCard(player);
        // Jika callback tidak terpasang (misalnya unit test), drop kartu pertama otomatis
        if (player.handSize() > Player::MAX_HAND_SIZE) {
            SkillCard* dropped = player.hand()[0];
            player.removeFromHand(0);
            skillDeck_.discard(dropped);
        }
    }
}

void Game::movePlayer(Player& player, int steps, bool collectGoSalary) {
    player.move(steps, collectGoSalary, *this);
}

void Game::teleportPlayer(Player& player, int targetIndex) {
    player.teleport(targetIndex, *this);
}

void Game::sendToJail(Player& player) {
    player.goToJail(*this);
}

void Game::awardGoSalary(Player& player) {
    player.receiveGoSalary(*this);
}

void Game::cmdRollDice() {
    lastJailByTripleDouble_ = false;
    Player& player = currentPlayer();
    if (player.isJailed()) {
        player.rollDice(*this);
        return;
    }
    auto [d1, d2] = dice_.roll();
    if (cb_.onDiceRolled) cb_.onDiceRolled(d1, d2);
    TransactionLogger::log(currentTurn_, player.username(), "DADU",
        "Lempar: " + to_string(d1) + "+" + to_string(d2) + "=" + to_string(d1+d2));
    if (dice_.doubleCount() == 3) {
        lastJailByTripleDouble_ = true;
        player.goToJail(*this);
        return;
    }
    player.setHasRolled(true);
    movePlayer(player, d1 + d2);
    processLanding(player, player.position(), d1 + d2);
}

void Game::cmdSetDice(int d1, int d2) {
    lastJailByTripleDouble_ = false;
    Player& player = currentPlayer();
    if (player.isJailed()) {
        player.setDice(d1, d2, *this);
        return;
    }
    dice_.setRoll(d1, d2);
    if (cb_.onDiceRolled) cb_.onDiceRolled(d1, d2);
    TransactionLogger::log(currentTurn_, player.username(), "DADU",
        "Atur manual: " + to_string(d1) + "+" + to_string(d2) + "=" + to_string(d1+d2));
    if (dice_.doubleCount() == 3) {
        lastJailByTripleDouble_ = true;
        player.goToJail(*this);
        return;
    }
    player.setHasRolled(true);
    movePlayer(player, d1 + d2);
    processLanding(player, player.position(), d1 + d2);
}

void Game::handleJailTurn(Player& player) {
    player.handleJailTurn(*this);
}

void Game::processLanding(Player& player, int tileIndex, int diceTotal) {
    Tile* tile = board_->getTile(tileIndex);
    if (tile->type() == TileType::GO_TO_JAIL) { tile->onLanded(player, *this); return; }
    lastDiceTotal_ = diceTotal;
    tile->onLanded(player, *this);
}

void Game::handlePropertyPurchase(Player& player, Property& prop) {
    prop.handlePurchase(player, *this);
}

void Game::handleRentPayment(Player& payer, Property& prop, int diceTotal) {
    prop.handleRent(payer, diceTotal, *this);
}

void Game::handleTaxPPH(Player& player) {
    if (player.isShielded()) {
        player.setShielded(false);
        TransactionLogger::log(currentTurn_, player.username(), "SHIELD", "ShieldCard mencegah pajak PPH");
        return;
    }
    if (cb_.onTaxPPH) { cb_.onTaxPPH(player); return; }
    resolveTaxPPHChoice(player, false);
}

void Game::resolveTaxPPHChoice(Player& player, bool usePercentage) {
    int tax = tax_.pphFlat;
    string detail = "PPH flat M" + to_string(tax);
    if (usePercentage) {
        int wealth = player.netWorth();
        tax = static_cast<int>(wealth * tax_.pphPercent);
        detail = "PPH " + to_string((int)(tax_.pphPercent * 100)) + "% = M" + to_string(tax);
    }
    if (!player.canAfford(tax)) { handleBankruptcy(player, nullptr, tax); return; }
    bank_.collect(player, tax);
    TransactionLogger::log(currentTurn_, player.username(), "PAJAK", detail);
}

void Game::handleTaxPBM(Player& player) {
    if (player.isShielded()) {
        player.setShielded(false);
        std::cout << "[SHIELD ACTIVE]: ShieldCard melindungimu! Pajak PBM dibatalkan.\n";
        TransactionLogger::log(currentTurn_, player.username(), "SHIELD", "ShieldCard mencegah pajak PBM");
        return;
    }
    int tax = tax_.pbmFlat;
    if (!player.canAfford(tax)) {
        std::cout << "Kamu tidak mampu membayar pajak PBM M" << tax << "!\n"
                  << "Uang kamu saat ini: M" << player.balance() << "\n";
        handleBankruptcy(player, nullptr, tax);
        return;
    }
    int before = player.balance();
    bank_.collect(player, tax);
    std::cout << "Pajak sebesar M" << tax << " langsung dipotong.\n"
              << "Uang kamu: M" << before << " -> M" << player.balance() << "\n";
    TransactionLogger::log(currentTurn_, player.username(), "PAJAK", "PBM flat M" + to_string(tax));
}

void Game::handleFestival(Player& player) {
    TransactionLogger::log(currentTurn_, player.username(), "FESTIVAL", "Mendarat di petak Festival");
    if (cb_.onFestival) cb_.onFestival(player);
}

void Game::applyFestival(Player& player, const string& code) {
    player.applyFestival(code, *this);
}

void Game::handleAuction(Property& prop) {
    prop.handleAuction(*this);
}

void Game::finishAuction(Player& winner, Property& prop, int finalBid) {
    prop.finishAuction(winner, finalBid, *this);
}

void Game::handleBankruptcy(Player& debtor, Player* creditor, int required) {
    debtor.declareBankruptcy(creditor, required, *this);
}

void Game::cmdBuild(const string& code) {
    currentPlayer().buildProperty(code, *this);
}

void Game::cmdMortgage(const string& code) {
    currentPlayer().mortgageProperty(code, *this);
}

void Game::cmdRedeem(const string& code) {
    currentPlayer().redeemProperty(code, *this);
}

void Game::cmdUseSkillCard(int handIndex) {
    currentPlayer().useSkillCard(handIndex, *this);
}

void Game::cmdDropCard(int handIndex) {
    currentPlayer().dropSkillCard(handIndex, *this);
}

void Game::cmdLiquidateSell(const string& code) {
    Player& player = currentPlayer();
    Property* prop = board_->getProperty(code);
    if (!prop) throw invalid_argument("Properti tidak ditemukan.");
    
    prop->performLiquidation(player, *this);
}

void Game::cmdPrintLog(int n) const { TransactionLogger::print(n); }
void Game::refreshPropertyCounts(Player* player) {
    if (!player) return;
    board_->recalcMonopoly(player);
    board_->recalcRailroadCount(player);
    board_->recalcUtilityCount(player);
}

void Game::cmdSave(const string& path) const { (void)path; }
void Game::cmdLoad(const string& path) { (void)path; }
}