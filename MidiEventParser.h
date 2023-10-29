#pragma once
#include "commons.h"
#include "MidiEvent.h"

class MidiEventParser {
private:
    static std::vector<BASS_MIDI_EVENT> getBassEvents(HSTREAM handle) {
        DWORD eventCount = BASS_MIDI_StreamGetEvents(handle, -1, 0, nullptr);
        std::vector<BASS_MIDI_EVENT> events(eventCount);
        BASS_MIDI_StreamGetEvents(handle, -1, 0, events.data());
        return events;
    }

    static std::vector<BASS_MIDI_MARK> getBassMarks(HSTREAM handle) {
        DWORD markCount = BASS_MIDI_StreamGetMarks(handle, -1, BASS_MIDI_MARK_MARKER, nullptr);
        std::vector<BASS_MIDI_MARK> marks(markCount);
        BASS_MIDI_StreamGetMarks(handle, -1, BASS_MIDI_MARK_MARKER, marks.data());
        return marks;
    }

public:
	static std::vector<MidiEvent> getEvents(HSTREAM handle) {
        std::vector<BASS_MIDI_EVENT> bassEvents = getBassEvents(handle);

        auto noteOnIndexesPtr = std::make_unique<std::array<std::array<std::vector<int>, 128>, 16>>();
        auto& noteOnIndexes = *noteOnIndexesPtr.get();

        std::vector<MidiEvent> results;
        for (auto& bassEvent : bassEvents) {
            MidiEvent event(handle, bassEvent);

            if (event.event == MIDI_EVENT_NOTE_ON) {
                noteOnIndexes[event.chan][event.key].push_back(int(results.size()));
            }
            else if (event.event == MIDI_EVENT_NOTE_OFF || event.event == MIDI_EVENT_NOTE_STOP) {
                for (int& index : noteOnIndexes[event.chan][event.key]) {
                    results[index].linkNoteEnd(event);
                }
                noteOnIndexes[event.chan][event.key].clear();
            }
            else if (event.event == MIDI_EVENT_NOTESOFF || event.event == MIDI_EVENT_SOUNDOFF) {
                for (int key = 0; key < 128; key++) {
                    for (int& index : noteOnIndexes[event.chan][key]) {
                        results[index].linkNoteEnd(event);
                    }
                    noteOnIndexes[event.chan][key].clear();
                }
            }

            results.push_back(event);
        }

        return results;
	}
};