#pragma once
#include "commons.h"
#include "PresetScores.h"
#include "MidiState.h"
#include "MidiChannelState.h"

class PresetStatistics {
public:
	class Entry {
	public:
		int notes;
		double volumeSum;

		void addNote(double volume) {
			notes++;
			volumeSum += volume;
		}

		double getScore() const {
			return volumeSum / notes;
		}
	};

private:
	// key is program, 128 - drums
	std::array<std::unordered_map<int, Entry>, 16> stats;

	Entry& getStat(int midiChannel, MidiChannelState const& channelState) {
		return stats[midiChannel][channelState.useDrums ? 128 : channelState.program];
	}

public:
	void load(std::vector<BASS_MIDI_EVENT>& events) {
		MidiState midiState;
		for (auto& event : events) {
			midiState.processEvent(event);
			if (event.event != MIDI_EVENT_NOTE) {
				continue;
			}

			int velocity = MidiState::getVelocity(event);
			if (velocity == 0 || velocity == 255) {
				continue;
			}

			MidiChannelState& channelState = midiState.getChannel(event.chan);
			getStat(event.chan, channelState).addNote(channelState.getNoteVolume(velocity));
		}
	}

	double getMidiScore(int midiChannel, MidiChannelState const& channelState) {
		return getStat(midiChannel, channelState).getScore();
	}

	PresetScores getPresetsScores(InstrumentBase& base, std::array<double, 128>& similarities) {
		PresetScores scores;
		for (int i = 0; i < 16; i++) {
			auto& channelStats = stats[i];
			for (auto& [program, entry] : channelStats) {
				// skip drums
				if (program == 128) {
					continue;
				}

				auto preset = base.getGmPreset(program);
				if (preset) {
					scores.addScore(PresetSetup(preset.value()), entry.notes, similarities[program]);
				}
			}
		}
		return scores;
	}

	std::vector<int> getPrograms(InstrumentBase& base, PresetSetup& setup) {
		std::vector<int> results;

		for (int i = 0; i < 16; i++) {
			auto& channelStats = stats[i];
			for (auto& [program, _] : channelStats) {
				// skip drums
				if (program == 128) {
					continue;
				}

				auto preset = base.getGmPreset(program);
				if (preset && PresetSetup(preset.value()).getIndex() == setup.getIndex()) {
					results.push_back(program);
				}
			}
		}

		return results;
	}
};