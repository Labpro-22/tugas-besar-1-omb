#include "ui/gameCLI.h"
#include "data/savemanager.h"
#include "core/propertytypes.h"
#include "core/tiles.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <climits>
#include <fstream>
#include <map>
using namespace std;
namespace Nimonspoli {

GameCLI::GameCLI(Game& game) : game_(game), printer_(game) {}
void GameCLI::registerCallbacks() {
    GameCallbacks cb;

    cb.onOfferPurchase = [this](Property& prop) -> bool {
        return promptBuyStreet(prop);
    };
    cb.onAuction = [this](Property& prop) {
        runAuction(prop);
    };
    cb.onTaxPPH = [this](Player&) {
        promptPPH();
    };
    cb.onFestival = [this](Player&) {
        promptFestival();
    };
    cb.onLiquidation = [this](Player&, int required, Player* creditor) {
        promptLiquidation(required, creditor);
    };
    cb.onTeleport = [this](Player&) -> int {
        return promptTeleportIndex();
    };
    cb.onLasso = [this](Player& caster) -> Player* {
        return promptLassoTarget(caster);
    };
    cb.onDemolition = [this](Player& caster) -> string {
        return promptDemolitionCode(caster);
    };
    cb.onDropCard = [this](Player& p) {
        promptDropCard(p);
    };
    cb.onDiceRolled = [](int d1, int d2) {
        cout << "Hasil: " << d1 << " + " << d2 << " = " << (d1 + d2) << "\n";
    };

    cb.onAutoPurchase = [](Property& prop) {
        cout << "\nKamu mendarat di " << prop.name() << " (" << prop.code() << ")!\n";
        cout << "Properti ini " << prop.name() << " otomatis menjadi milikmu!\n";
    };

    cb.onOfferPurchase = [this](Property& prop) -> bool {
        return promptBuyStreet(prop);
    };
    game_.setCallbacks(move(cb));
}

void GameCLI::run() {
    cout
        << "\n"
        << "  ███╗   ██╗██╗███╗   ███╗ ██████╗ ███╗  ██╗███████╗██████╗  ██████╗ ██╗     ██╗\n"
        << "  ████╗  ██║██║████╗ ████║██╔═══██╗████╗ ██║██╔════╝██╔══██╗██╔═══██╗██║     ██║\n"
        << "  ██╔██╗ ██║██║██╔████╔██║██║   ██║██╔██╗██║███████╗██████╔╝██║   ██║██║     ██║\n"
        << "  ██║╚██╗██║██║██║╚██╔╝██║██║   ██║██║╚████║╚════██║██╔═══╝ ██║   ██║██║     ██║\n"
        << "  ██║ ╚████║██║██║ ╚═╝ ██║╚██████╔╝██║ ╚███║███████║██║     ╚██████╔╝███████╗██║\n"
        << "  ╚═╝  ╚═══╝╚═╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚══╝╚══════╝╚═╝      ╚═════╝╚══════╝╚═╝\n\n";

    registerCallbacks();
    showMainMenu();
}

void GameCLI::showMainMenu() {
    cout << "1. New Game\n2. Load Game\n";
    int choice = promptInt("Pilih", 1, 2);
    if (choice == 1) setupNewGame();
    else             setupLoadGame();
    gameLoop();
}

void GameCLI::setupNewGame() {
    int n = promptInt("Jumlah pemain ", 2, 4);
    for (int i = 0; i < n; ++i) {
        string name = prompt("Username pemain " + to_string(i + 1));
        while (name.empty()) name = prompt("Username tidak boleh kosong");
        game_.addPlayer(name);
    }
    game_.randomizeTurnOrder();
    game_.initCardDecks();
    cout << "Permainan dimulai!\n";
    game_.logger().log(0, "SISTEM", "MULAI", to_string(n) + " pemain");
}

void GameCLI::setupLoadGame() {
    string path = prompt("Nama file save");
    if (!SaveManager::load(game_, path)) {
        cout << "Gagal memuat. Memulai game baru...\n";
        setupNewGame();
    } else {
        cout << "Dimuat. Melanjutkan giliran " << game_.currentPlayer().username() << "...\n";
    }
}

void GameCLI::gameLoop() {
    while (!game_.isOver()) processTurn();
    printer_.printWinner();
}

void GameCLI::processTurn() {
    Player& player = game_.currentPlayer();
    if (player.isBankrupt()) { game_.advanceTurn(); return; }

    game_.runTurn();

    printer_.printTurnHeader();
    turnOver_ = false;

    if (player.isJailed()) {
        cout << "Kamu di penjara (percobaan ke-" << player.jailTurns()
                  << "). Opsi: BAYAR_DENDA | LEMPAR_DADU\n";
    }

    while (!turnOver_ && !game_.isOver()) {
        string line = prompt(player.username() + "> ");
        if (line.empty()) continue;
        try {
            dispatch(line);
        } catch (const exception& e) {
            printError(e.what());
        }
    }

    // Jangan advance turn jika pemain dapat double (giliran tambahan untuk pemain yang sama).
    // doubleCount > 0 && isDouble() berarti lemparan terakhir adalah double.
    bool gotDouble = game_.dice().isDouble() && game_.dice().doubleCount() > 0
                     && !player.isJailed() && !player.isBankrupt();
    if (!game_.isOver()) {
        if (gotDouble) {
            // Giliran tambahan: reset state lempar dan penggunaan kartu,
            // tapi pertahankan doubleCount agar triple-double bisa terdeteksi.
            player.setHasRolled(false);
            player.setUsedCard(false);
        } else {
            game_.dice().resetDoubleCount();
            game_.advanceTurn();
        }
    }
}

void GameCLI::dispatch(const string& line) {
    istringstream ss(line);
    string cmd; ss >> cmd;
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    string rest;
    getline(ss, rest);
    if (!rest.empty() && rest.front() == ' ') rest.erase(rest.begin());

    if      (cmd == "CETAK_PAPAN")       cmdCetakPapan();
    else if (cmd == "LEMPAR_DADU")       cmdLemparDadu();
    else if (cmd == "ATUR_DADU")         cmdAturDadu(rest);
    else if (cmd == "CETAK_AKTA")        cmdCetakAkta(rest);
    else if (cmd == "CETAK_PROPERTI")    cmdCetakProperti();
    else if (cmd == "GADAI")             cmdGadai();
    else if (cmd == "TEBUS")             cmdTebus();
    else if (cmd == "BANGUN")            cmdBangun();
    else if (cmd == "SIMPAN")            cmdSimpan(rest);
    else if (cmd == "MUAT")              cmdMuat(rest);
    else if (cmd == "CETAK_LOG")         cmdCetakLog(rest);
    else if (cmd == "GUNAKAN_KEMAMPUAN") cmdGunakanKemampuan();
    else if (cmd == "BANTUAN" || cmd == "HELP") printHelp();
    else if (cmd == "BAYAR_DENDA") {
        Player& p = game_.currentPlayer();
        if (!p.isJailed()) { printError("Kamu tidak sedang di penjara."); return; }
        int fine = game_.specialConfig().jailFine;
        if (!p.canAfford(fine)) { printError("Uang tidak cukup (M" + to_string(fine) + ")."); return; }
        game_.bank().collect(p, fine);
        p.setStatus(PlayerStatus::ACTIVE);
        p.resetJailTurns();
        game_.logger().log(game_.currentTurn(), p.username(), "KELUAR_PENJARA", "Bayar denda M" + to_string(fine));
        cout << "Denda dibayar. Kamu bebas! Saldo: M" << p.balance() << "\n";
    }
    else if (cmd == "SELESAI" || cmd == "END_TURN") {
        if (!game_.currentPlayer().hasRolled())
            printError("Kamu belum melempar dadu!");
        else
            turnOver_ = true;
    }
    else printError("Perintah tidak dikenal: " + cmd + ". Ketik BANTUAN untuk daftar perintah.");
}

void GameCLI::cmdCetakPapan()  { printer_.printBoard(); }
void GameCLI::cmdCetakProperti() { printer_.printProperties(); }

void GameCLI::cmdLemparDadu() {
    Player& player = game_.currentPlayer();
    if (player.hasRolled() && !game_.dice().isDouble()) {
        printError("Sudah melempar dadu. Ketik SELESAI untuk mengakhiri giliran.");
        return;
    }
    cout << "Mengocok dadu...\n";
    game_.cmdRollDice();
    auto& d = game_.dice();
    if (d.doubleCount() == 3) {
        cout << "Tiga kali double — masuk penjara!\n";
        turnOver_ = true;
    } else if (d.isDouble() && d.doubleCount() > 0) {
        cout << "Double! Giliran tambahan ke-" << d.doubleCount() << "\n";
        turnOver_ = true;  // keluar dari while loop, processTurn akan cek isDouble
    } else {
        turnOver_ = false; 
    }
}

void GameCLI::cmdAturDadu(const string& args) {
    Player& player = game_.currentPlayer();
    istringstream ss(args);
    int d1, d2;
    if (!(ss >> d1 >> d2)) { printError("Format: ATUR_DADU X Y  (1-6)"); return; }
    cout << "Dadu diatur secara manual.\n";
    game_.cmdSetDice(d1, d2);
    auto& d = game_.dice();
    if (d.doubleCount() == 3) {
        cout << "Tiga kali double — masuk penjara!\n";
        turnOver_ = true;
    } else if (d.isDouble() && d.doubleCount() > 0) {
        cout << "Double! Giliran tambahan ke-" << d.doubleCount() << "\n";
        turnOver_ = true;
    } else {
        turnOver_ = false;
    }
}

void GameCLI::cmdCetakAkta(const string& args) {
    string code = args;
    if (code.empty()) code = prompt("Masukkan kode petak");
    transform(code.begin(), code.end(), code.begin(), ::toupper);
    printer_.printDeed(code);
}

void GameCLI::cmdGadai() {
    Player& player = game_.currentPlayer();
    vector<Property*> eligible;
    for (auto* p : player.properties())
        if (!p->isMortgaged()) eligible.push_back(p);

    if (eligible.empty()) { cout << "Tidak ada properti yang dapat digadaikan.\n"; return; }

    cout << "=== Properti yang Dapat Digadaikan ===\n";
    for (int i = 0; i < (int)eligible.size(); ++i)
        cout << i+1 << ". " << eligible[i]->name()
                  << " (" << eligible[i]->code() << ")"
                  << " Nilai Gadai: M" << eligible[i]->mortgageValue() << "\n";

    int choice = promptInt("Pilih nomor properti (0 untuk batal)", 0, (int)eligible.size());
    if (choice == 0) return;

    try {
        string code = eligible[choice-1]->code();
        int val = eligible[choice-1]->mortgageValue();
        game_.cmdMortgage(code);
        cout << eligible[choice-1]->name() << " berhasil digadaikan.\n"
                  << "Kamu menerima M" << val << ". Saldo: M" << player.balance() << "\n";
    } catch (const exception& e) { printError(e.what()); }
}

void GameCLI::cmdTebus() {
    Player& player = game_.currentPlayer();
    vector<Property*> mortgaged;
    for (auto* p : player.properties())
        if (p->isMortgaged()) mortgaged.push_back(p);

    if (mortgaged.empty()) { cout << "Tidak ada properti yang digadaikan.\n"; return; }

    cout << "=== Properti yang Sedang Digadaikan ===\n";
    for (int i = 0; i < (int)mortgaged.size(); ++i) cout << i+1 << ". " << mortgaged[i]->name() << " [M] Harga Tebus: M" << mortgaged[i]->buyPrice() << "\n";
    cout << "Saldo kamu: M" << player.balance() << "\n";

    int choice = promptInt("Pilih nomor (0 untuk batal)", 0, (int)mortgaged.size());
    if (choice == 0) return;

    try {
        string name = mortgaged[choice-1]->name();
        game_.cmdRedeem(mortgaged[choice-1]->code());
        cout << name << " berhasil ditebus! Saldo: M" << player.balance() << "\n";
    } catch (const exception& e) { printError(e.what()); }
}

void GameCLI::cmdBangun() {
    Player& player = game_.currentPlayer();

    map<ColorGroup, vector<Street*>> groups;
    for (auto* prop : player.properties()) {
        if (prop->type() != PropertyType::STREET) continue;
        auto* s = static_cast<Street*>(prop);
        if (s->hasMonopoly()) groups[s->colorGroup()].push_back(s);
    }

    if (groups.empty()) {
        cout << "Tidak ada color group yang memenuhi syarat.\n" << "Kamu harus memonopoli seluruh petak dalam satu color group.\n";
        return;
    }

    cout << "=== Color Group yang Memenuhi Syarat ===\n";
    vector<ColorGroup> groupList;
    int gi = 1;
    for (auto& [cg, streets] : groups) {
        bool allHotel = true;
        for (auto* s : streets) if (!s->hasHotel()) allHotel = false;
        if (allHotel) continue;
        cout << gi++ << ". [" << colorGroupToString(cg) << "]\n";
        for (auto* s : streets) {
            string lvl = s->hasHotel() ? "Hotel" : to_string(s->buildingLevel()) + " rumah";
            cout << "   - " << s->name() << " (" << s->code() << "): " << lvl << " (next: M" << s->nextBuildCost() << ")\n";
        }
        groupList.push_back(cg);
    }
    if (groupList.empty()) {cout << "Semua properti sudah di level maksimal.\n"; return;}

    cout << "Saldo kamu: M" << player.balance() << "\n";
    int gchoice = promptInt("Pilih color group (0 untuk batal)", 0, (int)groupList.size());
    if (gchoice == 0) return;

    ColorGroup chosen = groupList[gchoice - 1];
    auto& streets = groups[chosen];


    int minLevel = INT_MAX;
    for (auto* s : streets) if (!s->hasHotel()) minLevel = min(minLevel, s->buildingLevel());

    vector<Street*> buildable;
    int si = 1;
    cout << "Color group [" << colorGroupToString(chosen) << "]:\n";
    for (auto* s : streets) {
        bool eligible = !s->hasHotel() && s->buildingLevel() == minLevel;
        string lvl = s->hasHotel() ? "Hotel" : to_string(s->buildingLevel()) + " rumah";
        cout << (eligible ? to_string(si) : " ") << ". " << s->name() << " (" << s->code() << "): " << lvl << (eligible ? " <- dapat dibangun" : "") << "\n";
        if (eligible) { buildable.push_back(s); ++si; }
    }

    int schoice = promptInt("Pilih petak (0 untuk batal)", 0, (int)buildable.size());
    if (schoice == 0) return;

    Street* target = buildable[schoice - 1];
    int cost = target->nextBuildCost();

    if (target->buildingLevel() == Street::MAX_HOUSES) {
        if (!promptYN("Upgrade ke hotel? Biaya: M" + to_string(cost) + " (y/n)")) return;
    }

    try {
        game_.cmdBuild(target->code());
        string lvlStr = target->hasHotel() ? "Hotel" : to_string(target->buildingLevel()) + " rumah";
        cout << "Berhasil membangun " << lvlStr << " di " << target->name() << ". Biaya: M" << cost << ". Saldo: M" << player.balance() << "\n";
    } catch (const exception& e) { printError(e.what()); }
}

void GameCLI::cmdSimpan(const string& args) {
    string path = args.empty() ? prompt("Nama file") : args;
    {
        ifstream existing(path);
        if (existing.good()) {
            if (!promptYN("File \"" + path + "\" sudah ada. Timpa? (y/n)")) return;
        }
    }
    try {
        SaveManager::save(game_, path);
        game_.logger().log(game_.currentTurn(), game_.currentPlayer().username(), "SIMPAN", "Disimpan ke " + path);
        cout << "Permainan berhasil disimpan ke: " << path << "\n";
    } catch (const exception& e) { printError(e.what()); }
}

void GameCLI::cmdMuat(const string& args) {
    string path = args.empty() ? prompt("Nama file") : args;
    if (!SaveManager::load(game_, path))
        printError("Gagal memuat! File rusak atau format tidak dikenali.");
    else
        cout << "Dimuat. Melanjutkan giliran " << game_.currentPlayer().username() << "...\n";
}

void GameCLI::cmdCetakLog(const string& args) {
    int n = 0;
    if (!args.empty()) { try { n = stoi(args); } catch (...) {} }
    game_.cmdPrintLog(n);
}

void GameCLI::cmdGunakanKemampuan() {
    Player& player = game_.currentPlayer();
    if (player.isJailed()) {cout << "Tidak bisa menggunakan kartu kemampuan saat di penjara.\n"; return;}
    if (player.hasRolled()) {cout << "Kartu kemampuan hanya bisa digunakan SEBELUM melempar dadu.\n"; return;}
    if (player.usedCardThisTurn()) {cout << "Sudah menggunakan kartu pada giliran ini (maks 1x).\n"; return;}
    if (player.handSize() == 0) {cout << "Kamu tidak memiliki kartu kemampuan.\n"; return;}

    cout << "Daftar Kartu Kemampuan:\n";
    for (int i = 0; i < player.handSize(); ++i) cout << i+1 << ". " << player.hand()[i]->description() << "\n";
    cout << "0. Batal\n";

    int choice = promptInt("Pilih kartu", 0, player.handSize());
    if (choice == 0) return;

    try {
        game_.cmdUseSkillCard(choice - 1);
        cout << "Kartu digunakan!\n";
    } catch (const exception& e) { printError(e.what()); }
}

bool GameCLI::promptBuyStreet(Property& prop) {
    cout << "\nKamu mendarat di " << prop.name() << " (" << prop.code() << ")!\n";
    printer_.printDeed(prop.code());
    cout << "Saldo kamu: M" << game_.currentPlayer().balance() << "\n";
    if (!game_.currentPlayer().canAfford(prop.buyPrice())) {
        cout << "Uang tidak cukup untuk membeli properti ini.\n";
        return false;
    }
    return promptYN("Beli seharga M" + to_string(prop.buyPrice()) + "? (y/n)");
}

void GameCLI::runAuction(Property& prop) {
    auto active = game_.activePlayers();
    Player* trigger = &game_.currentPlayer();
    vector<Player*> order = game_.board().auctionOrder(trigger, active);

    cout << "\nProperti " << prop.name() << " (" << prop.code() << ") akan dilelang!\n" << "Urutan lelang dimulai dari pemain setelah " << trigger->username() << ".\n";

    int highBid   = 0;
    Player* winner = nullptr;
    int passCount  = 0;
    int needed     = game_.board().auctionPassesNeeded(order);
    size_t idx = 0;

    while (true) {
        Player* cur = order[idx % order.size()];
        cout << "Giliran: " << cur->username() << "\n";
        if (winner) cout << "Penawaran tertinggi: M" << highBid << " (" << winner->username() << ")\n";
        else cout << "Penawaran tertinggi: belum ada\n";

        cout << "Aksi (PASS / BID <jumlah>): ";
        string line; getline(cin, line);
        istringstream ss(line);
        string action; ss >> action;
        transform(action.begin(), action.end(), action.begin(), ::toupper);

        if (action == "PASS") {
            ++passCount;
            game_.logger().log(game_.currentTurn(), cur->username(), "LELANG", "PASS");
        } else if (action == "BID") {
            string rawAmount;
            ss >> rawAmount;
            int amount = -1;
            if (!rawAmount.empty()) {
                string digits;
                for (char c : rawAmount) {
                    if (isdigit(static_cast<unsigned char>(c))) digits.push_back(c);
                }
                if (!digits.empty()) {
                    try {
                        amount = stoi(digits);
                    } catch (...) {
                        amount = -1;
                    }
                }
            }
            if (amount <= 0) {
                cout << "Masukkan BID dengan angka valid, contoh: BID 100\n";
                continue;
            }
            if (amount <= highBid) {cout << "Penawaran harus lebih dari M" << highBid << "\n"; continue;}
            if (!cur->canAfford(amount)) {cout << "Uang tidak cukup (saldo: M" << cur->balance() << ")\n"; continue;}
            highBid   = amount;
            winner    = cur;
            passCount = 0;
            game_.logger().log(game_.currentTurn(), cur->username(), "LELANG", "BID M" + to_string(amount));
            cout << "Penawaran tertinggi: M" << highBid << " (" << winner->username() << ")\n";
        } else {
            cout << "Masukkan PASS atau BID <jumlah>\n";
            continue;
        }

        if (passCount >= needed && winner != nullptr) {
            cout << "\nLelang selesai!\n"
                      << "Pemenang: " << winner->username()
                      << " | Harga akhir: M" << highBid << "\n"
                      << "Properti " << prop.name() << " kini dimiliki "
                      << winner->username() << ".\n";
            game_.finishAuction(*winner, prop, highBid);
            return;
        }
        if (passCount >= (int)order.size() && winner == nullptr) {
            cout << "Minimal satu pemain harus melakukan bid. " << order[0]->username() << " wajib bid.\n";
            idx  = 0;
            passCount = 0;
            continue;
        }

        ++idx;
    }
}

void GameCLI::promptPPH() {
    Player& player = game_.currentPlayer();
    cout << "\nKamu mendarat di Pajak Penghasilan (PPH)!\n"
              << "1. Bayar flat M" << game_.taxConfig().pphFlat << "\n"
              << "2. Bayar " << (int)(game_.taxConfig().pphPercent * 100)
              << "% dari total kekayaan\n"
              << "(Pilih SEBELUM menghitung kekayaan!)\n";

    int choice = promptInt("Pilihan", 1, 2);
    if (choice == 1) {
        int flat = game_.taxConfig().pphFlat;
        game_.resolveTaxPPHChoice(player, false);
        if (player.isBankrupt()) cout << "Uang tidak cukup untuk bayar M" << flat << "! Memproses kebangkrutan...\n";
        else cout << "Pajak M" << flat << " dibayar. Saldo: M" << player.balance() << "\n";
    } else {
        int wealth = player.netWorth();
        int tax    = static_cast<int>(wealth * game_.taxConfig().pphPercent);
        cout << "Total kekayaan:\n" << "  Uang tunai   : M" << player.balance() << "\n" << "  Total        : M" << wealth << "\n" << "  Pajak " << (int)(game_.taxConfig().pphPercent*100) << "%   : M" << tax << "\n";
        game_.resolveTaxPPHChoice(player, true);
        if (player.isBankrupt()) cout << "Uang tidak cukup! Memproses kebangkrutan...\n";
        else cout << "Pajak M" << tax << " dibayar. Saldo: M" << player.balance() << "\n";
    }
}

void GameCLI::promptFestival() {
    Player& player = game_.currentPlayer();
    vector<Property*> streets;
    for (auto* p : player.properties())
        if (p->type() == PropertyType::STREET) streets.push_back(p);

    if (streets.empty()) {
        cout << "Kamu tidak memiliki properti Street.\n"; return;
    }

    cout << "\nKamu mendarat di petak Festival!\nDaftar properti milikmu:\n";
    for (auto* s : streets)
        cout << "- " << s->code() << " (" << s->name() << ")\n";

    while (true) {
        string code = prompt("Masukkan kode properti");
        transform(code.begin(), code.end(), code.begin(), ::toupper);
        try {
            game_.applyFestival(player, code);
            auto* prop = game_.board().getProperty(code);
            auto* s    = static_cast<Street*>(prop);
            if (s->festival().multiplier() > (1 << 3))
                cout << "Efek sudah maksimum (x8). Durasi di-reset 3 giliran.\n";
            else
                cout << "Efek festival! Sewa x" << s->festival().multiplier() << " selama " << s->festival().duration() << " giliran.\n";
            return;
        } catch (const exception& e) { printError(e.what()); }
    }
}

void GameCLI::promptLiquidation(int required, Player* creditor) {
    Player& player = game_.currentPlayer();
    int maxLiq = player.maxLiquidation();

    cout << "\n=== PANEL LIKUIDASI ===\n"
              << "Kewajiban  : M" << required << "\n"
              << "Saldo kamu : M" << player.balance() << "\n"
              << "Kekurangan : M" << (required - player.balance()) << "\n"
              << "Potensi likuidasi : M" << maxLiq << "\n\n";

    if (player.balance() + maxLiq < required) {
        cout << "Tidak cukup untuk menutup kewajiban.\n"
                  << player.username() << " dinyatakan BANGKRUT!\n";
        player.setStatus(PlayerStatus::BANKRUPT);
        turnOver_ = true;
        return;
    }

    cout << "Kamu wajib melikuidasi aset hingga kewajiban terpenuhi.\n";

    while (player.balance() < required && !player.properties().empty()) {
        vector<pair<string, Property*>> opts;
        int n = 1;
        cout << "\n[Jual ke Bank]\n";
        for (auto* p : player.properties()) {
            if (!p->isMortgaged()) {
                cout << n++ << ". " << p->name() << " (" << p->code()
                          << ") Nilai jual: M" << p->liquidationValue() << "\n";
                opts.push_back({"SELL", p});
            }
        }
        cout << "[Gadaikan]\n";
        for (auto* p : player.properties()) {
            if (!p->isMortgaged()) {
                cout << n++ << ". " << p->name() << " (" << p->code() << ") Nilai gadai: M" << p->mortgageValue() << "\n";
                opts.push_back({"GADAI", p});
            }
        }

        int choice = promptInt("Pilih aksi (0 jika sudah cukup)", 0, n-1);
        if (choice == 0) break;

        auto [action, prop] = opts[choice - 1];
        if (action == "SELL") {
            try {
                game_.cmdLiquidateSell(prop->code());
                cout << prop->name() << " terjual ke Bank. Saldo: M" << player.balance() << "\n";
            } catch (const exception& e) { printError(e.what()); }
        } else {
            try {
                game_.cmdMortgage(prop->code());
                cout << prop->name() << " digadaikan. Saldo: M"
                          << player.balance() << "\n";
            } catch (const exception& e) { printError(e.what()); }
        }
    }

    if (player.balance() >= required) {
        if (creditor) {
            game_.bank().transfer(player, *creditor, required);
            cout << "Kewajiban M" << required << " lunas kepada " << creditor->username() << ". Saldo: M" << player.balance() << "\n";
        } else {
            game_.bank().collect(player, required);
            cout << "Kewajiban M" << required << " lunas ke Bank. Saldo: M" << player.balance() << "\n";
        }
    } else {
        cout << player.username() << " dinyatakan BANGKRUT!\n";
        player.setStatus(PlayerStatus::BANKRUPT);
        turnOver_ = true;
    }
}

void GameCLI::promptDropCard(Player& player) {
    cout << "\nPERINGATAN: Tangan penuh (" << player.handSize() << " kartu, maks " << Player::MAX_HAND_SIZE << ")!\n"
         << "Kamu harus membuang 1 kartu:\n";
    for (int i = 0; i < player.handSize(); ++i)
        cout << i+1 << ". " << player.hand()[i]->description() << "\n";
    int choice = promptInt("Pilih kartu yang dibuang", 1, player.handSize());
    // cmdDropCard pakai currentPlayer, jadi kita bypass langsung
    SkillCard* card = player.hand()[choice - 1];
    player.removeFromHand(choice - 1);
    game_.skillDeck().discard(card);
    game_.logger().log(game_.currentTurn(), player.username(), "DROP_KARTU", "Membuang " + card->description());
    cout << "Kartu dibuang. Sisa: " << player.handSize() << " kartu.\n";
}

int GameCLI::promptTeleportIndex() {
    cout << "TeleportCard — pilih petak tujuan:\n";
    for (int i = 0; i < game_.board().size(); ++i) {
        auto* t = game_.board().getTile(i);
        cout << setw(3) << i << " " << t->code() << " (" << t->name() << ")\n";
    }
    return promptInt("Masukkan nomor indeks petak", 0, game_.board().size() - 1);
}

Player* GameCLI::promptLassoTarget(Player& caster) {
    auto active = game_.activePlayers();
    vector<Player*> others;
    for (auto* p : active) if (p != &caster) others.push_back(p);
    if (others.empty()) { cout << "Tidak ada pemain lain.\n"; return nullptr; }

    cout << "LassoCard — pilih pemain yang ditarik:\n";
    for (int i = 0; i < (int)others.size(); ++i) cout << i+1 << ". " << others[i]->username()<< " (di " << game_.board().getTile(others[i]->position())->name() << ")\n";
    int choice = promptInt("Pilih", 1, (int)others.size());
    return others[choice - 1];
}

string GameCLI::promptDemolitionCode(Player& caster) {
    auto active = game_.activePlayers();
    vector<pair<Player*,Property*>> targets;
    int n = 1;
    cout << "DemolitionCard — pilih properti lawan untuk dihancurkan:\n";
    for (auto* p : active) {
        if (p == &caster) continue;
        for (auto* prop : p->properties()) {
            if (prop->type() == PropertyType::STREET) {
                auto* s = static_cast<Street*>(prop);
                if (s->buildingLevel() > 0) {
                    cout << n++ << ". " << p->username() << " | " << prop->name() << " (" << prop->code() << ") lv" << s->buildingLevel() << "\n";
                    targets.push_back({p, prop});
                }
            }
        }
    }
    if (targets.empty()) { cout << "Tidak ada target.\n"; return ""; }
    int choice = promptInt("Pilih", 1, (int)targets.size());
    return targets[choice-1].second->code();
}

string GameCLI::prompt(const string& msg) const {
    cout << msg << ": ";
    string line;
    getline(cin, line);
    while (!line.empty() && isspace((unsigned char)line.front())) line.erase(line.begin());
    while (!line.empty() && isspace((unsigned char)line.back()))  line.pop_back();
    return line;
}

int GameCLI::promptInt(const string& msg, int lo, int hi) const {
    while (true) {
        string in = prompt(msg + " (" + to_string(lo) + "-" + to_string(hi) + ")");
        try {
            int v = stoi(in);
            if (v >= lo && v <= hi) return v;
        } catch (...) {}
        cout << "Masukkan angka antara " << lo << " dan " << hi << ".\n";
    }
}

bool GameCLI::promptYN(const string& msg) const {
    while (true) {
        string s = prompt(msg);
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        cout << "Masukkan y atau n.\n";
    }
}

void GameCLI::printHelp() const {
    cout <<
        "\nPerintah yang tersedia:\n"
        "  CETAK_PAPAN          Tampilkan papan permainan\n"
        "  LEMPAR_DADU          Lempar dadu secara acak\n"
        "  ATUR_DADU X Y        Atur dadu manual (1-6)\n"
        "  CETAK_AKTA [KODE]    Tampilkan akta kepemilikan\n"
        "  CETAK_PROPERTI       Tampilkan properti milikmu\n"
        "  GADAI                Gadaikan properti\n"
        "  TEBUS                Tebus properti yang digadaikan\n"
        "  BANGUN               Bangun rumah/hotel\n"
        "  GUNAKAN_KEMAMPUAN    Gunakan kartu kemampuan\n"
        "  BAYAR_DENDA          Bayar denda keluar penjara\n"
        "  SIMPAN [FILE]        Simpan permainan\n"
        "  CETAK_LOG [N]        Tampilkan N log terakhir\n"
        "  SELESAI              Akhiri giliran\n"
        "  BANTUAN              Tampilkan menu ini\n\n";
}

void GameCLI::printError(const string& msg) const {
    cout << "[!] " << msg << "\n";
}

}