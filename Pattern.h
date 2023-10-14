#pragma once
#include "commons.h"
#include "Column.h"

class Pattern {
public:
	static const int CHANNELS = 8;

	int id;
	std::array<Column, CHANNELS> columns;

	explicit Pattern(int id, int rows) : id(id), columns{ Column(rows), Column(rows), Column(rows), Column(rows), Column(rows), Column(rows), Column(rows), Column(rows) } {}

	Column& getColumn(NesChannel channel) {
		return columns[int(channel)];
	}

	const Column& getColumn(NesChannel channel) const {
		return columns[int(channel)];
	}

	Cell& getCell(NesChannel channel, int row) {
		return getColumn(channel).cells[row];
	}

	const Cell& getCell(NesChannel channel, int row) const {
		return getColumn(channel).cells[row];
	}

	int getRows() const {
		return int(columns[0].cells.size());
	}

	void exportTxt(std::wofstream& file, std::array<int, CHANNELS>& columnSizes) const {
		file << "PATTERN " << hex2(id) << std::endl;
		for (int y = 0; y < getRows(); y++) {
			file << "ROW " << hex2(y);
			for (int x = 0; x < CHANNELS; x++) {
				file << " : ";
				getCell(NesChannel(x), y).exportTxt(file, columnSizes[x], NesChannel(x));
			}
			file << std::endl;
		}
		file << std::endl;
	}
};