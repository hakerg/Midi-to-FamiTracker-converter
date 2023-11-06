#pragma once
#include "commons.h"
#include "FamiTrackerFile.h"
#include "InstrumentSelector.h"
#include "MidiState.h"
#include "NesState.h"
#include "PitchCalculator.h"
#include "MidiEventParser.h"

class Converter {
private:
    static constexpr int ROWS_PER_PATTERN = 256;
    static constexpr double MAX_DETUNE_SEMITONES = 0.2;
    static constexpr bool ADJUST_SPEED = true;

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

    Cell* getPreviousCell(NesChannel channel, int rows) {
        int row = nesState.getRow() - rows;
        return row < 0 ? nullptr : &(getPattern(row / ROWS_PER_PATTERN)->getCell(channel, row % ROWS_PER_PATTERN));
    }

    Cell& getCurrentCell(NesChannel channel) {
        int row = nesState.getRow();
		return getPattern(row / ROWS_PER_PATTERN)->getCell(channel, row % ROWS_PER_PATTERN);
    }

    void stopNote(NesChannel nesChannel, Cell::Type stopType) {
		// ignore if already stopped
        if (stopType == Cell::Type::RELEASE) {
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (!note || !note->playing || !note->triggerData.preset.needRelease) { // the last condition causes that note has playing=true after release, but that's fine
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
            if (note && note->event.chan == chan && note->event.key == key) {
                stopNote(nesChannel, stopType);
            }
        }
    }

    void stopAllNotes(int chan, Cell::Type stopType) {
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (note && note->event.chan == chan) {
                stopNote(nesChannel, stopType);
            }
        }
    }

    Note getNesNote(NesChannel nesChannel, int midiKey) {
        // triangle needs to be played octave higher to sound better
        return Note(nesChannel == NesChannel::TRIANGLE ? midiKey + 12 : midiKey, nesChannel != NesChannel::NOISE);
    }

    void midiNoteOn(MidiEvent const& event, int eventIndex) {
        auto triggerData = instrumentSelector.getNoteTriggers(event, eventIndex, midiState.getChannel(event.chan), nesState);

        for (auto& data : triggerData) {
            double canInterruptSeconds = nesState.seconds + data.preset.uninterruptedTicks / 60.0;
            nesState.setNote(data.nesChannel, PlayingNesNote(event, nesState.seconds, data, canInterruptSeconds));

            Cell& currentCell = getCurrentCell(data.nesChannel);
            currentCell.Note(data.preset.note ? data.preset.note.value() : getNesNote(data.nesChannel, event.key), data.preset.instrument);

            // optional was set above, so shouldn't be empty
            setNesPitch(event.chan, data.nesChannel, nesState.getNote(data.nesChannel).value());
            setNesVolume(event.chan, data.nesChannel, event.velocity);

            // set duty if requested
            if (std::optional<NesDuty> duty = data.getNesDuty(); duty) {
                currentCell.Duty(duty.value());
            }

            if (data.nesChannel == NesChannel::NOISE || data.nesChannel == NesChannel::DPCM) {
                continue;
            }

            // mute previous tick to clearly hear the note
            Cell* previousCell = getPreviousCell(data.nesChannel, 1);
            if (previousCell && (previousCell->type == Cell::Type::EMPTY || previousCell->type == Cell::Type::RELEASE)) {
                Cell* previousCell2 = getPreviousCell(data.nesChannel, 2);
                if (previousCell2 && (previousCell2->type == Cell::Type::EMPTY || previousCell2->type == Cell::Type::RELEASE)) {
                    previousCell->volume = 0;
                }
            }
        }
    }

    int getDetunedPeriod(NesChannel nesChannel, double key) const {
        // detune unnecessary with these channels
        if (nesChannel == NesChannel::NOISE || nesChannel == NesChannel::DPCM) {
            return 0;
        }

        auto basePeriod = int(round(PitchCalculator::calculatePeriodByKey(nesChannel, key)));
        int delta = 0;
        while (nesState.countToneOverlappingChannels(nesChannel, PitchCalculator::calculateFrequencyByPeriod(nesChannel, double(basePeriod) + delta)) > 0) {
            if (delta <= 0) {
                delta = -delta + 1;
            }
            else {
                delta = -delta;
            }
        }

        int resultPeriod = basePeriod + delta;
        // detune cannot be too high
        double resultKey = PitchCalculator::calculateKeyByPeriod(nesChannel, resultPeriod);
        if (std::abs(resultKey - key) > MAX_DETUNE_SEMITONES) {
            return basePeriod;
        }

        return resultPeriod;
    }

    void setNesPitch(int midiChan, NesChannel nesChannel, PlayingNesNote& note) {
        // ignore for noise and DPCM
        if (nesChannel == NesChannel::NOISE || nesChannel == NesChannel::DPCM) {
            return;
        }

        double targetKey = midiState.getChannel(midiChan).getPitchedKey(note.event.key);
        int targetPeriod = getDetunedPeriod(nesChannel, targetKey);

        double basePeriod = PitchCalculator::calculatePeriodByKey(nesChannel, note.keyAfterPitch);
        int finePitch = 128 + int(round(basePeriod - targetPeriod));
        if (finePitch >= 0 && finePitch <= 255) {
            getCurrentCell(nesChannel).FinePitch(finePitch);
            note.frequencyAfterPitch = PitchCalculator::calculateFrequencyByPeriod(nesChannel, targetPeriod);
        }
        else {
            // pitch is out of range for FamiTracker
            // we need to create a new note, but Preset should be non-decaying to make it less noticeable
            // otherwise just stop the note because there is no way to play it :(
            if (note.playing && note.triggerData.preset.needRelease) {
                auto key = int(round(targetKey));

                // fixes stack overflow
                if (key == note.keyAfterPitch) {
                    return;
                }

                getCurrentCell(nesChannel).Note(getNesNote(nesChannel, key), note.triggerData.preset.instrument);
                note.keyAfterPitch = key;
                setNesPitch(midiChan, nesChannel, note);
            }
            else {
                stopNote(nesChannel, Cell::Type::STOP);
            }
        }
    }

