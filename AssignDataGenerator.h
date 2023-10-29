#pragma once
#include "commons.h"
#include "MidiState.h"
#include "AssignData.h"
#include "InstrumentBase.h"
#include "Preset.h"
#include "SoundWaveProfile.h"

class AssignDataGenerator {
private:
	static constexpr double NOTES_BELOW_RANGE_PUNISHMENT = 2;
	static constexpr double NOTE_INTERRUPTING_PUNISHMENT = 4;
	static constexpr bool USE_VRC6 = true;

	static std::vector<int> getSortedIndices(std::array<double, 16> const& array) {
		std::vector<int> indices(16);
		std::iota(indices.begin(), indices.end(), 0);
		std::sort(indices.begin(), indices.end(), [&](int a, int b) { return array[a] > array[b]; });
		return indices;
	}

	static double getAudioSimilarity(Preset const& preset, AssignChannelData const& channelData) {
		return SoundWaveProfile(preset.channel, preset.duty).getSimilarity(SoundWaveProfile(channelData.getChannel(), channelData.duty));
	}

	int getChordCount(int midiChannel, int noteCount) const {
		auto it = chords[midiChannel].find(noteCount);
		if (it == chords[midiChannel].end()) {
			return 0;
		}
		else {
			return it->second;
		}
	}

	int countPlayedNotes(AssignData const& assignData, int chan) const {
		int count = 0;
		for (int i = 1; i <= assignData.getNesData(chan).nesChannels.size(); i++) {
			count += getChordCount(chan, i);
		}
		return count;
	}

	double getChannelChordMultiplier(AssignData const& assignData, int chan) const {
		int count = 0;
		double multiplier = 0;
		for (int i = 1; i <= assignData.getNesData(chan).nesChannels.size(); i++) {
			int chordCount = getChordCount(chan, i);
			count += chordCount;
			multiplier += chordCount / double(i * 3.0 - 2);
		}
		return multiplier / count;
	}

	double getPlayedNotesRatio(AssignData const& assignData, int chan) const {
		return notes[chan] == 0 ? 0 : countPlayedNotes(assignData, chan) / double(notes[chan]);
	}

	double getScore(AssignData& assignData) const {
		if (assignData.score) {
			return assignData.score.value();
		}

		assignData.score = 0;
		for (int chan = 0; chan < 16; chan++) {
			const AssignChannelData& nesData = assignData.getNesData(chan);
			if (nesData.nesChannels.empty()) {
				continue;
			}

			std::optional<Preset> preset = instrumentBase->getGmPreset(originMidiState.getChannel(chan).program);
			if (!preset) {
				continue;
			}

			double playedNotesRatio = getPlayedNotesRatio(assignData, chan);
			double notesScore = notes[chan] * playedNotesRatio;

			double outOfRangePunishment = 0;
			for (int i = 0; i < Pattern::CHANNELS; i++) {
				outOfRangePunishment += NOTES_BELOW_RANGE_PUNISHMENT * notesOutOfRange[chan][i] * playedNotesRatio * nesData.getChannelRatio(NesChannel(i));
			}

			double noteInterruptingPunishment = NOTE_INTERRUPTING_PUNISHMENT * countInterruptingNotes(assignData, chan);

			notesScore -= outOfRangePunishment + noteInterruptingPunishment;

			// getAudioSimilarity() checks how much provided nesData is similar to original instrument from the base
			double audioSimilarity = getAudioSimilarity(preset.value(), nesData);
			double channelChordMultiplier = getChannelChordMultiplier(assignData, chan);

			double channelScore = notesScore * avgVolumes[chan] * audioSimilarity * channelChordMultiplier;
			assignData.score.value() += channelScore;
		}

		return assignData.score.value();
	}

	double countInterruptingNotes(AssignData const& assignData, int chan) const {
		double count = 0;
		auto& nesChannels = assignData.getNesData(chan).nesChannels;
		double playedNotesRatio = getPlayedNotesRatio(assignData, chan);
		for (int chan2 = 0; chan2 < 16; chan2++) {
			auto& nesChannels2 = assignData.getNesData(chan2).nesChannels;
			double playedNotesRatio2 = getPlayedNotesRatio(assignData, chan2);
			for (NesChannel nesChannel : nesChannels) {
				if (assignData.isAssigned(chan2, nesChannel)) {
					count += interruptingNotes[chan][chan2] * playedNotesRatio * playedNotesRatio2 / (double(nesChannels.size()) * double(nesChannels2.size()));
				}
			}
		}
		return count;
	}

