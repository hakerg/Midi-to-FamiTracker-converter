#pragma once

class NesHeightVolumeController {
private:
	static constexpr double SPLIT_POINT = 72;

public:
	static double getHeightVolumeMultiplier(double key) {
		if (key <= SPLIT_POINT) {
			return 1;
		}
		double a = -1.0 / (128 - SPLIT_POINT);
		double b = 1.0 - a * SPLIT_POINT;
		return max(0, a * key + b);
	}
};