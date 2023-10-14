#pragma once
#include "commons.h"
#include "Instrument.h"
#include "Pattern.h"
#include "Preset.h"

class NoteTriggerData {
public:
	NesChannel nesChannel = NesChannel(-1);
	Preset preset;
	NesDuty duty = NesDuty(-1);
	double midiScore = 0;

	NoteTriggerData() = default;

	NoteTriggerData(NesChannel nesChannel, Preset const& preset, double midiScore) :
		nesChannel(nesChannel), preset(preset), duty(convertDuty(preset.duty, nesChannel)), midiScore(midiScore) {}

	static NesDuty convertDuty(Preset::Duty duty, NesChannel nesChannel) {
		bool isVrc6 = (nesChannel == NesChannel::PULSE3 || nesChannel == NesChannel::PULSE4 || nesChannel == NesChannel::SAWTOOTH);
		switch (duty) {
		case Preset::Duty::PULSE_12:
			return isVrc6 ? NesDuty::VPULSE_12 : NesDuty::PULSE_12;
		case Preset::Duty::PULSE_25:
			return isVrc6 ? NesDuty::VPULSE_25 : NesDuty::PULSE_25;
		case Preset::Duty::PULSE_50:
			return isVrc6 ? NesDuty::VPULSE_50 : NesDuty::PULSE_50;
		case Preset::Duty::NOISE_NORMAL:
			return NesDuty::NOISE_NORMAL;
		case Preset::Duty::NOISE_LOOP:
			return NesDuty::NOISE_LOOP;
		default:
			return NesDuty(-1);
		}
	}
};