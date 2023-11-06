#pragma once
#include "commons.h"
#include "Pattern.h"

class NesChannelSetIterator {
private:
	std::byte flags;
	int index;

	bool isValid() const {
		return (flags & std::byte(1 << index)) != std::byte(0);
	}

	void goToNext() {
		while (index < Pattern::CHANNELS && !isValid()) {
			index++;
		}
	}

public:
	NesChannelSetIterator(std::byte flags, int index) : flags(flags), index(index) {
		if (flags == std::byte(0)) {
			this->index = Pattern::CHANNELS;
		}
		else {
			goToNext();
		}
	}

	bool operator != (NesChannelSetIterator const& other) const {
		return index != other.index;
	}

	NesChannel operator * () const {
		return NesChannel(index);
	}

	NesChannelSetIterator& operator ++ () {
		index++;
		goToNext();
		return *this;
	}
};

class NesChannelSet {
public:
	std::byte flags{};

	NesChannelSet() = default;

	explicit NesChannelSet(std::unordered_set<NesChannel> const& nesChannels) {
		for (NesChannel nesChannel : nesChannels) {
			insert(nesChannel);
		}
	}

	bool contains(NesChannel nesChannel) const {
		return (flags & std::byte(1 << int(nesChannel))) != std::byte(0);
	}

	void insert(NesChannel nesChannel) {
		flags |= std::byte(1 << int(nesChannel));
	}

	void erase(NesChannel nesChannel) {
		flags &= std::byte(0xFF - (1 << int(nesChannel)));
	}

	bool empty() const {
		return flags == std::byte(0);
	}

	size_t size() const {
		return std::bitset<8>(int(flags)).count();
	}

	NesChannelSetIterator begin() const {
		return {flags, 0};
	}

	NesChannelSetIterator end() const {
		return {flags, Pattern::CHANNELS};
	}
};