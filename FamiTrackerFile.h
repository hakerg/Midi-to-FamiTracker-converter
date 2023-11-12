#pragma once
#include "commons.h"
#include "Track.h"
#include "Instrument.h"
#include "Macro.h"
#include "DpcmSample.h"

class FamiTrackerFile {
public:

    enum class Machine { NTSC = 0, PAL = 1 };
    enum class Expansion { NES = 0, VRC6 = 1, VRC7 = 2, FDS = 4, MMC5 = 8, N163 = 16 };

    std::wstring title;
    std::wstring author;
    std::wstring copyright;
    std::wstring comment;

    Machine machine = Machine::NTSC;
	int framerate = 0; // 0 - default: 60 for NTSC, 50 for PAL. Default recommended for NSF conversion
    Expansion expansion = Expansion::VRC6;
    bool newVibratoStyle = true;
    int minTempo = 32;
    int n163Channels = 1;

    std::vector<std::shared_ptr<Macro<MacroType::VOLUME>>> volumeMacros;
    std::vector<std::shared_ptr<Macro<MacroType::ARPEGGIO>>> arpeggioMacros;
    std::vector<std::shared_ptr<Macro<MacroType::PITCH>>> pitchMacros;
    std::vector<std::shared_ptr<Macro<MacroType::HI_PITCH>>> hiPitchMacros;
    std::vector<std::shared_ptr<Macro<MacroType::DUTY>>> dutyMacros;
    std::vector<std::shared_ptr<DpcmSample>> dpcmSamples;
    std::vector<std::shared_ptr<Instrument>> instruments;
    std::vector<std::shared_ptr<Track>> tracks;

    int getActualFramerate() const {
        if (framerate == 0) {
            return machine == Machine::NTSC ? 60 : 50;
        }
        return framerate;
    }

    std::shared_ptr<Macro<MacroType::VOLUME>> addVolumeMacro(std::vector<int> const& values, int loopPosition = -1, int releasePosition = -1) {
        int id = volumeMacros.empty() ? 0 : volumeMacros.back()->id + 1;
        volumeMacros.push_back(std::make_shared<Macro<MacroType::VOLUME>>(id, loopPosition, releasePosition, values));
		return volumeMacros.back();
    }

    std::shared_ptr<Macro<MacroType::ARPEGGIO>> addArpeggioMacro(std::vector<int> const& values, ArpeggioType arpeggioType, int loopPosition = -1, int releasePosition = -1) {
        int id = arpeggioMacros.empty() ? 0 : arpeggioMacros.back()->id + 1;
        arpeggioMacros.push_back(std::make_shared<Macro<MacroType::ARPEGGIO>>(id, loopPosition, releasePosition, values, arpeggioType));
        return arpeggioMacros.back();
    }

    std::shared_ptr<Macro<MacroType::PITCH>> addPitchMacro(std::vector<int> const& values, int loopPosition = -1, int releasePosition = -1) {
        int id = pitchMacros.empty() ? 0 : pitchMacros.back()->id + 1;
        pitchMacros.push_back(std::make_shared<Macro<MacroType::PITCH>>(id, loopPosition, releasePosition, values));
        return pitchMacros.back();
    }

    std::shared_ptr<Macro<MacroType::HI_PITCH>> addHiPitchMacro(std::vector<int> const& values, int loopPosition = -1, int releasePosition = -1) {
        int id = hiPitchMacros.empty() ? 0 : hiPitchMacros.back()->id + 1;
        hiPitchMacros.push_back(std::make_shared<Macro<MacroType::HI_PITCH>>(id, loopPosition, releasePosition, values));
        return hiPitchMacros.back();
    }

    std::shared_ptr<Macro<MacroType::DUTY>> addDutyMacro(std::vector<int> const& values, int loopPosition = -1, int releasePosition = -1) {
        int id = dutyMacros.empty() ? 0 : dutyMacros.back()->id + 1;
        dutyMacros.push_back(std::make_shared<Macro<MacroType::DUTY>>(id, loopPosition, releasePosition, values));
        return dutyMacros.back();
    }

