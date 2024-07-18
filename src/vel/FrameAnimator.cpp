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

	//unsigned int FrameAnimator::update(float frameTime)
	//{
	//	if (this->paused)
	//		return this->currentFrame;

	//	float numberOfFramesToPlayPerSecond = 1.0f / this->framesPerSecond;

	//	unsigned int nextFrame = (unsigned int)(this->currentCycleTime / numberOfFramesToPlayPerSecond);

	//	if (nextFrame == this->frameCount) // 0 based
	//	{
	//		if (this->pauseAfterCycles == this->currentCycle)
	//		{
	//			this->paused = true;
	//			this->currentCycle = 1;
	//		}
	//		else
	//		{
	//			this->currentCycle += 1;
	//		}
	//		
	//		this->currentCycleTime = 0.0f;

	//		return this->currentFrame;
	//	}

	//	this->currentCycleTime += frameTime;

	//	this->currentFrame = nextFrame;

	//	return this->currentFrame;
	//}

	unsigned int FrameAnimator::update(float frameTime)
	{
		if (this->paused)
			return this->currentFrame;

		float numberOfFramesToPlayPerSecond = 1.0f / this->framesPerSecond;

		unsigned int nextFrame = (unsigned int)(this->currentCycleTime / numberOfFramesToPlayPerSecond);
		unsigned int nextFrameReversed = (unsigned int)(this->frameCount - nextFrame);
		nextFrameReversed = nextFrameReversed - 1; // zero based index

		//std::cout << "nextFrame: " << nextFrame << "\n"
		//	<< "nextFrameReversed: " << nextFrameReversed << std::endl;

		if (nextFrame == this->frameCount) // 0 based
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

			return this->currentFrame;
		}

		this->currentCycleTime += frameTime;

		if (!this->reverse)
			this->currentFrame = nextFrame;
		else
			this->currentFrame = nextFrameReversed;

		//std::cout << "this->currentFrame: " << this->currentFrame << std::endl;

		return this->currentFrame;
	}



}