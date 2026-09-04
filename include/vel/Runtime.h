#pragma once

#include <memory>
#include <chrono>

#include <vel/Config.h>
#include <vel/Window.h>
#include <vel/AudioDevice.h>
#include <vel/GPU.h>

/*

The intent of this "Runtime" is to carve out a section of global state, but
manage it responsibly. For example, only internal library classes labeled as
friends can modify the data. The library consumer can access read only variants
through methods. If there are specific cases where it makes sense that the library
consumer could need to modify the value, specific methods are provided for those
specific tasks.

Hopefully this provides a middleground between full blown global state everywhere,
and the tedious, nonsensical, strict adhearance to dependency injection

*/

namespace vel
{
    class Runtime
    {
        friend class HeadlessApp;
        friend class App;
        friend class HeadlessScene;
        friend class Scene;
        friend class Camera;
        friend class Stage;
        friend class Actor;

    private:
        static Config _config;
        static std::unique_ptr<Window> _window;
        static std::unique_ptr<GPU> _gpu;
        static std::unique_ptr<AudioDevice> _audioDevice;

        static unsigned int _nextId;
        static std::chrono::steady_clock::time_point _startTime;
        static int _currentSimTick;
        static double _currentRunTime;
        static double _deltaTime;
        static double _frameTime;
        static double _frameRate;

    public:
        Runtime() = delete;
        ~Runtime() = delete;

        Runtime(const Runtime&) = delete;
        Runtime& operator=(const Runtime&) = delete;

        static bool init(const Config& config);
        static void shutdown();

        static const Config& config();
        static AudioDevice* audioDevice();

        static std::chrono::steady_clock::time_point startTime();

        static int currentSimTick();
        static double currentRunTime();
        static double deltaTime();
        static double frameTime();
        static double frameRate();
        static double seconds();
        static const InputState& inputState();

    };
}