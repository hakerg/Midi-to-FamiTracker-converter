#pragma once
#include "commons.h"
#include "FamiTrackerFile.h"
#include "InstrumentSelector.h"
#include "MidiState.h"
#include "NesState.h"
#include "PitchCalculator.h"

class Converter {
private:
    static constexpr int ROWS_PER_PATTERN = 256;
    static constexpr double MAX_DETUNE_SEMITONES = 0.2;

    std::array<double, Pattern::CHANNELS> nesVolumeFactor = { 0.6, 0.6, 1.0, 0.8, 1.0, 0.6, 0.6, 0.6 };

    InstrumentSelector instrumentSelector;
    FamiTrackerFile file;
    std::shared_ptr<Track> track;

    MidiState midiState;
    NesState nesState = NesState(60);

    std::shared_ptr<Pattern> getPattern(int pattern) {
        while (track->patterns.size() <= pattern) {
            track->addPatternAndOrder();
        }
        return track->patterns[pattern];
    }

    Cell* getPreviousCell(NesChannel channel) {
        int row = nesState.getRow() - 1;
        return row < 0 ? nullptr : &(getPattern(row / ROWS_PER_PATTERN)->getCell(channel, row % ROWS_PER_PATTERN));
    }

    Cell& getCurrentCell(NesChannel channel) {
        int row = nesState.getRow();
		return getPattern(row / ROWS_PER_PATTERN)->getCell(channel, row % ROWS_PER_PATTERN);
    }

    void stopNote(NesChannel nesChannel, Cell::Type stopType) {
        // release stops the drum samples, so it needs to be ignored
        if (stopType == Cell::Type::RELEASE && (nesChannel == NesChannel::DPCM || nesChannel == NesChannel::NOISE)) {
            return;
        }

		// ignore if already stopped
        if (stopType == Cell::Type::RELEASE) {
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (!note || !note->playing) {
                return;
            }
        }

        getCurrentCell(nesChannel).type = stopType;
        nesState.resetNote(nesChannel);
    }

