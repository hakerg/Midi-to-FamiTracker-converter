#pragma once
#include "commons.h"
#include "MidiState.h"
#include "AssignData.h"
#include "InstrumentBase.h"
#include "Preset.h"
#include "SoundWaveProfile.h"
#include "MidiChannelNotesData.h"

class AssignDataGenerator {
private:
	static constexpr double NOTES_BELOW_RANGE_PUNISHMENT = 2;
	static constexpr double NOTE_INTERRUPTING_PUNISHMENT = 2;
	static constexpr double NOTE_REPEATING_PUNISHMENT = 0.75;
	static constexpr bool USE_VRC6 = true;
	static constexpr int SEARCH_DEPTH = 4;
	// TODO: punishment for too high notes
	// TODO: punishment for triangle volume difference (DOOM E2M4)

	MidiState originMidiState;
	int eventIndex;
	InstrumentBase* instrumentBase;
	std::array<MidiChannelNotesData, MidiState::CHANNEL_COUNT> midiData;

	static double getAudioSimilarity(Preset const& preset, AssignChannelData const& channelData) {
		return SoundWaveProfile(preset.channel, preset.duty).getSimilarity(SoundWaveProfile(channelData.getChannel(), channelData.duty));
	}

	bool hasPulseDuty(AssignData const& assignData, Preset::Duty duty) const {
		for (auto& nesData : assignData.nesData) {
			using enum NesChannel;
			if (nesData.nesChannels.empty() || nesData.duty != duty) {
				continue;
			}

			if (nesData.isAssigned(PULSE1) ||
				nesData.isAssigned(PULSE2) ||
				nesData.isAssigned(PULSE3) ||
				nesData.isAssigned(PULSE4)) {

				return true;
			}
		}
		return false;
	}

	int getDutyDiversityScore(AssignData const& assignData) const {
		using enum Preset::Duty;

		int diversity = 1;

		for (Preset::Duty duty : {PULSE_12, PULSE_25, PULSE_50}) {
			if (hasPulseDuty(assignData, duty)) {
				diversity++;
			}
		}

		return diversity;
	}

	double calculateScore(AssignData const& assignData) const {
		double score = 0;

		for (int chan = 0; chan < MidiState::CHANNEL_COUNT; chan++) {
			const AssignChannelData& nesData = assignData.getNesData(chan);
			if (nesData.nesChannels.empty()) {
				continue;
			}

			std::optional<Preset> preset = instrumentBase->getGmPreset(originMidiState.getChannel(chan).program);
			if (!preset) {
				continue;
			}

			const MidiChannelNotesData& channelData = midiData[chan];
			auto assignedChannelCount = int(nesData.nesChannels.size());
			double notesScore = channelData.playedNotes[assignedChannelCount];

			double outOfRangePunishment = 0;
			double playedNotesRatio = channelData.getPlayedNotesRatio(assignedChannelCount);
			for (int i = 0; i < Pattern::CHANNELS; i++) {
				outOfRangePunishment += NOTES_BELOW_RANGE_PUNISHMENT * channelData.notesOutOfRange[i] * playedNotesRatio * nesData.getChannelRatio(NesChannel(i));
			}

			double noteInterruptingPunishment = NOTE_INTERRUPTING_PUNISHMENT * countInterruptingNotes(assignData, chan);
			double noteRepeatingPunishment = NOTE_REPEATING_PUNISHMENT * countRepeatingNotes(assignData, chan);

			notesScore -= outOfRangePunishment + noteInterruptingPunishment + noteRepeatingPunishment;

			// getAudioSimilarity() checks how much provided nesData is similar to original instrument from the base
			double audioSimilarity = getAudioSimilarity(preset.value(), nesData);
			double channelChordMultiplier = channelData.chordScoreMultiplier[assignedChannelCount];

			double channelScore = notesScore * channelData.getAverageVolume() * audioSimilarity * channelChordMultiplier;
			score += channelScore;
		}

		return score * getDutyDiversityScore(assignData);
	}

	double getScore(AssignData& assignData) const {
		if (assignData.score) {
			return assignData.score.value();
		}
		assignData.score = calculateScore(assignData);
		return assignData.score.value();
	}

