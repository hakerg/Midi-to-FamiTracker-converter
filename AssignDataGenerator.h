#pragma once
#include "commons.h"
#include "MidiState.h"
#include "AssignData.h"
#include "InstrumentBase.h"
#include "Preset.h"
#include "SoundWaveProfile.h"
#include "MidiChannelNotesData.h"
#include "NesHeightVolumeController.h"

class IndexedAssignData {
public:
	AssignData data;
	int eventIndex;
	std::array<int, MidiState::CHANNEL_COUNT> programs;

	IndexedAssignData(AssignData const& data, int eventIndex, std::array<int, MidiState::CHANNEL_COUNT> const& programs) : data(data), eventIndex(eventIndex), programs(programs) {}
};

class AssignDataGenerator {
private:

	class ScoredAssignData {
	public:
		AssignData data;
		double score;

		ScoredAssignData(AssignData const& data, double score) : data(data), score(score) {}
	};

	static constexpr double NOTES_BELOW_RANGE_PUNISHMENT = 2;
	static constexpr double NOTE_INTERRUPTING_PUNISHMENT = 16;
	static constexpr bool USE_VRC6 = true;
	static constexpr int SEARCH_DEPTH = 8;

	static std::array<std::vector<AssignChannelData>, 1024> assignConfigurationLookup; // index is 10 bits

	std::array<int, MidiState::CHANNEL_COUNT> programs;
	int eventIndex;
	InstrumentBase* instrumentBase;
	std::array<MidiChannelNotesData, MidiState::CHANNEL_COUNT> midiData;
	IndexedAssignData lastData;
	ScoredAssignData bestData{ AssignData({}), 0 };

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

			std::optional<Preset> preset = instrumentBase->getGmPreset(programs[chan]);
			if (!preset) {
				continue;
			}

			const MidiChannelNotesData& channelData = midiData[chan];
			if (channelData.notes <= 0) {
				continue;
			}

			auto assignedChannelCount = int(nesData.nesChannels.count());
			double notesScore = channelData.playedNotes[assignedChannelCount];

			double outOfRangePunishment = 0;
			double playedNotesRatio = channelData.getPlayedNotesRatio(assignedChannelCount);
			for (int i = 0; i < channelData.notesOutOfRange.size(); i++) {
				outOfRangePunishment += NOTES_BELOW_RANGE_PUNISHMENT * channelData.notesOutOfRange[i] * playedNotesRatio * nesData.getChannelRatio(NesChannel(i));
			}

			double noteInterruptingPunishment = 0;
			if (preset->order == Preset::Order::MELODIC) { // otherwise we assume that Preset::Order handles channel priorities and note interrupting
				noteInterruptingPunishment = NOTE_INTERRUPTING_PUNISHMENT * countInterruptingNotes(assignData, chan);
			}

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
			if (midiData[chan2].notes <= 0) {
				continue;
			}

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

		double playedRatioPerChannel = midiData[chan].getPlayedNotesRatioPerAssignedChannel(nesChannels);

