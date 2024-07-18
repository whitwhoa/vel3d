#pragma once

namespace vel
{

	class FrameAnimator
	{
	private:
		float				frameCount;
		float				framesPerSecond;
		unsigned int		currentFrame;
		float				currentCycleTime;
		unsigned int		currentCycle;
		bool				paused;
		unsigned int		pauseAfterCycles;
		bool				reverse;

	public:
		FrameAnimator(float frameCount, float fps);

		unsigned int		update(float frameTime);

		void				setFramesPerSecond(float fps);
		void				setPauseAfterCycles(unsigned int c);
		void				setPaused(bool p);
		bool				getPaused();

		unsigned int		getCurrentFrame();

		void				setReverse(bool r);
		bool				getIsPlayingReversed();

	};
}