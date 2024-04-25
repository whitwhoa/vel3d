#pragma once

#include <string>
#include <chrono>
#include <map>


#include "glm/glm.hpp"


#include "vel/InputState.h"
#include "vel/Config.h"


struct GLFWwindow;
struct GLFWusercontext;
struct ImFont;

namespace vel
{

    class Window
    {
       
    private:
        bool				windowMode;
        glm::ivec2			windowSize;
        glm::ivec2          resolution;
        
		bool				cursorHidden;
		bool				useImGui;
		bool				vsync;
        InputState			inputState;
        GLFWwindow*			glfwWindow;
        float				scrollX = 0.0;
        float				scrollY = 0.0;

		std::map<std::string, ImFont*> imguiFonts;
		bool				imguiFrameOpen;

        void				setMouse();
        void				setScroll();
        void				setCallbacks();

    public:
							Window(Window&&) = default;
							Window(Config c);
							~Window();
        void				setTitle(std::string title);
        bool				shouldClose();
        void				setToClose();
        void				update();
		void				updateInputState();
        const InputState*	getInputState() const;
        void				swapBuffers();
		void				renderGui();
        //void				vsync();


		ImFont* 			getImguiFont(std::string key) const;
		void				hideMouseCursor();
		void				showMouseCursor();
		bool				getImguiFrameOpen();

        glm::ivec2          getResolution();
        glm::ivec2          getWindowSize();


    };
    
    
};
