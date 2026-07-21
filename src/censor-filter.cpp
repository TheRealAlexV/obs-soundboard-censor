#include "censor-filter.h"
#include <obs-frontend-api.h>
#include <obs-data.h>

static const char *censor_filter_get_name(void *)
{
	return obs_module_text("CensorFilter");
}

static void censor_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "bleep_file", "");
	obs_data_set_default_double(settings, "bleep_volume", 0.8);
}

static void censor_filter_update(void *data, obs_data_t *settings)
{
	struct censor_filter_data *cfd = (struct censor_filter_data *)data;
	const char *newFile = obs_data_get_string(settings, "bleep_file");
	float newVol = (float)obs_data_get_double(settings, "bleep_volume");

	pthread_mutex_lock(&cfd->mutex);

	if (!cfd->bleep_file.empty() && cfd->bleep_file != newFile)
		cfd->bleep_audio.unload();
	cfd->bleep_file = newFile;
	cfd->bleep_volume = newVol;

	if (!cfd->bleep_audio.loaded() && newFile[0] != '\0')
		cfd->bleep_audio.load(newFile);

	cfd->bleep_position = 0;

	pthread_mutex_unlock(&cfd->mutex);
}

static obs_properties_t *censor_filter_properties(void *)
{
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_path(props, "bleep_file",
				obs_module_text("BleepFile"),
				OBS_PATH_FILE,
				obs_module_text("AudioFiles"), nullptr);
	obs_properties_add_float_slider(props, "bleep_volume",
					obs_module_text("BleepVolume"),
					0.0, 1.0, 0.01);
	return props;
}

static void censor_hotkey_pressed(void *data, obs_hotkey_id id,
				  obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	struct censor_filter_data *cfd = (struct censor_filter_data *)data;
	pthread_mutex_lock(&cfd->mutex);
	cfd->hotkey_active = pressed;
	if (pressed)
		cfd->bleep_position = 0;
	pthread_mutex_unlock(&cfd->mutex);
}

static void *censor_filter_create(obs_data_t *settings, obs_source_t *filter)
{
	struct censor_filter_data *cfd = new censor_filter_data();
	cfd->context = filter;
	cfd->bleep_volume = 0.8f;
	cfd->hotkey_active = false;
	cfd->bleep_position = 0;

	pthread_mutex_init(&cfd->mutex, nullptr);

	cfd->hotkey_id = obs_hotkey_register_source(filter,
		"censor_filter_hotkey",
		obs_module_text("HoldToCensor"),
		censor_hotkey_pressed, cfd);

	censor_filter_update(cfd, settings);
	return cfd;
}

static void censor_filter_destroy(void *data)
{
	struct censor_filter_data *cfd = (struct censor_filter_data *)data;
	obs_hotkey_unregister(cfd->hotkey_id);
	pthread_mutex_destroy(&cfd->mutex);
	delete cfd;
}

static struct obs_audio_data *
censor_filter_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct censor_filter_data *cfd = (struct censor_filter_data *)data;

	pthread_mutex_lock(&cfd->mutex);

	if (!cfd->hotkey_active || !cfd->bleep_audio.loaded()) {
		pthread_mutex_unlock(&cfd->mutex);
		return audio;
	}

	const AudioSamples &bleep = cfd->bleep_audio.samples();
	size_t totalBleepFrames = bleep.total_frames;
	size_t numChannels = bleep.channels;
	if (totalBleepFrames == 0 || numChannels == 0) {
		pthread_mutex_unlock(&cfd->mutex);
		return audio;
	}

	float **adata = (float **)audio->data;
	uint32_t audioFrames = audio->frames;

	for (size_t ch = 0; ch < numChannels && ch < MAX_AUDIO_CHANNELS; ch++) {
		if (!adata[ch]) continue;
		for (uint32_t i = 0; i < audioFrames; i++) {
			size_t bleepPos = (cfd->bleep_position + i) % totalBleepFrames;
			size_t sampleIdx = bleepPos * numChannels + ch;
			adata[ch][i] = (sampleIdx < bleep.data.size())
				? bleep.data[sampleIdx] * cfd->bleep_volume
				: 0.0f;
		}
	}

	cfd->bleep_position = (cfd->bleep_position + audioFrames) % totalBleepFrames;

	pthread_mutex_unlock(&cfd->mutex);
	return audio;
}

void register_censor_filter()
{
	static struct obs_source_info info = {};
	info.id = "censor_filter";
	info.type = OBS_SOURCE_TYPE_FILTER;
	info.output_flags = OBS_SOURCE_AUDIO;
	info.get_name = censor_filter_get_name;
	info.create = censor_filter_create;
	info.destroy = censor_filter_destroy;
	info.update = censor_filter_update;
	info.get_defaults = censor_filter_defaults;
	info.get_properties = censor_filter_properties;
	info.filter_audio = censor_filter_filter_audio;
	obs_register_source(&info);
}
