#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "spdlog/spdlog.h"

#include "vel/Config.h"
#include "vel/functions.h"


namespace vel
{
    Config::Config(const std::string& dataDir) :  
        path(dataDir + "/config.ini"),
        dataDir(dataDir),
        appName("MyApp"),
        appExeName("MyApp.exe"),
        windowSize("1280x720"),
        resolution("1280x720"),
        logicTick(60.0),
        maxFps(10000.0),
        mouseSensitivity(0.05),
        windowWidth(1280),
        windowHeight(720),
        resolutionWidth(1280),
        resolutionHeight(720),
        windowMode(true),
        vsync(false),
        fxaa(false),
        cursorHidden(false),
        lockResToWin(true),
        openglDebugContext(false)
	{
        // User Config Parameters, if config.ini file exists
        std::optional<std::unordered_map<std::string, std::string>> ucpOpt = this->loadFromFile();
        if (ucpOpt)
        {
            std::unordered_map<std::string, std::string> ucp = ucpOpt.value();

            // TODO: input validation

            if(ucp.count("windowMode") > 0)
                windowMode = ucp["windowMode"] == "0" ? false : true;
            if (ucp.count("windowSize") > 0) 
            {
                windowSize = ucp["windowSize"];
                windowWidth = std::stoi(explode_string(windowSize, 'x')[0]);
                windowHeight = std::stoi(explode_string(windowSize, 'x')[1]);
            }
            if (ucp.count("resolution") > 0) 
            {
                resolution = ucp["resolution"];
                resolutionWidth = std::stoi(explode_string(resolution, 'x')[0]);
                resolutionHeight = std::stoi(explode_string(resolution, 'x')[1]);
            }
            if (ucp.count("maxFps") > 0)
                maxFps = std::stof(ucp["maxFps"]);
            if (ucp.count("mouseSensitivity") > 0)
                mouseSensitivity = std::stof(ucp["mouseSensitivity"]);
            if (ucp.count("vsync") > 0)
                vsync = ucp["vsync"] == "0" ? false : true;
            if (ucp.count("lockResToWin") > 0)
                lockResToWin = ucp["lockResToWin"] == "0" ? false : true;

            // Disabling this for the time being, because we realized that while it is implemented
            // and works, we did not take into account that the post process effect would also be
            // applied to user interface elements. Once we have that lined out, we will re-enable
            //if (ucp.count("fxaa") > 0)
            //    fxaa = ucp["fxaa"] == "0" ? false : true;
        }
    };

    std::optional<std::unordered_map<std::string, std::string>> Config::loadFromFile()
    {
        std::ifstream conf;

        conf.open(this->path);

        if (!conf.is_open()) 
        {
            SPDLOG_DEBUG("Config::loadFromFile: Failed to load config.ini file at path: {}", this->path);
            return std::nullopt;
        }

        std::unordered_map<std::string, std::string> returnMap;

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

    void Config::saveToFile() const
    {
        std::ifstream in(this->path);

        if (!in.is_open())
        {
            SPDLOG_ERROR("Config::saveToFile: Failed to open config.ini at path: {}", this->path);
            return;
        }

        std::vector<std::string> lines;
        std::string line;

        while (std::getline(in, line))
        {
            if (line.rfind("windowMode=", 0) == 0)
                line = "windowMode=" + std::to_string(this->windowMode ? 1 : 0);
            else if (line.rfind("windowSize=", 0) == 0)
                line = "windowSize=" + this->windowSize;
            else if (line.rfind("resolution=", 0) == 0)
                line = "resolution=" + this->resolution;
            else if (line.rfind("maxFps=", 0) == 0)
            {
                std::ostringstream maxFpsSS;
                maxFpsSS << std::fixed << std::setprecision(1) << this->maxFps;
                line = "maxFps=" + maxFpsSS.str();
            }
            else if (line.rfind("mouseSensitivity=", 0) == 0)
            {
                std::ostringstream mouseSenSS;
                mouseSenSS << std::fixed << std::setprecision(3) << this->mouseSensitivity;
                line = "mouseSensitivity=" + mouseSenSS.str();
            }
            else if (line.rfind("vsync=", 0) == 0)
                line = "vsync=" + std::to_string(this->vsync ? 1 : 0);
            
            lines.push_back(line);
        }

        in.close();

        std::ofstream out(this->path, std::ios::trunc);

        if (!out.is_open())
        {
            SPDLOG_ERROR("Config::saveToFile: Failed to write config.ini at path: {}", this->path);
            return;
        }

        for (const auto& l : lines)
            out << l << '\n';
    }

}