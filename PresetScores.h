#pragma once
#include "commons.h"
#include "PresetSetup.h"

class PresetScores {
private:
	std::array<double, 8> dividers{};
	std::array<double, 8> scores{};

	int getBestIndex() {
		int bestIndex = 0;
		for (int i = 0; i < 8; i++) {
			if (scores[i] > scores[bestIndex]) {
				bestIndex = i;
			}
		}
		return bestIndex;
	}

	double getScoreDivider() {
		double divider = 0;
		for (int i = 0; i < 8; i++) {
			if (dividers[i] > divider) {
				divider = dividers[i];
			}
		}
		return divider;
	}

public:
	void addScore(PresetSetup const& setup, double value, double similarity) {
		int index = setup.getIndex();
		if (index >= 0) {
			dividers[index] += value;
			scores[index] += value * similarity;
		}
	}

	PresetSetup getBestSetup() {
		return PresetSetup(getBestIndex());
	}

	double getTotalScore() {
		double divider = getScoreDivider();
		double score = 0;
		for (int i = 0; i < 8; i++) {
			score += scores[i] / divider;
		}
		return score;
	}
};