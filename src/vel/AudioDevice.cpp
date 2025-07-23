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
		// Wipe currentBGM
		for (auto& groupPair : this->currentBGM) 
		{
			for (auto& soundPair : groupPair.second) 
			{
				ma_sound_uninit(soundPair.second);
				delete soundPair.second;
			}
		}
		this->currentBGM.clear();

		// Wipe managedSFX
		for (auto& groupPair : this->managedSFX)
		{
			for (ma_sound* sound : groupPair.second) 
			{
				ma_sound_uninit(sound);
				delete sound;
			}
		}
		this->managedSFX.clear();

		// Wipe unmanagedSFX
		for (auto& groupPair : this->unmanagedSFX)
		{
			for (ma_sound* sound : groupPair.second) 
			{
				ma_sound_uninit(sound);
				delete sound;
			}
		}
		this->unmanagedSFX.clear();

		// Wipe bgmSounds
		this->bgmSounds.clear();

		// Wipe sfxSounds
		for (auto& soundPair : this->sfxSounds)
		{
			ma_sound_uninit(soundPair.second);
			delete soundPair.second;
		}
		this->sfxSounds.clear();

		// Wipe sound groups and engine
		ma_sound_group_uninit(&this->bgmVolGroup);
		ma_sound_group_uninit(&this->sfxVolGroup);
		ma_engine_uninit(&this->engine);
	}

	unsigned int AudioDevice::generateGroupKey()
	{
		unsigned int key = AudioDevice::nextGroupKey++;

		this->managedSFX.try_emplace(key);
		this->unmanagedSFX.try_emplace(key);
		this->currentBGM.try_emplace(key);

		return key;
	}

	void AudioDevice::setCurrentGroupKey(unsigned int key)
	{
		this->currentGroupKey = key;
	}

	std::string AudioDevice::loadBGM(const std::string& path)
	{
		std::filesystem::path p(path);
		std::string name = p.stem().string();

		auto it = this->usages.find(name);
		if (it != this->usages.end()) 
		{
			it->second += 1;

			LOG_TO_CLI_AND_FILE("Existing BGM, bypass reload: " + name);
		}
		else
		{
			this->bgmSounds[name] = path;
			this->usages[name] = 1;

			LOG_TO_CLI_AND_FILE("Loading new BGM: " + name);
		}

		return name;
	}

	std::string AudioDevice::loadSFX(const std::string& path)
	{
		std::filesystem::path p(path);
		std::string name = p.stem().string();

		auto it = this->usages.find(name);
		if (it != this->usages.end())
		{
			it->second += 1;

			LOG_TO_CLI_AND_FILE("Existing SFX, bypass reload: " + name);
		}
		else
		{
			ma_sound* sfx = new ma_sound;
			if (ma_sound_init_from_file(&this->engine, path.c_str(), MA_SOUND_FLAG_DECODE, &this->sfxVolGroup, NULL, sfx) != MA_SUCCESS)
			{
				delete sfx;
				LOG_CRASH("Failed to load SFX: " + path);
			}

			this->sfxSounds[name] = sfx;
			this->usages[name] = 1;

			LOG_TO_CLI_AND_FILE("Loading new SFX: " + name);
		}

		return name;
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
		delete s;
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
			ma_sound_stop(pair.second);

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
			ma_sound_start(pair.second);

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

	void AudioDevice::playBGM(const std::string& name)
	{
		auto& bgmMap = this->currentBGM.at(this->currentGroupKey);
		if (bgmMap.find(name) != bgmMap.end())
		{
			LOG_TO_CLI_AND_FILE("Skipping duplicate BGM init for: " + name);
			return;
		}

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

	void AudioDevice::removeGroup(unsigned int key)
	{
		auto& activeBGM = this->currentBGM.at(key);
		for (const auto& pair : activeBGM)
		{
			ma_sound_stop(pair.second);
			ma_sound_uninit(pair.second);
			delete pair.second;
		}
		this->currentBGM.erase(key);

		auto& activeManagedSFX = this->managedSFX.at(key);
		for (ma_sound* s : activeManagedSFX)
		{
			ma_sound_stop(s);
			ma_sound_uninit(s);
			delete s;
		}
		this->managedSFX.erase(key);

		auto& activeUnmanagedSFX = this->unmanagedSFX.at(key);
		for (ma_sound* s : activeUnmanagedSFX)
		{
			ma_sound_stop(s);
			ma_sound_uninit(s);
			delete s;
		}
		this->unmanagedSFX.erase(key);
	}

	void AudioDevice::removeSound(const std::string& name)
	{
		auto it = this->usages.find(name);
		if (it == this->usages.end())
		{
			LOG_CRASH("Attempting to remove sound that does not exist: " + name);
		}

		this->usages[name]--;

		if (this->usages[name] == 0)
		{
			LOG_TO_CLI_AND_FILE("Full remove sound template: " + name);

			auto itBGM = this->bgmSounds.find(name);
			if (itBGM != this->bgmSounds.end())
			{
				this->bgmSounds.erase(name);
				return;
			}
			
			ma_sound* tmpSFX = this->sfxSounds[name];
			ma_sound_uninit(tmpSFX);
			delete tmpSFX;
			this->sfxSounds.erase(name);

			return;
		}

		LOG_TO_CLI_AND_FILE("Decrement sound template usage count, retain: " + name);
	}

}