    std::shared_ptr<DpcmSample> addDpcmSample(std::wstring const& name, std::vector<uint8_t> const& data) {
        int id = dpcmSamples.empty() ? 0 : dpcmSamples.back()->id + 1;
        dpcmSamples.push_back(std::make_shared<DpcmSample>(id, name, data));
        return dpcmSamples.back();
    }

    std::shared_ptr<Instrument> addInstrument(std::wstring const& name,
        std::shared_ptr<Macro<MacroType::VOLUME>> volumeMacro = {},
        std::shared_ptr<Macro<MacroType::ARPEGGIO>> arpeggioMacro = {},
        std::shared_ptr<Macro<MacroType::PITCH>> pitchMacro = {},
        std::shared_ptr<Macro<MacroType::HI_PITCH>> hiPitchMacro = {},
        std::shared_ptr<Macro<MacroType::DUTY>> dutyMacro = {}) {

        int id = instruments.empty() ? 0 : instruments.back()->getNesId() + 1;
        instruments.push_back(std::make_shared<Instrument>(id, name, volumeMacro, arpeggioMacro, pitchMacro, hiPitchMacro, dutyMacro));
        return instruments.back();
    }

    std::shared_ptr<Track> addTrack(int rows = 64, std::wstring const& name = L"New song") {
        tracks.emplace_back(std::make_shared<Track>(rows, name));
        return tracks.back();
    }

    void exportTxt(std::wstring const& path) const {
        std::wofstream file(path);
        if (!file.is_open()) {
            std::wcout << "Failed to open file " << path << std::endl;
            return;
        }

        file << "# FamiTracker text export 0.4.2" << std::endl;
        file << std::endl;

        file << "# Song information" << std::endl;
        file << "TITLE           \"" << title << "\"" << std::endl;
        file << "AUTHOR          \"" << author << "\"" << std::endl;
        file << "COPYRIGHT       \"" << copyright << "\"" << std::endl;
        file << std::endl;

        file << "# Song comment" << std::endl;
        file << "COMMENT \"" << comment << "\"" << std::endl;
        file << std::endl;

        file << "# Global settings" << std::endl;
        file << "MACHINE         " << int(machine) << std::endl;
        file << "FRAMERATE       " << framerate << std::endl;
        file << "EXPANSION       " << int(expansion) << std::endl;
        file << "VIBRATO         " << int(newVibratoStyle) << std::endl;
        file << "SPLIT           " << minTempo << std::endl;
        file << std::endl;

        if (int(expansion) & int(Expansion::N163)) {
			file << "# Namco 163 global settings" << std::endl;
            file << "N163CHANNELS    " << n163Channels << std::endl;
        }

        file << "# Macros" << std::endl;
        for (auto& macro : volumeMacros) {
            macro->exportTxt(file);
        }
        for (auto& macro : arpeggioMacros) {
            macro->exportTxt(file);
        }
        for (auto& macro : pitchMacros) {
            macro->exportTxt(file);
        }
        for (auto& macro : hiPitchMacros) {
            macro->exportTxt(file);
        }
        for (auto& macro : dutyMacros) {
            macro->exportTxt(file);
        }
        file << std::endl;

        file << "# DPCM samples" << std::endl;
        for (auto& dpcmSample : dpcmSamples) {
            dpcmSample->exportTxt(file);
        }
        file << std::endl;

        file << "# Instruments" << std::endl;
        for (auto& instrument : instruments) {
            instrument->exportTxt(file);
        }
        file << std::endl;

        file << "# Tracks" << std::endl;
        file << std::endl;
        for (auto& track : tracks) {
            track->exportTxt(file);
        }

        file << "# End of export" << std::endl;
        file.close();
    }
};