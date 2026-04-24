#pragma once
#include <string>
#include "core/types.h"

namespace Nimonspoli {

class Player;  // forward decl
class Game;    // forward decl

// Abstract base 
class Card {
public:
    explicit Card(const std::string& description) : description_(description) {}
    virtual ~Card() = default;

    virtual void execute(Player& player, Game& game) = 0;
    const std::string& description() const { return description_; }

protected:
    std::string description_;
};

// Abstract Chance Card
class ChanceCard : public Card {
public:
    explicit ChanceCard(const std::string& desc) : Card(desc) {}
    virtual void execute(Player& player, Game& game) = 0;
};

class ChanceGoNearestStation : public ChanceCard {
public:
    ChanceGoNearestStation();
    void execute(Player& player, Game& game) override;
};

class ChanceMoveBack3 : public ChanceCard {
public:
    ChanceMoveBack3();
    void execute(Player& player, Game& game) override;
};

class ChanceGoToJail : public ChanceCard {
public:
    ChanceGoToJail();
    void execute(Player& player, Game& game) override;
};

// Abstract Community Chest Card
class CommunityCard : public Card {
public:
    explicit CommunityCard(const std::string& desc) : Card(desc) {}
    virtual void execute(Player& player, Game& game) = 0;
};

class CommunityBirthday : public CommunityCard {
public:
    CommunityBirthday();
    void execute(Player& player, Game& game) override;
};

class CommunityDoctor : public CommunityCard {
public:
    CommunityDoctor();
    void execute(Player& player, Game& game) override;
};

class CommunityElection : public CommunityCard {
public:
    CommunityElection();
    void execute(Player& player, Game& game) override;
};


// Abstract SkillCard 
class SkillCard : public Card {
public:
    SkillCard(const std::string& desc, SkillCardType type, int value = 0)
        : Card(desc), skillType_(type), value_(value) {}

    virtual void use(Player& player, Game& game) = 0;

    // SkillCards dipake dlu baru ngeroll
    void execute(Player& player, Game& game) override { use(player, game); }

    SkillCardType skillType()        const { return skillType_; }
    int           value()            const { return value_; }
    int           remainingDuration() const { return remainingDuration_; }
    void          tickDuration()           { if (remainingDuration_ > 0) --remainingDuration_; }
    void          setDuration(int d)       { remainingDuration_ = d; }

protected:
    SkillCardType skillType_;
    int           value_             = 0;
    int           remainingDuration_ = 0;
};

class MoveCard : public SkillCard {
public:
    // steps di random
    explicit MoveCard(int steps)
        : SkillCard("Maju " + std::to_string(steps) + " petak",
                    SkillCardType::MOVE, steps) {}
    void use(Player& player, Game& game) override;
};

class DiscountCard : public SkillCard {
public:
    // pct dirandom
    explicit DiscountCard(int pct)
        : SkillCard("Diskon " + std::to_string(pct) + "% untuk pembelian",
                    SkillCardType::DISCOUNT, pct) {
        remainingDuration_ = 1;
    }
    void use(Player& player, Game& game) override;
};

class ShieldCard : public SkillCard {
public:
    ShieldCard()
        : SkillCard("Immune tagihan atau sanksi selama 1 giliran",
                    SkillCardType::SHIELD) {
        remainingDuration_ = 1;
    }
    void use(Player& player, Game& game) override;
};

class TeleportCard : public SkillCard {
public:
    TeleportCard()
        : SkillCard("Pindah ke petak manapun di papan",
                    SkillCardType::TELEPORT) {}
    void use(Player& player, Game& game) override;
};
class LassoCard : public SkillCard {
public:
    LassoCard()
        : SkillCard("Tarik satu pemain lawan ke petak kamu",
                    SkillCardType::LASSO) {}
    void use(Player& player, Game& game) override;
};
class DemolitionCard : public SkillCard {
public:
    DemolitionCard()
        : SkillCard("Hancurkan satu properti milik pemain lawan",
                    SkillCardType::DEMOLITION) {}
    void use(Player& player, Game& game) override;
};
} 