#pragma once
#include "commons.h"
#include "Pattern.h"
#include "PatternOrderSequence.h"

class Track {
private:
	std::array<int, int(NesChannel::CHANNEL_COUNT)> findColumnSizes() const {
		std::array<int, int(NesChannel::CHANNEL_COUNT)> columns = { 1, 1, 1, 1, 1, 1, 1, 1 };
		for (auto& pattern : patterns) {
			for (int x = 0; x < columns.size(); x++) {
				for (auto const& cell : pattern->getColumn(NesChannel(x)).cells) {
					columns[x] = max(columns[x], int(cell.effects.size()));
				}
			}
		}
		return columns;
	}

public:
	int rows;
	int speed = 6;
	int tempo = 150;
	std::wstring name;

	std::vector<std::shared_ptr<PatternOrderSequence>> patternOrder;
	std::vector<std::shared_ptr<Pattern>> patterns;

	Track(int rows, std::wstring const& name) : rows(rows), name(name) {}

	std::shared_ptr<Pattern>& getPatternFromOrder(int order, NesChannel channel) {
		return patternOrder[order]->getPattern(channel);
	}

	const std::shared_ptr<Pattern>& getPatternFromOrder(int order, NesChannel channel) const {
		return patternOrder[order]->getPattern(channel);
	}

	Column& getColumnFromOrder(int order, NesChannel channel) {
		return getPatternFromOrder(order, channel)->getColumn(channel);
	}

	const Column& getColumnFromOrder(int order, NesChannel channel) const {
		return getPatternFromOrder(order, channel)->getColumn(channel);
	}

	std::shared_ptr<Pattern> addPattern() {
		int id = patterns.empty() ? 0 : patterns.back()->id + 1;
		patterns.push_back(std::make_shared<Pattern>(id, rows));
		return patterns.back();
	}

	std::shared_ptr<PatternOrderSequence> addOrder(std::shared_ptr<Pattern> pattern) {
		patternOrder.push_back(std::make_shared<PatternOrderSequence>(pattern));
		return patternOrder.back();
	}

	std::shared_ptr<Pattern> addPatternAndOrder() {
		auto pattern = addPattern();
		addOrder(pattern);
		return pattern;
	}

	void exportTxt(std::wofstream& file) const {
		std::array<int, int(NesChannel::CHANNEL_COUNT)> columnSizes = findColumnSizes();

		file << "TRACK " << rows << " " << speed << " " << tempo << " \"" << name << "\"" << std::endl;
		file << "COLUMNS :";
		for (int column : columnSizes) {
			file << " " << column;
		}
		file << std::endl;
		file << std::endl;

		for (int y = 0; y < patternOrder.size(); y++) {
			file << "ORDER " << hex2(y) << " :";
			for (int x = 0; x < int(NesChannel::CHANNEL_COUNT); x++) {
				file << " " << hex2(getPatternFromOrder(y, NesChannel(x))->id);
			}
			file << std::endl;
		}
		file << std::endl;

		for (auto& pattern : patterns) {
			pattern->exportTxt(file, columnSizes);
		}
		file << std::endl;
	}
};