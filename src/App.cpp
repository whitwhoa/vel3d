#include <limits>
#include <thread> 
#include <chrono>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#endif

#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/App.h>


using namespace std::chrono_literals;

namespace vel
{
    App::App() :
		activeScene(nullptr),
		shouldClose(false),
		fixedLogicTime(1.0 / Runtime::_config.logicTick),
		lastRunTime(0.0),
		deltaTimeClamp(0.25),
		lastFrameTime(0.0),
		accumulator(0.0f),
		lastFrameTimeCalculation(0.0),
		averageFrameTime(0.0)
    {		
#ifdef _WIN32

		const MMRESULT r = timeBeginPeriod(1);
		if(r != TIMERR_NOERROR)
			SPDLOG_WARN("App::App(): unable to adjust windows time interval");

#endif

    }

	App::~App()
	{
#ifdef _WIN32

		timeEndPeriod(1);

#endif
	}

	void App::update() {}

	Scene* App::getActiveScene()
	{
		return this->activeScene;
	}

	void App::removeScene(unsigned int id)
	{
		SPDLOG_DEBUG("App::removeScene: Removing Scene: {}", id);

		size_t i = 0;
		for (auto& s : this->scenes)
		{
			if (s->getId() == id)
				break;
			
			i++;
		}

		this->scenes.erase(this->scenes.begin() + i);
	}

	void App::swapScene(unsigned int id)
	{
		SPDLOG_DEBUG("App::swapScene: Swapping to Scene: {}", id);

		for (auto& s : this->scenes)
		{
			if (s->getId() == id)
			{
				// pause current scene audio if it holds a valid group key
				if (this->activeScene && this->activeScene->getAudioDeviceGroupKey() != -1)
					Runtime::_audioDevice->pauseCurrentGroup();

				// update active scene
				this->activeScene = s.get();
				
				// swap group keys in audio device and unpause all sounds if scene holds valid group key
				if (this->activeScene->getAudioDeviceGroupKey() != -1)
				{
					Runtime::_audioDevice->setCurrentGroupKey(this->activeScene->getAudioDeviceGroupKey());
					Runtime::_audioDevice->unpauseCurrentGroup();
				}
			}
		}
	}

    bool App::addScene(std::unique_ptr<Scene> scene, bool makeActive)
    {
		SPDLOG_DEBUG("App::addScene: Adding new Scene");
		
		this->scenes.push_back(std::move(scene));

		Scene* ptrScene = this->scenes.back().get();

		if (makeActive)
		{
			this->activeScene = ptrScene;

			if (this->activeScene->getAudioDeviceGroupKey() != -1)
				Runtime::_audioDevice->setCurrentGroupKey(this->activeScene->getAudioDeviceGroupKey());
		}

		return ptrScene->internalLoad();
    }

    void App::close()
    {
        this->shouldClose = true;      
    }

	void App::calculateAverageFrameTime()
	{
		if (Runtime::seconds() - this->lastFrameTimeCalculation >= 1.0)
		{
			this->lastFrameTimeCalculation = Runtime::seconds();

			double average = 0.0;
			for (auto& v : this->averageFrameTimeArray)
				average += v;

			average /= static_cast<double>(this->averageFrameTimeArray.size());

			this->averageFrameTime = average;
			Runtime::_frameRate = 1.0 / average;

			this->averageFrameTimeArray.clear();
		}

		this->averageFrameTimeArray.push_back(Runtime::_frameTime);
	}

	void App::checkWindowSize()
	{
		if (Runtime::_config.lockResToWin && Runtime::_window->getWindowSizeChanged())
		{
			Runtime::_window->setWindowSizeChanged(false);
			glm::ivec2 ws = Runtime::_window->getWindowSize();

			for (auto& s : this->scenes)
				s->updateAllCameraResolutions(ws.x, ws.y);
		}
	}

	void App::checkResolution()
	{
		if (Runtime::_window->getResolutionChanged())
		{
			Runtime::_window->setResolutionChanged(false);
			glm::ivec2 res = Runtime::_window->getResolution();

			for (auto& s : this->scenes)
				s->updateAllCameraResolutions(res.x, res.y);
		}
	}

