#pragma once
#include "commons.h"
#include "Instrument.h"
#include "Preset.h"
#include "FamiTrackerFile.h"
#include "SampleBase.h"
#include "PitchCalculator.h"

class InstrumentBase {
private:

	class Dpcm {
	private:
		void fillSampleByLoweringPitch(std::shared_ptr<Instrument> instrument, int key, int lowestKey, double scaleTuning) {
			double targetKey = lowestKey + (double(key) - lowestKey) * scaleTuning;

			int bestIndex = 0;
			int bestPitch = 0;
			double bestScore = -999999;

			for (int i = 0; i < instrument->dpcmSamples.size(); i++) {
				KeyDpcmSample const& sample = instrument->dpcmSamples[i];
				if (sample.note.key < lowestKey) {
					continue;
				}
				for (int pitch = 0; pitch <= sample.pitch; pitch++) {
					double calculatedKey = PitchCalculator::calculatePitchedDmcKey(sample.note.key, pitch);
					double score = (double(pitch) - sample.pitch) * 0.25 - std::abs(targetKey - calculatedKey);
					if (score > bestScore) {
						bestIndex = i;
						bestPitch = pitch;
						bestScore = score;
					}
				}
			}

			KeyDpcmSample sample = instrument->dpcmSamples[bestIndex];
			sample.note = Note(key);
			sample.pitch = bestPitch;
			instrument->dpcmSamples.push_back(sample);
		}

		KeyDpcmSample& findSample(std::vector<KeyDpcmSample>& dpcmSamples, int key) {
			for (auto& sample : dpcmSamples) {
				if (sample.note.key == key) {
					return sample;
				}
			}
			return dpcmSamples[0];
		}

		// needs at least 5 samples in a row
		void fillSamplesByLoweringPitch(std::shared_ptr<Instrument> instrument, double scaleTuning) {
			int lowestKey = Note::MAX_KEY;
			int highestKey = Note::MIN_KEY;

			for (auto& sample : instrument->dpcmSamples) {
				lowestKey = min(lowestKey, sample.note.key);
				highestKey = max(highestKey, sample.note.key);
			}

			// fill lower by changing pitch
			for (int i = lowestKey - 1; i >= Note::MIN_KEY; i--) {
				fillSampleByLoweringPitch(instrument, i, lowestKey, scaleTuning);
			}

			// fill higher by looping octaves
			for (int i = highestKey + 1; i <= Note::MAX_KEY; i++) {
				KeyDpcmSample sample = findSample(instrument->dpcmSamples, i - 12);
				sample.note = Note(i);
				instrument->dpcmSamples.push_back(sample);
			}
		}

		void fillTimpani() {
			int firstKey = Note(0, Tone::A_).key;
			for (int i = 0; i < samples.timpaniA_1.size(); i++) {
				timpani->dpcmSamples.emplace_back(Note(firstKey + i, true), samples.timpaniA_1[i]);
			}

			fillSamplesByLoweringPitch(timpani, 1);
		}

		std::shared_ptr<Instrument> createSingleSampleInstrument(FamiTrackerFile& file, const std::wstring& name, std::shared_ptr<DpcmSample> sample, double scaleTuning) {
			std::shared_ptr<Instrument> instrument = file.addInstrument(name);
			instrument->dpcmSamples.emplace_back(Note(60), sample);
			fillSamplesByLoweringPitch(instrument, scaleTuning);
			return instrument;
		}

	public:
		SampleBase samples;
		std::shared_ptr<Instrument> hit;
		std::shared_ptr<Instrument> timpani;
		std::shared_ptr<Instrument> synthDrum;
		std::shared_ptr<Instrument> melodicTom;
		std::shared_ptr<Instrument> woodblock;
		std::shared_ptr<Instrument> taiko;
		std::shared_ptr<Instrument> standardDrums;
		std::shared_ptr<Instrument> hardDrums;
		std::shared_ptr<Instrument> analogDrums;
		std::shared_ptr<Instrument> orchestraDrums;

