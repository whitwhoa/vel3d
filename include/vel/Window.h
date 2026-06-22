#pragma once

#include <string>
#include <chrono>
#include <map>


#include "glm/glm.hpp"


#include "vel/InputState.h"
#include "vel/Config.h"


struct GLFWwindow;
struct GLFWusercontext;

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
        bool                resolutionChanged;
        
		bool				cursorHidden;
		bool				vsync;
        InputState			inputState;
        GLFWwindow*			glfwWindow;
        int 				scroll;

        double              mouseAccumDX;
        double              mouseAccumDY;
        double              lastMouseX;
        double              lastMouseY;
        bool                firstMouse;

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

		void				updateInputState();
        const InputState*	getInputState() const;
        void				swapBuffers();


		void				hideMouseCursor();
		void				showMouseCursor();


        glm::ivec2          getResolution();
        glm::ivec2          getWindowSize();
        bool                getLockResToWin();

        void                setWindowSize(glm::ivec2 ws);
        void                setResolution(glm::ivec2 res);

        bool                getWindowSizeChanged();
        void                setWindowSizeChanged(bool sc);

        void                setResolutionChanged(bool b);
        bool                getResolutionChanged();

        static bool         glfwKeyToVelKey(int glfwKey, VEL_KEY& out);


    };
    
    
};
