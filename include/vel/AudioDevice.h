#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include "miniaudio/miniaudio.h"
#include "glm/glm.hpp"

namespace vel
{
	class AssetManager;

	class AudioDevice
	{
	private:
		friend class AssetManager;

		ma_engine engine;
		ma_sound_group sfxGroup;
		ma_sound_group bgmGroup;

		std::unordered_map<std::string, ma_sound*> sounds; /* Holds sound templates, AssetManager tracks scene usage*/

		ma_sound* currentBGM; /* Track which background track is currently playing */
		
		std::vector<ma_sound*> managedSFX; /* When a sound is loaded, it can be flagged as managed. If flagged as managed, a pointer
													to the copy of the sound will be saved in this vector, and when the sound is no longer
													playing, its memory will be freed and the pointer removed from the vector. If however
													the sound is not managed, it is up to the application to track the state of the sound
													and free its memory when no longer required by passing the pointer into the freeSound()
													public member of AudioDevice (as we want AudioDevice to be a wrapper around the miniaudio
													library) */
		
		void loadBGM(const std::string& path); /* Loads a background track, meaning a track that streams and loops and is part of
													the bgmGroup sound group */

		void loadSFX(const std::string& path, bool spatial, bool looping);

	public:
		AudioDevice();
		~AudioDevice();

		// getting and setting position need to wrap ma_audio listener functionality

		void freeSound(ma_sound* s); /* Removes sound from engine that was flagged to not be managed by AudioDevice */

	};
}
