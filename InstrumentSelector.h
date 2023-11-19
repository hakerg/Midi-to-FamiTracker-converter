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
#include "ChannelAssigner.h"

class InstrumentSelector {
private:
	static constexpr double MIN_NOTE_SECONDS = 1 / 20.0; // 3 frames

	InstrumentBase base;
	std::vector<IndexedAssignData> channelAssignData{};

	static std::unordered_set<int> getSplitEventIndexes(std::vector<MidiEvent> const& events) {
		std::unordered_set<int> splits;
		MidiState state;
		std::array<int, MidiState::CHANNEL_COUNT> sectionsNotes{};

		for (int i = 0; i < events.size(); i++) {
			const MidiEvent& event = events[i];
			MidiChannelState& channelState = state.getChannel(event.chan);
			int& sectionNotes = sectionsNotes[event.chan];

			int oldProgram = channelState.program;
			bool oldDrums = channelState.useDrums;

			state.processEvent(event);

			bool programUpdated = (oldProgram != channelState.program || oldDrums != channelState.useDrums);
			if (programUpdated && sectionNotes > 0) {
				splits.insert(i);
				sectionsNotes.fill(0);
			}

			if (event.event == MIDI_EVENT_NOTE_ON) {
				sectionNotes++;
			}
		}

		splits.insert(int(events.size()));

		std::cout << "Created " << splits.size() << " split points" << std::endl;
		return splits;
	}

	void fillChannelAssignData(std::vector<MidiEvent> const& events, std::unordered_set<int> const& splitPoints, FileSettingsJson const& settings) {
		MidiState state;
		auto assigner = std::make_unique<ChannelAssigner>(base, settings);

		for (int i = 0; i < events.size(); i++) {
			const MidiEvent& event = events[i];
			state.processEvent(event);

			if (event.event == MIDI_EVENT_NOTE_ON) {
				assigner->addNote(event, state);
			}

			if (splitPoints.contains(i + 1)) {
				channelAssignData.push_back(assigner->generateAssignData(i + 1, state));
			}
		}
	}

	const AssignData& getAssign(int eventIndex) const {
		for (int i = 1; i < channelAssignData.size(); i++) {
			if (channelAssignData[i].eventIndex > eventIndex) {
				return channelAssignData[size_t(i) - 1].data;
			}
		}
		return channelAssignData.back().data;
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
		score.set(NOTE_HEIGHT, checkedTrigger.lowerKeysFirst ? -event.key : event.key);
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
		score.set(NOTE_HEIGHT, triggerData.lowerKeysFirst ? -note->event.key : note->event.key);
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
	void preprocess(std::vector<MidiEvent> const& events, FamiTrackerFile& file, FileSettingsJson const& settings) {
		base.fillBase(file);
		fillChannelAssignData(events, getSplitEventIndexes(events), settings);
	}

	std::vector<NoteTriggerData> getNoteTriggers(MidiEvent const& event, int eventIndex, MidiChannelState const& midiState, NesState const& nesState, FileSettingsJson const& settings) const {
		std::vector<NoteTriggerData> result;

		// drums can have multiple triggers (i.e. noise and dpcm for the same note)
		// for normal instruments only one trigger is selected
		if (midiState.useDrums) {
			std::optional<Preset> preset = base.getDrumPreset(midiState.program, event.key);
			if (preset) {
				std::vector<NesChannel> nesChannels = preset->getValidNesChannels();
				for (auto const& nesChannel : nesChannels) {
					addTriggerIfPossible(result, { NoteTriggerData(nesChannel, preset->duty, preset.value(), settings.lowerKeysFirst[event.chan])}, event, nesState);
				}
			}
		}
		else {
			const AssignChannelData& nesData = getAssign(eventIndex).getNesData(event.chan);
			std::optional<Preset> preset = base.getGmPreset(midiState.program);
			if (preset) {
				addTriggerIfPossible(result, nesData.getTriggers(preset.value(), settings.lowerKeysFirst[event.chan]), event, nesState);
			}
		}

		return result;
	}
};