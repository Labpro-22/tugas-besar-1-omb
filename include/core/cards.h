#pragma once
#include "core/types.h"
#include <string>


namespace Nimonspoli {

class Player; // forward decl
class Game;   // forward decl

// Abstract base
class Card {
public:
  explicit Card(const std::string &description) : description_(description) {}
  virtual ~Card() = default;

  virtual void execute(Player &player, Game &game) = 0;
  const std::string &description() const { return description_; }

protected:
  std::string description_;
};

// Chance cards
class ChanceCard : public Card {
public:
  explicit ChanceCard(ChanceEffect effect);
  void execute(Player &player, Game &game) override;
  ChanceEffect effect() const { return effect_; }

private:
  ChanceEffect effect_;
};

// Community Chest cards
class CommunityCard : public Card {
public:
  explicit CommunityCard(CommunityEffect effect);
  void execute(Player &player, Game &game) override;
  CommunityEffect effect() const { return effect_; }

private:
  CommunityEffect effect_;
};

// Abstract SkillCard
class SkillCard : public Card {
public:
  SkillCard(const std::string &desc, SkillCardType type, int value = 0)
      : Card(desc), skillType_(type), value_(value) {}

  virtual void use(Player &player, Game &game) = 0;

  // Dipanggil saat kartu ditarik dari deck
  virtual void onDraw() {}

  // SkillCards dipake dlu baru ngeroll
  void execute(Player &player, Game &game) override { use(player, game); }

  SkillCardType skillType() const { return skillType_; }
  int value() const { return value_; }
  int remainingDuration() const { return remainingDuration_; }
  void tickDuration() {
    if (remainingDuration_ > 0)
      --remainingDuration_;
  }
  void setDuration(int d) { remainingDuration_ = d; }

protected:
  SkillCardType skillType_;
  int value_ = 0;
  int remainingDuration_ = 0;
};

class MoveCard : public SkillCard {
public:
  explicit MoveCard(int steps = 0) : SkillCard("Maju sekian petak", SkillCardType::MOVE, steps) {
      if (steps > 0) description_ = "Maju " + std::to_string(steps) + " petak";
  }
  void onDraw() override;
  void use(Player &player, Game &game) override;
};

class DiscountCard : public SkillCard {
public:
  explicit DiscountCard(int pct = 0)
      : SkillCard("Diskon % untuk pembelian", SkillCardType::DISCOUNT, pct) {
    if (pct > 0) description_ = "Diskon " + std::to_string(pct) + "% untuk pembelian";
    remainingDuration_ = 1;
  }
  void onDraw() override;
  void use(Player &player, Game &game) override;
};

class ShieldCard : public SkillCard {
public:
  ShieldCard()
      : SkillCard("Immune tagihan atau sanksi selama 1 giliran",
                  SkillCardType::SHIELD) {
    remainingDuration_ = 1;
  }
  void use(Player &player, Game &game) override;
};

class TeleportCard : public SkillCard {
public:
  TeleportCard()
      : SkillCard("Pindah ke petak manapun di papan", SkillCardType::TELEPORT) {
  }
  void use(Player &player, Game &game) override;
};
class LassoCard : public SkillCard {
public:
  LassoCard()
      : SkillCard("Tarik satu pemain lawan ke petak kamu",
                  SkillCardType::LASSO) {}
  void use(Player &player, Game &game) override;
};
class DemolitionCard : public SkillCard {
public:
  DemolitionCard()
      : SkillCard("Hancurkan satu properti milik pemain lawan",
                  SkillCardType::DEMOLITION) {}
  void use(Player &player, Game &game) override;
};
} // namespace Nimonspoli