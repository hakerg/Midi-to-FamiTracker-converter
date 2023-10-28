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
			double targetKey = lowestKey + (key - lowestKey) * scaleTuning;

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
					double score = (pitch - sample.pitch) * 0.25 - std::abs(targetKey - calculatedKey);
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
	std::unordered_map<int, std::array<std::vector<Preset>, 128>> drums;

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
		auto reverseCymbalValues = createDecayValues(1, 15, 1, 2);
		reverseCymbalValues.push_back(0);
		auto reverseCymbalMacro = file.addVolumeMacro(reverseCymbalValues, -1, 29);

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
		std::shared_ptr<Instrument> longDecay = file.addInstrument(L"Long decay", longDecayMacro);
		std::shared_ptr<Instrument> piano = file.addInstrument(L"Piano", pianoMacro);
		
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
		gm[0] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_50) };
		gm[1] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_50) };
		gm[2] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_50) };
		gm[3] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_50) };
		gm[4] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_50) };
		gm[5] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_50) };
		gm[6] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_12) };
		gm[7] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_12) };

		// chromatic percussion
		gm[8] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[9] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[10] = { Preset(Preset::Channel::PULSE, longDecay, Preset::Duty::PULSE_50) };
		gm[11] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[12] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[13] = { Preset(Preset::Channel::PULSE, longDecay, Preset::Duty::PULSE_50) };
		gm[14] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[15] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_12) };

		// organ
		gm[16] = { Preset(Preset::Channel::PULSE, attack, Preset::Duty::PULSE_25) };
		gm[17] = { Preset(Preset::Channel::PULSE, attack, Preset::Duty::PULSE_25) };
		gm[18] = { Preset(Preset::Channel::PULSE, attack, Preset::Duty::PULSE_25) };
		gm[19] = { Preset(Preset::Channel::PULSE, attack, Preset::Duty::PULSE_25) };
		gm[20] = { Preset(Preset::Channel::PULSE, attack, Preset::Duty::PULSE_25) };
		gm[21] = { Preset(Preset::Channel::PULSE, attack, Preset::Duty::PULSE_25) };
		gm[22] = { Preset(Preset::Channel::PULSE, attack, Preset::Duty::PULSE_25) };
		gm[23] = { Preset(Preset::Channel::PULSE, attack, Preset::Duty::PULSE_25) };

		// guitar
		gm[24] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_25) };
		gm[25] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_25) };
		gm[26] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_25) };
		gm[27] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_25) };
		gm[28] = { Preset(Preset::Channel::PULSE, longDecayCut, Preset::Duty::PULSE_25) };
		gm[29] = { Preset(Preset::Channel::PULSE, guitarDistortion) };
		gm[30] = { Preset(Preset::Channel::PULSE, guitarDistortion) };
		gm[31] = { Preset(Preset::Channel::PULSE, guitarDistortion) };

		// bass
		gm[32] = { Preset(Preset::Channel::TRIANGLE, loop) };
		gm[33] = { Preset(Preset::Channel::TRIANGLE, loop) };
		gm[34] = { Preset(Preset::Channel::TRIANGLE, loop) };
		gm[35] = { Preset(Preset::Channel::TRIANGLE, loop) };
		gm[36] = { Preset(Preset::Channel::TRIANGLE, loop) };
		gm[37] = { Preset(Preset::Channel::TRIANGLE, loop) };
		gm[38] = { Preset(Preset::Channel::TRIANGLE, loop) };
		gm[39] = { Preset(Preset::Channel::TRIANGLE, loop) };

		// strings
		gm[40] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[41] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[42] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[43] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[44] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[45] = { Preset(Preset::Channel::PULSE, decay, Preset::Duty::PULSE_50) };
		gm[46] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[47] = { Preset(Preset::Channel::DPCM, dpcm.timpani) };

		// ensemble
		gm[48] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[49] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[50] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[51] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[52] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[53] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_50) };
		gm[54] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_50) };
		gm[55] = { Preset(Preset::Channel::DPCM, dpcm.hit) };

		// brass
		gm[56] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_12) };
		gm[57] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_12) };
		gm[58] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_12) };
		gm[59] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_12) };
		gm[60] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_12) };
		gm[61] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_12) };
		gm[62] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_12) };
		gm[63] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_12) };

		// reed
		gm[64] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_25) };
		gm[65] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_25) };
		gm[66] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_25) };
		gm[67] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_25) };
		gm[68] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_25) };
		gm[69] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_25) };
		gm[70] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_25) };
		gm[71] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_25) };

		// pipe
		gm[72] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_50) };
		gm[73] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_50) };
		gm[74] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_50) };
		gm[75] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_50) };
		gm[76] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_50) };
		gm[77] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_50) };
		gm[78] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_50) };
		gm[79] = { Preset(Preset::Channel::PULSE, loopVibrato, Preset::Duty::PULSE_50) };

		// synth lead
		gm[80] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_50) };
		gm[81] = { Preset(Preset::Channel::SAWTOOTH, loop) };
		gm[82] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_50) };
		gm[83] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_50) };
		gm[84] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_25) };
		gm[85] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_50) };
		gm[86] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_25) };
		gm[87] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_12) };

		// synth pad
		gm[88] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[89] = { Preset(Preset::Channel::TRIANGLE, pad) };
		gm[90] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_25) };
		gm[91] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[92] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[93] = { Preset(Preset::Channel::SAWTOOTH, pad) };
		gm[94] = { Preset(Preset::Channel::PULSE, pad, Preset::Duty::PULSE_50) };
		gm[95] = { Preset(Preset::Channel::TRIANGLE, pad) };

		// synth effects
		gm[96] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[97] = { Preset(Preset::Channel::PULSE, pad, Preset::Duty::PULSE_25) };
		gm[98] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[99] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_25) };
		gm[100] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_50) };
		gm[101] = { Preset(Preset::Channel::TRIANGLE, pad) };
		gm[102] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_25) };
		gm[103] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_12) };

		// ethnic
		gm[104] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_25) };
		gm[105] = { Preset(Preset::Channel::PULSE, pianoCut, Preset::Duty::PULSE_25) };
		gm[106] = { Preset(Preset::Channel::PULSE, longDecay, Preset::Duty::PULSE_12) };
		gm[107] = { Preset(Preset::Channel::PULSE, piano, Preset::Duty::PULSE_12) };
		gm[108] = { Preset(Preset::Channel::PULSE, longDecay, Preset::Duty::PULSE_50) };
		gm[109] = { Preset(Preset::Channel::PULSE, attack, Preset::Duty::PULSE_50) };
		gm[110] = { Preset(Preset::Channel::PULSE, strings, Preset::Duty::PULSE_50) };
		gm[111] = { Preset(Preset::Channel::PULSE, loop, Preset::Duty::PULSE_25) };

		// percussive
		gm[112] = { Preset(Preset::Channel::PULSE, longDecay, Preset::Duty::PULSE_50) };
		gm[113] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_LOOP, 11, Note(12)) };
		gm[114] = { Preset(Preset::Channel::PULSE, longDecay, Preset::Duty::PULSE_50) };
		gm[115] = { Preset(Preset::Channel::DPCM, dpcm.woodblock) };
		gm[116] = { Preset(Preset::Channel::DPCM, dpcm.taiko) };
		gm[117] = { Preset(Preset::Channel::DPCM, dpcm.melodicTom) };
		gm[118] = { Preset(Preset::Channel::DPCM, dpcm.synthDrum) };
		gm[119] = { Preset(Preset::Channel::NOISE, reverseCymbal, Preset::Duty::NOISE_NORMAL, 1, Note(10)) };

		// sound effects
		gm[120] = {};
		gm[121] = {};
		gm[122] = {};
		gm[123] = {};
		gm[124] = {};
		gm[125] = {};
		gm[126] = {};
		gm[127] = { Preset(Preset::Channel::NOISE, decay, Preset::Duty::NOISE_NORMAL, {}, Note(1), 5) };



		// drums
		drums[0][49] = { Preset(Preset::Channel::NOISE, crash, Preset::Duty::NOISE_NORMAL, 1, Note(9), 15) }; // crash 1
		drums[0][57] = { Preset(Preset::Channel::NOISE, crash, Preset::Duty::NOISE_NORMAL, 2, Note(10), 15) }; // crash 2
		drums[0][52] = { Preset(Preset::Channel::NOISE, crash, Preset::Duty::NOISE_NORMAL, 3, Note(6), 15) }; // chinese cymbal
		drums[0][55] = { Preset(Preset::Channel::NOISE, crash, Preset::Duty::NOISE_NORMAL, 4, Note(13), 15) }; // splash
		drums[0][39] = { Preset(Preset::Channel::NOISE, clap, Preset::Duty::ANY, 7, Note(8)) }; // clap
		drums[0][46] = { Preset(Preset::Channel::NOISE, openHat, Preset::Duty::NOISE_NORMAL, 8, Note(9)) }; // open hi-hat
		drums[0][51] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_LOOP, 9, Note(9)) }; // ride cymbal 1
		drums[0][53] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_LOOP, 9, Note(9)) }; // ride bell
		drums[0][59] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_LOOP, 9, Note(9)) }; // ride cymbal 2
		drums[0][42] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_NORMAL, 10, Note(12)) }; // hi-hat
		drums[0][44] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_NORMAL, 10, Note(12)) }; // hi-hat
		drums[0][80] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_LOOP, 12, Note(15)) }; // triangle closed
		drums[0][81] = { Preset(Preset::Channel::NOISE, decay, Preset::Duty::NOISE_LOOP, 12, Note(15)) }; // triangle open
		drums[0][38] = { Preset(Preset::Channel::NOISE, snare, Preset::Duty::NOISE_NORMAL, 13, Note(12)) }; // snare
		drums[0][40] = { Preset(Preset::Channel::NOISE, snare, Preset::Duty::NOISE_NORMAL, 13, Note(12)) }; // snare
		drums[0][35] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_NORMAL, 14, Note(12)) }; // kick
		drums[0][36] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_NORMAL, 14, Note(12)) }; // kick
		bindDrumSample(dpcm.samples.snare, dpcm.standardDrums, 0, 38, 1, 15);
		bindDrumSample(dpcm.samples.snare, dpcm.standardDrums, 0, 40, 1, 15);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, 0, 41, 2, 10);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, 0, 43, 2, 11);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, 0, 45, 2, 12);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, 0, 47, 2, 13);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, 0, 48, 2, 14);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, 0, 50, 2, 15);
		bindDrumSample(dpcm.samples.kick, dpcm.standardDrums, 0, 35, 3, 15);
		bindDrumSample(dpcm.samples.kick, dpcm.standardDrums, 0, 36, 3, 15);
		bindDrumSample(dpcm.samples.timbale, dpcm.standardDrums, 0, 65, 4, 15);
		bindDrumSample(dpcm.samples.timbale, dpcm.standardDrums, 0, 66, 4, 14);
		bindDrumSample(dpcm.samples.stick, dpcm.standardDrums, 0, 37, 5, 15);

		// hard drums
		drums[16] = drums[0];
		drums[16][38] = { Preset(Preset::Channel::NOISE, hardSnare, Preset::Duty::NOISE_NORMAL, 5, Note(6), 5) }; // snare
		drums[16][40] = { Preset(Preset::Channel::NOISE, hardSnare, Preset::Duty::NOISE_NORMAL, 5, Note(6), 5) }; // snare
		drums[16][41] = { Preset(Preset::Channel::NOISE, decay, Preset::Duty::NOISE_NORMAL, 6, Note(2), 5) }; // tom
		drums[16][43] = { Preset(Preset::Channel::NOISE, decay, Preset::Duty::NOISE_NORMAL, 6, Note(3), 5) }; // tom
		drums[16][45] = { Preset(Preset::Channel::NOISE, decay, Preset::Duty::NOISE_NORMAL, 6, Note(4), 5) }; // tom
		drums[16][47] = { Preset(Preset::Channel::NOISE, decay, Preset::Duty::NOISE_NORMAL, 6, Note(5), 5) }; // tom
		drums[16][48] = { Preset(Preset::Channel::NOISE, decay, Preset::Duty::NOISE_NORMAL, 6, Note(6), 5) }; // tom
		drums[16][50] = { Preset(Preset::Channel::NOISE, decay, Preset::Duty::NOISE_NORMAL, 6, Note(7), 5) }; // tom

		// electronic drums
		drums[24] = drums[16];
		bindDrumSample(dpcm.samples.hardKick, dpcm.hardDrums, 24, 35, 3, 15);
		bindDrumSample(dpcm.samples.hardKick, dpcm.hardDrums, 24, 36, 3, 15);

		// analog drums
		drums[25] = drums[0];
		drums[25][46] = { Preset(Preset::Channel::NOISE, analogOpenHat, Preset::Duty::NOISE_NORMAL, 8, Note(12)) }; // open hi-hat
		bindDrumSample(dpcm.samples.analogSnare, dpcm.analogDrums, 25, 38, 1, 15);
		bindDrumSample(dpcm.samples.analogSnare, dpcm.analogDrums, 25, 40, 1, 15);
		bindDrumSample(dpcm.samples.analogKick, dpcm.analogDrums, 25, 35, 3, 15);
		bindDrumSample(dpcm.samples.analogKick, dpcm.analogDrums, 25, 36, 3, 15);

		// other analog drums
		drums[26] = drums[27] = drums[30] = drums[25];

		// orchestra
		drums[48] = drums[0];
		drums[48][38] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_NORMAL, 5, Note(9)) }; // snare
		drums[48][40] = { Preset(Preset::Channel::NOISE, fastDecay, Preset::Duty::NOISE_NORMAL, 5, Note(9)) }; // snare
		drums[48][59] = { Preset(Preset::Channel::NOISE, crash, Preset::Duty::NOISE_NORMAL, 1, Note(9), 15) }; // crash 1
		bindDrumSample(dpcm.samples.orchestraKick, dpcm.orchestraDrums, 48, 35, 3, 15);
		bindDrumSample(dpcm.samples.orchestraKick, dpcm.orchestraDrums, 48, 36, 3, 15);
		bindDrumSample(dpcm.samples.stick, dpcm.orchestraDrums, 48, 39, 5, 15);
		for (int key = 41; key <= 53; key++) {
			drums[48][key] = { Preset(Preset::Channel::DPCM, dpcm.timpani, Preset::Duty::ANY, 2, Note(key)) };
		}

		// clear sfx
		for (int i = Note::MIN_KEY; i <= Note::MAX_KEY; i++) {
			drums[56][i] = {};
		}
	}

	void bindDrumSample(std::shared_ptr<DpcmSample> sample, std::shared_ptr<Instrument> instrument, int program, int key, int priorityOrder, int pitch) {
		instrument->dpcmSamples.emplace_back(Note(key), sample, pitch, false);

		std::vector<Preset>& presets = drums[program][key];
		removeIf(presets, [](Preset const& preset) { return preset.channel == Preset::Channel::DPCM; });
		presets.emplace_back(Preset::Channel::DPCM, instrument, Preset::Duty::ANY, priorityOrder, Note(key));
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