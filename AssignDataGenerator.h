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
	static constexpr double TONE_OVERLAP_PUNISHMENT = 0.3;

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
			notesScore -= NOTES_BELOW_RANGE_PUNISHMENT * notesBelowNesRange[chan] * playedNotesRatio * nesData.getPoorFreqRangeChannelRatio();
			notesScore -= NOTE_INTERRUPTING_PUNISHMENT * countInterruptingNotes(assignData, chan);
			notesScore -= TONE_OVERLAP_PUNISHMENT * countOverlappingNotes(assignData, chan);

			// getAudioSimilarity() checks how much provided nesData is similar to original instrument from the base
			assignData.score.value() += notesScore * avgVolumes[chan] * getAudioSimilarity(preset.value(), nesData);
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

	double countOverlappingNotes(AssignData const& assignData, int chan) const {
		double count = 0;
		double playedNotesRatio = getPlayedNotesRatio(assignData, chan);
		for (int chan2 = 0; chan2 < 16; chan2++) {
			double playedNotesRatio2 = getPlayedNotesRatio(assignData, chan2);
			count += soundOverlappingNotes[chan][chan2] * playedNotesRatio * playedNotesRatio2;
		}
		return count;
	}

	static std::vector<AssignChannelData> getMelodicAssignConfigurations(Preset const& preset) {
		std::vector<AssignChannelData> results;

		std::array<Preset::Duty, 4> pulseDuties = { Preset::Duty::ANY, Preset::Duty::PULSE_12, Preset::Duty::PULSE_25, Preset::Duty::PULSE_50 };
		for (Preset::Duty duty : pulseDuties) {
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE4 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE3 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE3, NesChannel::PULSE4 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE2 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE2, NesChannel::PULSE4 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE2, NesChannel::PULSE3 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE2, NesChannel::PULSE3, NesChannel::PULSE4 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE4 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE3 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE3, NesChannel::PULSE4 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE2 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE2, NesChannel::PULSE4 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE2, NesChannel::PULSE3 }));
			results.push_back(AssignChannelData(duty, { NesChannel::PULSE1, NesChannel::PULSE2, NesChannel::PULSE3, NesChannel::PULSE4 }));
		}

		// cannot use triangle to play i.e. piano, because triangle does not decay
		if (preset.instrument && preset.instrument->volumeMacro && preset.instrument->volumeMacro->releasePosition >= 0) {
			results.push_back(AssignChannelData(Preset::Duty::ANY, { NesChannel::TRIANGLE }));
		}
		results.push_back(AssignChannelData(Preset::Duty::ANY, { NesChannel::SAWTOOTH }));

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

	// count of notes playable only in VRC6 channels
	std::array<int, 16> notesBelowNesRange{};

	// sum of volumes for notes above
	std::array<double, 16> volumeSum{};

	// volumeSum / notes
	std::array<double, 16> avgVolumes{};

	// how many chords with specific note count for each channel
	std::array<std::unordered_map<int, int>, 16> chords{};

	// [interrupting chan][interruped chan]
	std::array<std::array<int, 16>, 16> interruptingNotes{};

	// [overlapping chan][overlapped chan]
	std::array<std::array<int, 16>, 16> soundOverlappingNotes{};

	explicit AssignDataGenerator(MidiState const& midiState, double seconds, InstrumentBase* instrumentBase) :
		originMidiState(midiState), originSeconds(seconds), instrumentBase(instrumentBase) {}

	void addNote(int chan, int key, int velocity, MidiState& currentState) {
		// drums are processed in InstrumentSelector::getNoteTriggers
		if (currentState.getChannel(chan).useDrums) {
			return;
		}

		notes[chan]++;
		// needed to lower MIN_PLAYABLE_KEY, because triangle is played octave higher anyway
		if (key < Note::MIN_PLAYABLE_KEY - 12) {
			notesBelowNesRange[chan]++;
		}
		volumeSum[chan] += currentState.getChannel(chan).getNoteVolume(velocity);
		avgVolumes[chan] = volumeSum[chan] / notes[chan];
		auto noteCount = int(currentState.getChannel(chan).noteVelocities.size());
		chords[chan][noteCount]++;

		for (int chan2 = 0; chan2 < 16; chan2++) {
			if (currentState.getChannel(chan2).useDrums) {
				continue;
			}

			for (auto const& [key2, _] : currentState.getChannel(chan2).noteVelocities) {
				if (chan == chan2 && key == key2) {
					continue;
				}

				if (chan != chan2) {
					interruptingNotes[chan][chan2]++;
				}

				// check if two keys are the same tone (may be different octave)
				if (key % 12 == key2 % 12) {
					soundOverlappingNotes[chan][chan2]++;
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