	void App::execute()
	{
		// Hitch resistance
		const int maxStepsPerTick = 5;                       // cap catch-up
		const double maxDebtClamp = this->fixedLogicTime * 4.0; // drop excessive debt

		// Frame cap state
		const double renderCapHz = static_cast<double>(Runtime::_config.maxFps); // <= 0 = uncapped
		const double targetFrameSec = (renderCapHz > 0.0) ? (1.0 / renderCapHz) : 0.0;
		double capNextTimeSec = Runtime::seconds(); // Next time we are allowed to start a new frame

		while (true)
		{
			if (Runtime::_window->shouldClose())
				this->close();

			if (this->shouldClose)
				break;

			if (this->activeScene == nullptr)
				continue;

			// --------------------------------------------------------------------
			// 1) GPU queue depth control - do not let cpu issue new commands until
			//    the gpu has processed all previously sent commands
			// --------------------------------------------------------------------
			Runtime::_gpu->clientWaitSync();

			// --------------------------------------------------------------------
			// 2) Loop boundary timing
			// --------------------------------------------------------------------
			Runtime::_currentRunTime = Runtime::seconds();
			Runtime::_deltaTime = Runtime::_currentRunTime - this->lastRunTime;
			this->lastRunTime = Runtime::_currentRunTime;

			// Spiral of death prevention for deltaTime
			if (Runtime::_deltaTime > this->deltaTimeClamp)
				Runtime::_deltaTime = this->deltaTimeClamp;

			this->accumulator += Runtime::_deltaTime;

			// --------------------------------------------------------------------
			// 3) Input + OS events
			// --------------------------------------------------------------------
			this->checkWindowSize();
			this->checkResolution();
			Runtime::_window->updateInputState();

			// --------------------------------------------------------------------
			// 4) Fixed step simulation with bounded catch-up
			// --------------------------------------------------------------------
			int steps = 0;
			while (this->accumulator >= this->fixedLogicTime && steps < maxStepsPerTick)
			{
				Runtime::_currentSimTick++;

				const float flt = static_cast<float>(this->fixedLogicTime);

				this->activeScene->stepPhysics(flt);

				//double t1 = Runtime::seconds();
				this->activeScene->updateAnimators(flt);
				//double t2 = Runtime::seconds();
				//SPDLOG_TRACE("{:.15f}", t2 - t1);

				//double t1 = Runtime::seconds();
				this->activeScene->internalFixedLoop(flt);
				//double t2 = Runtime::seconds();
				//SPDLOG_TRACE("{:.15f}", t2 - t1);
				
				if (Runtime::_audioDevice)
					Runtime::_audioDevice->cleanUpManagedSFX();

				this->accumulator -= this->fixedLogicTime;
				++steps;
			}

			// Spiral of death prevention for accumulator
			if (this->accumulator > maxDebtClamp)
				this->accumulator = 0.0;

			// --------------------------------------------------------------------
			// 5) Render prep + interpolation
			// --------------------------------------------------------------------
			float renderLerp = static_cast<float>(std::clamp((this->accumulator / this->fixedLogicTime), 0.0, 1.0));

			float dt = static_cast<float>(Runtime::_deltaTime);
			
			//double t1 = Runtime::seconds();
			this->activeScene->lerpAnimators(renderLerp);
			//double t2 = Runtime::seconds();
			//SPDLOG_TRACE("{:.15f}", t2 - t1);

			this->activeScene->updateBillboards();

			this->activeScene->internalImmediateLoop(dt, renderLerp);

			this->activeScene->updateTextActors();

			// --------------------------------------------------------------------
			// 6) Render submit
			// --------------------------------------------------------------------
			this->activeScene->clearAllRenderTargetBuffers();
			//double t1 = Runtime::seconds();
			this->activeScene->draw(dt, renderLerp);
			//double t2 = Runtime::seconds();
			//SPDLOG_TRACE("{:.15f}", t2 - t1);

			// --------------------------------------------------------------------
			// 7) Insert fence after all GPU commands for this frame are queued
			// --------------------------------------------------------------------
			Runtime::_gpu->fenceAndFlush();

			// --------------------------------------------------------------------
			// 8) Swap buffers
			// --------------------------------------------------------------------
			Runtime::_window->swapBuffers();

			double now2 = Runtime::seconds();
			Runtime::_frameTime = now2 - this->lastFrameTime;
			this->lastFrameTime = now2;
			this->calculateAverageFrameTime();

			// --------------------------------------------------------------------
			// 9) User defined (optional, default does nothing, would be used for
			// swapping scenes for example)
			// --------------------------------------------------------------------
			this->update();

			// --------------------------------------------------------------------
			// 10) Frame cap (end-of-frame sleep)
			// --------------------------------------------------------------------
			if (targetFrameSec > 0.0)
			{
				// Schedule next target
				capNextTimeSec += targetFrameSec;

				double now3 = Runtime::seconds();

				// If we fell behind badly (breakpoint, hitch), resync so we don't "chase"
				if (now3 > capNextTimeSec + (targetFrameSec * 2.0))
					capNextTimeSec = now3 + targetFrameSec;

				double remainingSec = capNextTimeSec - now3;
				if (remainingSec > 0.0)
				{
					//// Coarse sleep most of it
					//if (remainingSec > 0.002) // ~2 ms
					//{
					//	auto coarseMs = static_cast<long long>((remainingSec - 0.002) * 1000.0);
					//	if (coarseMs > 0)
					//		std::this_thread::sleep_for(std::chrono::milliseconds(coarseMs));
					//}

					// Spin the last ~1 ms for precision
					while (Runtime::seconds() < capNextTimeSec)
					{
						// tight spin
					}
				}
			}
		}
	}





} // end of namespace