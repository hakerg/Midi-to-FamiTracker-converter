#pragma once
#include "commons.h"
#include "Macro.h"
#include "KeyDpcmSample.h"

class Instrument {
public:
	int _id;
	std::shared_ptr<Macro<MacroType::VOLUME>> volumeMacro;
	std::shared_ptr<Macro<MacroType::ARPEGGIO>> arpeggioMacro;
	std::shared_ptr<Macro<MacroType::PITCH>> pitchMacro;
	std::shared_ptr<Macro<MacroType::HI_PITCH>> hiPitchMacro;
	std::shared_ptr<Macro<MacroType::DUTY>> dutyMacro;
	std::wstring name;
	std::vector<KeyDpcmSample> dpcmSamples;

	explicit Instrument(int id, std::wstring const& name,
		std::shared_ptr<Macro<MacroType::VOLUME>> volumeMacro,
		std::shared_ptr<Macro<MacroType::ARPEGGIO>> arpeggioMacro,
		std::shared_ptr<Macro<MacroType::PITCH>> pitchMacro,
		std::shared_ptr<Macro<MacroType::HI_PITCH>> hiPitchMacro,
		std::shared_ptr<Macro<MacroType::DUTY>> dutyMacro) : _id(id),
		volumeMacro(volumeMacro), arpeggioMacro(arpeggioMacro), pitchMacro(pitchMacro), hiPitchMacro(hiPitchMacro), dutyMacro(dutyMacro), name(name) {}

	int getNesId() const {
		return _id;
	}

	int getVrc6Id() const {
		return _id + 32;
	}

	void exportTxt(std::wofstream& file) const {
		file << "INST2A03 " << getNesId()
			<< " " << (volumeMacro ? volumeMacro->id : -1)
			<< " " << (arpeggioMacro ? arpeggioMacro->id : -1)
			<< " " << (pitchMacro ? pitchMacro->id : -1)
			<< " " << (hiPitchMacro ? hiPitchMacro->id : -1)
			<< " " << (dutyMacro ? dutyMacro->id : -1)
			<< " \"" << name << "\"" << std::endl;

		for (auto& dpcmSample : dpcmSamples) {
			dpcmSample.exportTxt(file, getNesId());
		}

		file << "INSTVRC6 " << getVrc6Id()
			<< " " << (volumeMacro ? volumeMacro->id : -1)
			<< " " << (arpeggioMacro ? arpeggioMacro->id : -1)
			<< " " << (pitchMacro ? pitchMacro->id : -1)
			<< " " << (hiPitchMacro ? hiPitchMacro->id : -1)
			<< " " << (dutyMacro ? dutyMacro->id : -1)
			<< " \"" << name << "\"" << std::endl;
	}
};