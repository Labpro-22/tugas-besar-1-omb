#include "core/game.h"
#include "core/propertytypes.h"
#include "core/tiles.h"
#include <algorithm>
#include <random>
#include <stdexcept>
#include <iostream>
#include <climits>
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
    mt19937 rng(random_device{}());
    shuffle(turnOrder_.begin(), turnOrder_.end(), rng);
}

void Game::initCardDecks() {
    chanceDeck_.addCard(make_unique<ChanceCard>(ChanceEffect::GO_NEAREST_STATION));
    chanceDeck_.addCard(make_unique<ChanceCard>(ChanceEffect::MOVE_BACK_3));
    chanceDeck_.addCard(make_unique<ChanceCard>(ChanceEffect::GO_TO_JAIL));
    chanceDeck_.shuffle();
    communityDeck_.addCard(make_unique<CommunityCard>(CommunityEffect::BIRTHDAY));
    communityDeck_.addCard(make_unique<CommunityCard>(CommunityEffect::DOCTOR));
    communityDeck_.addCard(make_unique<CommunityCard>(CommunityEffect::ELECTION));
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
    distributeSkillCard(player);
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
    if (turnOrderIdx_ == 0) ++currentTurn_;
}

void Game::distributeSkillCard(Player& player) {
    SkillCard* card = skillDeck_.draw();
    if (player.handFull()) {
        // Harusnya UI udah catch ini, tapi mana tau ye kan
        player.addToHand(card);
    } else {
    player.addToHand(card);
}
}

void Game::movePlayer(Player& player, int steps, bool collectGoSalary) {
    int oldPos = player.position();
    int newPos = board_->advance(oldPos, steps);
    if (collectGoSalary && newPos < oldPos) awardGoSalary(player);
    player.setPosition(newPos);
    logger_.log(currentTurn_, player.username(), "GERAK", "Maju " + to_string(steps) + " petak -> " + board_->getTile(newPos)->name());
}

void Game::teleportPlayer(Player& player, int targetIndex) {
    player.setPosition(targetIndex);
    logger_.log(currentTurn_, player.username(), "TELEPORT", "Pindah ke " + board_->getTile(targetIndex)->name());
}

void Game::sendToJail(Player& player) {
    int jailIdx = 10;
    for (int i = 0; i < board_->size(); ++i) { if (board_->getTile(i)->type() == TileType::JAIL) { jailIdx = i; break; } }
    player.setPosition(jailIdx);
    player.setStatus(PlayerStatus::JAILED);
    player.resetJailTurns();
    dice_.resetDoubleCount();
    logger_.log(currentTurn_, player.username(), "PENJARA", "Masuk penjara");
}

void Game::awardGoSalary(Player& player) {
    bank_.pay(player, special_.goSalary);
    logger_.log(currentTurn_, player.username(), "GAJI", "Melewati/berhenti di GO, menerima M" + to_string(special_.goSalary));
}

void Game::cmdRollDice() {
    Player& player = currentPlayer();
    if (player.hasRolled() && !dice_.isDouble()) throw logic_error("Kamu sudah melempar dadu pada giliran ini.");
    if (player.isJailed()) { handleJailTurn(player); return; }
    auto [d1, d2] = dice_.roll();
    logger_.log(currentTurn_, player.username(), "DADU", "Lempar: " + to_string(d1) + "+" + to_string(d2) + "=" + to_string(d1+d2));
    if (dice_.doubleCount() == 3) { sendToJail(player); return; }
    player.setHasRolled(true);
    movePlayer(player, d1 + d2);
    processLanding(player, player.position(), d1 + d2);
}

void Game::cmdSetDice(int d1, int d2) {
    Player& player = currentPlayer();
    if (player.hasRolled() && !dice_.isDouble()) throw logic_error("Kamu sudah melempar dadu pada giliran ini.");
    dice_.setRoll(d1, d2);
    logger_.log(currentTurn_, player.username(), "DADU", "Atur manual: " + to_string(d1) + "+" + to_string(d2) + "=" + to_string(d1+d2));
    if (dice_.doubleCount() == 3) { sendToJail(player); return; }
    player.setHasRolled(true);
    movePlayer(player, d1 + d2);
    processLanding(player, player.position(), d1 + d2);
}

