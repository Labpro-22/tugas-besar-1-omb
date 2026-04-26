#include "data/savemanager.h"
#include "core/propertytypes.h"
#include "core/cards.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <vector>
#include <algorithm>

namespace Nimonspoli {

namespace {

struct SavedCard {
    string type;
    int value = 0;
    int dur = 0;
};

struct SavedPlayer {
    string username;
    int balance = 0;
    string posCode;
    string status;
    vector<SavedCard> cards;
};

unique_ptr<SkillCard> makeSkillCard(const string& type, int value = 0, int dur = 0) {
    unique_ptr<SkillCard> card;
    if (type == "MoveCard") {
        if (value <= 0) value = 1;
        card = make_unique<MoveCard>(value);
    } else if (type == "DiscountCard") {
        if (value <= 0) value = 10;
        card = make_unique<DiscountCard>(value);
    } else if (type == "ShieldCard") {
        card = make_unique<ShieldCard>();
    } else if (type == "TeleportCard") {
        card = make_unique<TeleportCard>();
    } else if (type == "LassoCard") {
        card = make_unique<LassoCard>();
    } else if (type == "DemolitionCard") {
        card = make_unique<DemolitionCard>();
    }

    if (card && dur > 0) card->setDuration(dur);
    return card;
}

} // namespace

void SaveManager::save(const Game& game, const string& path) {
    ofstream f(path);
    if (!f) throw runtime_error("Gagal menyimpan file: " + path);

    f << game.currentTurn() << " " << game.maxTurn() << "\n";
    f << game.players().size() << "\n";
    f << serializePlayers(game);
    auto active = game.activePlayers();
    for (auto* p : active)
        f << p->username() << " ";
    f << "\n";
    f << game.currentPlayer().username() << "\n";
    f << serializeProperties(game);
    f << serializeDeck(game);
    f << serializeLog(game);
}

string SaveManager::serializePlayers(const Game& game) {
    ostringstream ss;
    for (auto& p : game.players()) {
        string status;
        switch (p->status()) {
            case PlayerStatus::ACTIVE:   status = "ACTIVE";   break;
            case PlayerStatus::JAILED:   status = "JAILED";   break;
            case PlayerStatus::BANKRUPT: status = "BANKRUPT"; break;
        }
        string posCode = game.board().getTile(p->position())->code();
        ss << p->username() << " " << p->balance() << " "
           << posCode << " " << status << "\n";

        ss << p->handSize() << "\n";
        for (auto* card : p->hand()) {
            string type;
            int value = card->value();
            int dur   = card->remainingDuration();
            switch (card->skillType()) {
                case SkillCardType::MOVE:        type = "MoveCard";        break;
                case SkillCardType::DISCOUNT:    type = "DiscountCard";    break;
                case SkillCardType::SHIELD:      type = "ShieldCard";      break;
                case SkillCardType::TELEPORT:    type = "TeleportCard";    break;
                case SkillCardType::LASSO:       type = "LassoCard";       break;
                case SkillCardType::DEMOLITION:  type = "DemolitionCard";  break;
            }
            ss << type;
            if (card->skillType() == SkillCardType::MOVE ||
                card->skillType() == SkillCardType::DISCOUNT)
                ss << " " << value;
            if (card->skillType() == SkillCardType::DISCOUNT)
                ss << " " << dur;
            ss << "\n";
        }
    }
    return ss.str();
}

string SaveManager::serializeProperties(const Game& game) {
    ostringstream ss;
    const auto& props = game.board().properties();
    ss << props.size() << "\n";

    for (auto& [code, prop] : props) {
        string jenis, owner, status;
        int fmult = 1, fdur = 0;
        char buildChar = '0';

        switch (prop->type()) {
            case PropertyType::STREET:   jenis = "street";   break;
            case PropertyType::RAILROAD: jenis = "railroad"; break;
            case PropertyType::UTILITY:  jenis = "utility";  break;
        }
        owner  = prop->owner() ? prop->owner()->username() : "BANK";
        switch (prop->status()) {
            case PropertyStatus::BANK:       status = "BANK";       break;
            case PropertyStatus::OWNED:      status = "OWNED";      break;
            case PropertyStatus::MORTGAGED:  status = "MORTGAGED";  break;
        }

        if (prop->type() == PropertyType::STREET) {
            auto* s = static_cast<Street*>(prop.get());
            fmult = s->festival().multiplier();
            fdur  = s->festival().duration();
            int lvl = s->buildingLevel();
            if (lvl == Street::HOTEL)
                buildChar = 'H';
            else
                buildChar = '0' + lvl;
        }

        ss << code << " " << jenis << " " << owner << " " << status
           << " " << fmult << " " << fdur << " " << buildChar << "\n";
    }
    return ss.str();
}

string SaveManager::serializeDeck(const Game& game) {
    ostringstream ss;
    auto cards = game.skillDeck().peekDrawPile();
    ss << cards.size() << "\n";
    for (auto* c : cards) {
        switch (static_cast<SkillCard*>(c)->skillType()) {
            case SkillCardType::MOVE:       ss << "MoveCard\n";       break;
            case SkillCardType::DISCOUNT:   ss << "DiscountCard\n";   break;
            case SkillCardType::SHIELD:     ss << "ShieldCard\n";     break;
            case SkillCardType::TELEPORT:   ss << "TeleportCard\n";   break;
            case SkillCardType::LASSO:      ss << "LassoCard\n";      break;
            case SkillCardType::DEMOLITION: ss << "DemolitionCard\n"; break;
        }
    }
    return ss.str();
}

string SaveManager::serializeLog(const Game& game) {
    ostringstream ss;
    const auto& entries = TransactionLogger::entries();
    ss << entries.size() << "\n";
    for (auto& e : entries)
        ss << e.turn << " " << e.username << " " << e.action << " " << e.detail << "\n";
    return ss.str();
}

bool SaveManager::load(Game& game, const string& path) {
    ifstream f(path);
    if (!f) return false;

    try {
        int turn, maxTurn;
        if (!(f >> turn >> maxTurn)) return false;
        if (maxTurn != game.maxTurn()) return false;

        int numPlayers;
        if (!(f >> numPlayers)) return false;
        if (numPlayers < 2 || numPlayers > 4) return false;
        f.ignore();

        // Reset runtime state that will be reconstructed from file.
        game.players_.clear();
        game.turnOrder_.clear();
        game.turnOrderIdx_ = 0;
        game.currentTurn_ = turn;
        game.lastDiceTotal_ = 0;
        game.lastJailByTripleDouble_ = false;
        game.dice_.resetDoubleCount();
        TransactionLogger::loadEntries({});
        game.chanceDeck_ = CardDeck<ChanceCard>();
        game.communityDeck_ = CardDeck<CommunityCard>();
        game.skillDeck_ = CardDeck<SkillCard>();

        // Save nggak nyimpen Chance/Community deck, jadi harus bangun ulang deck default agar draw kartu gk crash
        game.chanceDeck_.addCard(make_unique<ChanceGoNearestStation>());
        game.chanceDeck_.addCard(make_unique<ChanceMoveBack3>());
        game.chanceDeck_.addCard(make_unique<ChanceGoToJail>());
        game.chanceDeck_.shuffle();
        
        game.communityDeck_.addCard(make_unique<CommunityBirthday>());
        game.communityDeck_.addCard(make_unique<CommunityDoctor>());
        game.communityDeck_.addCard(make_unique<CommunityElection>());
        game.communityDeck_.shuffle();

        for (auto& [_, prop] : game.board_->properties()) {
            prop->setOwner(nullptr);
            prop->setStatus(PropertyStatus::BANK);
            if (prop->type() == PropertyType::STREET) {
                auto* s = static_cast<Street*>(prop.get());
                s->demolishAll();
                s->festival().reset();
            }
        }

        deserializePlayers(game, f, numPlayers);

        // Active turn order line
        string activeLine;
        if (!getline(f, activeLine)) return false;
        istringstream activeSS(activeLine);
        vector<string> activeUsers;
        string u;
        while (activeSS >> u) activeUsers.push_back(u);

        // Current player line
        string currentUser;
        if (!getline(f, currentUser) || currentUser.empty()) return false;

        map<string, int> idxByUser;
        for (int i = 0; i < (int)game.players_.size(); ++i)
            idxByUser[game.players_[i]->username()] = i;

        vector<int> order;
        for (const auto& uname : activeUsers) {
            auto it = idxByUser.find(uname);
            if (it != idxByUser.end()) order.push_back(it->second);
        }
        for (int i = 0; i < (int)game.players_.size(); ++i) {
            if (find(order.begin(), order.end(), i) == order.end()) order.push_back(i);
        }
        if (order.empty()) return false;
        game.turnOrder_ = order;

        int curIdx = -1;
        for (int i = 0; i < (int)game.turnOrder_.size(); ++i) {
            if (game.players_[game.turnOrder_[i]]->username() == currentUser) {
                curIdx = i;
                break;
            }
        }
        if (curIdx < 0) return false;
        game.turnOrderIdx_ = curIdx;

        deserializeProperties(game, f);
        deserializeDeck(game, f);
        deserializeLog(game, f);
        return true;
    } catch (...) {
        return false;
    }
}

void SaveManager::deserializePlayers(Game& game, istream& in, int numPlayers) {
    vector<SavedPlayer> saved;
    saved.reserve(numPlayers);

    for (int i = 0; i < numPlayers; ++i) {
        SavedPlayer sp;
        if (!(in >> sp.username >> sp.balance >> sp.posCode >> sp.status)) {
            throw runtime_error("Format pemain tidak valid.");
        }
        in.ignore();

        int cardCount = 0;
        if (!(in >> cardCount)) throw runtime_error("Format jumlah kartu pemain tidak valid.");
        in.ignore();

        for (int c = 0; c < cardCount; ++c) {
            string line;
            if (!getline(in, line)) throw runtime_error("Format kartu pemain tidak valid.");
            istringstream ss(line);
            SavedCard sc;
            ss >> sc.type;
            ss >> sc.value >> sc.dur;
            sp.cards.push_back(sc);
        }
        saved.push_back(sp);
    }

    for (const auto& sp : saved) game.addPlayer(sp.username);

    for (int i = 0; i < (int)saved.size(); ++i) {
        Player& player = *game.players_[i];
        const SavedPlayer& sp = saved[i];

        int diff = sp.balance - player.balance();
        player += diff;

        bool foundPos = false;
        for (int t = 0; t < game.board().size(); ++t) {
            if (game.board().getTile(t)->code() == sp.posCode) {
                player.setPosition(t);
                foundPos = true;
                break;
            }
        }
        if (!foundPos) throw runtime_error("Kode tile pemain tidak valid: " + sp.posCode);

        if (sp.status == "JAILED")        player.setStatus(PlayerStatus::JAILED);
        else if (sp.status == "BANKRUPT") player.setStatus(PlayerStatus::BANKRUPT);
        else                               player.setStatus(PlayerStatus::ACTIVE);

        for (const auto& sc : sp.cards) {
            auto card = makeSkillCard(sc.type, sc.value, sc.dur);
            if (!card) continue;
            game.skillDeck_.addCard(std::move(card));
            SkillCard* handCard = game.skillDeck_.draw();
            player.addToHand(handCard);
        }
    }
}

void SaveManager::deserializeProperties(Game& game, istream& in) {
    int count; in >> count; in.ignore();
map<string, Player*> playerMap;
    for (auto& p : game.players())
        playerMap[p->username()] = p.get();

    for (int i = 0; i < count; ++i) {
        string code, jenis, ownerName, status;
        int fmult, fdur; char buildChar;
        in >> code >> jenis >> ownerName >> status >> fmult >> fdur >> buildChar;
        in.ignore();

        Property* prop = game.board().getProperty(code);
        if (!prop) continue;

        if (status == "OWNED") {
            prop->setStatus(PropertyStatus::OWNED);
            auto it = playerMap.find(ownerName);
            if (it != playerMap.end()) {
                prop->setOwner(it->second);
                it->second->addProperty(prop);
            }
        } else if (status == "MORTGAGED") {
            prop->setStatus(PropertyStatus::MORTGAGED);
            auto it = playerMap.find(ownerName);
            if (it != playerMap.end()) {
                prop->setOwner(it->second);
                it->second->addProperty(prop);
            }
        } else {
            prop->setStatus(PropertyStatus::BANK);
            prop->setOwner(nullptr);
        }

        if (prop->type() == PropertyType::STREET) {
            auto* s = static_cast<Street*>(prop);
            int lvl = (buildChar == 'H') ? Street::HOTEL : (buildChar - '0');
            s->setBuildingLevel(lvl);
            int boosts = 0;
            int m = fmult; while (m > 1) { m >>= 1; ++boosts; }
            s->festival().setState(fmult, fdur, boosts);
        }
    }
    for (auto& p : game.players())
        game.refreshPropertyCounts(p.get());
}

void SaveManager::deserializeDeck(Game& game, istream& in) {
    int count; in >> count;
    in.ignore();
    for (int i = 0; i < count; ++i) {
        string type; getline(in, type);
        auto card = makeSkillCard(type);
        if (card) game.skillDeck_.addCard(std::move(card));
    }
}

void SaveManager::deserializeLog(Game& game, istream& in) {
    int count; in >> count;
    in.ignore();
    vector<LogEntry> entries;
    for (int i = 0; i < count; ++i) {
        LogEntry e;
        string line; getline(in, line);
        istringstream ss(line);
        ss >> e.turn >> e.username >> e.action;
        getline(ss, e.detail);
        if (!e.detail.empty() && e.detail[0] == ' ')
            e.detail = e.detail.substr(1);
        entries.push_back(e);
    }
    TransactionLogger::loadEntries(entries);
}

} 