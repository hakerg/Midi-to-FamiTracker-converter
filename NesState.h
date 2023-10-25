#pragma once
#include "commons.h"
#include "PlayingNesNote.h"

class NesState {
public:
    std::array<std::optional<PlayingNesNote>, Pattern::CHANNELS> channels;
    double seconds = 0;

    std::optional<PlayingNesNote>& getNote(NesChannel channel) {
        return channels[int(channel)];
    }

    const std::optional<PlayingNesNote>& getNote(NesChannel channel) const {
        return channels[int(channel)];
    }

    void setNote(NesChannel channel, PlayingNesNote const& note) {
        getNote(channel) = note;
    }

    void resetNote(NesChannel channel) {
        if (getNote(channel)) {
            getNote(channel)->stop();
        }
    }

    int getRow() const {
        return getRow(seconds);
    }

    static int getRow(double seconds) {
        return int(round(seconds * 60));
    }
};