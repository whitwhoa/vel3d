#include <iostream>
#include <limits>
#include <thread> 
#include <chrono>


#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"


#include "vel/App.h"
#include "vel/Log.h"

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
		fixedLogicTime(0.0f),
		currentTime(0.0f),
		newTime(0.0f),
		frameTime(0.0f),
		accumulator(0.0f),
		lastFrameTimeCalculation(0.0f),
		averageFrameTime(0.0f),
		averageFrameRate(0.0f),
		canDisplayAverageFrameTime(false),
		pauseBufferClearAndSwap(false)
    {		
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
		LOG_TO_CLI_AND_FILE("Removing Scene: " + name);

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
		LOG_TO_CLI_AND_FILE("Swapping to Scene: " + name);

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
		//scene->swapWhenLoaded = swapWhenLoaded;

		LOG_TO_CLI_AND_FILE("Adding Scene: " + className);


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

    const float App::time() const
    {
        std::chrono::duration<float> t = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - this->startTime);
        return t.count();
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

	float App::getFrameTime()
	{
		return (float)this->frameTime;
	}

	float App::getLogicTime()
	{
		return (float)this->fixedLogicTime;
	}

	void App::calculateAverageFrameTime()
	{
		this->canDisplayAverageFrameTime = false;

		if (this->time() - this->lastFrameTimeCalculation >= 1.0)
		{
			this->lastFrameTimeCalculation = this->time();

			float average = 0.0;

			for (auto& v : this->averageFrameTimeArray)
				average += v;

			average = average / this->averageFrameTimeArray.size();

			this->averageFrameTime = average;
			this->averageFrameRate = 1000 / (average * 1000);

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

	float App::getCurrentTime()
	{
		return this->currentTime;
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

    void App::execute()
    {
        this->fixedLogicTime = 1 / this->config.LOGIC_TICK;
        this->currentTime = this->time();

        while (true)
        {
			if (this->shouldClose || this->window->shouldClose())
				break;

			if (this->activeScene == nullptr)
				continue;

            this->newTime = this->time();
            this->frameTime = this->newTime - this->currentTime;

			this->checkWindowSize();

            if (this->frameTime >= (1 / this->config.MAX_RENDER_FPS)) // cap max fps
            {
				this->calculateAverageFrameTime();
				this->displayAverageFrameTime();
				
                this->currentTime = this->newTime;

                // prevent spiral of death
                if (this->frameTime > 0.25)
                    this->frameTime = 0.25;

                this->accumulator += this->frameTime;

				this->window->updateInputState();
				this->window->update();
				
                while (this->accumulator >= this->fixedLogicTime)
                {
					this->currentSimTick++;

					this->activeScene->stepPhysics(this->fixedLogicTime);
					this->activeScene->updatePreviousTransforms();
					this->activeScene->fixedLoop(this->fixedLogicTime);
					this->activeScene->updateFixedAnimations(this->fixedLogicTime);
					this->activeScene->postPhysics(this->fixedLogicTime);

					if(this->audioDevice)
						this->audioDevice->cleanUpManagedSFX();

                    this->accumulator -= this->fixedLogicTime;
                }

				this->activeScene->updateAnimations(this->frameTime);

				this->activeScene->updateBillboards();

				float renderLerpInterval = (this->accumulator / this->fixedLogicTime);

				this->activeScene->immediateLoop(this->frameTime, renderLerpInterval);
				
				this->activeScene->updateTextActors();

				// clear all previous render target buffers, this is done here as doing it right before or right after
				// we draw, wouldn't work as far as I can tell at the moment as many stages can have many cameras and
				// many cameras can have many stages, meaning that if we clear render buffers after drawing to camera's render target
				// in one stage, if it's used in another stage things would be bad
				this->activeScene->clearAllRenderTargetBuffers(this->gpu);

                this->activeScene->draw(this->frameTime, renderLerpInterval);

				this->window->renderGui();

				this->window->swapBuffers();

				this->update();
            }

        }
    }

}