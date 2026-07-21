#ifdef _WIN32
#include <windows.h>
#endif
#include <obs-module.h>
#include <obs-frontend-api.h>

#include "soundboard-dock.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-soundboard-censor", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Soundboard and Censorship audio filter plugin for OBS Studio";
}

extern void register_censor_filter();

static SoundboardDock *g_dock = nullptr;

bool obs_module_load(void)
{
	register_censor_filter();

	g_dock = new SoundboardDock(nullptr);
	obs_frontend_add_dock(g_dock);

	blog(LOG_INFO, "obs-soundboard-censor loaded successfully");
	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "obs-soundboard-censor unloaded");
}
