#include <iostream>

#include "vel\FrameAnimator.h"

namespace vel
{

	FrameAnimator::FrameAnimator(float frameCount, float fps) :
		frameCount(frameCount),
		framesPerSecond(fps),
		currentFrame(0),
		currentCycleTime(0.0f),
		currentCycle(1),
		paused(false),
		pauseAfterCycles(0),
		reverse(false)
	{}

	void FrameAnimator::setReverse(bool r)
	{
		this->reverse = r;
	}

	bool FrameAnimator::getIsPlayingReversed()
	{
		return this->reverse;
	}

	unsigned int FrameAnimator::getCurrentFrame()
	{
		return this->currentFrame;
	}

	void FrameAnimator::setPaused(bool p)
	{
		this->paused = p;
	}

	bool FrameAnimator::getPaused()
	{
		return this->paused;
	}

	void FrameAnimator::setFramesPerSecond(float fps)
	{
		this->framesPerSecond = fps;
	}

	void FrameAnimator::setPauseAfterCycles(unsigned int c)
	{
		this->pauseAfterCycles = c;
	}

	unsigned int FrameAnimator::update(float frameTime)
	{
		if (this->paused)
			return this->currentFrame;

		float secondsPerFrame = 1.0f / this->framesPerSecond;

		this->currentCycleTime += frameTime;

		// Check if we completed a full animation cycle
		if (this->currentCycleTime >= secondsPerFrame * this->frameCount)
		{
			if (this->pauseAfterCycles == this->currentCycle)
			{
				this->paused = true;
				this->currentCycle = 1;
			}
			else
			{
				this->currentCycle += 1;
			}

			this->currentCycleTime = 0.0f;
		}

		unsigned int nextFrame = (unsigned int)(this->currentCycleTime / secondsPerFrame);

		// Clamp to last valid frame index
		if (nextFrame >= this->frameCount)
			nextFrame = this->frameCount - 1;

		unsigned int nextFrameReversed = (this->frameCount - nextFrame) - 1; // zero-based

		this->currentFrame = this->reverse ? nextFrameReversed : nextFrame;

		return this->currentFrame;
	}



}