	static std::vector<AssignChannelData> getMelodicAssignConfigurations(Preset const& preset) {
		std::vector<AssignChannelData> results;

		std::array<Preset::Duty, 4> pulseDuties = { Preset::Duty::ANY, Preset::Duty::PULSE_12, Preset::Duty::PULSE_25, Preset::Duty::PULSE_50 };
		for (Preset::Duty duty : pulseDuties) {
			if (USE_VRC6) {
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE4 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE3 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE3, NesChannel::PULSE4 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE2, NesChannel::PULSE4 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE2, NesChannel::PULSE3 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE2, NesChannel::PULSE3, NesChannel::PULSE4 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE4 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE3 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE3, NesChannel::PULSE4 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE2, NesChannel::PULSE4 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE2, NesChannel::PULSE3 }));
				results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE2, NesChannel::PULSE3, NesChannel::PULSE4 }));
			}
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE2 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE2 }));
		}

		// cannot use triangle to play i.e. piano, because triangle does not decay
		if (preset.instrument && preset.instrument->volumeMacro && preset.instrument->volumeMacro->releasePosition >= 0) {
			results.push_back(AssignChannelData(Preset::Duty::ANY, { NesChannel::TRIANGLE }));
		}
		if (USE_VRC6) {
			results.push_back(AssignChannelData(Preset::Duty::ANY, { NesChannel::SAWTOOTH }));
		}

		return results;
	}

	static std::vector<AssignChannelData> getAssignConfigurations(Preset const& preset) {
		switch (preset.channel) {
		case Preset::Channel::PULSE:
		case Preset::Channel::TRIANGLE:
		case Preset::Channel::SAWTOOTH:
			return getMelodicAssignConfigurations(preset);
		case Preset::Channel::NOISE:
			return { AssignChannelData(Preset::Duty::NOISE_NORMAL, { NesChannel::NOISE }) };
		case Preset::Channel::DPCM:
			return { AssignChannelData(Preset::Duty::ANY, { NesChannel::DPCM }) };
		default:
			return {};
		}
	}

	AssignData tryUnassignSomeChannels(AssignData& originalData, std::unordered_set<AssignData>& processed) const {
		if (processed.find(originalData) != processed.end()) {
			return originalData;
		}
		processed.insert(originalData);

		AssignData bestData = originalData;

		for (int chan = 0; chan < 16; chan++) {
			for (NesChannel nesChannel : originalData.getNesData(chan).nesChannels) {
				AssignData newData = originalData;
				newData.unassign(chan, nesChannel);

				if (getScore(newData) > getScore(originalData)) {
					newData = tryUnassignSomeChannels(newData, processed);
				}

				if (getScore(newData) > getScore(bestData)) {
					bestData = newData;
				}
			}
		}

		return bestData;
	}

	AssignData tryAssignChannel(AssignData const& originalData, int midiChannel, Preset const& preset, std::unordered_set<AssignData>& processed) const {
		AssignData bestData = originalData;

		for (AssignChannelData const& configuration : getAssignConfigurations(preset)) {
			AssignData newData = originalData;
			newData.assign(midiChannel, configuration);

			if (getScore(newData) > getScore(bestData)) {
				bestData = newData;
			}

			newData = tryUnassignSomeChannels(newData, processed);
			if (getScore(newData) > getScore(bestData)) {
				bestData = newData;
			}
		}

		return bestData;
	}

public:
	MidiState originMidiState;
	double originSeconds;
	InstrumentBase* instrumentBase;

	// how many notes appeared for every channel
	std::array<int, 16> notes{};

	// [midi channel][nes channel]
	std::array<std::array<int, Pattern::CHANNELS>, 16> notesOutOfRange{};

	// sum of volumes for notes above
	std::array<double, 16> volumeSum{};

	// volumeSum / notes
	std::array<double, 16> avgVolumes{};

	// how many chords with specific note count for each channel
	std::array<std::unordered_map<int, int>, 16> chords{};

	// [interrupting chan][interruped chan]
	std::array<std::array<int, 16>, 16> interruptingNotes{};

	explicit AssignDataGenerator(MidiState const& midiState, double seconds, InstrumentBase* instrumentBase) :
		originMidiState(midiState), originSeconds(seconds), instrumentBase(instrumentBase) {}

	void addNote(MidiEvent const& event, MidiState& currentState) {
		// drums are processed in InstrumentSelector::getNoteTriggers
		if (currentState.getChannel(event.chan).useDrums) {
			return;
		}

		notes[event.chan]++;
		for (int i = 0; i < Pattern::CHANNELS; i++) {
			if (!Note::isInPlayableRange(NesChannel(i), event.key)) {
				notesOutOfRange[event.chan][i]++;
			}
		}

		// added small amount to fix i.e. the strings in DOOM - E1M2
		volumeSum[event.chan] += currentState.getChannel(event.chan).getNoteVolume(event.velocity) + 0.01;

		avgVolumes[event.chan] = volumeSum[event.chan] / notes[event.chan];
		auto noteCount = int(currentState.getChannel(event.chan).noteVelocities.size());
		chords[event.chan][noteCount]++;

		for (int chan2 = 0; chan2 < 16; chan2++) {
			if (currentState.getChannel(chan2).useDrums) {
				continue;
			}

			for (auto const& [key2, _] : currentState.getChannel(chan2).noteVelocities) {
				if (event.chan == chan2 && event.key == key2) {
					continue;
				}

				if (event.chan != chan2) {
					interruptingNotes[event.chan][chan2]++; // TODO: add 1/30 seconds tolerancy
				}
			}
		}
	}

	AssignData generateAssignData(std::optional<AssignData> const& previousData) const {
		std::unordered_set<AssignData> processed;
		std::vector<int> channelOrder = getSortedIndices(volumeSum);

		AssignData bestData = previousData.value_or(AssignData(originSeconds));
		bestData.seconds = originSeconds;
		bestData.score.reset();
		bool iterate = true;
		while (iterate) {
			iterate = false;
			for (int chan : channelOrder) {
				if (notes[chan] <= 0) {
					continue;
				}

				const MidiChannelState& midiChannelState = originMidiState.getChannel(chan);
				std::optional<Preset> preset = instrumentBase->getGmPreset(midiChannelState.program);
				if (!preset) {
					continue;
				}

				AssignData newData = tryAssignChannel(bestData, chan, preset.value(), processed);
				if (getScore(newData) > getScore(bestData)) {
					bestData = newData;
					iterate = true;
				}
			}
		}

		return bestData;
	}
};