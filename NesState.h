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

    double getSeconds(int row) const {
        return row / rowsPerSecond;
    }

    int countChannelsWithSameKeyAndLength(NesChannel ignoreNesChannel, MidiEvent const& event, double timeTollerancy) const {
        using enum NesChannel;
        if (ignoreNesChannel == NOISE || ignoreNesChannel == DPCM || event.noteEndSeconds - seconds < timeTollerancy) {
            return 0;
        }
        int count = 0;
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            if (nesChannel == ignoreNesChannel || nesChannel == NOISE || nesChannel == DPCM) {
                continue;
            }
            const std::optional<PlayingNesNote>& note = getNote(nesChannel);
            if (note && note->playing && event.key == note->event.key && note->event.noteEndSeconds - seconds >= timeTollerancy &&
                std::abs(note->event.noteEndSeconds - event.noteEndSeconds) <= timeTollerancy) {

                count++;
            }
        }
        return count;
    }

    int countChannelsWithSameFrequency(NesChannel ignoreNesChannel, double frequency) const {
        using enum NesChannel;
        if (ignoreNesChannel == NOISE || ignoreNesChannel == DPCM) {
            return 0;
        }
        int count = 0;
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            if (nesChannel == ignoreNesChannel || nesChannel == NOISE || nesChannel == DPCM) {
                continue;
            }
            const std::optional<PlayingNesNote>& note = getNote(nesChannel);
            if (note && note->playing && std::abs(frequency - note->frequencyAfterPitch) < 1) {
                count++;
            }
        }
        return count;
    }
};