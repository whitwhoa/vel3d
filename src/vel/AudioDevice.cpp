#include <filesystem>

#include "vel/Log.h"
#include "vel/AudioDevice.h"


namespace vel
{
	unsigned int AudioDevice::nextSceneKey = 0;

	AudioDevice::AudioDevice() :
		currentSceneKey(0),
		paused(false)
	{
		if (ma_engine_init(NULL, &this->engine) != MA_SUCCESS) 
			LOG_CRASH("Failed to initialize audio engine.");

		ma_sound_group_init(&this->engine, 0, NULL, &this->sfxGroup);
		ma_sound_group_init(&this->engine, 0, NULL, &this->bgmGroup);

		ma_engine_listener_set_world_up(&this->engine, 0, 0.0f, 1.0f, 0.0f);
	}

	AudioDevice::~AudioDevice()
	{
		
	}

	unsigned int AudioDevice::generateSceneKey()
	{
		return AudioDevice::nextSceneKey++;
	}

	void AudioDevice::setCurrentSceneKey(unsigned int key)
	{
		this->currentSceneKey = key;
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

		this->bgmSounds[name] = bgm;
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

		this->sfxSounds[name] = sfx;
	}

	void AudioDevice::setDevicePosition(const glm::vec3& p)
	{
		ma_engine_listener_set_position(&this->engine, 0, p.x, p.y, p.z);
	}

	void AudioDevice::setDeviceDirection(const glm::vec3& d)
	{
		ma_engine_listener_set_direction(&this->engine, 0, d.x, d.y, d.z);
	}



}
