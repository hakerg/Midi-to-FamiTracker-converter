#pragma once
#include "commons.h"

class PlayScore {
private:
    std::array<double, 6> scores{};

public:
    enum class Level { INTERRUPTS, PLAYING, PLAYABLE_RANGE, NOTE_TIME, NOTE_HEIGHT, VELOCITY };

    explicit PlayScore(double defaultScore) {
        scores[0] = defaultScore;
    }

    bool isBetterThan(PlayScore other, bool resultOnEqual) const {
        for (int i = 0; i < scores.size(); i++) {
            if (scores[i] != other.scores[i]) {
                return scores[i] > other.scores[i];
            }
        }
        return resultOnEqual;
    }

    PlayScore substract(PlayScore other) const {
        PlayScore newScore(0);
        for (int i = 0; i < scores.size(); i++) {
            newScore.scores[i] = scores[i] - other.scores[i];
        }
        return newScore;
    }

    void set(Level level, double score) {
        scores[int(level)] = score;
    }
};