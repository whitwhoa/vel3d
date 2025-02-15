#pragma once

#include <memory>
#include <vector>
#include <deque>
#include <optional>
#include <chrono>

#include "vel/Config.h"
#include "vel/Window.h"
#include "vel/GPU.h"
#include "vel/Scene.h"
#include "vel/AssetManager.h"


struct GLFWusercontext;

namespace vel 
{
    class App
    {
    protected:
        Config											config;
        Window*							                window;
		GPU*	                						gpu;
		AssetManager*               				    assetManager;

        std::vector<std::unique_ptr<Scene>>				scenes;
		Scene*											activeScene;
		
        std::chrono::high_resolution_clock::time_point	startTime;
        int												currentSimTick;
        bool											shouldClose;
        float											fixedLogicTime;
        float											currentTime;
        float											newTime;
        float											frameTime;
        float											accumulator;
        std::vector<float>								averageFrameTimeArray;
        float											lastFrameTimeCalculation;

        float											averageFrameTime;
        float											averageFrameRate;
		bool											canDisplayAverageFrameTime;
		bool											pauseBufferClearAndSwap;

        void											displayAverageFrameTime();
        void											calculateAverageFrameTime();

        void                                            checkWindowSize();
    

    public:
        App(Config conf, Window* w, GPU* gpu, AssetManager* am);

        virtual void                                    update(); // for extension

        void											addScene(std::unique_ptr<Scene> scene, bool makeActive = false);
        const float									    time() const;
        const InputState*								getInputState() const;
        void											execute();
        void											close();

		float											getFrameTime();
		float											getLogicTime();


		ImFont*											getImguiFont(std::string key) const;
		void											hideMouseCursor();
		void											showMouseCursor();

		void											forceImguiRender();

		GPU*											getGPU();
		bool											getPauseBufferClearAndSwap();
		void											setPauseBufferClearAndSwap(bool in);

		AssetManager&									getAssetManager();

		void											removeScene(std::string name);
		void											swapScene(std::string name);
		bool											sceneExists(std::string name);

		Scene*											getActiveScene();

        float											getCurrentTime();

		

    };
};
