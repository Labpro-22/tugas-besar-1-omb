#pragma once
#include <utility>
#include <random>
#include <stdexcept>
using namespace std;

namespace Nimonspoli {

class Dice {
public:
    Dice() : rng_(random_device{}()), dist_(1, 6) {}

    // Random roll
    pair<int,int> roll() {
        die1_ = dist_(rng_);
        die2_ = dist_(rng_);
        trackDouble();
        return {die1_, die2_};
    }

    // Manual override (ATUR_DADU X Y)
    pair<int,int> setRoll(int d1, int d2) {
        if (d1 < 1 || d1 > 6 || d2 < 1 || d2 > 6)
            throw std::invalid_argument("Dice values must be between 1 and 6.");
        die1_ = d1;
        die2_ = d2;
        trackDouble();
        return {die1_, die2_};
    }

    int  die1()        const { return die1_; }
    int  die2()        const { return die2_; }
    int  total()       const { return die1_ + die2_; }
    bool isDouble()    const { return die1_ == die2_; }
    int  doubleCount() const { return doubleCount_; }

    void resetDoubleCount() { doubleCount_ = 0; }

private:
    void trackDouble() {
        if (isDouble()) ++doubleCount_;
        else            doubleCount_ = 0;
    }

    mt19937                    rng_;
    uniform_int_distribution<> dist_;
    int die1_        = 0;
    int die2_        = 0;
    int doubleCount_ = 0;
};

} 