    void stopNote(int chan, int key, Cell::Type stopType) {
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (note && note->midiChannel == chan && note->midiKey == key) {
                stopNote(nesChannel, stopType);
            }
        }
    }

    void stopAllNotes(int chan, Cell::Type stopType) {
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (note && note->midiChannel == chan) {
                stopNote(nesChannel, stopType);
            }
        }
    }

    void midiNoteOn(int chan, int key, int velocity, bool pitchChangeNote, int keyAfterPitch) {
        auto triggerData = instrumentSelector.getNoteTriggers(chan, key, velocity, midiState.getChannel(chan), nesState);

        for (auto& data : triggerData) {
            // used to slightly detune notes with the same frequency as on different channels to avoid audio wave overlapping/cancelling
            int detuneIndex = nesState.getDetuneIndex(data.nesChannel, key);
            if (!pitchChangeNote) {
                double canInterruptSeconds = nesState.seconds + data.uninterruptedTicks / 60.0;
                nesState.setNote(data.nesChannel, PlayingNesNote(chan, key, velocity, detuneIndex, nesState.seconds, data, canInterruptSeconds));
            }
            else {
                std::optional<PlayingNesNote>& note = nesState.getNote(data.nesChannel);
                if (note) {
                    note->keyAfterPitch = keyAfterPitch;
                    note->detuneIndex = detuneIndex;
                }
            }

            // triangle needs to be played octave higher to sound better
            Note nesKey(data.nesChannel == NesChannel::TRIANGLE ? keyAfterPitch + 12 : keyAfterPitch, data.nesChannel != NesChannel::NOISE);

            Cell& currentCell = getCurrentCell(data.nesChannel);
            currentCell.Note(data.note ? data.note.value() : nesKey, data.instrument);

            setNesPitch(chan, data.nesChannel, key, detuneIndex, keyAfterPitch);

            if (pitchChangeNote) {
                continue;
            }

            setNesVolume(chan, data.nesChannel, velocity);

            // set duty if requested
            if (std::optional<NesDuty> duty = data.getNesDuty(); duty) {
                currentCell.Duty(duty.value());
            }

            // mute previous tick to clearly hear the note
            if (data.nesChannel != NesChannel::NOISE && data.nesChannel != NesChannel::DPCM) {
                Cell* previousCell = getPreviousCell(data.nesChannel);
                if (previousCell && (previousCell->type == Cell::Type::EMPTY || previousCell->type == Cell::Type::RELEASE)) {
                    previousCell->volume = 0;
                }
            }
        }
    }

    void midiNote(int chan, int key, int velocity) {
        if (velocity == 0) {
            stopNote(chan, key, Cell::Type::RELEASE);
        }
        else if (velocity == 255) {
            stopNote(chan, key, Cell::Type::STOP);
        }
        else {
			midiNoteOn(chan, key, velocity, false, key);
        }
    }

	// keyAfterPitch is currently playing key due to pitch change, key is the original one from midi event
    void setNesPitch(int midiChan, NesChannel nesChannel, int key, int detuneIndex, int keyAfterPitch) {
        // ignore for noise and DPCM
        if (nesChannel == NesChannel::NOISE || nesChannel == NesChannel::DPCM) {
            return;
        }

        double targetKey = key + midiState.getChannel(midiChan).getNotePitch();
        double targetPeriod = PitchCalculator::calculatePeriodByKey(nesChannel, targetKey) - detuneIndex;

        // detune cannot be too high
        if (double resultKey = PitchCalculator::calculateKeyByPeriod(nesChannel, targetPeriod);
            std::abs(resultKey - targetKey) > MAX_DETUNE_SEMITONES) {

            setNesPitch(midiChan, nesChannel, key, 0, keyAfterPitch);
            return;
        }

        double basePeriod = PitchCalculator::calculatePeriodByKey(nesChannel, keyAfterPitch);
        int finePitch = 128 + int(round(basePeriod - targetPeriod));
        if (finePitch >= 0 && finePitch <= 255) {
            getCurrentCell(nesChannel).FinePitch(finePitch);
        }
        else if (std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel); note && note->playing) {
			// when pitch is out of range for FamiTracker, we need to create a new note
            midiNoteOn(midiChan, key, note->midiVelocity, true, int(round(targetKey)));
        }
    }

    void updateNesPitch(int midiChan) {
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (note && note->midiChannel == midiChan) {
                setNesPitch(midiChan, nesChannel, note->midiKey, note->detuneIndex, note->keyAfterPitch);
            }
        }
    }

    void setNesVolume(int midiChan, NesChannel nesChannel, int velocity) {
        // FamiTracker does not import volume for DPCM channel
        if (nesChannel == NesChannel::DPCM) {
            return;
        }

        double volumeF = midiState.getChannel(midiChan).getNoteVolume(velocity) * nesVolumeFactor[int(nesChannel)] / 8.0;
        getCurrentCell(nesChannel).volume = min(15, int(ceil(volumeF)));
    }

    void updateNesVolume(int midiChan) {
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (note && note->midiChannel == midiChan) {
                setNesVolume(midiChan, nesChannel, note->midiVelocity);
            }
        }
    }

    void resetControllers(int chan) {
        updateNesVolume(chan);
        updateNesPitch(chan);
    }

    void resetMidi() {
        for (int i = 0; i < 16; i++) {
            updateNesVolume(i);
            updateNesPitch(i);
        }
    }

    void processEvents(HSTREAM handle, std::vector<BASS_MIDI_EVENT>& events) {
        for (auto const& event : events) {
            nesState.seconds = BASS_ChannelBytes2Seconds(handle, event.pos);
            midiState.processEvent(event);

            switch (event.event) {
            case MIDI_EVENT_NOTE:
                midiNote(event.chan, MidiState::getKey(event), MidiState::getVelocity(event));
                break;
            case MIDI_EVENT_PITCH:
            case MIDI_EVENT_PITCHRANGE:
            case MIDI_EVENT_FINETUNE:
            case MIDI_EVENT_COARSETUNE:
                updateNesPitch(event.chan);
                break;
            case MIDI_EVENT_VOLUME:
            case MIDI_EVENT_EXPRESSION:
                updateNesVolume(event.chan);
                break;
            case MIDI_EVENT_RESET:
                resetControllers(event.chan);
                break;
            case MIDI_EVENT_SOUNDOFF:
                stopAllNotes(event.chan, Cell::Type::STOP);
                break;
            case MIDI_EVENT_NOTESOFF:
                stopAllNotes(event.chan, Cell::Type::RELEASE);
                break;
            // TODO: vibrato, portamento, arpeggio

            case MIDI_EVENT_SYSTEM:
            case MIDI_EVENT_SYSTEMEX:
                resetMidi();
                break;

            default:
                break;
            }
        }
    }

    static std::vector<BASS_MIDI_EVENT> getEvents(HSTREAM handle) {
        DWORD eventCount = BASS_MIDI_StreamGetEvents(handle, -1, 0, nullptr);
        std::vector<BASS_MIDI_EVENT> events(eventCount);
        BASS_MIDI_StreamGetEvents(handle, -1, 0, events.data());
        return events;
    }

    static std::vector<BASS_MIDI_MARK> getMarks(HSTREAM handle) {
        DWORD markCount = BASS_MIDI_StreamGetMarks(handle, -1, BASS_MIDI_MARK_MARKER, nullptr);
        std::vector<BASS_MIDI_MARK> marks(markCount);
        BASS_MIDI_StreamGetMarks(handle, -1, BASS_MIDI_MARK_MARKER, marks.data());
        return marks;
    }

    int getSpeedDivider(double songTimeSeconds) const {
        int maxRows = ROWS_PER_PATTERN * 0x80;
        int divider = 1;
        // 1 less for rounding error, let's be sure that halt event can be placed at the end of the song
        while (songTimeSeconds * nesState.rowsPerSecond / divider >= maxRows - 1) {
            divider++;
        }
        return divider;
    }

public:
    FamiTrackerFile convert(HSTREAM handle) {
        std::vector<BASS_MIDI_EVENT> events = getEvents(handle);
        int speedDivider = getSpeedDivider(events.empty() ? 0 : BASS_ChannelBytes2Seconds(handle, events.back().pos));
		nesState.rowsPerSecond /= speedDivider;

        std::cout << "Scrolling speed: " << nesState.rowsPerSecond << " rows per second" << std::endl;

        instrumentSelector.preprocess(handle, events, file);

        file.comment = L"Created using MidiToFamiTrackerConverter by hakerg";
        track = file.addTrack(ROWS_PER_PATTERN);
        track->speed = speedDivider;
        track->tempo = 150;

        resetMidi();

        processEvents(handle, events);

        getCurrentCell(NesChannel::DPCM).Halt();

		std::cout << "Created " << track->patterns.size() << " patterns, " << file.instruments.size() << " instruments and " << file.dpcmSamples.size() << " DPCM samples" << std::endl;
        return file;
    }
};