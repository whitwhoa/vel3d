
#include <cmath>

#include "ozz/base/maths/math_ex.h"

#include "vel/SkelAnimController.h"

namespace vel
{
	SkelAnimController::SkelAnimController() :
        timeRatio(0.f),
        prevTimeRatio(0.f),
        playbackSpeed(1.f),
        playing(true),
        loop(true)
	{}

    void SkelAnimController::reset()
    {
        this->prevTimeRatio = 0.0f;
        this->timeRatio = 0.0f;
        this->playbackSpeed = 1.f;
        this->playing = true;
    }

    int SkelAnimController::update(const ozz::animation::Animation& animation, float dt)
    {
        float newRatio = this->timeRatio;

        if (this->playing)
        {
            newRatio = this->timeRatio + dt * this->playbackSpeed / animation.duration();
        }

        // Must be called even if time doesn't change, in order to update previous
        // frame time ratio. Uses setTimeRatio function in order to update
        // prevTimeRatio and wrap time value in the unit interval (depending on loop
        // mode).
        return this->setTimeRatio(newRatio);
    }

	int SkelAnimController::setTimeRatio(float ratio)
	{
        //  Number of loops completed within this->ratio, possibly negative if going backward.
        this->prevTimeRatio = this->timeRatio;

        if (this->loop) 
        {
            // Wraps in the unit interval [0:1]
            const float loops = std::floorf(ratio);
            this->timeRatio = ratio - loops;
            return static_cast<int>(loops);
        }
        else 
        {
            // Clamps in the unit interval [0:1].
            this->timeRatio = ozz::math::Clamp(0.f, ratio, 1.f);
            return 0;
        }
	}

    float SkelAnimController::getTimeRatio() const
    {
        return this->timeRatio;
    }

    float SkelAnimController::getPrevTimeRatio() const
    {
        return this->prevTimeRatio;
    }

    void SkelAnimController::setPlaybackSpeed(float s)
    {
        this->playbackSpeed = s;
    }

    float SkelAnimController::getPlaybackSpeed() const
    {
        return this->playbackSpeed;
    }

    void SkelAnimController::setLoop(bool l)
    {
        this->loop = l;
    }

    bool SkelAnimController::getLoop() const
    {
        return this->loop;
    }

    bool SkelAnimController::getPlaying() const
    {
        return this->playing;
    }

    void SkelAnimController::setPlaying(bool p)
    {
        this->playing = p;
    }

}