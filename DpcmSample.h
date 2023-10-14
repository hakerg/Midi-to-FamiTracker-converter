#pragma once
#include "commons.h"

class DpcmSample {
public:
	int id;
	std::wstring name;
	std::vector<uint8_t> data;

	DpcmSample(int id, std::wstring const& name, std::vector<uint8_t> const& data) : id(id), name(name), data(data) {}

	void exportTxt(std::wofstream& file) const {
		file << "DPCMDEF " << id << " " << data.size() << " \"" << name << "\"";
		for (int i = 0; i < data.size(); i++) {
			if (i % 32 == 0) {
				file << std::endl;
				file << "DPCM :";
			}
			file << " " << hex2(data[i]);
		}
		file << std::endl;
	}
};