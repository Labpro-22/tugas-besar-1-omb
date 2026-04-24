#pragma once
#include <string>
#include "core/types.h"

namespace Nimonspoli {

class Player;  // forward
class Game;    //forward

//Abstract
class Tile {
public:
    Tile(int index, const std::string& code,
         const std::string& name, TileType type)
        : index_(index), code_(code), name_(name), type_(type) {}

    virtual ~Tile() = default;
    virtual void onLanded(Player& player, Game& game) = 0;

    virtual std::string summary() const {
        return name_ + " (" + code_ + ")";
    }

    int                index() const { return index_; }
    const std::string& code()  const { return code_; }
    const std::string& name()  const { return name_; }
    TileType           type()  const { return type_; }

protected:
    int         index_;
    std::string code_;
    std::string name_;
    TileType    type_;
};


class Property;  // forward

class PropertyTile : public Tile {
public:
    PropertyTile(int index, const std::string& code,
                 const std::string& name, Property* property)
        : Tile(index, code, name, TileType::PROPERTY),
          property_(property) {}

    void      onLanded(Player& player, Game& game) override;
    Property* property() const { return property_; }
    
    std::string summary() const override;


private:
    Property* property_;   
};

class GoTile : public Tile {
public:
    GoTile(int index, int salary)
        : Tile(index, "GO", "Petak Mulai", TileType::GO), salary_(salary) {}

    void onLanded(Player& player, Game& game) override;
    int  salary() const { return salary_; }

private:
    int salary_;
};

class JailTile : public Tile {
public:
    explicit JailTile(int index)
        : Tile(index, "PEN", "Penjara", TileType::JAIL) {}

    void onLanded(Player& player, Game& game) override;
};

class FreeParkingTile : public Tile {
public:
    FreeParkingTile(int index)
        : Tile(index, "BBP", "Bebas Parkir", TileType::FREE_PARKING) {}

    void onLanded(Player& , Game&) override {}
};

class GoToJailTile : public Tile {
public:
    GoToJailTile(int index)
        : Tile(index, "PPJ", "Pergi ke Penjara", TileType::GO_TO_JAIL) {}

    void onLanded(Player& player, Game& game) override;
};

class TaxTile : public Tile {
public:
    TaxTile(int index, const std::string& code,
            const std::string& name, TaxType taxType,
            int flatAmount, float percentage)
        : Tile(index, code, name, taxType == TaxType::PPH ? TileType::TAX_PPH : TileType::TAX_PBM),
          taxType_(taxType), flatAmount_(flatAmount), percentage_(percentage) {}

    void    onLanded(Player& player, Game& game) override;
    TaxType taxType()    const { return taxType_; }
    int     flatAmount() const { return flatAmount_; }
    float   percentage() const { return percentage_; }

private:
    TaxType taxType_;
    int     flatAmount_;
    float   percentage_;
};

class FestivalTile : public Tile {
public:
    FestivalTile(int index)
        : Tile(index, "FES", "Festival", TileType::FESTIVAL) {}

    void onLanded(Player& player, Game& game) override;
};

class ChanceTile : public Tile {
public:
    ChanceTile(int index)
        : Tile(index, "KSP", "Kesempatan", TileType::CHANCE) {}

    void onLanded(Player& player, Game& game) override;
};

class CommunityTile : public Tile {
public:
    CommunityTile(int index)
        : Tile(index, "DNU", "Dana Umum", TileType::COMMUNITY) {}

    void onLanded(Player& player, Game& game) override;
};

}