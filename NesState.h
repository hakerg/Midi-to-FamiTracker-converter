#pragma once
#include "commons.h"
#include "PlayingNesNote.h"

class NesState {
public:
    std::array<std::optional<PlayingNesNote>, int(NesChannel::CHANNEL_COUNT)> channels;
    double seconds = 0;
    double rowsPerSecond;
    int rowShift = 0;

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

    void releaseNote(NesChannel channel) {
        if (getNote(channel)) {
            getNote(channel)->release();
        }
    }

    void killNote(NesChannel channel) {
        getNote(channel).reset();
    }

    int getRow() const {
        return getRow(seconds);
    }

    int getRow(double seconds_) const {
        return int(round(seconds_ * rowsPerSecond)) + rowShift;
    }

    double getSeconds(int row) const {
        return row / rowsPerSecond;
	}

	static bool isPulse(NesChannel channel) {
		using enum NesChannel;
		return channel == PULSE1 || channel == PULSE2 || channel == PULSE3 || channel == PULSE4;
	}

    int countPulseChannelsWithSameKeyAndLength(NesChannel ignoreNesChannel, MidiEvent const& event, double timeTollerancy) const {
        using enum NesChannel;
        if (!isPulse(ignoreNesChannel) || event.noteEndSeconds - seconds < timeTollerancy) {
            return 0;
        }
        int count = 0;
        for (int i = 0; i < int(NesChannel::CHANNEL_COUNT); i++) {
            auto nesChannel = NesChannel(i);
            if (nesChannel == ignoreNesChannel || !isPulse(nesChannel)) {
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

	static bool ignoreToneOverlap(NesChannel channel) {
		using enum NesChannel;
		return channel == NOISE || channel == DPCM || channel == TRIANGLE;
	}

    int countToneOverlappingChannels(NesChannel ignoreNesChannel, double frequency) const {
        using enum NesChannel;
        if (ignoreToneOverlap(ignoreNesChannel)) {
            return 0;
        }
        int count = 0;
        for (int i = 0; i < int(NesChannel::CHANNEL_COUNT); i++) {
            auto nesChannel = NesChannel(i);
            if (nesChannel == ignoreNesChannel || ignoreToneOverlap(nesChannel)) {
                continue;
            }
            const std::optional<PlayingNesNote>& note = getNote(nesChannel);
            if (note && note->playing && (
                std::abs(frequency - note->frequencyAfterPitch) < 1 ||
                std::abs(frequency * 0.5 - note->frequencyAfterPitch) < 1 ||
                std::abs(frequency - note->frequencyAfterPitch * 0.5) < 1)) {
                count++;
            }
        }
        return count;
    }
};