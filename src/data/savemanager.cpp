#include "data/savemanager.h"
#include "core/propertytypes.h"
#include "core/cards.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>

namespace Nimonspoli {

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
    const auto& entries = game.logger().entries();
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
        f >> turn >> maxTurn;

        int numPlayers;
        f >> numPlayers;
        f.ignore();

        deserializePlayers(game, f);
        string line;
        getline(f, line); 
        getline(f, line); 

        deserializeProperties(game, f);
        deserializeDeck(game, f);
        deserializeLog(game, f);
        return true;
    } catch (...) {
        return false;
    }
}

void SaveManager::deserializePlayers(Game& game, istream& in) {
    for (auto& playerPtr : game.players()) {
        Player& player = *playerPtr;
        string username, posCode, statusStr;
        int balance;
        in >> username >> balance >> posCode >> statusStr;
        in.ignore();

        int diff = balance - player.balance();
        player += diff;

        for (int i = 0; i < game.board().size(); ++i) {
            if (game.board().getTile(i)->code() == posCode) {
                player.setPosition(i);
                break;
            }
        }

        if (statusStr == "JAILED")        player.setStatus(PlayerStatus::JAILED);
        else if (statusStr == "BANKRUPT") player.setStatus(PlayerStatus::BANKRUPT);
        else                              player.setStatus(PlayerStatus::ACTIVE);
        int cardCount; in >> cardCount; in.ignore();
        for (int c = 0; c < cardCount; ++c) {
            string line; getline(in, line);
            istringstream ss(line);
            string type; ss >> type;
            int value = 0, dur = 0;
            ss >> value >> dur;
            unique_ptr<SkillCard> card;
            if      (type == "MoveCard")       card = make_unique<MoveCard>(value);
            else if (type == "DiscountCard")   card = make_unique<DiscountCard>(value);
            else if (type == "ShieldCard")     card = make_unique<ShieldCard>();
            else if (type == "TeleportCard")   card = make_unique<TeleportCard>();
            else if (type == "LassoCard")      card = make_unique<LassoCard>();
            else if (type == "DemolitionCard") card = make_unique<DemolitionCard>();
            if (card) {
                if (dur > 0) card->setDuration(dur);
                SkillCard* raw = card.get();
                game.skillDeck().addCard(move(card));
                player.addToHand(raw);
            }
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
        // Rebuild cards into deck — full implementation in milestone 2
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
    game.logger().loadEntries(entries);
}

} 