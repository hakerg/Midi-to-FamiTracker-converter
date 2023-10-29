#pragma once
#include "commons.h"
#include "MidiChannelState.h"
#include "MidiEvent.h"

class MidiState {
private:
    void noteOn(int chan, int key, int velocity) {
        getChannel(chan).noteVelocities.insert(std::pair<int, int>(key, velocity));
    }

    void noteOff(int chan, int key) {
        getChannel(chan).noteVelocities.erase(key);
    }

    void setProgram(int chan, int preset) {
        getChannel(chan).program = preset;
    }

    void setPitch(int chan, double semitones) {
        getChannel(chan).pitch = semitones;
    }

    void setBank(int chan, int bank) {
        getChannel(chan).bank = bank;
    }

    void setVolume(int chan, double volume) {
        getChannel(chan).volume = volume;
    }

    void setExpression(int chan, double volume) {
        getChannel(chan).expression = volume;
    }

    void stopAllNotes(int chan) {
        getChannel(chan).stopAllNotes();
    }

    void resetControllers(int chan) {
        getChannel(chan).expression = 1;
        getChannel(chan).pitch = 0;
    }

    void setPitchRange(int chan, int semitones) {
        getChannel(chan).pitchRange = semitones;
    }

    void setFineTune(int chan, double semitones) {
        getChannel(chan).fineTune = semitones;
    }

    void setCoarseTune(int chan, int semitones) {
        getChannel(chan).coarseTune = semitones;
    }

    void useDrums(int chan, bool useDrums) {
        getChannel(chan).useDrums = useDrums;
    }

    void resetMidi() {
        setTempo(120);
        for (int i = 0; i < 16; i++) {
            getChannel(i) = MidiChannelState();
        }
        getChannel(9).useDrums = true;
    }

    void setTempo(double bpm_) {
        bpm = bpm_;
    }

public:
	std::array<MidiChannelState, 16> channels;
	double bpm = 120;

    MidiState() {
        resetMidi();
    }

	MidiChannelState& getChannel(int channel) {
		return channels[channel];
	}

	const MidiChannelState& getChannel(int channel) const {
		return channels[channel];
	}

	void processEvent(const MidiEvent& event) {
        switch (event.event) {
        case MIDI_EVENT_NOTE_ON:
            noteOn(event.chan, event.key, event.velocity);
            break;
        case MIDI_EVENT_NOTE_OFF:
        case MIDI_EVENT_NOTE_STOP:
            noteOff(event.chan, event.key);
            break;
        case MIDI_EVENT_PROGRAM:
            setProgram(event.chan, event.param);
            break;
        case MIDI_EVENT_PITCH:
            setPitch(event.chan, event.param / 8192.0 - 1);
            break;
        case MIDI_EVENT_BANK:
            setBank(event.chan, event.param);
            break;
        case MIDI_EVENT_VOLUME:
            setVolume(event.chan, event.param / 127.0);
            break;
        case MIDI_EVENT_EXPRESSION:
            setExpression(event.chan, event.param / 127.0);
            break;
        case MIDI_EVENT_SOUNDOFF:
        case MIDI_EVENT_NOTESOFF:
            stopAllNotes(event.chan);
            break;
        case MIDI_EVENT_RESET:
            resetControllers(event.chan);
            break;
        case MIDI_EVENT_PITCHRANGE:
            setPitchRange(event.chan, event.param);
            break;
        case MIDI_EVENT_FINETUNE:
            setFineTune(event.chan, event.param / 8192.0 - 1);
            break;
        case MIDI_EVENT_COARSETUNE:
            setCoarseTune(event.chan, event.param - 64);
            break;
        case MIDI_EVENT_DRUMS:
            useDrums(event.chan, event.param >= 1);
            break;

        case MIDI_EVENT_SYSTEM:
        case MIDI_EVENT_SYSTEMEX:
            resetMidi();
            break;
        case MIDI_EVENT_TEMPO:
            setTempo(calculateBPM(event.param));
            break;

        default:
            break;
        }
	}

    static double calculateBPM(int microsPerQuarter) {
        return 60000000.0 / microsPerQuarter;
    }
};