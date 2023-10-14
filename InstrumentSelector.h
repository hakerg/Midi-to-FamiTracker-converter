#pragma once
#include "commons.h"
#include "NoteTriggerData.h"
#include "FamiTrackerFile.h"
#include "MidiChannelState.h"
#include "NesState.h"
#include "InstrumentBase.h"
#include "MidiState.h"
#include "PlayScore.h"
#include "PresetStatistics.h"
#include "InstrumentBaseModifier.h"

class InstrumentSelector {
private:
	InstrumentBase base;
	PresetStatistics stats;

	static int getPriorityScore(int basePriorityOrder, int checkedPriorityOrder) {
		if (basePriorityOrder < 0 || checkedPriorityOrder < 0) {
			return 0;
		}
		return basePriorityOrder - checkedPriorityOrder;
	}

	PlayScore calculatePlayScore(NesState const& nesState, NesChannel channel, Preset const& preset, int checkedMidiChannel, int checkedMidiKey, int checkedMidiVelocity,
		double checkedMidiScore) const {

		const PlayingNesNote& note = nesState.getNote(channel);
		const NoteTriggerData& triggerData = note.triggerData;
		int currentRow = nesState.getRow();

		// don't interrupt current note with higher priorityOrder sound (e.g. crash with hi-hat)
		if (nesState.seconds < note.canInterruptSeconds) {
			// it interrupts, check priorities
			if (getPriorityScore(triggerData.preset.drumKeyOrder, preset.drumKeyOrder) < 0) {
				return PlayScore(0, -1);
			}
			if (checkedMidiScore < triggerData.midiScore) {
				return PlayScore(0, -1);
			}
		}

		PlayScore score{};

		if (bool releaseDisabled = (channel == NesChannel::NOISE || channel == NesChannel::DPCM);
			!note.playing || (releaseDisabled && currentRow != NesState::getRow(note.startSeconds))) { // note released

			// bonus for the same midi channel
			score.score[0] = (checkedMidiChannel == note.midiChannel ? 2 : 1);
		}
		else {
			// note start time
			score.score[3] = currentRow - NesState::getRow(note.startSeconds);

			// note height
			score.score[4] = checkedMidiKey - note.midiKey;

			// velocity
			score.score[5] = checkedMidiVelocity - note.midiVelocity;

			// pulse 1-2 & triangle frequency limit
			score.score[6] = (checkedMidiKey < Note::MIN_PLAYABLE_KEY && (channel == NesChannel::PULSE1 || channel == NesChannel::PULSE2 || channel == NesChannel::TRIANGLE) ? 0 : 1);

			// bonus for the same midi channel
			score.score[7] = (checkedMidiChannel == note.midiChannel ? 1 : 0);
		}

		// drum key priority
		score.score[1] = getPriorityScore(triggerData.preset.drumKeyOrder, preset.drumKeyOrder);

		// midi channel priority
		score.score[2] = checkedMidiScore - triggerData.midiScore;

		// make comparison with default value deterministic
		score.score[8] = 1;
		return score;
	}

	void addTrigger(std::vector<NoteTriggerData>& result, Preset& preset, int chan, int key, int velocity, MidiChannelState const& midiState, NesState const& nesState) {
		std::vector<NesChannel> nesChannels = preset.getValidNesChannels();

		if (nesChannels.empty()) {
			return;
		}

		double midiScore = stats.getMidiScore(chan, midiState);
		NesChannel bestChannel = nesChannels[0];
		PlayScore bestScore(0, -1);
		for (NesChannel nesChannel : nesChannels) {
			PlayScore score = calculatePlayScore(nesState, nesChannel, preset, chan, key, velocity, midiScore);
			if (score.isBetterThan(bestScore, true)) {
				bestChannel = nesChannel;
				bestScore = score;
			}
		}
		if (bestScore.isBetterThan(PlayScore(), true)) {
			result.emplace_back(bestChannel, preset, midiScore);
		}
	}

public:
	void preprocess(std::vector<BASS_MIDI_EVENT>& events, FamiTrackerFile& file) {
		base.fillBase(file);
		stats.load(events);
		InstrumentBaseModifier::tryModifyInstrumentBase(stats, base);
	}

	std::vector<NoteTriggerData> getNoteTriggers(int chan, int key, int velocity, MidiChannelState const& midiState, NesState const& nesState) {
		std::vector<Preset> presets = base.getPresets(key, midiState);
		std::vector<NoteTriggerData> result;
		for (auto& preset : presets) {
			addTrigger(result, preset, chan, key, velocity, midiState, nesState);
		}
		return result;
	}
};