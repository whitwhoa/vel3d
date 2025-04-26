#include <fstream>
#include <sstream>
#include <iostream>

#include "vel/Config.h"
#include "vel/functions.h"


namespace vel
{
    Config::Config() :
        userConfigParams(this->loadFromFile("data/config.ini")),

        WINDOW_MODE(this->userConfigParams["windowMode"] == "0" ? false : true),
        WINDOW_WIDTH(std::stoi(this->userConfigParams["windowWidth"])),
        WINDOW_HEIGHT(std::stoi(this->userConfigParams["windowHeight"])),

        RESOLUTION_X(std::stoi(this->userConfigParams["resolutionX"])),
        RESOLUTION_Y(std::stoi(this->userConfigParams["resolutionY"])),

        MAX_RENDER_FPS(std::stof(this->userConfigParams["maxFps"])),

		MOUSE_SENSITIVITY(std::stof(this->userConfigParams["mouseSensitivity"])),

		VSYNC(this->userConfigParams["vsync"] == "0" ? false : true),

        LOCK_RES_TO_WIN(this->userConfigParams["lockResToWin"] == "0" ? false : true),

        FXAA(this->userConfigParams["fxaa"] == "0" ? false : true)

	{};

    std::map<std::string, std::string> Config::loadFromFile(std::string path)
    {
        std::ifstream conf;
        try
        {
            conf.open(path); // not throwing error when file path incorrect for some reason

            if (!conf.is_open()) 
            {
                std::cout << "Failed to load config.ini file at path: " << path << "\n";
                std::cin.get();
                exit(EXIT_FAILURE);
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
        catch (std::ifstream::failure e)
        {
            std::cout << "Failed to load config.ini file at path: " << e.what() << "\n";
            std::cin.get();
            exit(EXIT_FAILURE);
        }
    }

}