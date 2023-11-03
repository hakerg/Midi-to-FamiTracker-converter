#pragma once
#include "commons.h"
#include "Instrument.h"
#include "Pattern.h"

class Preset {
public:
	enum class Channel { PULSE, TRIANGLE, NOISE, DPCM, SAWTOOTH };
	enum class Duty { ANY, PULSE_12, PULSE_25, PULSE_50, NOISE_NORMAL, NOISE_LOOP };

	Channel channel;
	std::shared_ptr<Instrument> instrument;
	bool needRelease;
	Duty duty;
	int drumKeyOrder;
	std::optional<Note> note;
	int uninterruptedTicks;

	Preset(Channel channel, std::shared_ptr<Instrument> instrument, bool needRelease, Duty duty = Duty::ANY) :
		channel(channel), instrument(instrument), needRelease(needRelease), duty(duty), drumKeyOrder(0), uninterruptedTicks(0) {}

	Preset(Channel channel, std::shared_ptr<Instrument> instrument, bool needRelease, Duty duty, int drumKeyOrder, std::optional<Note> note, int uninterruptedTicks = 0) :
		channel(channel), instrument(instrument), needRelease(needRelease), duty(duty), drumKeyOrder(drumKeyOrder), note(note), uninterruptedTicks(uninterruptedTicks) {}

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
};