#pragma once
#include "commons.h"

enum class Tone { C_, CS, D_, DS, E_, F_, FS, G_, GS, A_, AS, B_ };

class Note {
public:
	static const int MIN_KEY = 24;
	static const int MIN_PLAYABLE_KEY = 33;
	static const int MAX_KEY = 119;
	int key;

	explicit Note(int key, bool bound = false) : key(bound ? max(MIN_KEY, min(MAX_KEY, key)) : key) {}

	Note(int octave, Tone tone) : Note(octave * 12 + 24 + int(tone)) {}

	int getExportOctave() const {
		return key / 12 - 2;
	}

	int getExportTone() const {
		return key % 12;
	}

	std::wstring toNoteString() const {
		static const std::array<std::wstring, 12> tones = { L"C-", L"C#", L"D-", L"D#", L"E-", L"F-", L"F#", L"G-", L"G#", L"A-", L"A#", L"B-" };
		return tones[getExportTone()] + std::to_wstring(getExportOctave()); // FamiTracker plays notes 2 octaves too high
	}

	std::wstring toHexString() const {
		return hex1(key) + L"-#";
	}
};