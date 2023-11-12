#pragma once
#include "commons.h"
#include "Note.h"
#include "Effect.h"
#include "Instrument.h"

class Cell {
private:
	std::wstring getInstrumentText(NesChannel channel) const {
		if (instrument) {
			return hex2(isVrc6(channel) ? instrument->getVrc6Id() : instrument->getNesId());
		}
		else {
			return L"..";
		}
	}

public:

	enum class Type { EMPTY, NOTE, STOP, RELEASE };

	Type type;
	std::optional<Note> note;
	std::shared_ptr<Instrument> instrument;
	int volume = -1;
	std::vector<Effect> effects{};

	explicit Cell(Type type = Type::EMPTY) : type(type) {}

	static bool isVrc6(NesChannel channel) {
		using enum NesChannel;
		return channel == PULSE3 || channel == PULSE4 || channel == SAWTOOTH;
	}

	void Note(Note note_, std::shared_ptr<Instrument> instrument_, int volume_ = -1) {
		type = Type::NOTE;
		note = note_;
		instrument = instrument_;
		volume = volume_;
	}

	void exportTxt(std::wofstream& file, int columnSize, NesChannel channel) const {
		switch (type) {
		using enum Cell::Type;
		case EMPTY:
			file << "...";
			break;
		case NOTE:
			file << (channel == NesChannel::NOISE ? note->toHexString() : note->toNoteString());
			break;
		case STOP:
			file << "---";
			break;
		case RELEASE:
			file << "===";
			break;
		}
		file << " " << getInstrumentText(channel) << " " << (volume == -1 ? L"." : hex1(volume));
		for (int i = 0; i < columnSize; i++) {
			file << " " << (i >= effects.size() ? L"..." : effects[i].string);
		}
	}

	void addEffect(std::wstring string) {
		// speed/tempo effect can be duplicated to set them both
		if (string[0] != L'F') {
			for (auto& effect : effects) {
				if (effect.string[0] == string[0]) {
					effect.string = string;
					return;
				}
			}
		}
		effects.emplace_back(string);
	}

	// Plays 3 notes alternately: base, base + semitones1, base + semitones2
	void Arpeggio(int semitones1, int semitones2) {
		addEffect(std::wstring(L"0") + hex1(semitones1) + hex1(semitones2));
	}

	// Continuously slides the pitch up
	void SlideUp(int pitchUnitsPerTick) {
		addEffect(std::wstring(L"1") + hex2(pitchUnitsPerTick));
	}

	// Continuously slides the pitch down
	void SlideDown(int pitchUnitsPerTick) {
		addEffect(std::wstring(L"2") + hex2(pitchUnitsPerTick));
	}

	// Automatically slides to new notes
	void Portamento(int pitchUnitsPerTick) {
		addEffect(std::wstring(L"3") + hex2(pitchUnitsPerTick));
	}

	// Sine vibrato
	void Vibrato(int speed, int depth) {
		addEffect(std::wstring(L"4") + hex1(speed) + hex1(depth));
	}

	// Sine tremolo
	void Tremolo(int speed, int depth) {
		addEffect(std::wstring(L"7") + hex1(speed) + hex1(depth));
	}

	// Affects volume as fractions of 8
	void VolumeSlideUp(int slideUp) {
		addEffect(std::wstring(L"A") + hex2(slideUp));
	}

	// Affects volume as fractions of 8
	void VolumeSlideDown(int slideDown) {
		addEffect(std::wstring(L"A") + hex2(slideDown * 16));
	}

	// Jump to order
	void Jump(int order) {
		addEffect(std::wstring(L"B") + hex2(order));
	}

	// Halt playback
	void Halt() {
		addEffect(std::wstring(L"C00"));
	}

	// Skip to next frame and jump to given row
	void Skip(int row) {
		addEffect(std::wstring(L"D") + hex2(row));
	}

	// 01 - 1F sets speed, 20 - FF sets tempo
	void SpeedAndTempo(int speed, int tempo) {
		std::erase_if(effects, [](Effect& effect) { return effect.string[0] == L'F'; });
		addEffect(std::wstring(L"F") + hex2(speed));
		addEffect(std::wstring(L"F") + hex2(tempo));
	}

	// 01 - 1F sets speed, 20 - FF sets tempo
	void SpeedOrTempo(int value) {
		std::erase_if(effects, [](Effect& effect) { return effect.string[0] == L'F'; });
		addEffect(std::wstring(L"F") + hex2(value));
	}

	// Delays current cell
	void Delay(int ticks) {
		addEffect(std::wstring(L"G") + hex2(ticks));
	}

	// Hardware sweep up
	void SweepUp(int period, int shiftValue) {
		addEffect(std::wstring(L"H") + hex1(period) + hex1(shiftValue));
	}

	// Hardware sweep down
	void SweepDown(int period, int shiftValue) {
		addEffect(std::wstring(L"I") + hex1(period) + hex1(shiftValue));
	}

	// 80 is in tune
	void FinePitch(int pitchUnits) {
		addEffect(std::wstring(L"P") + hex2(pitchUnits));
	}

	// Targeted note slide up
	void NoteSlideUp(int speed, int semitonesUp) {
		addEffect(std::wstring(L"Q") + hex1(speed) + hex1(semitonesUp));
	}

	// Targeted note slide down
	void NoteSlideDown(int speed, int semitonesDown) {
		addEffect(std::wstring(L"R") + hex1(speed) + hex1(semitonesDown));
	}

	// Cuts the active note after given number of ticks
	void DelayedCut(int ticks) {
		addEffect(std::wstring(L"S") + hex2(ticks));
	}

	// Pulse: 0-3, Noise: 0-1
	void Duty(NesDuty duty) {
		addEffect(std::wstring(L"V") + hex2(int(duty)));
	}

	// DPCM pitch override
	void PitchDpcm(int pitch) {
		addEffect(std::wstring(L"W") + hex2(pitch));
	}

	// Retrigger the DPCM sample with the duration of given ticks
	void Retrigger(int ticks) {
		addEffect(std::wstring(L"X") + hex2(ticks));
	}

	// Sample start offset multiplied by 64
	void SampleOffset(int offset) {
		addEffect(std::wstring(L"Y") + hex2(offset));
	}

	// Controls the DPCM delta counter
	void DeltaCounter(int value) {
		addEffect(std::wstring(L"Z") + hex2(value));
	}
};