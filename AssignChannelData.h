#pragma once
#include "commons.h"
#include "Preset.h"
#include "NoteTriggerData.h"

class AssignChannelData {
private:
	static std::bitset<int(NesChannel::CHANNEL_COUNT)> initBitset(std::initializer_list<NesChannel> const& nesChannels) {
		std::bitset<int(NesChannel::CHANNEL_COUNT)> ret;
		for (NesChannel nesChannel : nesChannels) {
			ret.set(int(nesChannel));
		}
		return ret;
	}

public:
	Preset::Duty duty = Preset::Duty::UNSPECIFIED;
	std::bitset<int(NesChannel::CHANNEL_COUNT)> nesChannels;

	AssignChannelData() = default;

	AssignChannelData(Preset::Duty duty, std::initializer_list<NesChannel> const& nesChannels) : duty(duty), nesChannels(initBitset(nesChannels)) {}

	std::vector<NoteTriggerData> getTriggers(Preset const& preset, bool lowerKeysFirst) const {
		std::vector<NoteTriggerData> results;
		forEachAssignedChannel([&results, &preset, lowerKeysFirst, this](NesChannel nesChannel) {
			results.emplace_back(nesChannel, duty, preset, lowerKeysFirst);
		});
		return results;
	}

	Preset::Channel getChannel() const {
		if (isAssigned(NesChannel::PULSE1) ||
			isAssigned(NesChannel::PULSE2) ||
			isAssigned(NesChannel::PULSE3) ||
			isAssigned(NesChannel::PULSE4)) {
			return Preset::Channel::PULSE;
		}
		if (isAssigned(NesChannel::TRIANGLE)) {
			return Preset::Channel::TRIANGLE;
		}
		if (isAssigned(NesChannel::NOISE)) {
			return Preset::Channel::NOISE;
		}
		if (isAssigned(NesChannel::DPCM)) {
			return Preset::Channel::DPCM;
		}
		if (isAssigned(NesChannel::SAWTOOTH)) {
			return Preset::Channel::SAWTOOTH;
		}
		return Preset::Channel(-1);
	}

	bool isAssigned(NesChannel nesChannel) const {
		return nesChannels.test(int(nesChannel));
	}

	bool operator == (const AssignChannelData& other) const = default;

	int getUniqueId() const {
		return int(nesChannels.to_ulong()) | (int(duty) << nesChannels.size());
	}

	template <typename Callable> void forEachAssignedChannel(Callable action) const {
		if (nesChannels.none()) {
			return;
		}
		for (size_t i = 0; i < nesChannels.size(); ++i) {
			if (nesChannels.test(i)) {
				action(NesChannel(i));
			}
		}
	}
};