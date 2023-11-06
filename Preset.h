#pragma once
#include "commons.h"
#include "Instrument.h"
#include "Pattern.h"

class Preset {
public:
	enum class Channel { PULSE, TRIANGLE, NOISE, DPCM, SAWTOOTH };
	enum class Duty { UNSPECIFIED, PULSE_12, PULSE_25, PULSE_50, NOISE_NORMAL, NOISE_LOOP };

	enum class Order {
		MELODIC,

		CRASH1,
		CRASH2,
		CHINESE_CYMBAL,
		SPLASH,

		HIT,
		TIMBALE,
		TIMPANI,
		SNARE,
		CLAP,
		TOM,
		GUN,
		KICK,

		OPEN_HI_HAT,
		RIDE_CYMBAL,
		HI_HAT,
		
		STICK,
		TAIKO,
		AGOGO,
		TRIANGLE,
		TAMBOURINE,

		REVERSE_CYMBAL,

		SNARE_FILL,
		KICK_FILL
	};

	Channel channel;
	std::shared_ptr<Instrument> instrument;
	bool needRelease;
	Duty duty;
	Order order;
	std::optional<Note> note;
	int uninterruptedTicks;

	Preset(Channel channel, std::shared_ptr<Instrument> instrument, bool needRelease, Duty duty = Duty::UNSPECIFIED) :
		channel(channel), instrument(instrument), needRelease(needRelease), duty(duty), order(Order::MELODIC), uninterruptedTicks(0) {}

	Preset(Channel channel, std::shared_ptr<Instrument> instrument, bool needRelease, Duty duty, Order order, std::optional<Note> note, int uninterruptedTicks = 0) :
		channel(channel), instrument(instrument), needRelease(needRelease), duty(duty), order(order), note(note), uninterruptedTicks(uninterruptedTicks) {}

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

	bool isNonDecaying() const {
		if (!needRelease) {
			return false;
		}

		auto& volumeMacro = instrument->volumeMacro;
		auto loopIndex = int(volumeMacro->values.size() - 1);

		if (volumeMacro->loopPosition >= 0) {
			loopIndex = min(loopIndex, volumeMacro->loopPosition);
		}

		if (volumeMacro->releasePosition >= 0) {
			loopIndex = min(loopIndex, volumeMacro->releasePosition);
		}

		return volumeMacro->values[loopIndex] >= 2;
	}
};