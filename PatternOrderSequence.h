#pragma once
#include "commons.h"
#include "Pattern.h"

class PatternOrderSequence {
public:
	std::array<std::shared_ptr<Pattern>, Pattern::CHANNELS> columns;

	explicit PatternOrderSequence(std::array<std::shared_ptr<Pattern>, Pattern::CHANNELS> const& columns) : columns(columns) {}

	explicit PatternOrderSequence(std::shared_ptr<Pattern> pattern) : columns { pattern, pattern, pattern, pattern, pattern, pattern, pattern, pattern } {}

	std::shared_ptr<Pattern>& getPattern(NesChannel channel) {
		return columns[int(channel)];
	}

	const std::shared_ptr<Pattern>& getPattern(NesChannel channel) const {
		return columns[int(channel)];
	}

	Column& getColumn(NesChannel channel) {
		return getPattern(channel)->getColumn(channel);
	}

	const Column& getColumn(NesChannel channel) const {
		return getPattern(channel)->getColumn(channel);
	}
};