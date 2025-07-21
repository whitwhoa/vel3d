#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include "miniaudio/miniaudio.h"
#include "glm/glm.hpp"

/*
	TODO:
		> OK, so we just realized that we can save playback position of sounds and then restart them at these positions, so
			now we need to re-think our entire bgm sounds live forever idea. Can probably just blow everything away and
			reload when a scene is unloaded and then reloaded, so use the same style of flow as the rest of the assets in asset manager.
			Then the scene can track where background tracks should be resumed, as static members when/where necessary
		> Will still need the index or id based lookup since we could have multiple scenes loaded at any given time, we still
			need a way to stop and start sounds for specific scenes only
*/

namespace vel
{
	class AssetManager;

	class AudioDevice
	{
	private:
		friend class AssetManager;

		static unsigned int nextSceneKey;

		ma_engine engine; /* Core logic engine for processing sound */
		ma_sound_group sfxGroup; /* Sound group used for adjusting overall volume of sound effects */
		ma_sound_group bgmGroup; /* Sound group used for adjusting overall volume of background tracks */

		/* 
			Holds background sounds. Pointers held in this structure are not re-created every time they are played, they are resumed from the position 
			where they were stopped. 
		*/
		std::unordered_map<std::string, ma_sound*> bgmSounds; 

		/* 
			Holds sound templates, AssetManager tracks scene usage. When an object needs to play a sound, we obtain the pointer of the sound template 
			via lookup of the string name. This isn't great, but objects won't be invoking sounds every tick, and if it ever becomes an issue, we can 
			optimize down the road 
		*/
		std::unordered_map<std::string, ma_sound*> sfxSounds; 

		/* 
			Key that we are currently using for indexing into the below structures 
		*/
		unsigned int currentSceneKey; 

		/*
			Denotes whether or not our state is currently paused. This is required so that when we call cleanUpManagedSFX(), we can skip cleanup
			if below is true.
		*/
		bool paused;

		/* 
			Track which background sounds are currently playing 
		*/
		std::unordered_map<unsigned int, std::vector<ma_sound*>> currentBGMs; 

		/* 
			When a sound is played, it can be flagged as managed. If flagged as managed, a pointer to the copy of the sound will be saved in this 
			container, and when the sound is no longer playing, its memory will be freed and the pointer removed from the container. If however the sound 
			is not managed, it is up to the application to track the state of the sound and free its memory when no longer required by passing the pointer 
			into the freeSound() public member of AudioDevice (as we want AudioDevice to be a wrapper around the miniaudio library) 
		*/
		std::unordered_map<unsigned int, std::unordered_set<ma_sound*>> managedSFX; 

		/* 
			When we play a sound that the user has designated as being unmanaged, we stash it here so that we still know of it's existance for when we need
			to pause all audio. It is removed when user calls freeSound(). 
		*/
		std::unordered_map<unsigned int, std::unordered_set<ma_sound*>> unmanagedSFX;


		/* 
			Loads a background track into bgmSounds, meaning a track that streams and loops and is part of the bgmGroup sound group 
		*/
		void loadBGM(const std::string& path);

		/* 
			Loads an SFX track into sfxSounds, meaning a track that is preloaded, might be spatial, and might loop 
		*/
		void loadSFX(const std::string& path, bool spatial, bool looping);

		/* 
			Loop through all managedSFX and remove those which have stopped. Do not run if audio paused, as all sounds will be stopped during the 
			pause (miniaudio apparently does not distinguish between playing but paused, vs concluded) 
		*/
		void cleanUpManagedSFX();


	public:
		AudioDevice();
		~AudioDevice();

		unsigned int generateSceneKey();
		void setCurrentSceneKey(unsigned int key);

		/* 
			Sets the position of the listener 
		*/
		void setDevicePosition(const glm::vec3& p);

		/* 
			Sets direction of the listener 
		*/
		void setDeviceDirection(const glm::vec3& d);

		/* 
			Removes sound from engine that was flagged to not be managed by AudioDevice 
		*/
		void freeSound(ma_sound* s); 

		/* 
			Creates and plays a copy of a sound after locating template sound from map. If not managed, pointer to new object is returned and user
			is responsible for cleanup. If managed, then memory is tracked by AudioDevice and cleaned up when the sound concludes 
		*/
		ma_sound* play2DSFX(const std::string& name, bool managed);

	};
}
