#include <iostream>
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



#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"

#include "vel/logger.hpp"
#include "vel/App.h"

using namespace std::chrono_literals;

namespace vel
{
    App::App(Config conf, Window* w, GPU* gpu, AssetManager* am) :
        config(conf),
		window(w),
		gpu(gpu),
		assetManager(am),
		audioDevice(nullptr),

		activeScene(nullptr),
        startTime(std::chrono::steady_clock::now()),
		currentSimTick(0),
		shouldClose(false),

		fixedLogicTime(1.0 / this->config.LOGIC_TICK),

		loopTime(0.0),
		lastLoopTime(0.0),
		loopTimeClamp(0.25),

		frameTime(0.0),
		lastFrameTime(0.0),

		accumulator(0.0f),
		lastFrameTimeCalculation(0.0),
		averageFrameTime(0.0),
		averageFrameRate(0.0),
		pauseBufferClearAndSwap(false)
    {		
#ifdef _WIN32

		const MMRESULT r = timeBeginPeriod(1);
		if(r != TIMERR_NOERROR)
			VEL3D_LOG_WARN("App::App(): unable to adjust windows time interval");

#endif

		this->assetManager->loadShader("debug", "debug.vert", "", "debug.frag"); // used for bullet's debug drawer
		this->assetManager->loadShader("screen", "screen.vert", "", "screen.frag"); // join all frame buffer textures together before post-processing
		this->assetManager->loadShader("post", "post.vert", "", "post.frag"); // join all frame buffer textures together before post-processing
		this->assetManager->loadShader("composite", "composite.vert", "", "composite.frag"); // used for rendering texture to screen buffer
		this->assetManager->loadShader("text", "uber.vert", "", "uber.frag", {"IS_TEXT"}); // used for rendering text

		Texture* ptrDefaultWhite = this->assetManager->loadTexture("defaultWhite", this->config.DATA_DIR + "/textures/defaults/default.jpg");

		this->gpu->setScreenShader(this->assetManager->getShader("screen"));
		this->gpu->setPostShader(this->assetManager->getShader("post"));
		this->gpu->setCompositeShader(this->assetManager->getShader("composite"));
		this->gpu->setDefaultWhiteTextureHandle(ptrDefaultWhite->frames.at(0).dsaHandle);

    }

	App::~App()
	{
#ifdef _WIN32

		timeEndPeriod(1);

#endif
	}

	void App::update() {}

	void App::setAudioDevice(AudioDevice* ad)
	{
		this->audioDevice = ad;
	}

	Scene* App::getActiveScene()
	{
		return this->activeScene;
	}

	void App::forceImguiRender()
	{
		this->window->renderGui();
	}

	void App::showMouseCursor()
	{
		this->window->showMouseCursor();
	}

	void App::hideMouseCursor()
	{
		this->window->hideMouseCursor();
	}

	ImFont* App::getImguiFont(std::string key) const
	{
		return this->window->getImguiFont(key);
	}

	void App::removeScene(const std::string& name)
	{
		VEL3D_LOG_DEBUG("App::removeScene: Removing Scene: {}", name);

		size_t i = 0;
		for (auto& s : this->scenes)
		{
			if (s->getName() == name)
				break;
			
			i++;
		}

		this->scenes.erase(this->scenes.begin() + i);
	}
	
	bool App::sceneExists(const std::string& name)
	{
		for (auto& s : this->scenes)
			if (s->getName() == name)
				return true;

		return false;
	}

	Scene* App::getScene(const std::string& name)
	{
		for (auto& s : this->scenes)
			if (s->getName() == name)
				return s.get();

		return nullptr;
	}

