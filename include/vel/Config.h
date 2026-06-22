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
		std::string path;

    public:
											Config(const std::string& dataDir = "data");

		std::string							DATA_DIR;
		std::string							APP_EXE_NAME;
		std::string							APP_NAME;
		double								LOGIC_TICK;
		double								MAX_RENDER_FPS;
		double								MOUSE_SENSITIVITY;
		int									WINDOW_WIDTH;
		int									WINDOW_HEIGHT;
		int									RESOLUTION_X;
		int									RESOLUTION_Y;
		bool								WINDOW_MODE;
		bool								VSYNC;
		bool								FXAA;
		bool								CURSOR_HIDDEN;
		bool								LOCK_RES_TO_WIN;
		bool                                OPENGL_DEBUG_CONTEXT;		
    };
}