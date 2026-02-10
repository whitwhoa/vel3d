#pragma once

#include "ozz/animation/runtime/animation.h"

namespace vel
{
	class SkelAnimController
	{
	private:
		// Current animation time ratio, in the unit interval [0,1], where 0 is the
		// beginning of the animation, 1 is the end.
		float timeRatio;

		// Time ratio of the previous update.
		float prevTimeRatio;

		// Playback speed, can be negative in order to play the animation backward.
		float playbackSpeed;

		// Animation play mode state: play/pause.
		bool playing;

		// Animation loop mode.
		bool loop;

	public:
		SkelAnimController();

		// Sets animation current time.
		// Returns the number of loops that happened during update. A positive number
		// means looping going foward, a negative number means looping going backward.
		int setTimeRatio(float ratio);

		// Gets animation current time.
		float getTimeRatio() const;

		// Gets animation time ratio of last update. Useful when the range between
		// previous and current frame needs to pe processed.
		float getPrevTimeRatio() const;

		// Sets playback speed.
		void setPlaybackSpeed(float s);

		// Gets playback speed.
		float getPlaybackSpeed() const;

		// Sets loop modes. If true, animation time is always clamped between 0 and 1.
		void setLoop(bool l);

		// Gets loop mode.
		bool getLoop() const;

		// Get if animation is playing, otherwise it is paused.
		bool getPlaying() const;

		void setPlaying(bool p);

		// Updates animation time if in "play" state, according to playback speed and
		// given frame time dt.
		// Returns the number of loops that happened during update. A positive number
		// means looping going foward, a negative number means looping going backward.
		int update(const ozz::animation::Animation& animation, float dt);

		// Resets all parameters to their default value.
		void reset();

	};
}