#pragma once
#include "commons.h"
#include "FamiTrackerFile.h"
#include "InstrumentSelector.h"
#include "MidiState.h"
#include "NesState.h"
#include "PitchCalculator.h"

class Converter {
private:
    static const int ROWS_PER_PATTERN = 256;

    std::array<double, Pattern::CHANNELS> nesVolumeFactor = { 0.6, 0.6, 1.0, 0.8, 1.0, 0.6, 0.6, 0.5 };

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

    Cell& getCurrentCell(NesChannel channel) {
        int row = nesState.getRow();
		return getPattern(row / ROWS_PER_PATTERN)->getCell(channel, row % ROWS_PER_PATTERN);
    }

    bool willOverlapAudio(NesChannel ignoreChannel, int key, int detuneIndex, bool checkOtherOctaves) {
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            if (nesChannel == ignoreChannel) {
                continue;
            }
            if (nesChannel == NesChannel::NOISE || nesChannel == NesChannel::DPCM) {
                continue;
            }
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (note && note->playing && detuneIndex == note->detuneIndex && ((checkOtherOctaves && note->midiKey % 12 == key % 12) || (!checkOtherOctaves && note->midiKey == key))) {
                return true;
            }
        }
        return false;
    }

    int getDetuneIndex(NesChannel ignoreChannel, int key) {
        // detune unnecessary with these channels
        if (ignoreChannel == NesChannel::NOISE || ignoreChannel == NesChannel::DPCM) {
            return 0;
        }

        // index means fractions of semitones
        int index = 0;
        // check other octaves only if detune is not so high
        while (willOverlapAudio(ignoreChannel, key, index, index >= -1 && index <= -1)) {
            if (index <= 0) {
                index = -index + 1;
            }
            else {
                index = -index;
            }
        }
        return index;
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

    void midiNoteOn(int chan, int key, int velocity) {
        auto triggerData = instrumentSelector.getNoteTriggers(chan, key, velocity, midiState.getChannel(chan), nesState);

        for (auto& data : triggerData) {
            // uninterrupted time was too long, so it's divided
            double canInterruptSeconds = nesState.seconds + data.uninterruptedTicks / 60.0;
            // used to slightly detune notes with the same frequency as on different channels to avoid audio wave overlapping/cancelling
            int detuneIndex = getDetuneIndex(data.nesChannel, key);
            nesState.setNote(data.nesChannel, PlayingNesNote(chan, key, velocity, detuneIndex, nesState.seconds, data, canInterruptSeconds));

            // triangle needs to be played octave higher to sound better
            Note nesKey(data.nesChannel == NesChannel::TRIANGLE ? key + 12 : key, data.nesChannel != NesChannel::NOISE);

            Cell& currentCell = getCurrentCell(data.nesChannel);
            currentCell.Note(data.note ? data.note.value() : nesKey, data.instrument);
            setNesVolume(chan, data.nesChannel, velocity);
            setNesPitch(chan, data.nesChannel, key, detuneIndex);

            // set duty if requested
            std::optional<NesDuty> duty = data.getNesDuty();
            if (duty) {
                currentCell.Duty(duty.value());
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
			midiNoteOn(chan, key, velocity);
        }
    }

    void setNesPitch(int midiChan, NesChannel nesChannel, int key, int detuneIndex) {
        // ignore for noise and DPCM
        if (nesChannel == NesChannel::NOISE || nesChannel == NesChannel::DPCM) {
            return;
        }

        double basePeriod = PitchCalculator::calculatePeriodByKey(nesChannel, key);
        double pitchedPeriod = PitchCalculator::calculatePeriodByKey(nesChannel, key + 0.1 * detuneIndex + midiState.getChannel(midiChan).getNotePitch());
        getCurrentCell(nesChannel).FinePitch(max(0, min(255, 128 + int(round(basePeriod - pitchedPeriod)))));
    }

    void updateNesPitch(int midiChan) {
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (note && note->midiChannel == midiChan) {
                setNesPitch(midiChan, nesChannel, note->midiKey, note->detuneIndex);
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