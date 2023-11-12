#pragma once
#include "commons.h"
#include "MidiState.h"
#include "AssignData.h"
#include "InstrumentBase.h"
#include "Preset.h"
#include "SoundWaveProfile.h"
#include "MidiChannelNotesData.h"
#include "NesHeightVolumeController.h"

class AssignDataGenerator {
private:

	class ScoredAssignData {
	public:
		AssignData data;
		double score;

		ScoredAssignData(AssignData const& data, double score) : data(data), score(score) {}
	};

	static constexpr double NOTES_BELOW_RANGE_PUNISHMENT = 2;
	static constexpr double NOTE_INTERRUPTING_PUNISHMENT = 2;
	static constexpr double NOTE_REPEATING_PUNISHMENT = 0.75;
	static constexpr bool USE_VRC6 = true;
	static constexpr int SEARCH_DEPTH = 7;

	MidiState originMidiState;
	int eventIndex;
	InstrumentBase* instrumentBase;
	std::array<MidiChannelNotesData, MidiState::CHANNEL_COUNT> midiData;
	static std::array<std::vector<AssignChannelData>, 1024> assignConfigurationLookup; // index is 10 bits

	static double getAudioSimilarity(Preset const& preset, AssignChannelData const& channelData) {
		return SoundWaveProfile(preset.channel, preset.duty).getSimilarity(SoundWaveProfile(channelData.getChannel(), channelData.duty));
	}

	double calculateScore(AssignData const& assignData) const {
		double score = 0;

		for (int chan = 0; chan < MidiState::CHANNEL_COUNT; chan++) {
			const AssignChannelData& nesData = assignData.getNesData(chan);
			if (nesData.nesChannels.none()) {
				continue;
			}

			std::optional<Preset> preset = instrumentBase->getGmPreset(originMidiState.getChannel(chan).program);
			if (!preset) {
				continue;
			}

			const MidiChannelNotesData& channelData = midiData[chan];
			auto assignedChannelCount = int(nesData.nesChannels.count());
			double notesScore = channelData.playedNotes[assignedChannelCount];

			double outOfRangePunishment = 0;
			double playedNotesRatio = channelData.getPlayedNotesRatio(assignedChannelCount);
			for (int i = 0; i < channelData.notesOutOfRange.size(); i++) {
				outOfRangePunishment += NOTES_BELOW_RANGE_PUNISHMENT * channelData.notesOutOfRange[i] * playedNotesRatio * nesData.getChannelRatio(NesChannel(i));
			}

			double noteInterruptingPunishment = NOTE_INTERRUPTING_PUNISHMENT * countInterruptingNotes(assignData, chan);

			notesScore -= outOfRangePunishment + noteInterruptingPunishment;

			// getAudioSimilarity() checks how much provided nesData is similar to original instrument from the base
			double audioSimilarity = getAudioSimilarity(preset.value(), nesData);
			double channelChordMultiplier = channelData.chordScoreMultiplier[assignedChannelCount];

			double channelScore = notesScore * channelData.getAverageVolume() * audioSimilarity * channelChordMultiplier;
			score += channelScore;
		}

		return score;
	}

	double countInterruptingNotes(AssignData const& assignData, int chan) const {
		double count = 0;

		auto& nesChannels = assignData.getNesData(chan).nesChannels;

		for (int chan2 = 0; chan2 < MidiState::CHANNEL_COUNT; chan2++) {
			auto& nesChannels2 = assignData.getNesData(chan2).nesChannels;
			if (nesChannels2.none()) {
				continue;
			}

			auto commonChannels = nesChannels & nesChannels2;
			if (commonChannels.none()) {
				continue;
			}

			double playedRatioPerChannel2 = midiData[chan2].getPlayedNotesRatioPerAssignedChannel(nesChannels2);

			count += midiData[chan].interruptingNotes[chan2] * playedRatioPerChannel2 * double(commonChannels.count());
		}

		// Microsoft please fix C28020
		double playedRatioPerChannel = midiData[chan].getPlayedNotesRatioPerAssignedChannel(nesChannels);

		return count * playedRatioPerChannel;
	}

