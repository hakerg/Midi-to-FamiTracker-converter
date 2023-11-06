#pragma once
#include "commons.h"
#include "Instrument.h"
#include "Pattern.h"
#include "Preset.h"

class NoteTriggerData {
public:
	NesChannel nesChannel;
	Preset::Duty duty;
	Preset preset;

	NoteTriggerData(NesChannel nesChannel, Preset::Duty duty, Preset const& preset) :
		nesChannel(nesChannel), duty(duty), preset(preset) {}

	std::optional<NesDuty> getNesDuty() const {
		switch (duty) {
		case Preset::Duty::PULSE_12:
			return Cell::isVrc6(nesChannel) ? NesDuty::VPULSE_12 : NesDuty::PULSE_12;
		case Preset::Duty::PULSE_25:
			return Cell::isVrc6(nesChannel) ? NesDuty::VPULSE_25 : NesDuty::PULSE_25;
		case Preset::Duty::PULSE_50:
			return Cell::isVrc6(nesChannel) ? NesDuty::VPULSE_50 : NesDuty::PULSE_50;
		case Preset::Duty::NOISE_NORMAL:
			return NesDuty::NOISE_NORMAL;
		case Preset::Duty::NOISE_LOOP:
			return NesDuty::NOISE_LOOP;
		default:
			return {};
		}
	}
};