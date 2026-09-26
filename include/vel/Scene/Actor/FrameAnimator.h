#pragma once

#include <cstdint>

namespace vel
{
	class FrameAnimator
	{
	private:
		uint32_t		frameCount;
		uint32_t		currentFrame;
		float			currentCycleTime;
		uint32_t		currentCycle;
		
	public:
		float			framesPerSecond;
		uint32_t		pauseAfterCycles;
		bool			reverse;
		bool			paused;

		FrameAnimator(uint32_t frameCount, float fps);

		uint32_t		update(float frameTime);
		uint32_t		getCurrentFrame();
	};
}