#pragma once
#include "commons.h"
#include "DpcmSample.h"
#include "Note.h"

class KeyDpcmSample {
public:
	Note note;
	std::shared_ptr<DpcmSample> sample;
	int pitch;
	bool loop;
	int unknown = 0;
	int dCounter;

	KeyDpcmSample(Note note, std::shared_ptr<DpcmSample> sample, int pitch = 15, bool loop = false, int dCounter = -1) :
		note(note), sample(sample), pitch(pitch), loop(loop), dCounter(dCounter) {}

	void exportTxt(std::wofstream& file, int instrumentId) const {
		file << "KEYDPCM " << instrumentId << " " << note.getExportOctave() << " " << note.getExportTone() << " " << sample->id << " " << pitch
			<< " " << int(loop) << " " << unknown << " " << dCounter << std::endl;
	}
};