void Game::handleJailTurn(Player& player) {
    if (player.jailTurns() >= 3) {
        if (!player.canAfford(special_.jailFine)) { handleBankruptcy(player, nullptr); return; }
        bank_.collect(player, special_.jailFine);
        player.setStatus(PlayerStatus::ACTIVE);
        player.resetJailTurns();
        logger_.log(currentTurn_, player.username(), "KELUAR_PENJARA", "Bayar denda M" + to_string(special_.jailFine) + " (wajib)");
        cmdRollDice();
        return;
    }
    auto [d1, d2] = dice_.roll();
    logger_.log(currentTurn_, player.username(), "DADU_PENJARA", "Lempar: " + to_string(d1) + "+" + to_string(d2));
    if (dice_.isDouble()) {
        player.setStatus(PlayerStatus::ACTIVE);
        player.resetJailTurns();
        dice_.resetDoubleCount();
        logger_.log(currentTurn_, player.username(), "KELUAR_PENJARA", "Dadu double!");
        movePlayer(player, d1 + d2);
        processLanding(player, player.position(), d1 + d2);
    } else {
        player.incrementJailTurns();
        logger_.log(currentTurn_, player.username(), "PENJARA", "Gagal keluar (percobaan " + to_string(player.jailTurns()) + "/3)");
    }
}

void Game::processLanding(Player& player, int tileIndex, int diceTotal) {
    Tile* tile = board_->getTile(tileIndex);
    if (tile->type() == TileType::GO_TO_JAIL) { tile->onLanded(player, *this); return; }
    if (player.isShielded()) {
        logger_.log(currentTurn_, player.username(), "SHIELD", "ShieldCard melindungi di " + tile->name());
        player.setShielded(false);
        return;
    }
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
    if (cb_.onTaxPPH) { cb_.onTaxPPH(player); return; }
    int flat = tax_.pphFlat;
    if (!player.canAfford(flat)) { handleBankruptcy(player, nullptr); return; }
    bank_.collect(player, flat);
    logger_.log(currentTurn_, player.username(), "PAJAK", "PPH flat M" + to_string(flat));
}

void Game::handleTaxPBM(Player& player) {
    int tax = tax_.pbmFlat;
    if (!player.canAfford(tax)) { handleBankruptcy(player, nullptr); return; }
    bank_.collect(player, tax);
    logger_.log(currentTurn_, player.username(), "PAJAK", "PBM flat M" + to_string(tax));
}

void Game::handleFestival(Player& player) {
    logger_.log(currentTurn_, player.username(), "FESTIVAL", "Mendarat di petak Festival");
    if (cb_.onFestival) cb_.onFestival(player);
}

void Game::applyFestival(Player& player, const string& code) {
    Property* prop = board_->getProperty(code);
    if (!prop || prop->type() != PropertyType::STREET) throw invalid_argument("Kode properti tidak valid.");
    if (prop->owner() != &player) throw invalid_argument("Properti bukan milikmu.");
    auto* street = static_cast<Street*>(prop);
    street->applyFestivalBoost(player, *this);
}

void Game::handleAuction(Property& prop) {
    prop.handleAuction(*this);
}

void Game::finishAuction(Player& winner, Property& prop, int finalBid) {
    prop.finishAuction(winner, finalBid, *this);
}