	static int getAssignConfigurationId(Preset::Duty duty, Preset::Channel channel, bool nonDecaying, int maxChannelCount) {
		return int(duty) // 3b
			+ (int(channel) << 3) // 3b
			+ (int(nonDecaying) << 6) // 1b
			+ (min(4, maxChannelCount) << 7); // 3b
	}

	static std::vector<AssignChannelData> getMelodicAssignConfigurations(Preset::Duty duty, bool nonDecaying, int maxChannelCount) {
		using enum Preset::Duty;
		using enum NesChannel;
		std::vector<AssignChannelData> results;

		if (maxChannelCount <= 0) {
			return results;
		}

		results.push_back(AssignChannelData(duty, { PULSE2 }));
		results.push_back(AssignChannelData(duty, { PULSE1 }));
		if (maxChannelCount >= 2) {
			results.push_back(AssignChannelData(duty, { PULSE1, PULSE2 }));
		}

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

			results.push_back(AssignChannelData(UNSPECIFIED, { SAWTOOTH }));
		}

		// cannot use triangle to play i.e. piano, because triangle does not decay
		if (nonDecaying) {
			results.push_back(AssignChannelData(UNSPECIFIED, { TRIANGLE }));
		}

		return results;
	}

	static std::vector<AssignChannelData> getAssignConfigurations(Preset::Duty duty, Preset::Channel channel, bool nonDecaying, int maxChannelCount) {
		switch (channel) {
		case Preset::Channel::PULSE:
		case Preset::Channel::TRIANGLE:
		case Preset::Channel::SAWTOOTH:
			return getMelodicAssignConfigurations(duty, nonDecaying, maxChannelCount);
		case Preset::Channel::NOISE:
			return { AssignChannelData(duty, { NesChannel::NOISE }) };
		case Preset::Channel::DPCM:
			return { AssignChannelData(duty, { NesChannel::DPCM }) };
		default:
			return {};
		}
	}

	const std::vector<AssignChannelData>& getAssignConfigurations(Preset const& preset, int maxChannelCount) const {
		return assignConfigurationLookup[getAssignConfigurationId(preset.duty, preset.channel, preset.isNonDecaying(), maxChannelCount)];
	}

	void replaceIfBetter(ScoredAssignData& scoredData, AssignData const& assignData, bool includeDutyDiversity) const {
		double newScore = calculateScore(assignData);
		if (includeDutyDiversity) {
			newScore *= getDutyDiversityScore(assignData);
		}

		if (newScore > scoredData.score) {
			scoredData.data = assignData;
			scoredData.score = newScore;
		}
	}

	void tryUnassignSomeChannels(ScoredAssignData& bestData, AssignData const& workingData, int depth, std::unordered_set<AssignData>& checked) const {
		if (depth == 0) {
			return;
		}
		depth--;

		for (int chan = 0; chan < MidiState::CHANNEL_COUNT; chan++) {
			if (midiData[chan].notes <= 0) {
				continue;
			}

			workingData.getNesData(chan).forEachAssignedChannel([&bestData, &workingData, depth, &checked, chan, this](NesChannel nesChannel) {
				AssignData newData = workingData.unassign(chan, nesChannel);

				if (!checked.insert(newData).second) {
					return;
				}

				replaceIfBetter(bestData, newData, false);

				tryAssignSomeChannels(bestData, newData, depth, checked);
			});
		}

		tryAssignSomeChannels(bestData, workingData, depth, checked);
	}

	void tryAssignSomeChannels(ScoredAssignData& bestData, AssignData const& workingData, int depth, std::unordered_set<AssignData>& checked) const {
		if (depth == 0) {
			return;
		}
		depth--;

		for (int chan = 0; chan < MidiState::CHANNEL_COUNT; chan++) {
			if (midiData[chan].notes <= 0) {
				continue;
			}

			std::optional<Preset> preset = instrumentBase->getGmPreset(originMidiState.getChannel(chan).program);
			if (!preset) {
				continue;
			}

			for (AssignChannelData const& configuration : getAssignConfigurations(preset.value(), int(midiData[chan].chords.size()))) {
				AssignData newData = workingData.assign(chan, configuration);

				if (!checked.insert(newData).second) {
					continue;
				}

				replaceIfBetter(bestData, newData, false);

				tryUnassignSomeChannels(bestData, newData, depth, checked);
			}
		}

		tryUnassignSomeChannels(bestData, workingData, depth, checked);
	}

	static int countPulseDuty(AssignData const& assignData, Preset::Duty duty) {
		int count = 0;
		for (auto& nesData : assignData.nesData) {
			if (nesData.duty == duty && nesData.getChannel() == Preset::Channel::PULSE) {
				count += int(nesData.nesChannels.count());
			}
		}
		return count;
	}

	static double getDutyDiversityScore(AssignData const& assignData) {
		using enum Preset::Duty;

		std::array<double, 3> counts{};
		counts[0] = countPulseDuty(assignData, PULSE_12);
		counts[1] = countPulseDuty(assignData, PULSE_25);
		counts[2] = countPulseDuty(assignData, PULSE_50);

		std::ranges::sort(counts);

		return 3 + counts[0] * 2 + counts[1];
	}

	void diverseDuty(ScoredAssignData& bestData, AssignData const& workingData, int chan = 0) const {
		using enum Preset::Duty;
		if (chan >= MidiState::CHANNEL_COUNT) {
			return;
		}

		if (const AssignChannelData& nesData = workingData.getNesData(chan); 
			nesData.nesChannels.none() || nesData.getChannel() != Preset::Channel::PULSE || nesData.duty == UNSPECIFIED) {
			return;
		}

		for (Preset::Duty duty : {PULSE_12, PULSE_25, PULSE_50}) {
			AssignData newData = workingData.setDuty(chan, duty);
			replaceIfBetter(bestData, newData, true);
			diverseDuty(bestData, newData, chan + 1);
		}
	}

	static void initLookupsDeeper(Preset::Duty duty, Preset::Channel channel, bool nonDecaying) {
		for (int maxChannelCount = 0; maxChannelCount <= 4; maxChannelCount++) {
			assignConfigurationLookup[getAssignConfigurationId(duty, channel, nonDecaying, maxChannelCount)] = getAssignConfigurations(duty, channel, nonDecaying, maxChannelCount);
		}
	}

