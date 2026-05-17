#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>


namespace vel
{
    class Config
    {
    private:
        std::optional<std::map<std::string, std::string>> loadFromFile(const std::string& path);


    public:
											Config(const std::string& dataDir = "data");
        // Application defined
		std::string							DATA_DIR;
		double								LOGIC_TICK = 60.0;
		bool								CURSOR_HIDDEN = false;
        bool                                OPENGL_DEBUG_CONTEXT = false;
		std::string							APP_EXE_NAME = "MyApp.exe";
		std::string							APP_NAME = "MyApp";
		
        

        // User defined via config.ini
		bool								WINDOW_MODE;
        int									WINDOW_WIDTH;
        int									WINDOW_HEIGHT;

		int									RESOLUTION_X;
		int									RESOLUTION_Y;

		double								MAX_RENDER_FPS;
		double								MOUSE_SENSITIVITY;
		bool								VSYNC;

		bool								LOCK_RES_TO_WIN;

		bool								FXAA;
    };
}