#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>


namespace vel
{
    class Config
    {
	private:
		std::optional<std::unordered_map<std::string, std::string>> loadFromFile();
		std::string		path;

	public:
		std::string		dataDir;
		std::string		appName;
		std::string		appExeName;
		std::string		windowSize;
		std::string		resolution;
		double			logicTick;
		double			maxFps;
		double			mouseSensitivity;
		int				windowWidth;
		int				windowHeight;
		int				resolutionWidth;
		int				resolutionHeight;
		bool			windowMode;
		bool			vsync;
		bool			fxaa;
		bool			cursorHidden;
		bool			lockResToWin;
		bool            openglDebugContext;
        
		
		Config(const std::string& dataDir = "data");
		
		void saveToFile() const;
    };
}