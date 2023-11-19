#pragma once
#include "commons.h"
#include "json.hpp"
#include "MidiState.h"
#include "Note.h"
#include "Preset.h"

class FileSettingsJson {
private:
	template<class T> void load(T& value, std::string const& name) {
		load(value, name, json);
	}

	template<class T> static void load(T& value, std::string const& name, nlohmann::json const& json) {
		value = json.value(name, value);
	}

	static void load(std::bitset<int(NesChannel::CHANNEL_COUNT)>& value, std::string const& name, nlohmann::json const& json) {
		if (!json.contains(name)) {
			return;
		}
		value.reset();
		for (auto const& channel : json[name]) {
			value.set(channel);
		}
	}

public:
	nlohmann::json json{};

	bool useVrc6 = true;
	int searchDepth = 8;
	int rowsPerPattern = 256;
	double maxDetuneSemitones = 0.125;
	bool adjustSpeed = true;
	bool mergeEmptyRows = true;
	double minDetuneHz = 0.5;
	bool preemptiveNoteCut = true;
	bool preemptiveNoteCutNoise = false;

	std::array<bool, MidiState::CHANNEL_COUNT> channelsEnabled{};
	std::array<double, MidiState::CHANNEL_COUNT> detuneSemitones{};
	std::array<double, MidiState::CHANNEL_COUNT> volumeMultiplier{};
	std::array<int, MidiState::CHANNEL_COUNT> minNesChannels{};
	std::array<int, MidiState::CHANNEL_COUNT> maxNesChannels{};
	std::array<bool, MidiState::CHANNEL_COUNT> lowerKeysFirst{};

	std::array<std::bitset<int(NesChannel::CHANNEL_COUNT)>, MidiState::CHANNEL_COUNT> allowedNesChannels{};

	explicit FileSettingsJson(std::wstring const& path) {
		channelsEnabled.fill(true);
		detuneSemitones.fill(0);
		volumeMultiplier.fill(1);
		minNesChannels.fill(0);
		maxNesChannels.fill(4);
		lowerKeysFirst.fill(false);

		allowedNesChannels[0].set();
		allowedNesChannels.fill(allowedNesChannels[0]);

		std::ifstream file(path);
		if (!file) {
			return;
		}

		json = nlohmann::json::parse(file);

		load(useVrc6, "use_vrc6");
		load(searchDepth, "search_depth");
		load(rowsPerPattern, "rows_per_pattern");
		load(maxDetuneSemitones, "max_detune_semitones");
		load(adjustSpeed, "adjust_speed");
		load(mergeEmptyRows, "merge_empty_rows");
		load(minDetuneHz, "min_detune_hz");
		load(preemptiveNoteCut, "preemptive_note_cut");
		load(preemptiveNoteCutNoise, "preemptive_note_cut_noise");

		for (auto const& channel : json["disabled_channels"]) {
			channelsEnabled[channel] = false;
		}

		for (auto const& channelJson : json["midi_channels"]) {
			int channel = channelJson["id"];
			load(detuneSemitones[channel], "detune_semitones", channelJson);
			load(volumeMultiplier[channel], "volume_multiplier", channelJson);
			load(minNesChannels[channel], "min_nes_channels", channelJson);
			load(maxNesChannels[channel], "max_nes_channels", channelJson);
			load(lowerKeysFirst[channel], "lower_keys_first", channelJson);

			load(allowedNesChannels[channel], "allowed_nes_channels", channelJson);
		}
	}
};