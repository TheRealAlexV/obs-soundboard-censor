#include <windows.h>
#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-soundboard-censor", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Soundboard and Censorship audio filter plugin for OBS Studio";
}

extern void register_censor_filter();

bool obs_module_load(void)
{
	register_censor_filter();
	blog(LOG_INFO, "obs-soundboard-censor loaded successfully");
	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "obs-soundboard-censor unloaded");
}
