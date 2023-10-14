#pragma once
#include "commons.h"
#include "InstrumentBase.h"
#include "PresetStatistics.h"

class InstrumentBaseModifier {
private:
	static bool tryModifyOnce(PresetStatistics& stats, InstrumentBase& base, std::array<double, 128>& similarities) {
		PresetScores scores = stats.getPresetsScores(base, similarities);
		PresetSetup setupToChange = scores.getBestSetup();
		std::vector<int> programs = stats.getPrograms(base, setupToChange);

		double bestScore = scores.getTotalScore();
		std::cout << "Instruments score: " << bestScore << std::endl;

		PresetSetup bestSetup = setupToChange;
		int bestProgram = -1;
		for (int program : programs) {
			for (int i = 0; i < 8; i++) {
				auto newSetup = PresetSetup(i);

				base.updatePreset(program, newSetup);
				double oldSimilarity = similarities[program];
				similarities[program] *= setupToChange.getSimilarity(newSetup);

				PresetScores newScores = stats.getPresetsScores(base, similarities);
				double newScore = newScores.getTotalScore();

				base.updatePreset(program, setupToChange);
				similarities[program] = oldSimilarity;

				if (newScore > bestScore) {
					bestScore = newScore;
					bestSetup = newSetup;
					bestProgram = program;
				}
			}
		}

		if (bestProgram >= 0) {
			base.updatePreset(bestProgram, bestSetup);
			similarities[bestProgram] = setupToChange.getSimilarity(bestSetup);
			std::cout << "Change setup for program " << bestProgram << ", similarity: " << similarities[bestProgram] << std::endl;
			return true;
		}
		else {
			return false;
		}
	}

public:
	static void tryModifyInstrumentBase(PresetStatistics& stats, InstrumentBase& base) {
		std::array<double, 128> similarities{};
		similarities.fill(1);

		while (tryModifyOnce(stats, base, similarities));
	}
};