		return count * playedRatioPerChannel;
	}

	static int getAssignConfigurationId(Preset::Duty duty, Preset::Channel channel, bool isSimpleLoop, int maxChannelCount) {
		return int(duty) // 3b
			+ (int(channel) << 3) // 3b
			+ (int(isSimpleLoop) << 6) // 1b
			+ (min(4, maxChannelCount) << 7); // 3b
	}

	static std::vector<AssignChannelData> getMelodicAssignConfigurations(Preset::Duty duty, bool isSimpleLoop, int maxChannelCount) {
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
		if (isSimpleLoop) {
			results.push_back(AssignChannelData(UNSPECIFIED, { TRIANGLE }));
		}

		return results;
	}

	static std::vector<AssignChannelData> getAssignConfigurations(Preset::Duty duty, Preset::Channel channel, bool isSimpleLoop, int maxChannelCount) {
		switch (channel) {
		case Preset::Channel::PULSE:
		case Preset::Channel::TRIANGLE:
		case Preset::Channel::SAWTOOTH:
			return getMelodicAssignConfigurations(duty, isSimpleLoop, maxChannelCount);
		case Preset::Channel::NOISE:
			return { AssignChannelData(duty, { NesChannel::NOISE }) };
		case Preset::Channel::DPCM:
			return { AssignChannelData(duty, { NesChannel::DPCM }) };
		default:
			return {};
		}
	}

	const std::vector<AssignChannelData>& getAssignConfigurations(Preset const& preset, int maxChannelCount) const {
		return assignConfigurationLookup[getAssignConfigurationId(preset.duty, preset.channel, preset.isSimpleLoop(), maxChannelCount)];
	}

	void replaceIfBetter(AssignData const& assignData, bool includeDutyDiversity) {
		double newScore = calculateScore(assignData);
		if (includeDutyDiversity) {
			newScore *= getDutyDiversityScore(assignData);
		}

		if (newScore > bestData.score) {
			bestData.data = assignData;
			bestData.score = newScore;
		}
	}

	void tryUnassignSomeChannels(AssignData const& workingData, int depth, std::unordered_set<AssignData>& checked) {

		if (depth == 0) {
			return;
		}
		depth--;

		for (int chan = 0; chan < MidiState::CHANNEL_COUNT; chan++) {
			if (midiData[chan].notes <= 0) {
				continue;
			}

			workingData.getNesData(chan).forEachAssignedChannel([&workingData, depth, &checked, chan, this](NesChannel nesChannel) {
				AssignData newData = workingData;
				newData.unassign(chan, nesChannel);

				if (!checked.insert(newData).second) {
					return;
				}

				replaceIfBetter(newData, false);

				tryAssignSomeChannels(newData, depth, checked);
			});
		}

		tryAssignSomeChannels(workingData, depth, checked);
	}

	void tryAssignSomeChannels(AssignData const& workingData, int depth, std::unordered_set<AssignData>& checked) {

		if (depth == 0) {
			return;
		}
		depth--;

		for (int chan = 0; chan < MidiState::CHANNEL_COUNT; chan++) {
			if (midiData[chan].notes <= 0) {
				continue;
			}

			std::optional<Preset> preset = instrumentBase->getGmPreset(programs[chan]);
			if (!preset) {
				continue;
			}

			for (AssignChannelData const& configuration : getAssignConfigurations(preset.value(), int(midiData[chan].noteCountAtNotesOn.size()))) {
				AssignData newData = workingData;
				newData.assign(chan, configuration);

				if (!checked.insert(newData).second) {
					continue;
				}

				replaceIfBetter(newData, false);

				tryUnassignSomeChannels(newData, depth, checked);
			}
		}

		tryUnassignSomeChannels(workingData, depth, checked);
	}

	static double getDutyDiversityScore(AssignData const& assignData) {
		using enum Preset::Duty;

		std::array<double, 3> counts{};
		counts[0] = assignData.countPulseDuty(PULSE_12);
		counts[1] = assignData.countPulseDuty(PULSE_25);
		counts[2] = assignData.countPulseDuty(PULSE_50);

		std::ranges::sort(counts);

		return 3 + counts[0] * 2 + counts[1];
	}

	void diverseDuty(AssignData const& workingData, int chan = 0) {
		using enum Preset::Duty;
		if (chan >= MidiState::CHANNEL_COUNT) {
			return;
		}

		if (const AssignChannelData& nesData = workingData.getNesData(chan); 
			nesData.nesChannels.none() || nesData.getChannel() != Preset::Channel::PULSE || nesData.duty == UNSPECIFIED) {
			return;
		}

		for (Preset::Duty duty : {PULSE_12, PULSE_25, PULSE_50}) {
			AssignData newData = workingData;
			newData.setDuty(chan, duty);
			replaceIfBetter(newData, true);
			diverseDuty(newData, chan + 1);
		}
	}

	void calculateMidiData() {
		for (auto& channelData : midiData) {
			channelData.calculate();
		}
	}

	AssignData getCleanInitData() const {
		AssignData ret = lastData.data;
		for (int i = 0; i < MidiState::CHANNEL_COUNT; i++) {
			if (midiData[i].notes <= 0 || programs[i] != lastData.programs[i]) {
				ret.reset(i);
			}
		}
		return ret;
	}

	static std::array<int, MidiState::CHANNEL_COUNT> getPrograms(MidiState const& midiState) {
		std::array<int, MidiState::CHANNEL_COUNT> programs{};
		for (int i = 0; i < programs.size(); i++) {
			programs[i] = midiState.getChannel(i).program;
		}
		return programs;
	}

	static void initLookupsDeeper(Preset::Duty duty, Preset::Channel channel, bool isSimpleLoop) {
		for (int maxChannelCount = 0; maxChannelCount <= 4; maxChannelCount++) {
			assignConfigurationLookup[getAssignConfigurationId(duty, channel, isSimpleLoop, maxChannelCount)] = getAssignConfigurations(duty, channel, isSimpleLoop, maxChannelCount);
		}
	}

public:
	static void initLookups() {
		for (int dutyI = 0; dutyI < int(Preset::Duty::DUTY_COUNT); dutyI++) {
			auto duty = Preset::Duty(dutyI);

			for (int channelI = 0; channelI < int(Preset::Channel::CHANNEL_COUNT); channelI++) {
				auto channel = Preset::Channel(channelI);

				for (int isSimpleLoopI = 0; isSimpleLoopI < 2; isSimpleLoopI++) {
					auto isSimpleLoop = bool(isSimpleLoopI);

					initLookupsDeeper(duty, channel, isSimpleLoop);
				}
			}
		}
	}

	explicit AssignDataGenerator(MidiState const& midiState, int eventIndex, InstrumentBase* instrumentBase, IndexedAssignData const& lastData) :
		programs(getPrograms(midiState)), eventIndex(eventIndex), instrumentBase(instrumentBase), lastData(lastData) {}

	void addNote(MidiEvent const& event, MidiState& currentState) {
		midiData[event.chan].addNote(event, currentState);
	}

	IndexedAssignData generateAssignData() {
		calculateMidiData();
		bestData.data = getCleanInitData();
		bestData.score = calculateScore(bestData.data);

		std::unordered_set<AssignData> checked{ bestData.data };

		while (true) {
			double oldScore = bestData.score;
			tryAssignSomeChannels(bestData.data, SEARCH_DEPTH, checked);
			if (bestData.score <= oldScore) {
				break;
			}
		}

		std::cout << "Diversing pulse duties..." << std::endl;

		bestData.score = 0;
		diverseDuty(bestData.data);

		return IndexedAssignData(bestData.data, eventIndex, programs);
	}

	bool hasAnyNotes(int chan) const {
		return midiData[chan].notes > 0;
	}

	void setProgram(int chan, int program) {
		programs[chan] = program;
	}
};