void Game::handleBankruptcy(Player& debtor, Player* creditor) {
    logger_.log(currentTurn_, debtor.username(), "BANGKRUT", creditor ? "Kreditor: " + creditor->username() : "Kreditor: Bank");
    if (cb_.onLiquidation && debtor.maxLiquidation() >= 0) {
        cb_.onLiquidation(debtor, 0, creditor);
        if (!debtor.isBankrupt()) return;
    }
    debtor.setStatus(PlayerStatus::BANKRUPT);
    if (creditor) {
        for (auto* prop : debtor.properties()) {
            prop->setOwner(creditor);
            creditor->addProperty(prop);
            refreshPropertyCounts(creditor);
        }
        creditor->operator+=(debtor.balance());
        debtor.operator-=(debtor.balance());
        logger_.log(currentTurn_, debtor.username(), "BANGKRUT", "Semua aset dialihkan ke " + creditor->username());
    } else {
        for (auto* prop : debtor.properties()) {
            if (prop->type() == PropertyType::STREET) static_cast<Street*>(prop)->demolishAll();
            prop->setOwner(nullptr);
            prop->setStatus(PropertyStatus::BANK);
            handleAuction(*prop);
        }
        bank_.collect(debtor, debtor.balance());
        logger_.log(currentTurn_, debtor.username(), "BANGKRUT", "Semua properti dikembalikan ke Bank dan dilelang");
    }
    debtor.clearProperties();
}

void Game::cmdBuild(const string& code) {
    Player& player = currentPlayer();
    Property* prop = board_->getProperty(code);
    if (!prop || prop->type() != PropertyType::STREET) throw invalid_argument("Kode properti tidak valid atau bukan Street.");
    if (prop->owner() != &player) throw invalid_argument("Properti bukan milikmu.");
    auto* street = static_cast<Street*>(prop);
    street->buildHouseOrHotel(player, *this);
}

void Game::cmdMortgage(const string& code) {
    Player& player = currentPlayer();
    Property* prop = board_->getProperty(code);
    if (!prop || prop->owner() != &player) throw invalid_argument("Properti tidak ditemukan atau bukan milikmu.");
    prop->performMortgage(player, *this);
}

void Game::cmdRedeem(const string& code) {
    Player& player = currentPlayer();
    Property* prop = board_->getProperty(code);
    if (!prop || prop->owner() != &player) throw invalid_argument("Properti tidak ditemukan atau bukan milikmu.");
    prop->performRedeem(player, *this);
}

void Game::cmdUseSkillCard(int handIndex) {
    Player& player = currentPlayer();
    if (player.hasRolled()) throw logic_error("Kartu kemampuan hanya bisa digunakan SEBELUM melempar dadu.");
    if (player.usedCardThisTurn()) throw logic_error("Penggunaan kartu dibatasi maksimal 1 kali dalam 1 giliran.");
    if (handIndex < 0 || handIndex >= player.handSize()) throw out_of_range("Indeks kartu tidak valid.");
    SkillCard* card = player.hand()[handIndex];
    player.removeFromHand(handIndex);
    card->use(player, *this);
    player.setUsedCard(true);
    skillDeck_.discard(card);
    logger_.log(currentTurn_, player.username(), "KARTU", "Pakai " + card->description());
}

void Game::cmdDropCard(int handIndex) {
    Player& player = currentPlayer();
    if (handIndex < 0 || handIndex >= player.handSize()) throw out_of_range("Indeks kartu tidak valid.");
    SkillCard* card = player.hand()[handIndex];
    player.removeFromHand(handIndex);
    skillDeck_.discard(card);
    logger_.log(currentTurn_, player.username(), "DROP_KARTU", "Membuang " + card->description());
}

void Game::cmdPrintLog(int n) const { logger_.print(n); }
void Game::refreshPropertyCounts(Player* player) {
    if (!player) return;
    board_->recalcMonopoly(player);
    board_->recalcRailroadCount(player);
    board_->recalcUtilityCount(player);
}

void Game::applyLasso(Player& caster, Player& target) {
    int dest = caster.position();
    target.setPosition(dest);
    logger_.log(currentTurn_, caster.username(), "KARTU", "LassoCard: " + target.username() + " ditarik ke " + board_->getTile(dest)->name());
    board_->getTile(dest)->onLanded(target, *this);
}

void Game::cmdSave(const string& path) const { (void)path; }
void Game::cmdLoad(const string& path) { (void)path; }
}
