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
        startTime(std::chrono::high_resolution_clock::now()),
		currentSimTick(0),
		shouldClose(false),
		fixedLogicTime(0.0),
		currentTime(0.0),
		newTime(0.0),
		frameTime(0.0),
		frameTimeClamp(0.25),
		accumulator(0.0f),
		lastFrameTimeCalculation(0.0),
		averageFrameTime(0.0),
		averageFrameRate(0.0),
		canDisplayAverageFrameTime(false),
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

    void App::addScene(std::unique_ptr<Scene> scene, bool makeActive)
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

		ptrScene->load();
    }

    void App::close()
    {
        this->shouldClose = true;
        this->window->setToClose();        
    }

    const double App::getRuntimeSec() const
    {
		using clock = std::chrono::high_resolution_clock;
		return std::chrono::duration<double>(clock::now() - this->startTime).count();
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

	double App::getFrameTime()
	{
		return this->frameTime;
	}

	double App::getLogicTime()
	{
		return this->fixedLogicTime;
	}

	double App::getCurrentTime()
	{
		return this->currentTime;
	}

	void App::calculateAverageFrameTime()
	{
		this->canDisplayAverageFrameTime = false;

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
			this->canDisplayAverageFrameTime = true;
		}

		this->averageFrameTimeArray.push_back(this->frameTime);
	}

    void App::displayAverageFrameTime()
    {
		std::string message = "CurrentTime: " + std::to_string(this->currentTime) + " | FPS: " + std::to_string(this->averageFrameRate);
		this->window->setTitle(message);
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

	std::chrono::high_resolution_clock::time_point& App::getStartTime()
	{
		return this->startTime;
	}

	bool App::accumulate()
	{
		// prevent spiral of death
		if (this->frameTime > this->frameTimeClamp)
			this->frameTime = this->frameTimeClamp;

		this->accumulator += this->frameTime;

		return true;
	}

   // void App::execute()
   // {
   //     this->fixedLogicTime = 1.0 / this->config.LOGIC_TICK;
   //     this->currentTime = this->getRuntimeSec();

   //     while (true)
   //     {
			//if (this->shouldClose || this->window->shouldClose())
			//	break;

			//if (this->activeScene == nullptr)
			//	continue;

   //         this->newTime = this->getRuntimeSec();
   //         this->frameTime = this->newTime - this->currentTime;

			//this->checkWindowSize();

   //         if (this->frameTime >= (1.0 / this->config.MAX_RENDER_FPS)) // cap max fps
   //         {
			//	//VEL3D_LOG_TRACE("frameTime:{}", this->frameTime);

			//	//this->calculateAverageFrameTime();
			//	//this->displayAverageFrameTime();
			//	
   //             this->currentTime = this->newTime;

			//	if (!this->accumulate())
			//		continue;

			//	this->window->updateInputState();
			//	this->window->update();
			//	
   //             while (this->accumulator >= this->fixedLogicTime)
   //             {
			//		this->currentSimTick++;

			//		const float flt = static_cast<float>(this->fixedLogicTime);


			//		this->preLogicUpdate(this->activeScene);


			//		this->activeScene->stepPhysics(flt);
			//		this->activeScene->updatePreviousTransforms();
			//		this->activeScene->fixedLoop(flt);
			//		this->activeScene->updateFixedAnimations(flt);
			//		this->activeScene->postPhysics(flt);

			//		if(this->audioDevice)
			//			this->audioDevice->cleanUpManagedSFX();


			//		this->postLogicUpdate(this->activeScene);


   //                 this->accumulator -= this->fixedLogicTime;
   //             }

			//	const float ft = static_cast<float>(this->frameTime);
			//	const float renderLerp = static_cast<float>(this->accumulator / this->fixedLogicTime);

			//	this->activeScene->updateAnimations(ft);
			//	this->activeScene->updateBillboards();
			//	this->activeScene->immediateLoop(ft, renderLerp);
			//	this->activeScene->updateTextActors();

			//	// clear all previous render target buffers, this is done here as doing it right before or right after
			//	// we draw, wouldn't work as far as I can tell at the moment as many stages can have many cameras and
			//	// many cameras can have many stages, meaning that if we clear render buffers after drawing to camera's render target
			//	// in one stage, if it's used in another stage things would be bad
			//	this->activeScene->clearAllRenderTargetBuffers(this->gpu);
			//	this->activeScene->draw(ft, renderLerp);
			//	this->window->renderGui();
			//	this->window->swapBuffers();
			//	this->update();
   //         }

   //     }
   // }

	void App::execute()
	{
		this->fixedLogicTime = 1.0 / this->config.LOGIC_TICK;

		// Time anchors
		this->currentTime = this->getRuntimeSec();
		this->newTime = this->currentTime;

		// Hitch resistance
		const int    maxStepsPerTick = 5;                 // cap catch-up
		const double maxDebtClamp = this->fixedLogicTime * 4.0; // drop excessive debt

		// Optional render cap (0 or negative = uncapped; VSYNC can still pace)
		const bool   capRender = (this->config.MAX_RENDER_FPS > 0.0);
		const double targetRenderDt = capRender ? (1.0 / this->config.MAX_RENDER_FPS) : 0.0;
		double       nextRenderTime = this->currentTime; // first frame can go now

		//int rendersPerLogic = 0;

		while (true)
		{
			if (this->shouldClose || this->window->shouldClose())
				break;

			if (this->activeScene == nullptr)
				continue;

			// --- Frame time bookkeeping (always) ---
			this->newTime = this->getRuntimeSec();
			this->frameTime = this->newTime - this->currentTime;
			this->currentTime = this->newTime;

			//VEL3D_LOG_DEBUG("{}", this->frameTime);

			if (!this->accumulate())
				continue;

			//VEL3D_LOG_DEBUG("{}", this->accumulator);

			// Process window/input once per presented frame
			this->checkWindowSize();
			this->window->updateInputState();
			this->window->update();

			//rendersPerLogic++;

			// --- Fixed-step simulation with bounded catch-up ---
			int steps = 0;
			while (this->accumulator >= this->fixedLogicTime && steps < maxStepsPerTick)
			{
				//VEL3D_LOG_DEBUG("{}", rendersPerLogic);
				//rendersPerLogic = 0;

				this->currentSimTick++;
				this->activeScene->setTick(this->currentSimTick);

				const float flt = static_cast<float>(this->fixedLogicTime);

				this->activeScene->stepPhysics(flt);
				this->activeScene->updatePreviousTransforms();
				this->activeScene->fixedLoop(flt);
				this->activeScene->updateFixedAnimations(flt);
				this->activeScene->postPhysics(flt);

				if (this->audioDevice)
					this->audioDevice->cleanUpManagedSFX();

				this->accumulator -= this->fixedLogicTime;
				++steps;
			}

			// Spiral-of-death failsafe
			if (this->accumulator > maxDebtClamp)
				this->accumulator = 0.0;

			// --- Render with interpolation ---
			const float ft = static_cast<float>(this->frameTime);
			const double lerpRaw = this->accumulator / this->fixedLogicTime;
			const float  renderLerp = static_cast<float>(std::clamp(lerpRaw, 0.0, 1.0));

			this->activeScene->updateAnimations(ft);
			this->activeScene->updateBillboards();
			this->activeScene->immediateLoop(ft, renderLerp);
			this->activeScene->updateTextActors();

			this->activeScene->clearAllRenderTargetBuffers(this->gpu);
			this->activeScene->draw(ft, renderLerp);
			this->window->renderGui();
			this->window->swapBuffers();
			this->update();

			// --- Render pacing (no more if-gate) ---
			if (capRender)
			{
				// Set/advance the next render deadline
				if (nextRenderTime <= 0.0)
					nextRenderTime = this->currentTime + targetRenderDt;
				else
					nextRenderTime += targetRenderDt;

				// If we fell behind badly, re-sync (avoid long busy-spins)
				const double now = this->getRuntimeSec();
				if (nextRenderTime < now - this->frameTimeClamp)
					nextRenderTime = now;

				// Busy-spin until the render deadline (no Sleep)
				while (this->getRuntimeSec() < nextRenderTime) {
					// pure spin; keep it tight for stable cadence
					// (if you want lower CPU, add an occasional yield—but that adds jitter)
				}
			}
		}
	}



}