		void createInstruments(FamiTrackerFile& file) {
			hit = createSingleSampleInstrument(file, L"Orchestra hit", samples.hit, 0);
			synthDrum = createSingleSampleInstrument(file, L"Synth drum", samples.synthDrum, 0.5);
			melodicTom = createSingleSampleInstrument(file, L"Melodic tom", samples.tom, 0.5);
			woodblock = createSingleSampleInstrument(file, L"Woodblock", samples.stick, 0.5);
			taiko = createSingleSampleInstrument(file, L"Taiko", samples.taiko, 0.5);

			timpani = file.addInstrument(L"Timpani");
			fillTimpani();

			standardDrums = file.addInstrument(L"Standard drums");
			hardDrums = file.addInstrument(L"Hard drums");
			analogDrums = file.addInstrument(L"Analog drums");
			orchestraDrums = file.addInstrument(L"Orchestra drums");
		}
	};

	Dpcm dpcm;

	std::array<std::optional<Preset>, 128> gm;
	std::unordered_map<int, std::array<std::vector<Preset>, 128>> drums{};

	std::vector<int> createDecayValues(int start, int last, int delta, int period) {
		std::vector<int> values;
		for (int i = start; i != last + delta; i += delta) {
			for (int p = 0; p < period; p++) {
				values.push_back(i);
			}
		}
		return values;
	}

