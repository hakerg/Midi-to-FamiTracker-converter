#pragma once
#include "commons.h"
#include "Cell.h"

class Column {
public:
	std::vector<Cell> cells;

	explicit Column(int rows) {
		cells.resize(rows);
	}
};