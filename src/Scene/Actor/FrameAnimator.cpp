#include <cmath>

#include <vel/Scene/Actor/FrameAnimator.h>

#include <vel/Util/Assert.h>

namespace vel
{
	FrameAnimator::FrameAnimator(uint32_t frameCount, float fps) :
		frameCount(frameCount),
		framesPerSecond(fps),
		currentFrame(0),
		currentCycleTime(0.0f),
		currentCycle(1),
		paused(false),
		pauseAfterCycles(0),
		reverse(false)
	{
		VEL_ASSERT(this->frameCount > 0, "FrameAnimator::FrameAnimator(): Frame count must be greater than zero.");
		VEL_ASSERT(std::isfinite(this->framesPerSecond) && this->framesPerSecond > 0.0f, "FrameAnimator::FrameAnimator(): Frames per second must be finite and greater than zero.");
	}

	unsigned int FrameAnimator::getCurrentFrame()
	{
		return this->currentFrame;
	}

	unsigned int FrameAnimator::update(float frameTime)
	{
		if (this->paused || this->frameCount == 1)
			return this->currentFrame;

		float secondsPerFrame = 1.f / this->framesPerSecond;

		this->currentCycleTime += frameTime;

		// Check if we completed a full animation cycle
		if (this->currentCycleTime >= secondsPerFrame * static_cast<float>(this->frameCount))
		{
			if (this->pauseAfterCycles == this->currentCycle)
			{
				this->paused = true;
				this->currentCycle = 1;
			}
			else
			{
				this->currentCycle++;
			}

			this->currentCycleTime = 0.f;
		}

		uint32_t nextFrame = static_cast<uint32_t>(this->currentCycleTime / secondsPerFrame);

		// Clamp to last valid frame index
		if (nextFrame >= this->frameCount)
			nextFrame = this->frameCount - 1;

		this->currentFrame = this->reverse ? (this->frameCount - 1) - nextFrame : nextFrame;

		return this->currentFrame;
	}
}