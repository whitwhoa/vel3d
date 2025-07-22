#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include "miniaudio/miniaudio.h"
#include "glm/glm.hpp"

/*
	I've been pretty strict with usage of smart pointers up to this point. The way I'm going to work with this library, at least
	initially, it's easier to just work with the raw pointers. Famous last words...

	TODO:
	Can we put tracking logic in this class, then disassociate it from AssetManager? That will make AudioDevice its own self contained
	object that manages all aspects of Audio files and playback, etc
	
	^^ That should work just fine, just have to keep in mind that if you want to re-use loaded assets between scenes, to always load the
	next scene before unloading the last scene, assuming you have all of your logic for loading and unloading sounds in Scene, but I mean,
	that goes for anything, even the assets that use the asset manager, actually, yeah, so you'd load and unload sounds in the scene, like
	add methods for doing that like we do for all of the other assets, and instead of using asset manager for audio, use the audio device,
	so we'll need each scene to hold a pointer into an audio device if it is not headless...wait, scenes are already headless, Scene extends
	HeadlessScene, so that should be just fine

	App will hold pointer to AudioDevice, and pass it into each Scene during the call to app->addScene() just like it does for the instance
	of asset manager
*/

namespace vel
{
	class AssetManager;

	class AudioDevice
	{
	private:
		friend class AssetManager;

		static unsigned int nextGroupKey;

		ma_engine engine; /* Core logic engine for processing sound */
		ma_sound_group sfxVolGroup; /* Sound group used for adjusting overall volume of sound effects */
		ma_sound_group bgmVolGroup; /* Sound group used for adjusting overall volume of background tracks */

		/* 
			Key is filename without extension, value is the full path to the file. Since streams are not copyable, when we need a new one,
			we re-initialize it. When user calls playBGM(const std::string& name) we use the name to pull the path from here, then initialize
			it into it's group container
		*/
		std::unordered_map<std::string, std::string> bgmSounds;

		/* 
			Holds sound templates, AssetManager tracks scene usage. When an object needs to play a sound, we obtain the pointer of the sound template 
			via lookup of the string name. This isn't great, but objects won't be invoking sounds every tick, and if it ever becomes an issue, we can 
			optimize down the road 
		*/
		std::unordered_map<std::string, ma_sound*> sfxSounds;

		/* 
			Key that we are currently using for indexing into the below structures 
		*/
		unsigned int currentGroupKey;

		/*
			Denotes whether or not our state is currently paused. This is required so that when we call cleanUpManagedSFX(), we can skip cleanup
			if below is true.
		*/
		bool paused;

		/* 
			Track which background sounds are currently playing. I know, an unordered_map of an unordered_map looks quite gross, but these tracks
			are not frequently accessed, and this gets us closer to our goal instead of stopping for a month and trying to concoct the most optimized
			holy grail data structure, that in the end probably ends up performing worse anyway....just keep it simple until you have a reason not to!
		*/
		std::unordered_map<unsigned int, std::unordered_map<std::string, ma_sound*>> currentBGM;

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
			Loads a background track into bgmSounds, meaning a track that streams and loops and is part of the bgmVolGroup sound group 
		*/
		void loadBGM(const std::string& path);

		/* 
			Loads an SFX track into sfxSounds, meaning a track that is preloaded and is a member of sfxVolGroup
		*/
		void loadSFX(const std::string& path);

		/* 
			Loop through all managedSFX and remove those which have stopped. Do not run if audio paused, as all sounds will be stopped during the 
			pause (miniaudio apparently does not distinguish between playing but paused, vs concluded) 
		*/
		void cleanUpManagedSFX();


	public:
		AudioDevice();
		~AudioDevice();

		unsigned int generateGroupKey();
		void setCurrentGroupKey(unsigned int key);

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
			Stop/Start all sounds that are associated with the current group key
		*/
		void pauseCurrentGroup();
		void unpauseCurrentGroup();

		/* 
			Creates and plays a copy of a sound after locating template sound from map. Memory is tracked by AudioDevice 
			and cleaned up when the playback concludes. No pointer is returned because state is managed by AudioDevice, and
			there is no reason for the caller to modify anything about a 2D non-looping sound.
		*/
		void play2DOneShotSFX(const std::string& name);

		/*
			Creates an plays a copy of a sound after locating template sound from map. Pointer is returned, and memory must
			be managed by the caller
		*/
		ma_sound* play2DLoopingSFX(const std::string& name);

		/*
			Creates and plays a copy of a sound after locating template sound from map. Since this is a 3D sound, a caller might
			want to manage it's state, regardless as to whether or not it is looping. For example a chime or buzz or some effect
			sound that should only play once, but has a velocity associated with it. Thus, we have a singular method for playing
			one of these sounds, and pass various options as parameters, ie looping, managed, etc. An example of a sound that
			would be 3D that could be managed would be a random dog bark off in the distance. You want to play it, but don't need
			to manage it as it's stationary to that position and not looping. If sound is managed, then nullptr is returned. I suppose
			a better example of a sound that would be positional and managed would be gunfire from npc characters.
		*/
		ma_sound* play3DSFX(const std::string& name, glm::vec3 pos, bool managed = true, bool looping = false);

		/*
			Update 3DSFX Position
		*/
		void update3DSFXPosition(ma_sound* s, glm::vec3 pos);

		/*
			Initialize and play background track. Pulls filepath from bgmSounds, initializes a stream, saves pointer to currentBGM
		*/
		void initBGM(const std::string& name);

		/*
			Pause background track
		*/
		void pauseBGM(const std::string& name);

		/*
			Unpause background track
		*/
		void unpauseBGM(const std::string& name);

		/*
			Update BGM volume
		*/
		void updateBGMVolume(float vol);

		/*
			Update SFX volume
		*/
		void updateSFXVolume(float vol);
	};
}
