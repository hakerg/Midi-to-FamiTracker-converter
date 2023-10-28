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
	static constexpr bool LOWER_NOTES_FIRST = false;

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
				if (assignGenerator.notes[event.chan] > 0) {
					previousData = assignGenerator.generateAssignData(previousData);
					channelAssignData.push_back(previousData.value());
					assignGenerator = AssignDataGenerator(state, BASS_ChannelBytes2Seconds(handle, event.pos), &base);
				}
				else {
					assignGenerator.originMidiState = state;
				}
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

	PlayScore calculatePlayScore(NesState const& nesState, NoteTriggerData const& checkedTrigger, int checkedMidiKey, int checkedMidiVelocity) const {
		auto score = PlayScore(0);

		// don't interrupt current note with higher priorityOrder sound (e.g. crash with hi-hat)
		if (const std::optional<PlayingNesNote>& note = nesState.getNote(checkedTrigger.nesChannel);
			note && nesState.seconds < note->canInterruptSeconds && checkedTrigger.drumKeyOrder > note->triggerData.drumKeyOrder) {

			score.set(PlayScore::Level::INTERRUPTS, -1);
		}

		score.set(PlayScore::Level::PLAYING, 2);

		// frequency limit
		score.set(PlayScore::Level::PLAYABLE_RANGE, Note::isInPlayableRange(checkedTrigger.nesChannel, checkedMidiKey) ? 1 : 0);

		// note height | priority
		if (checkedTrigger.drumKeyOrder != Preset::DEFAULT_DRUM_KEY_ORDER) {
			score.set(PlayScore::Level::NOTE_HEIGHT, -checkedTrigger.drumKeyOrder);
		}
		else {
			score.set(PlayScore::Level::NOTE_HEIGHT, LOWER_NOTES_FIRST ? -checkedMidiKey : checkedMidiKey);
		}

		// velocity
		score.set(PlayScore::Level::VELOCITY, checkedMidiVelocity);
		return score;
	}

	PlayScore calculateCurrentPlayScore(NesState const& nesState, NesChannel nesChannel) {
		auto score = PlayScore(0);

		const std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
		if (!note) {
			return score;
		}

		int noteTimeRows = nesState.getRow() - nesState.getRow(note->startSeconds);
		const NoteTriggerData& triggerData = note->triggerData;
		if (bool releaseDisabled = (nesChannel == NesChannel::NOISE || nesChannel == NesChannel::DPCM);
			!note->playing || (releaseDisabled && noteTimeRows > 0)) {

			// note released
			score.set(PlayScore::Level::PLAYING, 1);
		}
		else {
			// note playing
			score.set(PlayScore::Level::PLAYING, 2);
		}

		// frequency limit of current note
		score.set(PlayScore::Level::PLAYABLE_RANGE, Note::isInPlayableRange(triggerData.nesChannel, note->midiKey) ? 1 : 0);

		// note start time
		score.set(PlayScore::Level::NOTE_TIME, -noteTimeRows);

		// note height | priority
		if (triggerData.drumKeyOrder != Preset::DEFAULT_DRUM_KEY_ORDER) {
			score.set(PlayScore::Level::NOTE_HEIGHT, -triggerData.drumKeyOrder);
		}
		else {
			score.set(PlayScore::Level::NOTE_HEIGHT, LOWER_NOTES_FIRST ? -note->midiKey : note->midiKey);
		}

		// velocity
		score.set(PlayScore::Level::VELOCITY, note->midiVelocity);

		return score;
	}

	void addTriggerIfPossible(std::vector<NoteTriggerData>& result, std::vector<NoteTriggerData> const& possibleTriggers, int key, int velocity, NesState const& nesState) {
		if (possibleTriggers.empty()) {
			return;
		}

		NoteTriggerData bestTrigger = possibleTriggers[0];
		PlayScore bestScore(-999999);
		for (NoteTriggerData const& trigger : possibleTriggers) {
			PlayScore currentScore = calculateCurrentPlayScore(nesState, trigger.nesChannel);
			PlayScore score = calculatePlayScore(nesState, trigger, key, velocity);
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