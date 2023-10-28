#pragma once
#include "commons.h"

enum class NesChannel { PULSE1, PULSE2, TRIANGLE, NOISE, DPCM, PULSE3, PULSE4, SAWTOOTH };

enum class Tone { C_, CS, D_, DS, E_, F_, FS, G_, GS, A_, AS, B_ };

class Note {
public:
	static constexpr int MIN_KEY = 24;
	static constexpr int MAX_KEY = 119;
	int key;

	explicit Note(int key, bool bound = false) : key(bound ? max(MIN_KEY, min(MAX_KEY, key)) : key) {}

	Note(int octave, Tone tone) : Note(octave * 12 + 24 + int(tone)) {}

	int getExportOctave() const {
		return key / 12 - 2; // FamiTracker plays notes 2 octaves too high
	}

	int getExportTone() const {
		return key % 12;
	}

	std::wstring toNoteString() const {
		static const std::array<std::wstring, 12> tones = { L"C-", L"C#", L"D-", L"D#", L"E-", L"F-", L"F#", L"G-", L"G#", L"A-", L"A#", L"B-" };
		return tones[getExportTone()] + std::to_wstring(getExportOctave());
	}

	std::wstring toHexString() const {
		return hex1(key) + L"-#";
	}

	static bool isInPlayableRange(NesChannel nesChannel, int key) {
		// range for triangle is lowered because we play it octave higher
		static constexpr std::array<int, 8> MIN_PLAYABLE_KEY = { 33, 33, 21, 0, 24, 24, 24, 24 };
		static constexpr std::array<int, 8> MAX_PLAYABLE_KEY = { 119, 119, 107, 127, 119, 119, 119, 119 };
		return key >= MIN_PLAYABLE_KEY[int(nesChannel)] && key <= MAX_PLAYABLE_KEY[int(nesChannel)];
	}
};