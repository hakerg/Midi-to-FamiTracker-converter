#pragma once
#include "commons.h"
#include "Cell.h"
#include "AssignChannelData.h"
#include "MidiState.h"

class AssignData {
public:
	int eventIndex;
	std::array<AssignChannelData, MidiState::CHANNEL_COUNT> nesData;
	std::optional<double> score;

	explicit AssignData(int eventIndex) : eventIndex(eventIndex) {}

	const AssignChannelData& getNesData(int midiChannel) const {
		return nesData[midiChannel];
	}

	bool isAssigned(int midiChannel, NesChannel nesChannel) const {
		return getNesData(midiChannel).isAssigned(nesChannel);
	}

	void assign(int midiChannel, AssignChannelData const& data) {
		nesData[midiChannel] = data;
		score.reset();
	}

	void unassign(int midiChannel, NesChannel nesChannel) {
		nesData[midiChannel].nesChannels.erase(nesChannel);
		score.reset();
	}

	bool operator == (const AssignData& other) const {
		for (int i = 0; i < nesData.size(); i++) {
			if (!(nesData[i] == other.nesData[i])) {
				return false;
			}
		}
		return true;
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