	void App::swapScene(const std::string& name)
	{
		VEL3D_LOG_DEBUG("App::swapScene: Swapping to Scene: {}", name);

		for (auto& s : this->scenes)
		{
			if (s->getName() == name)
			{
				// pause current scene audio if it holds a valid group key
				if (this->activeScene && this->activeScene->getAudioDeviceGroupKey() != -1)
					this->audioDevice->pauseCurrentGroup();

				// update active scene
				this->activeScene = s.get();
				
				// swap group keys in audio device and unpause all sounds if scene holds valid group key
				if (this->activeScene->getAudioDeviceGroupKey() != -1)
				{
					this->audioDevice->setCurrentGroupKey(this->activeScene->getAudioDeviceGroupKey());
					this->audioDevice->unpauseCurrentGroup();
				}
			}
		}
	}

    bool App::addScene(std::unique_ptr<Scene> scene, bool makeActive)
    {
		// TODO: unsure if this is still required???
		if(this->window != nullptr && this->window->getImguiFrameOpen())
			this->forceImguiRender();

		std::string className = typeid(*scene).name();// name is "class Test" when we need just "Test", so trim off "class "
		className.erase(0, 6);
		scene->setName(className);

		VEL3D_LOG_DEBUG("App::addScene: Adding Scene: {}", className);

		// inject window size and resolution into scene
		scene->setWindowSize(this->window->getWindowSize().x, this->window->getWindowSize().y);
		scene->setResolution(this->window->getResolution().x, this->window->getResolution().y);
		scene->setAssetManager(this->assetManager);
		scene->setInputState(this->getInputState());

		if (this->audioDevice)
			scene->setAudioDevice(this->audioDevice);

		scene->initRenderTarget();
		
		this->scenes.push_back(std::move(scene));

		Scene* ptrScene = this->scenes.back().get();

		if (makeActive)
		{
			this->activeScene = ptrScene;

			if (this->activeScene->getAudioDeviceGroupKey() != -1)
				this->audioDevice->setCurrentGroupKey(this->activeScene->getAudioDeviceGroupKey());
		}

		return ptrScene->load();
    }

    void App::close()
    {
        this->shouldClose = true;
        this->window->setToClose();        
    }

	const double App::getRuntimeSec() const
	{
		using namespace std::chrono;
		return duration<double>(steady_clock::now() - this->startTime).count();
	}

    const InputState* App::getInputState() const
    {
        return this->window->getInputState();
    }

	GPU* App::getGPU()
	{
		return this->gpu;
	}

	AssetManager& App::getAssetManager()
	{
		return *this->assetManager;
	}

	double App::getLoopTime()
	{
		return this->loopTime;
	}

	double App::getFrameTime()
	{
		return this->frameTime;
	}

	double App::getLogicTime()
	{
		return this->fixedLogicTime;
	}

	void App::calculateAverageFrameTime()
	{
		if (this->getRuntimeSec() - this->lastFrameTimeCalculation >= 1.0)
		{
			this->lastFrameTimeCalculation = this->getRuntimeSec();

			double average = 0.0;
			for (auto& v : this->averageFrameTimeArray)
				average += v;

			average /= static_cast<double>(this->averageFrameTimeArray.size());

			this->averageFrameTime = average;
			this->averageFrameRate = 1.0 / average;

			this->averageFrameTimeArray.clear();
		}

		this->averageFrameTimeArray.push_back(this->frameTime);
	}

	bool App::getPauseBufferClearAndSwap()
	{
		return this->pauseBufferClearAndSwap;
	}

	void App::setPauseBufferClearAndSwap(bool in)
	{
		this->pauseBufferClearAndSwap = in;
	}

	void App::checkWindowSize()
	{
		if (this->config.LOCK_RES_TO_WIN && this->window->getWindowSizeChanged())
		{
			this->window->setWindowSizeChanged(false);
			glm::ivec2 ws = this->window->getWindowSize();

			for (auto& s : this->scenes)
			{
				s->setWindowSize(ws.x, ws.y);
				s->setResolution(ws.x, ws.y);

				for (auto& c : s->getCamerasInUse())
				{
					if(!c->getFixedResolution())
						c->setResolution(ws.x, ws.y);
				}
			}
		}		
	}

