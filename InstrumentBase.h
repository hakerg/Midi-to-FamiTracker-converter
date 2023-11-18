#pragma once
#include "commons.h"
#include "Instrument.h"
#include "Preset.h"
#include "FamiTrackerFile.h"
#include "SampleBase.h"
#include "PitchCalculator.h"
#include "MidiState.h"

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
			sample.note = Note(NesChannel::DPCM, key);
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
				sample.note = Note(NesChannel::DPCM, i);
				instrument->dpcmSamples.push_back(sample);
			}
		}

		void fillTimpani() {
			int firstKey = Note(NesChannel::DPCM, 0, Tone::A_).key;
			for (int i = 0; i < samples.timpaniA_1.size(); i++) {
				timpani->dpcmSamples.emplace_back(Note(NesChannel::DPCM, firstKey + i), samples.timpaniA_1[i]);
			}

			fillSamplesByLoweringPitch(timpani, 1);
		}

		void fillHit() {
			int firstKey = Note(NesChannel::DPCM, 3, Tone::C_).key;
			for (int i = 0; i < samples.hitC_5.size(); i++) {
				hit->dpcmSamples.emplace_back(Note(NesChannel::DPCM, firstKey + i), samples.hitC_5[i]);
			}

			fillSamplesByLoweringPitch(hit, 1);
		}

		std::shared_ptr<Instrument> createSingleSampleInstrument(FamiTrackerFile& file, const std::wstring& name, std::shared_ptr<DpcmSample> sample, double scaleTuning) {
			std::shared_ptr<Instrument> instrument = file.addInstrument(name);
			instrument->dpcmSamples.emplace_back(Note(NesChannel::DPCM, 60), sample);
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
		std::shared_ptr<Instrument> electronicDrums;
		std::shared_ptr<Instrument> orchestraDrums;
		std::shared_ptr<Instrument> analogDrums;

		void createInstruments(FamiTrackerFile& file) {
			synthDrum = createSingleSampleInstrument(file, L"Synth drum", samples.synthDrum, 0.5);
			melodicTom = createSingleSampleInstrument(file, L"Melodic tom", samples.tom, 0.5);
			woodblock = createSingleSampleInstrument(file, L"Woodblock", samples.stick, 0.5);
			taiko = createSingleSampleInstrument(file, L"Taiko", samples.taiko, 0.5);

			timpani = file.addInstrument(L"Timpani");
			hit = file.addInstrument(L"Orchestra hit");
			fillTimpani();
			fillHit();

			standardDrums = file.addInstrument(L"Standard drums");
			electronicDrums = file.addInstrument(L"Electronic drums");
			orchestraDrums = file.addInstrument(L"Orchestra drums");
			analogDrums = file.addInstrument(L"Analog drums");
		}
	};

	Dpcm dpcm;

	std::array<std::optional<Preset>, MidiState::PROGRAM_COUNT> gm;
	std::unordered_map<int, std::array<std::optional<Preset>, MidiState::KEY_COUNT>> drums{};

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
		auto fastDecayMacro = file.addVolumeMacro({ 15, 11, 7, 3, 0 });
		auto decayMacro = file.addVolumeMacro(createDecayValues(15, 0, -1, 1));
		auto longDecayMacro = file.addVolumeMacro(createDecayValues(15, 0, -1, 2));
		auto pianoMacro = file.addVolumeMacro(createDecayValues(15, 0, -1, 6));
		auto longDecayMacroCut = file.addVolumeMacro(createDecayValues(15, 0, -1, 2), -1, 30);
		auto pianoMacroCut = file.addVolumeMacro(createDecayValues(15, 0, -1, 6), -1, 90);
		auto attackMacro = file.addVolumeMacro({ 3, 7, 11, 15, 0 }, -1, 3);
		auto stringsMacro = file.addVolumeMacro({ 3, 7, 11, 15, 5, 5, 5, 5, 5, 5, 5, 5, 0 }, -1, 3);
		auto reverseCymbalValues = createDecayValues(1, 10, 1, 2);
		reverseCymbalValues.push_back(0);
		auto reverseCymbalMacro = file.addVolumeMacro(reverseCymbalValues, -1, 19);
		auto hiHatMacro = file.addVolumeMacro({ 12, 12, 12, 12, 12, 0 });

		auto vibratoMacro = file.addPitchMacro({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 1, 1, 1, 1 }, 15);

		auto pluckDutyMacro = file.addDutyMacro({ 2, 1, 0, 1 });

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
		std::shared_ptr<Instrument> crash = file.addInstrument(L"Crash", longDecayMacro);
		std::shared_ptr<Instrument> reverseCymbal = file.addInstrument(L"Reverse cymbal", reverseCymbalMacro);
		std::shared_ptr<Instrument> hiHat = file.addInstrument(L"Hi-hat", hiHatMacro);

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
		gm[21] = { Preset(PULSE, attack, true, PULSE_12) };
		gm[22] = { Preset(PULSE, attack, true, PULSE_12) };
		gm[23] = { Preset(PULSE, attack, true, PULSE_12) };

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
		gm[47] = { Preset(DPCM, dpcm.timpani, false, UNSPECIFIED, Preset::Order::TIMPANI, {}) };

		// ensemble
		gm[48] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[49] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[50] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[51] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[52] = { Preset(PULSE, strings, true, PULSE_50) };
		gm[53] = { Preset(PULSE, loop, true, PULSE_50) };
		gm[54] = { Preset(PULSE, loop, true, PULSE_50) };
		gm[55] = { Preset(DPCM, dpcm.hit, false, UNSPECIFIED, Preset::Order::HIT, {}) };

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
		gm[113] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, Preset::Order::AGOGO, NoiseNote(12)) };
		gm[114] = { Preset(PULSE, longDecayCut, false, PULSE_50) };
		gm[115] = { Preset(DPCM, dpcm.woodblock, false, UNSPECIFIED, Preset::Order::STICK, {}) };
		gm[116] = { Preset(DPCM, dpcm.taiko, false, UNSPECIFIED, Preset::Order::TAIKO, {}) };
		gm[117] = { Preset(DPCM, dpcm.melodicTom, false, UNSPECIFIED, Preset::Order::TOM, {}) };
		gm[118] = { Preset(DPCM, dpcm.synthDrum, false, UNSPECIFIED, Preset::Order::TOM, {}) };
		gm[119] = { Preset(NOISE, reverseCymbal, true, NOISE_NORMAL, Preset::Order::REVERSE_CYMBAL, NoiseNote(10)) };

		// sound effects
		gm[120] = {};
		gm[121] = {};
		gm[122] = {};
		gm[123] = {};
		gm[124] = {};
		gm[125] = {};
		gm[126] = {};
		gm[127] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::GUN, NoiseNote(1), 5) };



		// drums
		drums[0][39] = { Preset(NOISE, fastDecay, false, UNSPECIFIED, Preset::Order::CLAP, NoiseNote(7)) }; // clap
		drums[0][42] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, Preset::Order::HI_HAT, NoiseNote(12)) }; // hi-hat
		drums[0][44] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, Preset::Order::HI_HAT, NoiseNote(12)) }; // hi-hat
		drums[0][46] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::OPEN_HI_HAT, NoiseNote(12)) }; // open hi-hat
		drums[0][49] = { Preset(NOISE, crash, false, NOISE_NORMAL, Preset::Order::CRASH, NoiseNote(9), 15) }; // crash 1
		drums[0][51] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, Preset::Order::RIDE_CYMBAL, NoiseNote(9)) }; // ride cymbal 1
		drums[0][52] = { Preset(NOISE, crash, false, NOISE_NORMAL, Preset::Order::CRASH, NoiseNote(6), 15) }; // chinese cymbal
		drums[0][53] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, Preset::Order::RIDE_CYMBAL, NoiseNote(9)) }; // ride bell
		drums[0][54] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, Preset::Order::TAMBOURINE, NoiseNote(14)) }; // tambourine
		drums[0][55] = { Preset(NOISE, crash, false, NOISE_NORMAL, Preset::Order::SPLASH, NoiseNote(13), 15) }; // splash
		drums[0][56] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, Preset::Order::RIDE_CYMBAL, NoiseNote(9)) }; // cowbell
		drums[0][57] = { Preset(NOISE, crash, false, NOISE_NORMAL, Preset::Order::CRASH, NoiseNote(10), 15) }; // crash 2
		drums[0][59] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, Preset::Order::RIDE_CYMBAL, NoiseNote(9)) }; // ride cymbal 2
		drums[0][67] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, Preset::Order::AGOGO, NoiseNote(12)) }; // high agogo
		drums[0][68] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, Preset::Order::AGOGO, NoiseNote(11)) }; // low agogo
		drums[0][80] = { Preset(NOISE, fastDecay, false, NOISE_LOOP, Preset::Order::TRIANGLE, NoiseNote(15)) }; // triangle closed
		drums[0][81] = { Preset(NOISE, decay, false, NOISE_LOOP, Preset::Order::TRIANGLE, NoiseNote(15)) }; // triangle open
		bindDrumSample(dpcm.samples.kick, dpcm.standardDrums, false, 0, 35, Preset::Order::KICK, 15);
		bindDrumSample(dpcm.samples.kick, dpcm.standardDrums, false, 0, 36, Preset::Order::KICK, 15);
		bindDrumSample(dpcm.samples.stick, dpcm.standardDrums, false, 0, 37, Preset::Order::STICK, 15);
		bindDrumSample(dpcm.samples.snare, dpcm.standardDrums, false, 0, 38, Preset::Order::SNARE, 15);
		bindDrumSample(dpcm.samples.snare, dpcm.standardDrums, false, 0, 40, Preset::Order::SNARE, 15);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 41, Preset::Order::TOM, 10);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 43, Preset::Order::TOM, 11);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 45, Preset::Order::TOM, 12);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 47, Preset::Order::TOM, 13);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 48, Preset::Order::TOM, 14);
		bindDrumSample(dpcm.samples.tom, dpcm.standardDrums, false, 0, 50, Preset::Order::TOM, 15);
		bindDrumSample(dpcm.samples.timbale, dpcm.standardDrums, false, 0, 65, Preset::Order::TIMBALE, 15);
		bindDrumSample(dpcm.samples.timbale, dpcm.standardDrums, false, 0, 66, Preset::Order::TIMBALE, 14);
		bindDrumSample(dpcm.samples.stick, dpcm.standardDrums, false, 0, 76, Preset::Order::STICK, 15);
		bindDrumSample(dpcm.samples.stick, dpcm.standardDrums, false, 0, 77, Preset::Order::STICK, 14);

		// hard drums
		drums[16] = drums[0];
		drums[16][38] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::SNARE, NoiseNote(7), 5) }; // snare
		drums[16][40] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::SNARE, NoiseNote(7), 5) }; // snare
		drums[16][41] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::TOM, NoiseNote(2), 5) }; // tom
		drums[16][43] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::TOM, NoiseNote(3), 5) }; // tom
		drums[16][45] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::TOM, NoiseNote(4), 5) }; // tom
		drums[16][47] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::TOM, NoiseNote(5), 5) }; // tom
		drums[16][48] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::TOM, NoiseNote(6), 5) }; // tom
		drums[16][50] = { Preset(NOISE, decay, false, NOISE_NORMAL, Preset::Order::TOM, NoiseNote(7), 5) }; // tom

		// electronic drums
		drums[24] = drums[16];
		drums[16][38] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, Preset::Order::SNARE, NoiseNote(7), 5) }; // snare
		drums[16][40] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, Preset::Order::SNARE, NoiseNote(7), 5) }; // snare
		drums[24][41] = {}; // tom
		drums[24][43] = {}; // tom
		drums[24][45] = {}; // tom
		drums[24][47] = {}; // tom
		drums[24][48] = {}; // tom
		drums[24][50] = {}; // tom
		bindDrumSample(dpcm.samples.synthDrum, dpcm.electronicDrums, false, 24, 41, Preset::Order::TOM, 10);
		bindDrumSample(dpcm.samples.synthDrum, dpcm.electronicDrums, false, 24, 43, Preset::Order::TOM, 11);
		bindDrumSample(dpcm.samples.synthDrum, dpcm.electronicDrums, false, 24, 45, Preset::Order::TOM, 12);
		bindDrumSample(dpcm.samples.synthDrum, dpcm.electronicDrums, false, 24, 47, Preset::Order::TOM, 13);
		bindDrumSample(dpcm.samples.synthDrum, dpcm.electronicDrums, false, 24, 48, Preset::Order::TOM, 14);
		bindDrumSample(dpcm.samples.synthDrum, dpcm.electronicDrums, false, 24, 50, Preset::Order::TOM, 15);

		// analog drums
		drums[25] = drums[0];
		drums[25][46] = { Preset(NOISE, hiHat, false, NOISE_NORMAL, Preset::Order::OPEN_HI_HAT, NoiseNote(12)) }; // open hi-hat
		bindDrumSample(dpcm.samples.analogKick, dpcm.analogDrums, false, 25, 35, Preset::Order::KICK, 15);
		bindDrumSample(dpcm.samples.analogKick, dpcm.analogDrums, false, 25, 36, Preset::Order::KICK, 15);
		bindDrumSample(dpcm.samples.analogSnare, dpcm.analogDrums, false, 25, 38, Preset::Order::SNARE, 15);
		bindDrumSample(dpcm.samples.analogSnare, dpcm.analogDrums, false, 25, 40, Preset::Order::SNARE, 15);

		// other analog drums
		drums[26] = drums[27] = drums[30] = drums[25];

		// orchestra
		drums[48] = drums[0];
		drums[48][38] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, Preset::Order::SNARE, NoiseNote(9)) }; // snare
		drums[48][40] = { Preset(NOISE, fastDecay, false, NOISE_NORMAL, Preset::Order::SNARE, NoiseNote(9)) }; // snare
		drums[48][59] = { Preset(NOISE, crash, false, NOISE_NORMAL, Preset::Order::CRASH, NoiseNote(9), 15) }; // crash 1
		bindDrumSample(dpcm.samples.stick, dpcm.orchestraDrums, false, 48, 39, Preset::Order::STICK, 15);
		drums[48][35] = { Preset(DPCM, dpcm.timpani, false, UNSPECIFIED, Preset::Order::KICK, Note(NesChannel::DPCM, 35)) };
		drums[48][36] = { Preset(DPCM, dpcm.timpani, false, UNSPECIFIED, Preset::Order::KICK, Note(NesChannel::DPCM, 36)) };
		for (int key = 41; key <= 53; key++) {
			drums[48][key] = { Preset(DPCM, dpcm.timpani, false, UNSPECIFIED, Preset::Order::TIMPANI, Note(NesChannel::DPCM, key))};
		}

		// clear sfx
		for (int i = Note::MIN_KEY; i <= Note::MAX_KEY; i++) {
			drums[56][i] = {};
		}
	}

	void bindDrumSample(std::shared_ptr<DpcmSample> sample, std::shared_ptr<Instrument> instrument, bool needRelease, int program, int key, Preset::Order order, int pitch) {
		instrument->dpcmSamples.emplace_back(Note(NesChannel::DPCM, key), sample, pitch, false);
		drums[program][key] = { Preset(Preset::Channel::DPCM, instrument, needRelease, Preset::Duty::UNSPECIFIED, order, Note(NesChannel::DPCM, key)) };
	}

	static Note NoiseNote(int key) {
		return {NesChannel::NOISE, key};
	}

public:
	void fillBase(FamiTrackerFile& file) {
		dpcm.samples.loadSamples(file);
		fillInstruments(file);
	}

	std::optional<Preset> getGmPreset(int program) const {
		return gm[program];
	}

	std::optional<Preset> getDrumPreset(int program, int key) const {
		return getMapValueOrDefault(drums, program, drums.find(0)->second)[key];
	}
};