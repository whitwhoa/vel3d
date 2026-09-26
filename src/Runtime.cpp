
#define STB_IMAGE_IMPLEMENTATION
#include <stb_headers/stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_headers/stb_truetype.h>

#include <vel/Runtime.h>
#include <vel/Util/Assert.h>

namespace vel
{
    bool Runtime::_initialized = false;

    Config Runtime::_config{};

    std::unique_ptr<Window> Runtime::_window = nullptr;
    std::unique_ptr<GPU> Runtime::_gpu = nullptr;
    std::unique_ptr<AudioDevice> Runtime::_audioDevice = nullptr;

    unsigned int Runtime::_nextId = 1;

    std::chrono::steady_clock::time_point Runtime::_startTime{};

    int Runtime::_currentSimTick = 0;
    double Runtime::_currentRunTime = 0.0;
    double Runtime::_deltaTime = 0.0;
    double Runtime::_frameTime = 0.0;
    double Runtime::_frameRate = 0.0;

    void Runtime::init(const Config& config)
    {
        VEL_ASSERT(!_initialized, "Runtime::init(): Runtime is already initialized.");

#ifdef VEL_USE_NVAPI
        initNvidiaApplicationProfile(config.appExeName, config.appName);
#endif

        _config = config;
        
        _startTime = std::chrono::steady_clock::now();
        _currentSimTick = 0;
        _currentRunTime = 0.0;
        _deltaTime = 0.0;
        _frameTime = 0.0;
        _frameRate = 0.0;

        if (_config.headless)
        {
            _initialized = true;
            return;
        }
            

        _window = std::make_unique<Window>(_config);
        _gpu = std::make_unique<GPU>();

        _audioDevice = std::make_unique<AudioDevice>();
        bool audioDeviceInitSuccess = _audioDevice->init();
        VEL_ASSERT(audioDeviceInitSuccess, "Runtime::init() - failed to initialize audioDevice");

        _initialized = true;
    }

    void Runtime::shutdown()
    {
        VEL_ASSERT(_initialized, "Runtime::shutdown(): Runtime is not initialized.");

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
        VEL_ASSERT(!_config.headless, "Runtime::inputState() - method cannot be called in headless mode");
        return *_window->getInputState();
    }

    

}