	std::chrono::steady_clock::time_point& App::getStartTime()
	{
		return this->startTime;
	}


	void App::execute()
	{
		// Hitch resistance
		const int maxStepsPerTick = 5;                       // cap catch-up
		const double maxDebtClamp = this->fixedLogicTime * 4.0; // drop excessive debt

		// Frame cap state
		const double renderCapHz = static_cast<double>(this->config.MAX_RENDER_FPS); // <= 0 = uncapped
		const double targetFrameSec = (renderCapHz > 0.0) ? (1.0 / renderCapHz) : 0.0;
		double capNextTimeSec = this->getRuntimeSec(); // Next time we are allowed to start a new frame

		while (true)
		{
			if (this->shouldClose || this->window->shouldClose())
				break;

			if (this->activeScene == nullptr)
				continue;

			// --------------------------------------------------------------------
			// 1) GPU queue depth control - do not let cpu issue new commands until
			//    the gpu has processed all previously sent commands
			// --------------------------------------------------------------------
			this->gpu->clientWaitSync();

			// --------------------------------------------------------------------
			// 2) Loop boundary timing
			// --------------------------------------------------------------------
			double now1 = this->getRuntimeSec();
			this->loopTime = now1 - this->lastLoopTime;
			this->lastLoopTime = now1;

			// Spiral of death prevention for loopTime
			if (this->loopTime > this->loopTimeClamp)
				this->loopTime = this->loopTimeClamp;

			this->accumulator += this->loopTime;

			// --------------------------------------------------------------------
			// 3) Input + OS events
			// --------------------------------------------------------------------
			this->checkWindowSize();
			this->window->updateInputState();
			this->window->update();

			// --------------------------------------------------------------------
			// 4) Fixed step simulation with bounded catch-up
			// --------------------------------------------------------------------
			int steps = 0;
			while (this->accumulator >= this->fixedLogicTime && steps < maxStepsPerTick)
			{
				this->currentSimTick++;
				this->activeScene->setTick(this->currentSimTick);

				const float flt = static_cast<float>(this->fixedLogicTime);

				this->activeScene->stepPhysics(flt);
				this->activeScene->updatePreviousTransforms();


				//double t1 = this->getRuntimeSec();
				this->activeScene->fixedLoop(flt);
				//double t2 = this->getRuntimeSec();
				//VEL3D_LOG_TRACE("{:.15f}", t2 - t1);


				this->activeScene->updateFixedAnimations(flt);
				this->activeScene->postPhysics(flt);

				if (this->audioDevice)
					this->audioDevice->cleanUpManagedSFX();

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

			float ft = static_cast<float>(this->loopTime);
			this->activeScene->updateAnimations(ft);
			this->activeScene->updateBillboards();

			this->activeScene->immediateLoop(ft, renderLerp);
			this->activeScene->updateTextActors();

			// --------------------------------------------------------------------
			// 6) Render submit
			// --------------------------------------------------------------------
			this->activeScene->clearAllRenderTargetBuffers(this->gpu);
			this->activeScene->draw(ft, renderLerp);
			this->window->renderGui();

			// --------------------------------------------------------------------
			// 7) Insert fence after all GPU commands for this frame are queued
			// --------------------------------------------------------------------
			this->gpu->fenceAndFlush();

			// --------------------------------------------------------------------
			// 8) Swap buffers
			// --------------------------------------------------------------------
			this->window->swapBuffers();

			double now2 = this->getRuntimeSec();
			this->frameTime = now2 - this->lastFrameTime;
			this->lastFrameTime = now2;
			this->calculateAverageFrameTime();
			this->activeScene->setFrameTime(this->frameTime);
			this->activeScene->setFrameRate(this->averageFrameRate);

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

				double now3 = this->getRuntimeSec();

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
					while (this->getRuntimeSec() < capNextTimeSec)
					{
						// tight spin
					}
				}
			}
		}
	}





} // end of namespace