	double countInterruptingNotes(AssignData const& assignData, int chan) const {
		double count = 0;

		auto& nesChannels = assignData.getNesData(chan).nesChannels;

		for (int chan2 = 0; chan2 < MidiState::CHANNEL_COUNT; chan2++) {
			auto& nesChannels2 = assignData.getNesData(chan2).nesChannels;
			if (nesChannels2.empty()) {
				continue;
			}

			double playedRatioPerChannel2 = midiData[chan2].getPlayedNotesRatioPerAssignedChannel(int(nesChannels2.size()));

			for (NesChannel nesChannel : nesChannels) {
				if (assignData.isAssigned(chan2, nesChannel)) {
					count += midiData[chan].interruptingNotes[chan2] * playedRatioPerChannel2;
				}
			}
		}

		double playedRatioPerChannel = midiData[chan].getPlayedNotesRatioPerAssignedChannel(int(nesChannels.size()));

		return count * playedRatioPerChannel;
	}

	double countRepeatingNotes(AssignData const& assignData, int chan) const {
		return 0; // TODO: count similar notes on different channels at the same time
	}

	static std::vector<AssignChannelData> getMelodicAssignConfigurations(Preset const& preset, int maxChannelCount) {
		using enum Preset::Duty;
		using enum NesChannel;
		std::vector<AssignChannelData> results;

		if (maxChannelCount == 0) {
			return results;
		}

		static const std::vector<Preset::Duty> unspecifiedDuty = { UNSPECIFIED };
		static const std::vector<Preset::Duty> pulseDuties = { PULSE_12, PULSE_25, PULSE_50 };

		for (const std::vector<Preset::Duty>& duties = (preset.duty == Preset::Duty::UNSPECIFIED ? unspecifiedDuty : pulseDuties); Preset::Duty duty : duties) {
			if (USE_VRC6) {
				results.push_back(AssignChannelData(duty, { PULSE4 }));
				results.push_back(AssignChannelData(duty, { PULSE3 }));
				if (maxChannelCount >= 2) {
					results.push_back(AssignChannelData(duty, { PULSE3, PULSE4 }));
					results.push_back(AssignChannelData(duty, { PULSE2, PULSE4 }));
					results.push_back(AssignChannelData(duty, { PULSE2, PULSE3 }));
					results.push_back(AssignChannelData(duty, { PULSE1, PULSE4 }));
					results.push_back(AssignChannelData(duty, { PULSE1, PULSE3 }));
				}
				if (maxChannelCount >= 3) {
					results.push_back(AssignChannelData(duty, { PULSE2, PULSE3, PULSE4 }));
					results.push_back(AssignChannelData(duty, { PULSE1, PULSE3, PULSE4 }));
					results.push_back(AssignChannelData(duty, { PULSE1, PULSE2, PULSE4 }));
					results.push_back(AssignChannelData(duty, { PULSE1, PULSE2, PULSE3 }));
				}
				if (maxChannelCount >= 4) {
					results.push_back(AssignChannelData(duty, { PULSE1, PULSE2, PULSE3, PULSE4 }));
				}
			}
			results.push_back(AssignChannelData(duty, { PULSE2 }));
			results.push_back(AssignChannelData(duty, { PULSE1 }));
			if (maxChannelCount >= 2) {
				results.push_back(AssignChannelData(duty, { PULSE1, PULSE2 }));
			}
		}

		// cannot use triangle to play i.e. piano, because triangle does not decay
		if (preset.isNonDecaying()) {
			results.push_back(AssignChannelData(UNSPECIFIED, { TRIANGLE }));
		}
		if (USE_VRC6) {
			results.push_back(AssignChannelData(UNSPECIFIED, { SAWTOOTH }));
		}

		return results;
	}

	static std::vector<AssignChannelData> getAssignConfigurations(Preset const& preset, int maxChannelCount) {
		switch (preset.channel) {
		case Preset::Channel::PULSE:
		case Preset::Channel::TRIANGLE:
		case Preset::Channel::SAWTOOTH:
			return getMelodicAssignConfigurations(preset, maxChannelCount);
		case Preset::Channel::NOISE:
			return { AssignChannelData(preset.duty, { NesChannel::NOISE }) };
		case Preset::Channel::DPCM:
			return { AssignChannelData(preset.duty, { NesChannel::DPCM }) };
		default:
			return {};
		}
	}

