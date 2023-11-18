#pragma once
#include "commons.h"

class MidiChannelState {
public:

    class Note {
    public:
        int velocity;
        double seconds;

        Note(int velocity, double seconds) : velocity(velocity), seconds(seconds) {}
    };

    std::unordered_map<int, Note> notes{}; // key is key ;)
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
        notes.clear();
    }

    bool isPlaying(int key) const {
        return notes.contains(key);
    }
};