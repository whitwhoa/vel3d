
#define STB_IMAGE_IMPLEMENTATION
#include <stb_headers/stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_headers/stb_truetype.h>

#include <vel/Runtime.h>

namespace vel
{
    Config Runtime::_config{};

    std::unique_ptr<Window> Runtime::_window = nullptr;
    std::unique_ptr<GPU> Runtime::_gpu = nullptr;
    std::unique_ptr<AudioDevice> Runtime::_audioDevice = nullptr;

    std::chrono::steady_clock::time_point Runtime::_startTime{};

    int Runtime::_currentSimTick = 0;
    double Runtime::_currentRunTime = 0.0;
    double Runtime::_deltaTime = 0.0;
    double Runtime::_frameTime = 0.0;
    double Runtime::_frameRate = 0.0;

    bool Runtime::init(const Config& config)
    {
        _config = config;
        
        _startTime = std::chrono::steady_clock::now();
        _currentSimTick = 0;
        _currentRunTime = 0.0;
        _deltaTime = 0.0;
        _frameTime = 0.0;
        _frameRate = 0.0;

        if (_config.headless)
            return true;

        _window = std::make_unique<Window>();
        if (!_window->init(_config))
            return false;

        _gpu = std::make_unique<GPU>(_config.fxaa);

        _audioDevice = std::make_unique<AudioDevice>();
        if (!_audioDevice->init())
            return false;
        

        return true;
    }

    void Runtime::shutdown()
    {
        _audioDevice.reset();
        _gpu.reset();
        _window.reset();

        _currentSimTick = 0;
        _currentRunTime = 0.0;
        _deltaTime = 0.0;
        _frameTime = 0.0;
        _frameRate = 0.0;
        _startTime = {};
    }

    const Config& Runtime::config()
    {
        return _config;
    }

    AudioDevice* Runtime::audioDevice()
    {
        return _audioDevice.get();
    }

    std::chrono::steady_clock::time_point Runtime::startTime()
    {
        return _startTime;
    }

    int Runtime::currentSimTick()
    {
        return _currentSimTick;
    }

    double Runtime::currentRunTime()
    {
        return _currentRunTime;
    }

    double Runtime::deltaTime()
    {
        return _deltaTime;
    }

    double Runtime::frameTime()
    {
        return _frameTime;
    }

    double Runtime::frameRate()
    {
        return _frameRate;
    }

    double Runtime::seconds()
    {
        using namespace std::chrono;
        return duration<double>(steady_clock::now() - _startTime).count();
    }

    const InputState& Runtime::inputState()
    {
        return *_window->getInputState();
    }

    

}