	AssignData tryUnassignSomeChannels(AssignData const& originalData, int depth, std::unordered_set<AssignData>& assigned, std::unordered_set<AssignData>& unassigned) const {
		if (depth == 0 || unassigned.contains(originalData)) {
			return originalData;
		}
		unassigned.insert(originalData);
		depth--;

		AssignData bestData = originalData;

		for (int chan = 0; chan < MidiState::CHANNEL_COUNT; chan++) {
			for (NesChannel nesChannel : originalData.getNesData(chan).nesChannels) {
				AssignData newData = originalData;
				newData.unassign(chan, nesChannel);

				if (getScore(newData) > getScore(bestData)) {
					bestData = newData;
				}

				newData = tryAssignSomeChannels(newData, depth, assigned, unassigned);
				if (getScore(newData) > getScore(bestData)) {
					bestData = newData;
				}
			}
		}

		return bestData;
	}

	AssignData tryAssignSomeChannels(AssignData const& originalData, int depth, std::unordered_set<AssignData>& assigned, std::unordered_set<AssignData>& unassigned) const {
		if (depth == 0 || assigned.contains(originalData)) {
			return originalData;
		}
		assigned.insert(originalData);
		depth--;

		AssignData bestData = originalData;

		for (int chan = 0; chan < MidiState::CHANNEL_COUNT; chan++) {
			if (midiData[chan].notes <= 0) {
				continue;
			}

			std::optional<Preset> preset = instrumentBase->getGmPreset(originMidiState.getChannel(chan).program);
			if (!preset) {
				continue;
			}

			for (AssignChannelData const& configuration : getAssignConfigurations(preset.value(), int(midiData[chan].chords.size()))) {
				AssignData newData = originalData;
				newData.assign(chan, configuration);

				if (getScore(newData) > getScore(bestData)) {
					bestData = newData;
				}

				newData = tryUnassignSomeChannels(newData, depth, assigned, unassigned);
				if (getScore(newData) > getScore(bestData)) {
					bestData = newData;
				}
			}
		}

		return bestData;
	}

public:
	explicit AssignDataGenerator(MidiState const& midiState, int eventIndex, InstrumentBase* instrumentBase) :
		originMidiState(midiState), eventIndex(eventIndex), instrumentBase(instrumentBase) {}

	void addNote(MidiEvent const& event, MidiState& currentState) {
		// drums are processed in InstrumentSelector::getNoteTriggers
		if (currentState.getChannel(event.chan).useDrums) {
			return;
		}

		MidiChannelNotesData& channelData = midiData[event.chan];
		channelData.notes++;
		for (int i = 0; i < Pattern::CHANNELS; i++) {
			if (!Note::isInPlayableRange(NesChannel(i), event.key)) {
				channelData.notesOutOfRange[i]++;
			}
		}

		// added small amount to fix i.e. the strings in DOOM - E1M2
		channelData.volumeSum += currentState.getChannel(event.chan).getNoteVolume(event.velocity) + 0.01;

		auto noteCount = int(currentState.getChannel(event.chan).noteVelocities.size());
		channelData.chords[noteCount]++;

		for (int chan2 = 0; chan2 < MidiState::CHANNEL_COUNT; chan2++) {
			if (currentState.getChannel(chan2).useDrums) {
				continue;
			}

			for (auto const& [key2, _] : currentState.getChannel(chan2).noteVelocities) {
				if (event.chan == chan2 && event.key == key2) {
					continue;
				}

				if (event.chan != chan2) {
					channelData.interruptingNotes[chan2]++;
				}
			}
		}
	}

	void calculateMidiData() {
		for (auto& channelData : midiData) {
			channelData.calculate();
		}
	}

	AssignData generateAssignData() const {
		std::unordered_set<AssignData> assigned;
		std::unordered_set<AssignData> unassigned;
		AssignData bestData(eventIndex);

		while (true) {
			AssignData newData = tryAssignSomeChannels(bestData, SEARCH_DEPTH, assigned, unassigned);
			if (getScore(newData) > getScore(bestData)) {
				bestData = newData;
			}
			else {
				break;
			}
		}

		return bestData;
	}

	bool hasAnyNotes(int chan) const {
		return midiData[chan].notes > 0;
	}

	void setMidiState(MidiState const& midiState) {
		originMidiState = midiState;
	}
};