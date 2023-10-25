#pragma once
#include "commons.h"
#include "NoteTriggerData.h"
#include "FamiTrackerFile.h"
#include "MidiChannelState.h"
#include "NesState.h"
#include "InstrumentBase.h"
#include "MidiState.h"
#include "PlayScore.h"
#include "AssignData.h"
#include "AssignDataGenerator.h"

class InstrumentSelector {
private:
	InstrumentBase base;
	std::vector<AssignData> channelAssignData;

	void fillChannelAssignData(HSTREAM handle, std::vector<BASS_MIDI_EVENT> const& events) {
		MidiState state;
		AssignDataGenerator assignGenerator(state, 0, &base);
		std::optional<AssignData> previousData;
		for (auto const& event : events) {
			state.processEvent(event);

			if (event.event == MIDI_EVENT_NOTE) {
				int velocity = MidiState::getVelocity(event);
				if (velocity != 0 && velocity != 255) {
					assignGenerator.addNote(event.chan, MidiState::getKey(event), velocity, state);
				}
			}
			else if (event.event == MIDI_EVENT_PROGRAM || event.event == MIDI_EVENT_DRUMS) {
				previousData = assignGenerator.generateAssignData(previousData);
				channelAssignData.push_back(previousData.value());
				assignGenerator = AssignDataGenerator(state, BASS_ChannelBytes2Seconds(handle, event.pos), &base);
			}
		}
		channelAssignData.push_back(assignGenerator.generateAssignData(previousData));
	}

	AssignData& getAssign(double seconds) {
		for (int i = 1; i < channelAssignData.size(); i++) {
			if (channelAssignData[i].seconds > seconds) {
				return channelAssignData[size_t(i) - 1];
			}
		}
		return channelAssignData.back();
	}

	static int getPriorityScore(int basePriorityOrder, int checkedPriorityOrder) {
		if (basePriorityOrder < 0 || checkedPriorityOrder < 0) {
			return 0;
		}
		return basePriorityOrder - checkedPriorityOrder;
	}

	PlayScore calculatePlayScore(NesState const& nesState, NoteTriggerData const& checkedTrigger, int checkedMidiKey, int checkedMidiVelocity) const {
		auto score = PlayScore(0, 0);

		const std::optional<PlayingNesNote>& note = nesState.getNote(checkedTrigger.nesChannel);
		if (!note) {
			return PlayScore(0, 2);
		}

		const NoteTriggerData& triggerData = note->triggerData;
		int currentRow = nesState.getRow();

		// don't interrupt current note with higher priorityOrder sound (e.g. crash with hi-hat)
		if (nesState.seconds < note->canInterruptSeconds && getPriorityScore(triggerData.drumKeyOrder, checkedTrigger.drumKeyOrder) < 0) {
			return PlayScore(0, -1);
		}

		if (bool releaseDisabled = (checkedTrigger.nesChannel == NesChannel::NOISE || checkedTrigger.nesChannel == NesChannel::DPCM);
			!note->playing || (releaseDisabled && currentRow != nesState.getRow(note->startSeconds))) { // note released

			score.scores[0] = 1;
		}
		else {
			// note start time
			score.scores[2] = currentRow - nesState.getRow(note->startSeconds);

			// note height
			score.scores[3] = checkedMidiKey - note->midiKey;

			// velocity
			score.scores[4] = checkedMidiVelocity - note->midiVelocity;

			// pulse 1-2 & triangle frequency limit
			score.scores[5] = (checkedMidiKey < Note::MIN_PLAYABLE_KEY && !Cell::isVrc6(checkedTrigger.nesChannel) ? 0 : 1);
		}

		// drum key priority
		score.scores[1] = getPriorityScore(triggerData.drumKeyOrder, checkedTrigger.drumKeyOrder);

		// make comparison with default value deterministic
		score.scores[6] = 1;
		return score;
	}

	void addTriggerIfPossible(std::vector<NoteTriggerData>& result, std::vector<NoteTriggerData> const& possibleTriggers, int key, int velocity, NesState const& nesState) {
		if (possibleTriggers.empty()) {
			return;
		}

		NoteTriggerData bestTrigger = possibleTriggers[0];
		PlayScore bestScore(0, -1);
		for (NoteTriggerData const& trigger : possibleTriggers) {
			PlayScore score = calculatePlayScore(nesState, trigger, key, velocity);
			if (score.isBetterThan(bestScore, true)) {
				bestTrigger = trigger;
				bestScore = score;
			}
		}
		if (bestScore.isBetterThan(PlayScore(0, 0), true)) {
			result.push_back(bestTrigger);
		}
	}

public:
	void preprocess(HSTREAM handle, std::vector<BASS_MIDI_EVENT> const& events, FamiTrackerFile& file) {
		base.fillBase(file);
		fillChannelAssignData(handle, events);
	}

	std::vector<NoteTriggerData> getNoteTriggers(int chan, int key, int velocity, MidiChannelState const& midiState, NesState const& nesState) {
		std::vector<NoteTriggerData> result;

		// drums can have multiple triggers (i.e. noise and dpcm for the same note)
		// for normal instruments only one trigger is selected
		if (midiState.useDrums) {
			std::vector<Preset> presets = base.getDrumPresets(midiState.program, key);
			for (auto const& preset : presets) {
				std::vector<NesChannel> nesChannels = preset.getValidNesChannels();
				for (auto const& nesChannel : nesChannels) {
					addTriggerIfPossible(result, { NoteTriggerData(nesChannel, preset.duty, preset) }, key, velocity, nesState);
				}
			}
		}
		else {
			const AssignChannelData& nesData = getAssign(nesState.seconds).getNesData(chan);
			std::optional<Preset> preset = base.getGmPreset(midiState.program);
			if (preset) {
				addTriggerIfPossible(result, nesData.getTriggers(preset.value()), key, velocity, nesState);
			}
		}

		return result;
	}
};