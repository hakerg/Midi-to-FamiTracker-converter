#pragma once
#include "commons.h"

class PlayScore {
public:
    std::array<double, 9> score;

    PlayScore() = default;

    PlayScore(int level, double score) {
        this->score[level] = score;
    }

    bool isBetterThan(PlayScore other, bool resultOnEqual) const {
        for (int i = 0; i < score.size(); i++) {
            if (score[i] != other.score[i]) {
                return score[i] > other.score[i];
            }
        }
        return resultOnEqual;
    }

    static PlayScore reduce(PlayScore a, PlayScore b) {
        PlayScore score{};
        for (int i = 0; i < score.score.size(); i++) {
            score.score[i] = a.score[i] + b.score[i];
        }
        return score;
    }
};