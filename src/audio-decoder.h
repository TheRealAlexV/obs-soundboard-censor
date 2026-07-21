#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>

struct AudioSamples {
	std::vector<float> data;
	uint32_t sample_rate;
	size_t channels;
	size_t total_frames;
};

class AudioDecoder {
public:
	AudioDecoder();
	~AudioDecoder();

	bool load(const std::string &filepath);
	void unload();

	const AudioSamples &samples() const { return _samples; }
	bool loaded() const { return _loaded; }

private:
	AudioSamples _samples;
	bool _loaded = false;
};
