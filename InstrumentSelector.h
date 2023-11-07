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
	static constexpr double MIN_NOTE_SECONDS = 1 / 30.0;

	InstrumentBase base;
	std::vector<AssignData> channelAssignData{};

	void fillChannelAssignData(HSTREAM handle, std::vector<MidiEvent> const& events) {
		MidiState state;
		AssignDataGenerator assignGenerator(state, 0, &base);
		for (int i = 0; i < events.size(); i++) {
			const MidiEvent& event = events[i];
			state.processEvent(event);

			if (event.event == MIDI_EVENT_NOTE_ON) {
				assignGenerator.addNote(event, state);
			}
			else if (event.event == MIDI_EVENT_PROGRAM || event.event == MIDI_EVENT_DRUMS) {
				if (assignGenerator.hasAnyNotes(event.chan)) {
					assignGenerator.calculateMidiData();
					channelAssignData.push_back(assignGenerator.generateAssignData());
					assignGenerator = AssignDataGenerator(state, i, &base);
				}
				else {
					assignGenerator.setMidiState(state);
				}
			}
		}
		assignGenerator.calculateMidiData();
		channelAssignData.push_back(assignGenerator.generateAssignData());
	}

	const AssignData& getAssign(int eventIndex) const {
		for (int i = 1; i < channelAssignData.size(); i++) {
			if (channelAssignData[i].eventIndex > eventIndex) {
				return channelAssignData[size_t(i) - 1];
			}
		}
		return channelAssignData.back();
	}

	static PlayScore calculatePlayScore(NesState const& nesState, NoteTriggerData const& checkedTrigger, MidiEvent const& event) {
		using enum PlayScore::Level;
		auto score = PlayScore(0);

		// don't interrupt current note with higher priorityOrder sound (e.g. crash with hi-hat)
		if (const std::optional<PlayingNesNote>& note = nesState.getNote(checkedTrigger.nesChannel);
			note && event.seconds < note->canInterruptSeconds && int(checkedTrigger.preset.order) > int(note->triggerData.preset.order)) {

			score.set(INTERRUPTS, -1);
		}

		score.set(PLAYING, 2);
		score.set(PLAYABLE_RANGE, Note::isInPlayableRange(checkedTrigger.nesChannel, event.key) ? 1 : 0);
		score.set(NOTE_TIME, 0);
		score.set(PRIORITY, -int(checkedTrigger.preset.order));
		score.set(NOTE_END_TIME, min(MIN_NOTE_SECONDS, event.noteEndSeconds - event.seconds));
		score.set(TONE_OVERLAP, -nesState.countPulseChannelsWithSameKeyAndLength(checkedTrigger.nesChannel, event, MIN_NOTE_SECONDS));
		score.set(NOTE_HEIGHT, checkedTrigger.lowerNotesFirst ? -event.key : event.key);
		score.set(VELOCITY, event.velocity);

		return score;
	}

	static PlayScore calculateCurrentPlayScore(NesState const& nesState, NesChannel nesChannel) {
		using enum PlayScore::Level;
		auto score = PlayScore(0);

		const std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
		if (!note) {
			// no current note, return default score
			return score;
		}

		const NoteTriggerData& triggerData = note->triggerData;

		score.set(INTERRUPTS, 0);
		score.set(PLAYING, note->playing ? 2 : 1);
		score.set(PLAYABLE_RANGE, Note::isInPlayableRange(triggerData.nesChannel, note->event.key) ? 1 : 0);
		score.set(NOTE_TIME, -max(0, nesState.seconds - note->event.seconds - MIN_NOTE_SECONDS));
		score.set(PRIORITY, -int(triggerData.preset.order));
		score.set(NOTE_END_TIME, min(MIN_NOTE_SECONDS, note->event.noteEndSeconds - nesState.seconds));
		score.set(TONE_OVERLAP, -nesState.countPulseChannelsWithSameKeyAndLength(nesChannel, note->event, MIN_NOTE_SECONDS));
		score.set(NOTE_HEIGHT, triggerData.lowerNotesFirst ? -note->event.key : note->event.key);
		score.set(VELOCITY, note->event.velocity);

		return score;
	}

	static void addTriggerIfPossible(std::vector<NoteTriggerData>& result, std::vector<NoteTriggerData> const& possibleTriggers, MidiEvent const& event, NesState const& nesState) {
		if (possibleTriggers.empty()) {
			return;
		}

		NoteTriggerData bestTrigger = possibleTriggers[0];
		PlayScore bestScore(-999999);
		for (NoteTriggerData const& trigger : possibleTriggers) {
			PlayScore currentScore = calculateCurrentPlayScore(nesState, trigger.nesChannel);
			PlayScore score = calculatePlayScore(nesState, trigger, event);
			score = score.substract(currentScore);
			if (score.isBetterThan(bestScore, true)) {
				bestTrigger = trigger;
				bestScore = score;
			}
		}
		if (bestScore.isBetterThan(PlayScore(0), true)) {
			result.push_back(bestTrigger);
		}
	}

public:
	void preprocess(HSTREAM handle, std::vector<MidiEvent> const& events, FamiTrackerFile& file) {
		base.fillBase(file);
		fillChannelAssignData(handle, events);
	}

	std::vector<NoteTriggerData> getNoteTriggers(MidiEvent const& event, int eventIndex, MidiChannelState const& midiState, NesState const& nesState) const {
		std::vector<NoteTriggerData> result;

		// drums can have multiple triggers (i.e. noise and dpcm for the same note)
		// for normal instruments only one trigger is selected
		if (midiState.useDrums) {
			std::vector<Preset> presets = base.getDrumPresets(midiState.program, event.key);
			for (auto const& preset : presets) {
				std::vector<NesChannel> nesChannels = preset.getValidNesChannels();
				for (auto const& nesChannel : nesChannels) {
					addTriggerIfPossible(result, { NoteTriggerData(nesChannel, preset.duty, preset, false) }, event, nesState);
				}
			}
		}
		else {
			const AssignChannelData& nesData = getAssign(eventIndex).getNesData(event.chan);
			std::optional<Preset> preset = base.getGmPreset(midiState.program);
			if (preset) {
				addTriggerIfPossible(result, nesData.getTriggers(preset.value()), event, nesState);
			}
		}

		return result;
	}
};