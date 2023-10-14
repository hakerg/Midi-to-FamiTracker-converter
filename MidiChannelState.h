#pragma once
#include "commons.h"

class MidiChannelState {
public:
    std::array<int, 128> velocities{};
    int program = 0;
    double volume = 1;
    int bank = 0;
    double expression = 1;
    double pitch = 0;
    int pitchRange = 2;
    bool useDrums = false;
    double fineTune = 0;
    int coarseTune = 0;

    double getNoteVolume(int velocity) const {
        return velocity * volume * expression;
    }

    double getNotePitch() const {
        return pitch * pitchRange + coarseTune + fineTune;
    }

    void stopAllNotes() {
        for (int i = 0; i < 128; i++) {
            velocities[i] = 0;
        }
    }

    std::vector<int> getActiveKeys() const {
		std::vector<int> keys;
        for (int i = 0; i < 128; i++) {
            if (velocities[i] > 0) {
                keys.push_back(i);
            }
        }
        return keys;
    }
};