	void fillInstruments(FamiTrackerFile& file) {
		using enum Preset::Channel;
		using enum Preset::Duty;
		auto loopMacro = file.addVolumeMacro({ 15, 0 }, -1, 0);

		auto fastDecayMacro = file.addVolumeMacro({ 12, 9, 6, 3, 1, 0 });
		auto decayMacro = file.addVolumeMacro(createDecayValues(15, 0, -1, 1));
		auto longDecayMacro = file.addVolumeMacro(createDecayValues(15, 0, -1, 2));
		auto pianoMacro = file.addVolumeMacro(createDecayValues(15, 0, -1, 6));

		auto longDecayMacroCut = file.addVolumeMacro(createDecayValues(15, 0, -1, 2), -1, 30);
		auto pianoMacroCut = file.addVolumeMacro(createDecayValues(15, 0, -1, 6), -1, 90);

		auto attackMacro = file.addVolumeMacro({ 3, 7, 11, 15, 0 }, -1, 3);
		auto stringsMacro = file.addVolumeMacro({ 3, 7, 11, 15, 5, 5, 5, 5, 5, 5, 5, 5, 0 }, -1, 3);
		auto openHatVolumeMacro = file.addVolumeMacro({ 15, 14, 12, 11, 10, 11, 12, 13, 13, 12, 11, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 });
		auto clapVolumeMacro = file.addVolumeMacro({ 11, 7, 12, 8, 13, 8, 6, 4, 2, 0 });
		auto analogOpenHatVolumeMacro = file.addVolumeMacro({ 12, 12, 12, 12, 12, 0 });
		auto reverseCymbalValues = createDecayValues(1, 10, 1, 2);
		reverseCymbalValues.push_back(0);
		auto reverseCymbalMacro = file.addVolumeMacro(reverseCymbalValues, -1, 19);

		auto snareArpeggioMacro = file.addArpeggioMacro({ -5, 2, 0 }, ArpeggioType::ABSOLUTE_);
		auto openHatArpeggioMacro = file.addArpeggioMacro({ 2, 0, 2, 3, 4 }, ArpeggioType::ABSOLUTE_);
		auto crashArpeggioMacro = file.addArpeggioMacro({ -4, 2, -6, -2, 0 }, ArpeggioType::ABSOLUTE_);
		auto clapArpeggioMacro = file.addArpeggioMacro({ -3, -6, -2, -6, -1 }, ArpeggioType::ABSOLUTE_);
		auto hardSnareMacro = file.addArpeggioMacro({ 0, 0, 0, 1, 1, 1, 2 }, ArpeggioType::ABSOLUTE_);

		auto vibratoMacro = file.addPitchMacro({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 1, 1, 1, 1 }, 15);

		auto pluckDutyMacro = file.addDutyMacro({ 2, 1, 0, 1 });
		auto clapDutyMacro = file.addDutyMacro({ 0, 1, 0, 1, 0 });

		std::shared_ptr<Instrument> loop = file.addInstrument(L"Loop", loopMacro);
		std::shared_ptr<Instrument> loopVibrato = file.addInstrument(L"Loop vibrato", loopMacro, {}, vibratoMacro);
		
		std::shared_ptr<Instrument> fastDecay = file.addInstrument(L"Fast decay", fastDecayMacro);
		std::shared_ptr<Instrument> decay = file.addInstrument(L"Decay", decayMacro);
		std::shared_ptr<Instrument> longDecayCut = file.addInstrument(L"Long decay cut", longDecayMacroCut);
		std::shared_ptr<Instrument> pianoCut = file.addInstrument(L"Piano cut", pianoMacroCut);
		
		std::shared_ptr<Instrument> attack = file.addInstrument(L"Attack", attackMacro);
		std::shared_ptr<Instrument> strings = file.addInstrument(L"Strings", stringsMacro, {}, vibratoMacro);
		std::shared_ptr<Instrument> pad = file.addInstrument(L"Pad", stringsMacro);
		std::shared_ptr<Instrument> guitarDistortion = file.addInstrument(L"Guitar distortion", loopMacro, {}, {}, {}, pluckDutyMacro);
		std::shared_ptr<Instrument> snare = file.addInstrument(L"Snare", fastDecayMacro, snareArpeggioMacro);
		std::shared_ptr<Instrument> openHat = file.addInstrument(L"Open hi-hat", openHatVolumeMacro, openHatArpeggioMacro);
		std::shared_ptr<Instrument> analogOpenHat = file.addInstrument(L"Analog open hi-hat", analogOpenHatVolumeMacro);
		std::shared_ptr<Instrument> crash = file.addInstrument(L"Crash", longDecayMacro, crashArpeggioMacro);
		std::shared_ptr<Instrument> clap = file.addInstrument(L"Clap", clapVolumeMacro, clapArpeggioMacro, {}, {}, clapDutyMacro);
		std::shared_ptr<Instrument> hardSnare = file.addInstrument(L"Hard snare", decayMacro, hardSnareMacro);
		std::shared_ptr<Instrument> reverseCymbal = file.addInstrument(L"Reverse cymbal", reverseCymbalMacro);

		dpcm.createInstruments(file);



		// piano
		gm[0] = { Preset(PULSE, pianoCut, true, PULSE_50) };
		gm[1] = { Preset(PULSE, pianoCut, true, PULSE_50) };
		gm[2] = { Preset(PULSE, pianoCut, true, PULSE_50) };
		gm[3] = { Preset(PULSE, pianoCut, true, PULSE_50) };
		gm[4] = { Preset(PULSE, pianoCut, true, PULSE_50) };
		gm[5] = { Preset(PULSE, pianoCut, true, PULSE_50) };
		gm[6] = { Preset(PULSE, pianoCut, true, PULSE_12) };
		gm[7] = { Preset(PULSE, pianoCut, true, PULSE_12) };

		// chromatic percussion
		gm[8] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[9] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[10] = { Preset(PULSE, longDecayCut, false, PULSE_50) };
		gm[11] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[12] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[13] = { Preset(PULSE, longDecayCut, false, PULSE_50) };
		gm[14] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[15] = { Preset(PULSE, pianoCut, false, PULSE_12) };

		// organ
		gm[16] = { Preset(PULSE, attack, true, PULSE_25) };
		gm[17] = { Preset(PULSE, attack, true, PULSE_25) };
		gm[18] = { Preset(PULSE, attack, true, PULSE_25) };
		gm[19] = { Preset(PULSE, attack, true, PULSE_25) };
		gm[20] = { Preset(PULSE, attack, true, PULSE_25) };
		gm[21] = { Preset(PULSE, attack, true, PULSE_25) };
		gm[22] = { Preset(PULSE, attack, true, PULSE_25) };
		gm[23] = { Preset(PULSE, attack, true, PULSE_25) };

		// guitar
		gm[24] = { Preset(PULSE, pianoCut, true, PULSE_25) };
		gm[25] = { Preset(PULSE, pianoCut, true, PULSE_25) };
		gm[26] = { Preset(PULSE, pianoCut, true, PULSE_25) };
		gm[27] = { Preset(PULSE, pianoCut, true, PULSE_25) };
		gm[28] = { Preset(PULSE, longDecayCut, true, PULSE_25) };
		gm[29] = { Preset(PULSE, guitarDistortion, true) };
		gm[30] = { Preset(PULSE, guitarDistortion, true) };
		gm[31] = { Preset(PULSE, guitarDistortion, true) };

		// bass
		gm[32] = { Preset(TRIANGLE, loop, true) };
		gm[33] = { Preset(TRIANGLE, loop, true) };
		gm[34] = { Preset(TRIANGLE, loop, true) };
		gm[35] = { Preset(TRIANGLE, loop, true) };
		gm[36] = { Preset(TRIANGLE, loop, true) };
		gm[37] = { Preset(TRIANGLE, loop, true) };
		gm[38] = { Preset(TRIANGLE, loop, true) };
		gm[39] = { Preset(TRIANGLE, loop, true) };

		// strings
		gm[40] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[41] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[42] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[43] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[44] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[45] = { Preset(PULSE, decay, false, PULSE_50) };
		gm[46] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[47] = { Preset(DPCM, dpcm.timpani, false) };

		// ensemble
		gm[48] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[49] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[50] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[51] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[52] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[53] = { Preset(PULSE, loop, true, PULSE_50) };
		gm[54] = { Preset(PULSE, loop, true, PULSE_50) };
		gm[55] = { Preset(DPCM, dpcm.hit, false) };

		// brass
		gm[56] = { Preset(PULSE, loop, true, PULSE_12) };
		gm[57] = { Preset(PULSE, loop, true, PULSE_12) };
		gm[58] = { Preset(PULSE, loop, true, PULSE_12) };
		gm[59] = { Preset(PULSE, loop, true, PULSE_12) };
		gm[60] = { Preset(PULSE, loop, true, PULSE_12) };
		gm[61] = { Preset(PULSE, loop, true, PULSE_12) };
		gm[62] = { Preset(PULSE, loop, true, PULSE_12) };
		gm[63] = { Preset(PULSE, loop, true, PULSE_12) };

		// reed
		gm[64] = { Preset(PULSE, loop, true, PULSE_25) };
		gm[65] = { Preset(PULSE, loop, true, PULSE_25) };
		gm[66] = { Preset(PULSE, loop, true, PULSE_25) };
		gm[67] = { Preset(PULSE, loop, true, PULSE_25) };
		gm[68] = { Preset(PULSE, loopVibrato, true, PULSE_25) };
		gm[69] = { Preset(PULSE, loopVibrato, true, PULSE_25) };
		gm[70] = { Preset(PULSE, loopVibrato, true, PULSE_25) };
		gm[71] = { Preset(PULSE, loop, true, PULSE_25) };

		// pipe
		gm[72] = { Preset(PULSE, loopVibrato, true, PULSE_50) };
		gm[73] = { Preset(PULSE, loopVibrato, true, PULSE_50) };
		gm[74] = { Preset(PULSE, loopVibrato, true, PULSE_50) };
		gm[75] = { Preset(PULSE, loopVibrato, true, PULSE_50) };
		gm[76] = { Preset(PULSE, loopVibrato, true, PULSE_50) };
		gm[77] = { Preset(PULSE, loopVibrato, true, PULSE_50) };
		gm[78] = { Preset(PULSE, loopVibrato, true, PULSE_50) };
		gm[79] = { Preset(PULSE, loopVibrato, true, PULSE_50) };

		// synth lead
		gm[80] = { Preset(PULSE, loop, true, PULSE_50) };
		gm[81] = { Preset(SAWTOOTH, loop, true) };
		gm[82] = { Preset(PULSE, loop, true, PULSE_50) };
		gm[83] = { Preset(PULSE, loop, true, PULSE_50) };
		gm[84] = { Preset(PULSE, loop, true, PULSE_25) };
		gm[85] = { Preset(PULSE, loop, true, PULSE_50) };
		gm[86] = { Preset(PULSE, loop, true, PULSE_25) };
		gm[87] = { Preset(PULSE, loop, true, PULSE_12) };

		// synth pad
		gm[88] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[89] = { Preset(TRIANGLE, pad, true) };
		gm[90] = { Preset(PULSE, loop, true, PULSE_25) };
		gm[91] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[92] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[93] = { Preset(SAWTOOTH, pad, true) };
		gm[94] = { Preset(PULSE, pad, true, PULSE_50) };
		gm[95] = { Preset(TRIANGLE, pad, true) };

		// synth effects
		gm[96] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[97] = { Preset(PULSE, pad, true, PULSE_25) };
		gm[98] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[99] = { Preset(PULSE, pianoCut, false, PULSE_25) };
		gm[100] = { Preset(PULSE, pianoCut, false, PULSE_50) };
		gm[101] = { Preset(TRIANGLE, pad, true) };
		gm[102] = { Preset(PULSE, strings, true, PULSE_25) };
		gm[103] = { Preset(PULSE, pianoCut, false, PULSE_12) };

		// ethnic
		gm[104] = { Preset(PULSE, pianoCut, false, PULSE_25) };
		gm[105] = { Preset(PULSE, pianoCut, true, PULSE_25) };
		gm[106] = { Preset(PULSE, longDecayCut, false, PULSE_12) };
		gm[107] = { Preset(PULSE, pianoCut, false, PULSE_12) };
		gm[108] = { Preset(PULSE, longDecayCut, false, PULSE_50) };
		gm[109] = { Preset(PULSE, attack, true, PULSE_50) };
		gm[110] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[111] = { Preset(PULSE, loop, true, PULSE_25) };

		// percussive
		gm[112] = { Preset(PULSE, longDecayCut, false, PULSE_50) };
		gm[113] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, 11, Note(12)) };
		gm[114] = { Preset(PULSE, longDecayCut, false, PULSE_50) };
		gm[115] = { Preset(DPCM, dpcm.woodblock, false) };
		gm[116] = { Preset(DPCM, dpcm.taiko, false) };
		gm[117] = { Preset(DPCM, dpcm.melodicTom, false) };
		gm[118] = { Preset(DPCM, dpcm.synthDrum, false) };
		gm[119] = { Preset(NOISE, reverseCymbal, true, NOISE_NORMAL, 1, Note(10)) };

		// sound effects
		gm[120] = {};
		gm[121] = {};
		gm[122] = {};
		gm[123] = {};
		gm[124] = {};
		gm[125] = {};
		gm[126] = {};
		gm[127] = { Preset(NOISE, decay, false, NOISE_NORMAL, {}, Note(1), 5) };



		// drums
		drums[0][49] = { Preset(NOISE, crash, false, NOISE_NORMAL, 1, Note(9), 15) }; // crash 1
		drums[0][57] = { Preset(NOISE, crash, false, NOISE_NORMAL, 2, Note(10), 15) }; // crash 2
		drums[0][52] = { Preset(NOISE, crash, false, NOISE_NORMAL, 3, Note(6), 15) }; // Chinese cymbal
		drums[0][55] = { Preset(NOISE, crash, false, NOISE_NORMAL, 4, Note(13), 15) }; // splash
		drums[0][39] = { Preset(NOISE, clap, false, ANY, 7, Note(8)) }; // clap
		drums[0][46] = { Preset(NOISE, openHat, false, NOISE_NORMAL, 8, Note(9)) }; // open hi-hat
		drums[0][51] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, 9, Note(9)) }; // ride cymbal 1
		drums[0][53] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, 9, Note(9)) }; // ride bell
		drums[0][59] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, 9, Note(9)) }; // ride cymbal 2
		drums[0][42] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, 10, Note(12)) }; // hi-hat
		drums[0][44] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, 10, Note(12)) }; // hi-hat
		drums[0][80] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, 12, Note(15)) }; // triangle closed
		drums[0][81] = { Preset(NOISE, decay, false, NOISE_LOOP, 12, Note(15)) }; // triangle open
		drums[0][38] = { Preset(NOISE, snare, false, NOISE_NORMAL, 13, Note(12)) }; // snare
		drums[0][40] = { Preset(NOISE, snare, false, NOISE_NORMAL, 13, Note(12)) }; // snare
		drums[0][35] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, 14, Note(12)) }; // kick
		drums[0][36] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, 14, Note(12)) }; // kick
		bindDrumSample(dpcm.samples.snare, dpcm.standardDrums, false, 0, 38, 1, 15);
		bindDrumSample(dpcm.samples.snare, dpcm.standardDrums, false, 0, 40, 1, 15);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 41, 2, 10);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 43, 2, 11);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 45, 2, 12);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 47, 2, 13);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 48, 2, 14);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 50, 2, 15);
		bindDrumSample(dpcm.samples.kick, dpcm.standardDrums, false, 0, 35, 3, 15);
		bindDrumSample(dpcm.samples.kick, dpcm.standardDrums, false, 0, 36, 3, 15);
		bindDrumSample(dpcm.samples.timbale, dpcm.standardDrums, false, 0, 65, 4, 15);
		bindDrumSample(dpcm.samples.timbale, dpcm.standardDrums, false, 0, 66, 4, 14);
		bindDrumSample(dpcm.samples.stick, dpcm.standardDrums, false, 0, 37, 5, 15);

		// hard drums
		drums[16] = drums[0];
		drums[16][38] = { Preset(NOISE, hardSnare, false, NOISE_NORMAL, 5, Note(6), 5) }; // snare
		drums[16][40] = { Preset(NOISE, hardSnare, false, NOISE_NORMAL, 5, Note(6), 5) }; // snare
		drums[16][41] = { Preset(NOISE, decay, false, NOISE_NORMAL, 6, Note(2), 5) }; // tom
		drums[16][43] = { Preset(NOISE, decay, false, NOISE_NORMAL, 6, Note(3), 5) }; // tom
		drums[16][45] = { Preset(NOISE, decay, false, NOISE_NORMAL, 6, Note(4), 5) }; // tom
		drums[16][47] = { Preset(NOISE, decay, false, NOISE_NORMAL, 6, Note(5), 5) }; // tom
		drums[16][48] = { Preset(NOISE, decay, false, NOISE_NORMAL, 6, Note(6), 5) }; // tom
		drums[16][50] = { Preset(NOISE, decay, false, NOISE_NORMAL, 6, Note(7), 5) }; // tom

		// electronic drums
		drums[24] = drums[16];
		bindDrumSample(dpcm.samples.hardKick, dpcm.hardDrums, false, 24, 35, 3, 15);
		bindDrumSample(dpcm.samples.hardKick, dpcm.hardDrums, false, 24, 36, 3, 15);

		// analog drums
		drums[25] = drums[0];
		drums[25][46] = { Preset(NOISE, analogOpenHat, false, NOISE_NORMAL, 8, Note(12)) }; // open hi-hat
		bindDrumSample(dpcm.samples.analogSnare, dpcm.analogDrums, false, 25, 38, 1, 15);
		bindDrumSample(dpcm.samples.analogSnare, dpcm.analogDrums, false, 25, 40, 1, 15);
		bindDrumSample(dpcm.samples.analogKick, dpcm.analogDrums, false, 25, 35, 3, 15);
		bindDrumSample(dpcm.samples.analogKick, dpcm.analogDrums, false, 25, 36, 3, 15);

		// other analog drums
		drums[26] = drums[27] = drums[30] = drums[25];

		// orchestra
		drums[48] = drums[0];
		drums[48][38] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, 5, Note(9)) }; // snare
		drums[48][40] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, 5, Note(9)) }; // snare
		drums[48][59] = { Preset(NOISE, crash, false, NOISE_NORMAL, 1, Note(9), 15) }; // crash 1
		bindDrumSample(dpcm.samples.orchestraKick, dpcm.orchestraDrums, false, 48, 35, 3, 15);
		bindDrumSample(dpcm.samples.orchestraKick, dpcm.orchestraDrums, false, 48, 36, 3, 15);
		bindDrumSample(dpcm.samples.stick, dpcm.orchestraDrums, false, 48, 39, 5, 15);
		for (int key = 41; key <= 53; key++) {
			drums[48][key] = { Preset(DPCM, dpcm.timpani, false, ANY, 2, Note(key)) };
		}

		// clear sfx
		for (int i = Note::MIN_KEY; i <= Note::MAX_KEY; i++) {
			drums[56][i] = {};
		}
	}

	void bindDrumSample(std::shared_ptr<DpcmSample> sample, std::shared_ptr<Instrument> instrument, bool needRelease, int program, int key, int priorityOrder, int pitch) {
		instrument->dpcmSamples.emplace_back(Note(key), sample, pitch, false);

		std::vector<Preset>& presets = drums[program][key];
		std::erase_if(presets, [](Preset const& preset) { return preset.channel == Preset::Channel::DPCM; });
		presets.emplace_back(Preset::Channel::DPCM, instrument, needRelease, Preset::Duty::ANY, priorityOrder, Note(key));
	}

public:
	void fillBase(FamiTrackerFile& file) {
		dpcm.samples.loadSamples(file);
		fillInstruments(file);
	}

	std::optional<Preset> getGmPreset(int program) const {
		return gm[program];
	}

	std::vector<Preset> getDrumPresets(int program, int key) {
		auto it = drums.find(program);
		if (it == drums.end()) {
			it = drums.find(0);
		}
		return it->second[key];
	}
};