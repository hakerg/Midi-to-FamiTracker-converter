#pragma once
#include "commons.h"

enum class MacroType { VOLUME, ARPEGGIO, PITCH, HI_PITCH, DUTY };
enum class ArpeggioType { ABSOLUTE_, FIXED, RELATIVE_ };

enum class NesDuty {
	PULSE_12 = 0, PULSE_25 = 1, PULSE_50 = 2, PULSE_75 = 3,
	NOISE_NORMAL = 0, NOISE_LOOP = 1,
	VPULSE_06 = 0, VPULSE_12 = 1, VPULSE_18 = 2, VPULSE_25 = 3, VPULSE_31 = 4, VPULSE_37 = 5, VPULSE_43 = 6, VPULSE_50 = 7
};

template <MacroType type> class Macro {
private:
	NesDuty convertToVrc6Duty(NesDuty value) const {
		switch (value) {
		case NesDuty::PULSE_12:
			return NesDuty::VPULSE_12;
		case NesDuty::PULSE_25:
			return NesDuty::VPULSE_25;
		case NesDuty::PULSE_50:
			return NesDuty::VPULSE_50;
		case NesDuty::PULSE_75:
			return NesDuty::VPULSE_25;
		default:
			return value;
		}
	}

public:
	int id;
	int loopPosition;
	int releasePosition;
	ArpeggioType arpeggioType;
	std::vector<int> values;

	explicit Macro(int id, int loopPosition, int releasePosition, std::vector<int> const& values, ArpeggioType arpeggioType = ArpeggioType::ABSOLUTE_) :
		id(id), loopPosition(loopPosition), releasePosition(releasePosition), arpeggioType(arpeggioType), values(values) {}

	void exportTxt(std::wofstream& file) const {
		file << "MACRO " << int(type) << " " << id << " " << loopPosition << " " << releasePosition << " " << int(arpeggioType) << " :";
		for (int value : values) {
			file << " " << value;
		}
		file << std::endl;

		file << "MACROVRC6 " << int(type) << " " << id << " " << loopPosition << " " << releasePosition << " " << int(arpeggioType) << " :";
		for (int value : values) {
			file << " " << (type == MacroType::DUTY ? int(convertToVrc6Duty(NesDuty(value))) : value);
		}
		file << std::endl;
	}
};