#pragma once
#include "commons.h"

constexpr DWORD MIDI_EVENT_NOTE_ON   = 129;
constexpr DWORD MIDI_EVENT_NOTE_OFF  = 130;
constexpr DWORD MIDI_EVENT_NOTE_STOP = 131;

// it splits MIDI_EVENT_NOTE to one of the 3 above, uses time in seconds and contains info about note length
class MidiEvent {
public:
	DWORD event;
	DWORD param;
	DWORD chan;
	double seconds;

	// only for MIDI_EVENT_NOTE_ON
	double noteEndSeconds;

	// only for MIDI_EVENT_NOTE_ON | MIDI_EVENT_NOTE_OFF | MIDI_EVENT_NOTE_STOP
	int key = -1;

	// only for MIDI_EVENT_NOTE_ON
	int velocity = -1;

	MidiEvent(HSTREAM handle, BASS_MIDI_EVENT& bassEvent) : event(bassEvent.event), param(bassEvent.param), chan(bassEvent.chan),
		seconds(BASS_ChannelBytes2Seconds(handle, bassEvent.pos)), noteEndSeconds(seconds) {
		if (event == MIDI_EVENT_NOTE) {
			key = LOBYTE(param);
			velocity = HIBYTE(param);
			if (velocity == 0) {
				event = MIDI_EVENT_NOTE_OFF;
			}
			else if (velocity == 255) {
				event = MIDI_EVENT_NOTE_STOP;
			}
			else {
				event = MIDI_EVENT_NOTE_ON;
			}
		}
	}

	// only for MIDI_EVENT_NOTE_ON
	void linkNoteEnd(MidiEvent const& noteEnd) {
		noteEndSeconds = noteEnd.seconds;
	}
};