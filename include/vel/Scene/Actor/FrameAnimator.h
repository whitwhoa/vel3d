#pragma once

namespace vel
{
	class FrameAnimator
	{
	private:
		float				frameCount;
		unsigned int		currentFrame;
		float				currentCycleTime;
		unsigned int		currentCycle;
		
	public:
		float				framesPerSecond;
		unsigned int		pauseAfterCycles;
		bool				reverse;
		bool				paused;

		FrameAnimator(float frameCount, float fps);

		unsigned int		update(float frameTime);
		unsigned int		getCurrentFrame();
	};
}