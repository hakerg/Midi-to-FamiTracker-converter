#pragma once
#include "commons.h"
#include "Preset.h"
#include "NoteTriggerData.h"
#include "NesChannelSet.h"

class AssignChannelData {
public:
	Preset::Duty duty = Preset::Duty::ANY;
	NesChannelSet nesChannels;

	AssignChannelData() = default;

	AssignChannelData(Preset::Duty duty, std::unordered_set<NesChannel> const& nesChannels) : duty(duty), nesChannels(nesChannels) {}

	std::vector<NoteTriggerData> getTriggers(Preset const& preset) const {
		std::vector<NoteTriggerData> results;
		for (NesChannel nesChannel : nesChannels) {
			results.emplace_back(nesChannel, duty, preset);
		}
		return results;
	}

	Preset::Channel getChannel() const {
		switch (*nesChannels.begin()) {
		case NesChannel::PULSE1:
		case NesChannel::PULSE2:
		case NesChannel::PULSE3:
		case NesChannel::PULSE4:
			return Preset::Channel::PULSE;
		case NesChannel::TRIANGLE:
			return Preset::Channel::TRIANGLE;
		case NesChannel::NOISE:
			return Preset::Channel::NOISE;
		case NesChannel::DPCM:
			return Preset::Channel::DPCM;
		case NesChannel::SAWTOOTH:
			return Preset::Channel::SAWTOOTH;
		default:
			return Preset::Channel(-1);
		}
	}

	double getPoorFreqRangeChannelRatio() const {
		int count = 0;
		for (NesChannel nesChannel : nesChannels) {
			if (nesChannel == NesChannel::PULSE1 || nesChannel == NesChannel::PULSE2 || nesChannel == NesChannel::TRIANGLE) {
				count++;
			}
		}
		return count / double(nesChannels.size());
	}

	bool isAssigned(NesChannel nesChannel) const {
		return nesChannels.contains(nesChannel);
	}

	bool operator == (const AssignChannelData& other) const {
		return duty == other.duty && nesChannels.flags == other.nesChannels.flags;
	}

	int getUniqueId() const {
		return int(nesChannels.flags) | (int(duty) << Pattern::CHANNELS);
	}
};