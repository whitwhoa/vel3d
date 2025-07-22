#include <filesystem>

#include "vel/Log.h"
#include "vel/AudioDevice.h"


namespace vel
{
	unsigned int AudioDevice::nextGroupKey = 0;

	AudioDevice::AudioDevice() :
		currentGroupKey(0),
		paused(false)
	{
		if (ma_engine_init(NULL, &this->engine) != MA_SUCCESS) 
			LOG_CRASH("Failed to initialize audio engine.");

		ma_sound_group_init(&this->engine, 0, NULL, &this->sfxVolGroup);
		ma_sound_group_init(&this->engine, 0, NULL, &this->bgmVolGroup);

		ma_engine_listener_set_world_up(&this->engine, 0, 0.0f, 1.0f, 0.0f);
	}

	AudioDevice::~AudioDevice()
	{
		// TODO: clean-up all the things.
	}

	unsigned int AudioDevice::generateGroupKey()
	{
		return AudioDevice::nextGroupKey++;
	}

	void AudioDevice::setCurrentGroupKey(unsigned int key)
	{
		this->currentGroupKey = key;
	}

	void AudioDevice::loadBGM(const std::string& path)
	{
		std::filesystem::path p(path);
		std::string name = p.stem().string();

		this->bgmSounds[name] = path;
	}

	void AudioDevice::loadSFX(const std::string& path)
	{
		std::filesystem::path p(path);
		std::string name = p.stem().string();

		ma_sound* sfx = new ma_sound;
		if (ma_sound_init_from_file(&this->engine, path.c_str(), MA_SOUND_FLAG_DECODE, &this->sfxVolGroup, NULL, sfx) != MA_SUCCESS)
		{
			delete sfx;
			LOG_CRASH("Failed to load sound: " + path);
		}

		this->sfxSounds[name] = sfx;
	}

	void AudioDevice::cleanUpManagedSFX()
	{
		if (this->paused)
			return;

		auto& sfxSet = this->managedSFX.at(this->currentGroupKey);
		for (auto it = sfxSet.begin(); it != sfxSet.end(); ) 
		{
			ma_sound* sound = *it;

			if (!ma_sound_is_playing(sound))
			{
				ma_sound_uninit(sound);
				delete sound;
				it = sfxSet.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void AudioDevice::freeSound(ma_sound* s)
	{
		ma_sound_uninit(s);
		this->unmanagedSFX.at(this->currentGroupKey).erase(s);
	}

	void AudioDevice::setDevicePosition(const glm::vec3& p)
	{
		ma_engine_listener_set_position(&this->engine, 0, p.x, p.y, p.z);
	}

	void AudioDevice::setDeviceDirection(const glm::vec3& d)
	{
		ma_engine_listener_set_direction(&this->engine, 0, d.x, d.y, d.z);
	}

	void AudioDevice::pauseCurrentGroup()
	{
		this->paused = true;

		auto& activeBGM = this->currentBGM.at(this->currentGroupKey);
		for (const auto& pair : activeBGM)
			ma_sound_stop(pair.second); // first is key, second is value

		auto& activeManagedSFX = this->managedSFX.at(this->currentGroupKey);
		for (ma_sound* s : activeManagedSFX)
			ma_sound_stop(s);

		auto& activeUnmanagedSFX = this->unmanagedSFX.at(this->currentGroupKey);
		for (ma_sound* s : activeUnmanagedSFX)
			ma_sound_stop(s);
	}

	void AudioDevice::unpauseCurrentGroup()
	{
		this->paused = false;

		auto& activeBGM = this->currentBGM.at(this->currentGroupKey);
		for (const auto& pair : activeBGM)
			ma_sound_start(pair.second); // first is key, second is value

		auto& activeManagedSFX = this->managedSFX.at(this->currentGroupKey);
		for (ma_sound* s : activeManagedSFX)
			ma_sound_start(s);

		auto& activeUnmanagedSFX = this->unmanagedSFX.at(this->currentGroupKey);
		for (ma_sound* s : activeUnmanagedSFX)
			ma_sound_start(s);
	}

	void AudioDevice::play2DOneShotSFX(const std::string& name)
	{
		ma_sound* clone = new ma_sound;
		if (ma_sound_init_copy(&this->engine, this->sfxSounds.at(name), 0, &this->sfxVolGroup, clone) == MA_SUCCESS) 
		{
			ma_sound_start(clone);
			this->managedSFX.at(this->currentGroupKey).insert(clone);
		}
		else 
		{
			delete clone;
			LOG_CRASH("Failed to play non-spatial one-shot sound.");
		}
	}

	ma_sound* AudioDevice::play2DLoopingSFX(const std::string& name)
	{
		ma_sound* clone = new ma_sound;
		if (ma_sound_init_copy(&this->engine, this->sfxSounds.at(name), 0, &this->sfxVolGroup, clone) == MA_SUCCESS)
		{
			ma_sound_set_looping(clone, MA_TRUE);
			ma_sound_start(clone);

			this->unmanagedSFX.at(this->currentGroupKey).insert(clone);
			return clone;
		}
		else
		{
			delete clone;
			LOG_CRASH("Failed to play non-spatial one-shot sound.");
		}
	}

	ma_sound* AudioDevice::play3DSFX(const std::string& name, glm::vec3 pos, bool managed, bool looping)
	{
		ma_sound* clone = new ma_sound;
		if (ma_sound_init_copy(&this->engine, this->sfxSounds.at(name), 0, &this->sfxVolGroup, clone) == MA_SUCCESS)
		{
			ma_sound_set_position(clone, pos.x, pos.y, pos.z);
			ma_sound_set_spatialization_enabled(clone, MA_TRUE);

			if (looping)
				ma_sound_set_looping(clone, MA_TRUE);

			ma_sound_start(clone);

			if (managed)
			{
				this->managedSFX.at(this->currentGroupKey).insert(clone);
				return nullptr;
			}

			this->unmanagedSFX.at(this->currentGroupKey).insert(clone);
			return clone;
		}
		else 
		{
			delete clone;
			LOG_CRASH("Failed to play positional sound.");
		}
	}

	void AudioDevice::update3DSFXPosition(ma_sound* s, glm::vec3 pos)
	{
		ma_sound_set_position(s, pos.x, pos.y, pos.z);
	}

	void AudioDevice::initBGM(const std::string& name)
	{
		auto& bgmMap = this->currentBGM.at(this->currentGroupKey);
		if (bgmMap.find(name) != bgmMap.end())
			return;

		std::string path = this->bgmSounds[name];

		ma_sound* bgm = new ma_sound;
		if (ma_sound_init_from_file(&this->engine, path.c_str(), MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_LOOPING, &this->bgmVolGroup, NULL, bgm) != MA_SUCCESS)
		{
			delete bgm;
			LOG_CRASH("Failed to load BGM track: " + path);
		}

		this->currentBGM.at(this->currentGroupKey)[name] = bgm;
	}

	void AudioDevice::pauseBGM(const std::string& name)
	{
		ma_sound_stop(this->currentBGM.at(this->currentGroupKey)[name]);
	}

	void AudioDevice::unpauseBGM(const std::string& name)
	{
		ma_sound_start(this->currentBGM.at(this->currentGroupKey)[name]);
	}

	void AudioDevice::updateBGMVolume(float vol)
	{
		ma_sound_group_set_volume(&this->bgmVolGroup, vol);
	}

	void AudioDevice::updateSFXVolume(float vol)
	{
		ma_sound_group_set_volume(&this->sfxVolGroup, vol);
	}

}
