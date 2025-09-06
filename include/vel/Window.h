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
        bool                lockResToWin;
        glm::ivec2          resolution;
        bool                windowSizeChanged;
        
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
							Window();
							~Window();
        bool                init(const Config& c);

        void				setTitle(const std::string& title);
        bool				shouldClose();
        void				setToClose();
        void				update();
		void				updateInputState();
        const InputState*	getInputState() const;
        void				swapBuffers();
		void				renderGui();
        //void				vsync();


		ImFont* 			getImguiFont(const std::string& key) const;
		void				hideMouseCursor();
		void				showMouseCursor();
		bool				getImguiFrameOpen();

        glm::ivec2          getResolution();
        glm::ivec2          getWindowSize();
        bool                getLockResToWin();

        void                setWindowSize(glm::ivec2 ws);
        void                setResolution(glm::ivec2 res);

        bool                getWindowSizeChanged();
        void                setWindowSizeChanged(bool sc);


    };
    
    
};
