#pragma once

#include <memory>
#include <vector>
#include <deque>
#include <optional>
#include <chrono>

#include <vel/Config.h>
#include <vel/Window.h>
#include <vel/Render/GPU.h>
#include <vel/AssetManager.h>
#include <vel/AudioDevice.h>
#include <vel/Scene/Scene.h>


struct GLFWusercontext;

namespace vel 
{
    class App
    {
    protected:
        std::vector<std::unique_ptr<Scene>>				scenes;
		Scene*											activeScene;
		
        bool											shouldClose;
        double											fixedLogicTime;

        double											lastRunTime;
        double                                          deltaTimeClamp;

        double                                          lastFrameTime;

        double											accumulator;
        std::vector<double>								averageFrameTimeArray;
        double											lastFrameTimeCalculation;
        double											averageFrameTime;


        void											calculateAverageFrameTime();

        void                                            checkWindowSize();
        void                                            checkResolution();

        virtual void                                    update();
    

    public:
        App();
        ~App();

        bool											addScene(std::unique_ptr<Scene> scene, bool makeActive = false);
        virtual void									execute();
        void											close();

		void											removeScene(const std::string& name);
		void											swapScene(const std::string& name);
		bool											sceneExists(const std::string& name);
        Scene*                                          getScene(const std::string& name);
		Scene*											getActiveScene();
        

		

    };
};
