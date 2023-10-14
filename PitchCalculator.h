#pragma once
#include "commons.h"
#include "Cell.h"

class PitchCalculator {
public:

	static constexpr double NTSC_CPU_FREQUENCY = 1789773;
	static constexpr std::array<double, 16> ntscDmcPeriods = { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54 };

	static double calculateFrequencyByPeriod(NesChannel channel, double period) {
		switch (channel) {
		case NesChannel::PULSE1:
		case NesChannel::PULSE2:
		case NesChannel::PULSE3:
		case NesChannel::PULSE4:
			return NTSC_CPU_FREQUENCY / (16 * (period + 1));
		case NesChannel::TRIANGLE:
			return NTSC_CPU_FREQUENCY / (32 * (period + 1));
		case NesChannel::SAWTOOTH:
			return NTSC_CPU_FREQUENCY / (14 * (period + 1));
		default:
			return -1;
		}
	}

	static double calculatePeriodByFrequency(NesChannel channel, double frequency) {
		switch (channel) {
		case NesChannel::PULSE1:
		case NesChannel::PULSE2:
		case NesChannel::PULSE3:
		case NesChannel::PULSE4:
			return (NTSC_CPU_FREQUENCY / (16 * frequency)) - 1;
		case NesChannel::TRIANGLE:
			return (NTSC_CPU_FREQUENCY / (32 * frequency)) - 1;
		case NesChannel::SAWTOOTH:
			return (NTSC_CPU_FREQUENCY / (14 * frequency)) - 1;
		default:
			return -1;
		}
	}

	static double calculateFrequencyByKey(double midiKey) {
		return 440 * pow(2, (midiKey - 69) / 12.0);
	}

	static double calculateKeyByFrequency(double frequency) {
		return 69 + 12 * log2(frequency / 440);
	}

	static double calculatePeriodByKey(NesChannel channel, double midiKey) {
		double freq = calculateFrequencyByKey(midiKey);
		return calculatePeriodByFrequency(channel, freq);
	}

	static double calculateKeyByPeriod(NesChannel channel, double period) {
		double freq = calculateFrequencyByPeriod(channel, period);
		return calculateKeyByFrequency(freq);
	}

	static double calculatePitchedDmcKey(int key, int pitch) {
		return key + log2(ntscDmcPeriods[15] / ntscDmcPeriods[pitch]) * 12;
	}
};