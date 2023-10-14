#pragma once
#include "commons.h"

class Effect {
public:
	std::wstring string;

	explicit Effect(std::wstring const& string) : string(string) {}
};