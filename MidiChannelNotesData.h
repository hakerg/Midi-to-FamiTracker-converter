#pragma once
#include "commons.h"
#include "Pattern.h"
#include "MidiState.h"

class MidiChannelNotesData {
private:
	int countPlayedNotes(int assignedChannelCount) const {
		int count = 0;
		for (int i = 1; i <= assignedChannelCount; i++) {
			count += getMapValueOrDefault(noteCountAtNotesOn, i, 0);
		}
		return count;
	}

public:
	// how many notes appeared
	int notes{};

	// how many notes unplayable by specific nes channels
	std::array<int, int(NesChannel::CHANNEL_COUNT)> notesOutOfRange{};

	// sum of note volumes
	double volumeSum{};

	// how many chords with specific note count
	std::unordered_map<int, int> noteCountAtNotesOn{};

	// how many times this channel interrupted other channels
	std::array<int, MidiState::CHANNEL_COUNT> interruptingNotes{};


	// how many notes can be played per assigned channel count
	std::array<int, 5> playedNotes{};

	void calculate() {
		for (int i = 0; i < playedNotes.size(); i++) {
			playedNotes[i] = countPlayedNotes(i);
		}
	}

	void addNote(MidiEvent const& event, MidiState const& currentState) {
		const MidiChannelState& channelState = currentState.getChannel(event.chan);
		// drums are processed in InstrumentSelector::getNoteTriggers
		if (channelState.useDrums) {
			return;
		}

		notes++;
		for (int i = 0; i < notesOutOfRange.size(); i++) {
			if (!Note::isInPlayableRange(NesChannel(i), event.key)) {
				notesOutOfRange[i]++;
			}
		}

		// added small amount to fix i.e. the strings in DOOM - E1M2
		volumeSum += channelState.getNoteVolume(event.velocity) + 0.01;

		auto noteCount = int(channelState.notes.size());
		noteCountAtNotesOn[noteCount]++;

		for (int chan2 = 0; chan2 < MidiState::CHANNEL_COUNT; chan2++) {
			const MidiChannelState& channelState2 = currentState.getChannel(chan2);
			if (event.chan == chan2 || channelState2.useDrums) {
				continue;
			}

			interruptingNotes[chan2] += int(channelState2.notes.size());
		}
	}

	double getAverageVolume() const {
		return volumeSum / notes;
	}
};