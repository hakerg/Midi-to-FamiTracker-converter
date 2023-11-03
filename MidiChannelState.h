#pragma once
#include "commons.h"

class MidiChannelState {
public:
    std::unordered_map<int, int> noteVelocities;
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

    double getPitchedKey(int key) const {
        return key + pitch * pitchRange + coarseTune + fineTune;
    }

    void stopAllNotes() {
        noteVelocities.clear();
    }

    bool isPlaying(int key) const {
        return noteVelocities.contains(key);
    }
};