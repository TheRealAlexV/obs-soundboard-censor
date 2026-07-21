#pragma once

#include <obs-module.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <pthread.h>
#include <string>

#include "audio-decoder.h"

struct censor_filter_data {
	obs_source_t *context;
	std::string bleep_file;
	float bleep_volume;

	AudioDecoder bleep_audio;
	size_t bleep_position;

	obs_hotkey_id hotkey_id;
	bool hotkey_active;

	pthread_mutex_t mutex;
};

void register_censor_filter();
