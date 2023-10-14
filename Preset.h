#pragma once
#include "commons.h"
#include "Instrument.h"
#include "Pattern.h"

class Preset {
public:
	enum class Channel { PULSE, TRIANGLE, NOISE, DPCM, SAWTOOTH };
	enum class Duty { PULSE_12, PULSE_25, PULSE_50, NOISE_NORMAL, NOISE_LOOP };

	Channel channel = Channel(-1);
	std::shared_ptr<Instrument> instrument;
	Duty duty = Duty(-1);
	int drumKeyOrder = -1;
	Note note{};
	bool uninterrupted = false;

	Preset() = default;

	Preset(Channel channel, std::shared_ptr<Instrument> instrument, Duty duty = Duty(-1), int drumKeyOrder = -1, Note note = {}, bool uninterrupted = false) :
		channel(channel), instrument(instrument), duty(duty), drumKeyOrder(drumKeyOrder), note(note), uninterrupted(uninterrupted) {}

	std::vector<NesChannel> getValidNesChannels() const {
		switch (channel) {
		case Channel::PULSE:
			return { NesChannel::PULSE1, NesChannel::PULSE2, NesChannel::PULSE3, NesChannel::PULSE4 };
		case Channel::TRIANGLE:
			return { NesChannel::TRIANGLE };
		case Channel::NOISE:
			return { NesChannel::NOISE };
		case Channel::DPCM:
			return { NesChannel::DPCM };
		case Channel::SAWTOOTH:
			return { NesChannel::SAWTOOTH };
		default:
			return {};
		}
	}

	int getUninterruptedTicks() const {
		return uninterrupted && instrument && instrument->volumeMacro ? int(instrument->volumeMacro->values.size()) : 0;
	}
};