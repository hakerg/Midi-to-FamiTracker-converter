#pragma once
#include "commons.h"
#include "Preset.h"

class PresetSetup {
private:
	static constexpr std::array<std::array<double, 8>, 8> similarityMatrix = {
		std::array<double, 8> { 1.0, 0.9, 0.7,  0.1,  0.0, 0.0, 0.0,  0.6 }, // PULSE_12
		std::array<double, 8> { 0.9, 1.0, 0.8,  0.2,  0.0, 0.0, 0.0,  0.5 }, // PULSE_25
		std::array<double, 8> { 0.7, 0.8, 1.0,  0.3,  0.0, 0.0, 0.0,  0.4 }, // PULSE_50

		std::array<double, 8> { 0.1, 0.2, 0.3,  1.0,  0.0, 0.0, 0.0,  0.05}, // TRIANGLE

		std::array<double, 8> { 0.0, 0.0, 0.0,  0.0,  1.0, 0.5, 0.0,  0.0 }, // NOISE_NORMAL
		std::array<double, 8> { 0.0, 0.0, 0.0,  0.0,  0.5, 1.0, 0.0,  0.0 }, // NOISE_LOOP
		std::array<double, 8> { 0.0, 0.0, 0.0,  0.0,  0.0, 0.0, 1.0,  0.0 }, // DPCM

		std::array<double, 8> { 0.6, 0.5, 0.4,  0.05, 0.0, 0.0, 0.0,  1.0 }  // SAWTOOTH
	};

public:
	Preset::Channel channel = Preset::Channel(-1);
	Preset::Duty duty = Preset::Duty(-1);

	explicit PresetSetup(Preset const& preset) : PresetSetup(preset.channel, preset.duty) {}

	PresetSetup(Preset::Channel channel, Preset::Duty duty) : channel(channel), duty(duty) {}

	explicit PresetSetup(int index) {
		switch (index) {
		case 0:
			channel = Preset::Channel::PULSE;
			duty = Preset::Duty::PULSE_12;
			break;
		case 1:
			channel = Preset::Channel::PULSE;
			duty = Preset::Duty::PULSE_25;
			break;
		case 2:
			channel = Preset::Channel::PULSE;
			duty = Preset::Duty::PULSE_50;
			break;
		case 3:
			channel = Preset::Channel::TRIANGLE;
			break;
		case 4:
			channel = Preset::Channel::NOISE;
			duty = Preset::Duty::NOISE_NORMAL;
			break;
		case 5:
			channel = Preset::Channel::NOISE;
			duty = Preset::Duty::NOISE_LOOP;
			break;
		case 6:
			channel = Preset::Channel::DPCM;
			break;
		case 7:
			channel = Preset::Channel::SAWTOOTH;
			break;
		default:
			break;
		}
	}

	int getIndex() const {
		switch (channel) {
		case Preset::Channel::PULSE:
			switch (duty) {
			case Preset::Duty::PULSE_12:
				return 0;
			case Preset::Duty::PULSE_25:
				return 1;
			case Preset::Duty::PULSE_50:
				return 2;
			default:
				return -1;
			}
		case Preset::Channel::TRIANGLE:
			return 3;
		case Preset::Channel::NOISE:
			return duty == Preset::Duty::NOISE_LOOP ? 5 : 4;
		case Preset::Channel::DPCM:
			return 6;
		case Preset::Channel::SAWTOOTH:
			return 7;
		default:
			return -1;
		}
	}

	double getSimilarity(PresetSetup const& other) const {
		int indexA = getIndex();
		int indexB = other.getIndex();
		if (indexA == -1 || indexB == -1) {
			return 0;
		}
		return similarityMatrix[indexA][indexB];
	}
};