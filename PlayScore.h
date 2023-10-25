#pragma once
#include "commons.h"

class PlayScore {
public:
    std::array<double, 7> scores{};

    PlayScore(int level, double score) {
        scores[level] = score;
    }

    bool isBetterThan(PlayScore other, bool resultOnEqual) const {
        for (int i = 0; i < scores.size(); i++) {
            if (scores[i] != other.scores[i]) {
                return scores[i] > other.scores[i];
            }
        }
        return resultOnEqual;
    }
};