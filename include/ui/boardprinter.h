#pragma once
#include <string>
#include "core/game.h"
using namespace std;
namespace Nimonspoli {

// ANSI 
namespace Color {
    inline const char* RESET   = "\033[0m";
    inline const char* BOLD    = "\033[1m";

    // Buat tiles
    inline const char* COKLAT      = "\033[38;5;130m"; 
    inline const char* BIRU_MUDA   = "\033[38;5;81m";  
    inline const char* MERAH_MUDA  = "\033[38;5;218m"; 
    inline const char* ORANGE_C    = "\033[38;5;208m"; 
    inline const char* MERAH       = "\033[38;5;196m"; 
    inline const char* KUNING      = "\033[38;5;226m"; 
    inline const char* HIJAU       = "\033[38;5;46m";  
    inline const char* BIRU_TUA    = "\033[38;5;21m";  
    inline const char* UTIL        = "\033[38;5;244m"; 
    inline const char* DEFAULT_C   = "\033[38;5;250m"; 
}

class BoardPrinter {
public:
    explicit BoardPrinter(const Game& game) : game_(game) {}

  
    void printBoard() const;
    void printDeed(const string& code) const;
    void printProperties() const;
    void printTurnHeader() const;

    void printWinner() const;

private:
    const Game& game_;

    // Board layout helpers
    void printTopRow()    const;   // tiles 20-30 (atas kiri ke atas kanan)
    void printSideRows()  const;   // tiles 31-39 (dari kanan ke kiri)
    void printBottomRow() const;   // tiles 0-10  (bawah kanan ke bawah kiri)
    void printCenter()    const;   // logo + legend + turn info

    string tileCell(int tileIndex) const;
    string colorCode(int tileIndex) const;
    string buildingStr(int level) const;
    string ownerStr(const Property* prop) const;
    string playerBadges(int tileIndex) const;

    const char* groupColor(ColorGroup cg) const;
};

} -