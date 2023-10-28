#pragma once
#include "commons.h"
#include "PlayingNesNote.h"

class NesState {
public:
    std::array<std::optional<PlayingNesNote>, Pattern::CHANNELS> channels;
    double seconds = 0;
    double rowsPerSecond;

    explicit NesState(double rowsPerSecond) : rowsPerSecond(rowsPerSecond) {}

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

    int getRow(double seconds_) const {
        return int(round(seconds_ * rowsPerSecond));
    }

    int countOverlapAudio(NesChannel ignoreChannel, int key) const {
        if (ignoreChannel == NesChannel::NOISE || ignoreChannel == NesChannel::DPCM) {
            return 0;
        }
        int count = 0;
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            if (nesChannel == ignoreChannel || nesChannel == NesChannel::NOISE || nesChannel == NesChannel::DPCM) {
                continue;
            }
            const std::optional<PlayingNesNote>& note = getNote(nesChannel);
            if (note && note->playing && note->midiKey == key) {
                count++;
            }
        }
        return count;
    }

    int countOverlapAudio(NesChannel ignoreChannel, int key, int detuneIndex) const {
        if (ignoreChannel == NesChannel::NOISE || ignoreChannel == NesChannel::DPCM) {
            return 0;
        }
        int count = 0;
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            if (nesChannel == ignoreChannel || nesChannel == NesChannel::NOISE || nesChannel == NesChannel::DPCM) {
                continue;
            }
            const std::optional<PlayingNesNote>& note = getNote(nesChannel);
            if (note && note->playing && detuneIndex == note->detuneIndex && note->midiKey == key) {
                count++;
            }
        }
        return count;
    }

    int getDetuneIndex(NesChannel ignoreChannel, int key) const {
        // detune unnecessary with these channels
        if (ignoreChannel == NesChannel::NOISE || ignoreChannel == NesChannel::DPCM) {
            return 0;
        }

        // this is the period delta
        int index = 0;
        while (countOverlapAudio(ignoreChannel, key, index) > 0) {
            if (index <= 0) {
                index = -index + 1;
            }
            else {
                index = -index;
            }
        }
        return index;
    }
};