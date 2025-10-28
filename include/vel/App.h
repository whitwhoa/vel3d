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
#include "vel/AudioDevice.h"


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
        AudioDevice*                                    audioDevice;

        std::vector<std::unique_ptr<Scene>>				scenes;
		Scene*											activeScene;
		
        std::chrono::high_resolution_clock::time_point	startTime;
        int												currentSimTick;
        bool											shouldClose;
        double											fixedLogicTime;
        double											currentTime;
        double											newTime;
        double											frameTime;
        double                                          frameTimeClamp;
        double											accumulator;
        std::vector<double>								averageFrameTimeArray;
        double											lastFrameTimeCalculation;

        double											averageFrameTime;
        double											averageFrameRate;
		bool											canDisplayAverageFrameTime;
		bool											pauseBufferClearAndSwap;

        void											displayAverageFrameTime();
        void											calculateAverageFrameTime();

        void                                            checkWindowSize();

        virtual void                                    update();
    

    public:
        App(Config conf, Window* w, GPU* gpu, AssetManager* am);
        ~App();

        void                                            setAudioDevice(AudioDevice* ad);

        void											addScene(std::unique_ptr<Scene> scene, bool makeActive = false);
        const double									getRuntimeSec() const;
        const InputState*								getInputState() const;
        virtual void									execute();
        void											close();

		double											getFrameTime();
        double											getLogicTime();
        double											getCurrentTime();


		ImFont*											getImguiFont(std::string key) const;
		void											hideMouseCursor();
		void											showMouseCursor();

		void											forceImguiRender();

		GPU*											getGPU();
		bool											getPauseBufferClearAndSwap();
		void											setPauseBufferClearAndSwap(bool in);

		AssetManager&									getAssetManager();

		void											removeScene(const std::string& name);
		void											swapScene(const std::string& name);
		bool											sceneExists(const std::string& name);
        Scene*                                          getScene(const std::string& name);

		Scene*											getActiveScene();

        std::chrono::high_resolution_clock::time_point& getStartTime();
        

		

    };
};