public:
	static void initLookups() {
		for (int dutyI = 0; dutyI < int(Preset::Duty::DUTY_COUNT); dutyI++) {
			auto duty = Preset::Duty(dutyI);

			for (int channelI = 0; channelI < int(Preset::Channel::CHANNEL_COUNT); channelI++) {
				auto channel = Preset::Channel(channelI);

				for (int nonDecayingI = 0; nonDecayingI < 2; nonDecayingI++) {
					auto nonDecaying = bool(nonDecayingI);

					initLookupsDeeper(duty, channel, nonDecaying);
				}
			}
		}
	}

	explicit AssignDataGenerator(MidiState const& midiState, int eventIndex, InstrumentBase* instrumentBase) :
		originMidiState(midiState), eventIndex(eventIndex), instrumentBase(instrumentBase) {}

	void addNote(MidiEvent const& event, MidiState& currentState) {
		// drums are processed in InstrumentSelector::getNoteTriggers
		if (currentState.getChannel(event.chan).useDrums) {
			return;
		}

		MidiChannelNotesData& channelData = midiData[event.chan];
		channelData.notes++;
		for (int i = 0; i < channelData.notesOutOfRange.size(); i++) {
			if (!Note::isInPlayableRange(NesChannel(i), event.key)) {
				channelData.notesOutOfRange[i]++;
			}
		}

		// added small amount to fix i.e. the strings in DOOM - E1M2
		channelData.volumeSum += currentState.getChannel(event.chan).getNoteVolume(event.velocity) * NesHeightVolumeController::getHeightVolumeMultiplier(event.key) + 0.01;

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
		ScoredAssignData bestData(AssignData(eventIndex), 0);
		std::unordered_set<AssignData> checked{};
		checked.insert(bestData.data);

		while (true) {
			double oldScore = bestData.score;
			tryAssignSomeChannels(bestData, bestData.data, SEARCH_DEPTH, checked);
			if (bestData.score <= oldScore) {
				break;
			}
		}

		std::cout << "Diversing pulse duties..." << std::endl;

		bestData.score = 0;
		diverseDuty(bestData, bestData.data);

		return bestData.data;
	}

	bool hasAnyNotes(int chan) const {
		return midiData[chan].notes > 0;
	}

	void setMidiState(MidiState const& midiState) {
		originMidiState = midiState;
	}
};