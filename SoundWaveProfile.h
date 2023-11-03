#pragma once
#include "commons.h"
#include "Preset.h"
#include "NoteTriggerData.h"

class SoundWaveProfile {
private:
	static constexpr std::array<std::array<double, 8>, 8> similarityMatrix = {
		std::array<double, 8> { 1   , 0.95, 0.85,  0.55,  0   , 0   , 0   ,  0.80 }, // PULSE_12
		std::array<double, 8> { 0.95, 1   , 0.90,  0.60,  0   , 0   , 0   ,  0.75 }, // PULSE_25
		std::array<double, 8> { 0.85, 0.90, 1   ,  0.65,  0   , 0   , 0   ,  0.70 }, // PULSE_50

		std::array<double, 8> { 0.55, 0.60, 0.65,  1   ,  0   , 0   , 0   ,  0.50 }, // TRIANGLE

		std::array<double, 8> { 0   , 0   , 0   ,  0   ,  1   , 0.50, 0   ,  0    }, // NOISE_NORMAL
		std::array<double, 8> { 0   , 0   , 0   ,  0   ,  0.50, 1   , 0   ,  0    }, // NOISE_LOOP
		std::array<double, 8> { 0   , 0   , 0   ,  0   ,  0   , 0   , 1   ,  0    }, // DPCM

		std::array<double, 8> { 0.80, 0.75, 0.70,  0.50,  0   , 0   , 0   ,  1    }  // SAWTOOTH
	};

	int getIndex() const {
		switch (channel) {
		using enum Preset::Duty;
		using enum Preset::Channel;
		case PULSE:
			switch (duty) {
			case PULSE_12:
				return 0;
			case PULSE_25:
				return 1;
			case PULSE_50:
				return 2;
			default:
				return -1;
			}
		case TRIANGLE:
			return 3;
		case NOISE:
			return duty == NOISE_NORMAL ? 4 : 5;
		case DPCM:
			return 6;
		case SAWTOOTH:
			return 7;
		default:
			return -1;
		}
	}

public:
	Preset::Channel channel;
	Preset::Duty duty;

	SoundWaveProfile(Preset::Channel channel, Preset::Duty duty) : channel(channel), duty(duty) {}

	double getSimilarity(SoundWaveProfile const& other) const {
		int indexA = getIndex();
		int indexB = other.getIndex();
		if (indexA == -1 || indexB == -1) {
			return channel == other.channel && duty == other.duty ? 1 : 0; // it allows to have duty ANY
		}
		return similarityMatrix[indexA][indexB];
	}
};