#pragma once

#include <string>
#include <map>
#include <vector>


namespace vel
{
	struct ImGuiFont
	{
		std::string key;
		std::string path;
		float		pixels;
	};

    class Config
    {
    private:
        std::map<std::string, std::string>	loadFromFile(std::string path);
        std::map<std::string, std::string>	userConfigParams;        


    public:
											Config();
        // Application defined
		float								LOGIC_TICK = 60.0f;
		bool								CURSOR_HIDDEN = false;
		bool								USE_IMGUI = false;
        //const std::string					LOG_PATH = "data/log.txt";
        bool                                OPENGL_DEBUG_CONTEXT = false;
		std::string							APP_EXE_NAME = "MyApp.exe";
		std::string							APP_NAME = "MyApp";
		

		std::vector<ImGuiFont>				imguiFonts;
        

        // User defined via config.ini
		bool								WINDOW_MODE;
        int									WINDOW_WIDTH;
        int									WINDOW_HEIGHT;

		int									RESOLUTION_X;
		int									RESOLUTION_Y;

		float								MAX_RENDER_FPS;
		float								MOUSE_SENSITIVITY;
		bool								VSYNC;

		bool								LOCK_RES_TO_WIN;

		bool								FXAA;
    };
}