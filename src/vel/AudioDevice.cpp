#include <filesystem>

#include "vel/Log.h"
#include "vel/AudioDevice.h"


namespace vel
{
	AudioDevice::AudioDevice() :
		currentBGM(nullptr)
	{
		if (ma_engine_init(NULL, &this->engine) != MA_SUCCESS) 
			LOG_CRASH("Failed to initialize audio engine.");

		ma_sound_group_init(&this->engine, 0, NULL, &this->sfxGroup);
		ma_sound_group_init(&this->engine, 0, NULL, &this->bgmGroup);
	}

	AudioDevice::~AudioDevice()
	{
		
	}

	void AudioDevice::loadBGM(const std::string& path)
	{
		std::filesystem::path p(path);
		std::string name = p.stem().string();

		ma_sound* bgm = new ma_sound;
		if (ma_sound_init_from_file(&this->engine, path.c_str(), MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_LOOPING, &this->bgmGroup, NULL, bgm) != MA_SUCCESS)
		{
			delete bgm;
			LOG_CRASH("Failed to load BGM track: " + path);
		}

		this->sounds[name] = bgm;
	}

	void AudioDevice::loadSFX(const std::string& path, bool spatial, bool looping)
	{
		std::filesystem::path p(path);
		std::string name = p.stem().string();

		ma_sound* sfx = new ma_sound;
		ma_uint32 flags = MA_SOUND_FLAG_DECODE | (looping ? MA_SOUND_FLAG_LOOPING : 0);
		if (ma_sound_init_from_file(&this->engine, path.c_str(), flags, &this->sfxGroup, NULL, sfx) != MA_SUCCESS)
		{
			delete sfx;
			LOG_CRASH("Failed to load sound: " + path);
		}

		if (spatial)
			ma_sound_set_spatialization_enabled(sfx, MA_TRUE);

		this->sounds[name] = sfx;
	}

}