    void updateNesPitch(int midiChan) {
        for (int i = 0; i < Pattern::CHANNELS; i++) {
            auto nesChannel = NesChannel(i);
            std::optional<PlayingNesNote>& note = nesState.getNote(nesChannel);
            if (note && note->event.chan == midiChan) {
                setNesPitch(midiChan, nesChannel, note.value());
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
            if (note && note->event.chan == midiChan) {
                setNesVolume(midiChan, nesChannel, note->event.velocity);
            }
        }
    }

    void resetControllers(int chan) {
        updateNesVolume(chan);
        updateNesPitch(chan);
    }

    void resetMidi() {
        for (int i = 0; i < MidiState::CHANNEL_COUNT; i++) {
            updateNesVolume(i);
            updateNesPitch(i);
        }
    }

    void processEvents(HSTREAM handle, std::vector<MidiEvent> const& events) {
        for (int i = 0; i < events.size(); i++) {
			MidiEvent const& event = events[i];
            nesState.seconds = event.seconds;
            midiState.processEvent(event);

            switch (event.event) {
            case MIDI_EVENT_NOTE_ON:
                midiNoteOn(event, i);
                break;
            case MIDI_EVENT_NOTE_OFF:
                stopNote(event.chan, event.key, Cell::Type::RELEASE);
                break;
            case MIDI_EVENT_NOTE_STOP:
                stopNote(event.chan, event.key, Cell::Type::STOP);
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

    int getSpeedDivider(double songTimeSeconds) const {
        int maxRows = ROWS_PER_PATTERN * 0x80;
        int divider = 1;
        // 1 less for rounding error, let's be sure that halt event can be placed at the end of the song
        while (songTimeSeconds * nesState.rowsPerSecond / divider >= maxRows - 1.0) {
            divider++;
        }
        return divider;
    }

    double getAdjustedTempoScore(std::vector<MidiEvent> const& events, double speed, double secondsLimit) {
        double score = 0;
        for (auto& event : events) {
            if (event.event != MIDI_EVENT_NOTE_ON) {
                continue;
            }
            if (event.seconds > secondsLimit) {
                break;
            }

            double seconds = event.seconds * speed;
            int row = nesState.getRow(seconds);
            double resultSeconds = nesState.getSeconds(row);
            score += max(0, 1 - std::abs(seconds - resultSeconds) * 30);
        }

        return score * max(0, 1 - std::abs(1 - speed) * 4);
    }

    double findBestAdjustedSpeed(std::vector<MidiEvent> const& events, double songLength) {
        double bestSpeed = 1;

        double secondsLimit = 10;

        double speedMin = 0.8;
        double speedMax = 1.2;
        double speedStep = 1 / 10000.0;

        while (true) {
            double bestScore = -1;
            double speed = speedMin;
            while (speed <= speedMax) {
                if (double score = getAdjustedTempoScore(events, speed, secondsLimit); score > bestScore) {
                    bestScore = score;
                    bestSpeed = speed;
                }

                speed += speedStep;
            }

            if (secondsLimit >= songLength) {
                break;
            }
            secondsLimit *= 2;
            speedStep *= 0.5;
            speedMin = bestSpeed - 16 * speedStep;
            speedMax = bestSpeed + 16 * speedStep;
        }

        return bestSpeed;
    }

    void adjustTempo(std::vector<MidiEvent>& events, double songLength) {
        double speed = findBestAdjustedSpeed(events, songLength);

        std::cout << "Speed multiplier: " << speed << std::endl;

        for (auto& event : events) {
            event.seconds *= speed;
            event.noteEndSeconds *= speed;
        }
    }

public:
    FamiTrackerFile convert(HSTREAM handle) {
        std::vector<MidiEvent> events = MidiEventParser::getEvents(handle);
        double songLength = (events.empty() ? 0 : events.back().seconds);
        int speedDivider = getSpeedDivider(songLength);
		nesState.rowsPerSecond /= speedDivider;

        std::cout << "Scrolling speed: " << nesState.rowsPerSecond << " rows per second" << std::endl;

        // slightly adjusts playing speed to make distances between notes even
        if (ADJUST_SPEED) {
            adjustTempo(events, songLength);
        }

        instrumentSelector.preprocess(handle, events, file);

        file.comment = L"Created using MidiToFamiTrackerConverter by hakerg";
        track = file.addTrack(ROWS_PER_PATTERN);
        track->speed = speedDivider;
        track->tempo = 150;

        resetMidi();

        std::cout << "Channels assigned, processing events..." << std::endl;

        processEvents(handle, events);

        getCurrentCell(NesChannel::DPCM).Halt();

		std::cout << "Created " << track->patterns.size() << " patterns, " << file.instruments.size() << " instruments and " << file.dpcmSamples.size() << " DPCM samples" << std::endl;
        return file;
    }
};