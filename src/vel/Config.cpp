#include <fstream>
#include <sstream>
#include <iostream>

#include "vel/logger.hpp"
#include "vel/Config.h"
#include "vel/functions.h"


namespace vel
{
    Config::Config(const std::string& dataDir) :       
        DATA_DIR(dataDir)
	{
        // Defaults
        WINDOW_MODE = true;
        WINDOW_WIDTH = 1280;
        WINDOW_HEIGHT = 720;
        RESOLUTION_X = 1280;
        RESOLUTION_Y = 720;
        MAX_RENDER_FPS = 10000.0; // why double?
        MOUSE_SENSITIVITY = 0.05;
        VSYNC = false;
        LOCK_RES_TO_WIN = true;
        FXAA = false;

        // User Config Parameters if config.ini file exists
        std::optional<std::map<std::string, std::string>> ucpOpt = this->loadFromFile(dataDir + "/config.ini");
        if (ucpOpt)
        {
            std::map<std::string, std::string> ucp = ucpOpt.value();

            if(ucp.count("windowMode") > 0)
                WINDOW_MODE = ucp["windowMode"] == "0" ? false : true;
            if (ucp.count("windowWidth") > 0)
                WINDOW_WIDTH = std::stoi(ucp["windowWidth"]);
            if (ucp.count("windowHeight") > 0)
                WINDOW_HEIGHT = std::stoi(ucp["windowHeight"]);
            if (ucp.count("resolutionX") > 0)
                RESOLUTION_X = std::stoi(ucp["resolutionX"]);
            if (ucp.count("resolutionY") > 0)
                RESOLUTION_Y = std::stoi(ucp["resolutionY"]);
            if (ucp.count("maxFps") > 0)
                MAX_RENDER_FPS = std::stof(ucp["maxFps"]);
            if (ucp.count("mouseSensitivity") > 0)
                MOUSE_SENSITIVITY = std::stof(ucp["mouseSensitivity"]);
            if (ucp.count("vsync") > 0)
                VSYNC = ucp["vsync"] == "0" ? false : true;
            if (ucp.count("lockResToWin") > 0)
                LOCK_RES_TO_WIN = ucp["lockResToWin"] == "0" ? false : true;
            if (ucp.count("fxaa") > 0)
                FXAA = ucp["fxaa"] == "0" ? false : true;
        }

    };

    std::optional<std::map<std::string, std::string>> Config::loadFromFile(const std::string& path)
    {
        std::ifstream conf;

        conf.open(path);

        if (!conf.is_open()) 
        {
            VEL3D_LOG_DEBUG("Config::loadFromFile: Failed to load config.ini file at path: {}", path);
            return std::nullopt;
        }

        std::map<std::string, std::string> returnMap;

        std::string line;
        while (std::getline(conf, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::vector<std::string> e = explode_string(iss.str(), '=');
            returnMap[e[0]] = e[1];
        }

        return returnMap;
    }

}