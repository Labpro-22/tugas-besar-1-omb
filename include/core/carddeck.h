#pragma once
#include <vector>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <memory>
using namespace std;

namespace Nimonspoli {

template <typename T>
class CardDeck {
public:
    CardDeck() : rng_(random_device{}()) {}

    void addCard(std::unique_ptr<T> card) {
        drawPile_.push_back(std::move(card));
    }

    void shuffle() {
        std::shuffle(drawPile_.begin(), drawPile_.end(), rng_);
    }

    T* draw() {
        if (drawPile_.empty()) reshuffleDiscard();
        if (drawPile_.empty()) throw runtime_error("CardDeck is empty.");
        T* card = drawPile_.back().release();
        drawPile_.pop_back();
        return inFlightPile_.back().get();
    }

    void discard(T* card) {
        for (auto it = inFlightPile_.begin(); it != inFlightPile_.end(); ++it) {
            if (it->get() == card) {
                discardPile_.push_back(std::move(*it));
                inFlightPile_.erase(it);
                return;
            }
        }
    }

    int remaining() const { return static_cast<int>(drawPile_.size()); }
    int discarded() const { return static_cast<int>(discardPile_.size()); }

    std::vector<T*> peekDrawPile() const {
        std::vector<T*> out;
        for (const auto& c : drawPile_) out.push_back(c.get());
        return out;
    }

    std::vector<T*> peekDiscardPile() const {
        std::vector<T*> out;
        for (const auto& c : discardPile_) out.push_back(c.get());
        return out;
    }

private:
    void reshuffleDiscard() {
        for (auto& c : discardPile_)
            drawPile_.push_back(std::move(c));
        discardPile_.clear();
        shuffle();
    }

    vector<unique_ptr<T>> drawPile_;
    vector<unique_ptr<T>> inFlightPile_;
    vector<unique_ptr<T>> discardPile_;
    mt19937               rng_;
};

}