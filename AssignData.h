#pragma once
#include "commons.h"
#include "Cell.h"
#include "AssignChannelData.h"
#include "MidiState.h"

class AssignData {
private:
	friend struct std::hash<AssignData>;

	std::array<AssignChannelData, MidiState::CHANNEL_COUNT> nesData;

public:
	explicit AssignData(std::array<AssignChannelData, MidiState::CHANNEL_COUNT> const& nesData) : nesData(nesData) {}

	const AssignChannelData& getNesData(int midiChannel) const {
		return nesData[midiChannel];
	}

	bool isAssigned(int midiChannel, NesChannel nesChannel) const {
		return getNesData(midiChannel).isAssigned(nesChannel);
	}

	void assign(int midiChannel, AssignChannelData const& data) {
		nesData[midiChannel] = data;
	}

	void reset(int midiChannel) {
		nesData[midiChannel] = AssignChannelData();
	}

	void unassign(int midiChannel, NesChannel nesChannel) {
		nesData[midiChannel].nesChannels.reset(int(nesChannel));
	}

	void setDuty(int midiChannel, Preset::Duty duty) {
		nesData[midiChannel].duty = duty;
	}

	bool operator == (const AssignData& other) const {
		for (int i = 0; i < nesData.size(); i++) {
			if (!(nesData[i] == other.nesData[i])) {
				return false;
			}
		}
		return true;
	}

	int countPulseDuty(Preset::Duty duty) const {
		int count = 0;
		for (auto& channelData : nesData) {
			if (channelData.duty == duty && channelData.getChannel() == Preset::Channel::PULSE) {
				count += int(channelData.nesChannels.count());
			}
		}
		return count;
	}
};

namespace std {
	template<> struct hash<AssignData> {
		std::size_t operator () (const AssignData& data) const {
			std::size_t h = 0;
			std::hash<int> hasher;
			for (const auto& nesData : data.nesData) {
				h ^= hasher(nesData.getUniqueId()) + 0x9e3779b9 + (h << 6) + (h >> 2);
			}
			return h;
		}
	};
}