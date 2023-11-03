#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <array>
#include <memory>
#include <optional>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <bitset>
#include <functional>

#include "bass.h"
#include "bassmidi.h"

std::wstring hex1(int number) {
	std::wstringstream stream;
	stream << std::uppercase << std::hex << std::setfill(L'0') << std::setw(1) << number;
	return stream.str();
}

std::wstring hex2(int number) {
	std::wstringstream stream;
	stream << std::uppercase << std::hex << std::setfill(L'0') << std::setw(2) << number;
	return stream.str();
}