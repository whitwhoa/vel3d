#include <iostream>


#include "spdlog/spdlog.h"

#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "vel/Window.h"
#include "vel/nvapi.hpp"


// stutter caused when not in fullscreen mode: https://stackoverflow.com/a/21663076/1609485
// https://www.reddit.com/r/opengl/comments/8754el/stuttering_with_learnopengl_tutorials/dwbp7ta?utm_source=share&utm_medium=web2x
// could be because of multiple monitors all running different refresh rates


namespace vel
{
	void APIENTRY glDebugOutput(GLenum source,
		GLenum type,
		unsigned int id,
		GLenum severity,
		GLsizei length,
		const char* message,
		const void* userParam)
	{
		// ignore non-significant error/warning codes
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
			return;

		// Convert enums to strings
		const char* sourceStr = "";
		switch (source)
		{
			case GL_DEBUG_SOURCE_API:             sourceStr = "API"; break;
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   sourceStr = "Window System"; break;
			case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader Compiler"; break;
			case GL_DEBUG_SOURCE_THIRD_PARTY:     sourceStr = "Third Party"; break;
			case GL_DEBUG_SOURCE_APPLICATION:     sourceStr = "Application"; break;
			case GL_DEBUG_SOURCE_OTHER:           sourceStr = "Other"; break;
			default:                              sourceStr = "Unknown"; break;
		}

		const char* typeStr = "";
		switch (type)
		{
			case GL_DEBUG_TYPE_ERROR:               typeStr = "Error"; break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated Behaviour"; break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "Undefined Behaviour"; break;
			case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Portability"; break;
			case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Performance"; break;
			case GL_DEBUG_TYPE_MARKER:              typeStr = "Marker"; break;
			case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "Push Group"; break;
			case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "Pop Group"; break;
			case GL_DEBUG_TYPE_OTHER:               typeStr = "Other"; break;
			default:                                typeStr = "Unknown"; break;
		}

		const char* severityStr = "";
		switch (severity)
		{
			case GL_DEBUG_SEVERITY_HIGH:         severityStr = "high"; break;
			case GL_DEBUG_SEVERITY_MEDIUM:       severityStr = "medium"; break;
			case GL_DEBUG_SEVERITY_LOW:          severityStr = "low"; break;
			case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "notification"; break;
			default:                             severityStr = "unknown"; break;
		}

		SPDLOG_DEBUG(
			"---------------\n"
			"Debug message ({}): {}\n"
			"Source: {}\n"
			"Type: {}\n"
			"Severity: {}",
			id, message, sourceStr, typeStr, severityStr
		);
	}


    Window::Window() :
		windowMode(true),
        windowSize(glm::ivec2(1280,720)),
		lockResToWin(true),
		resolution(glm::ivec2(1280, 720)),
		windowSizeChanged(false),
		resolutionChanged(false),
		cursorHidden(true),
		vsync(false),
		glfwWindow(nullptr),
		scroll(0),
		mouseAccumDX(0.0),
		mouseAccumDY(0.0),
		lastMouseX(0.0),
		lastMouseY(0.0),
		firstMouse(true)
    {

    }

    Window::~Window() 
    {
        glfwDestroyWindow(this->glfwWindow);
        glfwTerminate();
    }

	bool Window::init(const Config& c)
	{
		this->windowMode = c.windowMode;
		this->windowSize = glm::ivec2(c.windowWidth, c.windowHeight);
		this->lockResToWin = c.lockResToWin;
		this->resolution = c.lockResToWin ? glm::ivec2(c.windowWidth, c.windowHeight) : glm::ivec2(c.resolutionWidth, c.resolutionHeight);
		this->cursorHidden = c.cursorHidden;
		this->vsync = c.vsync;

		this->inputState.mouseSensitivity = c.mouseSensitivity;


#ifdef WINDOWS_BUILD
		initNvidiaApplicationProfile(c.appExeName, c.appName);
#endif

		// Initialize GLFW
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); //for borderless windowed
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		//glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

		if (c.openglDebugContext)
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

		glfwSetErrorCallback([](int error, const char* description) {
			SPDLOG_DEBUG("Window::init::glfwSetErrorCallback: {}", description);
		});

		if (this->windowMode)
		{
			if (!this->lockResToWin)
				glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

			this->glfwWindow = glfwCreateWindow(this->windowSize.x, this->windowSize.y, c.appName.c_str(), NULL, NULL);
		}
		else
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);

			this->windowSize = glm::ivec2(mode->width, mode->height);

			this->glfwWindow = glfwCreateWindow(mode->width, mode->height, c.appName.c_str(), monitor, NULL);
		}

		if (this->glfwWindow == NULL)
		{
			SPDLOG_DEBUG("Window::init: Failed to create GLFW window");

			glfwTerminate();

			return false;
		}
		else
		{
			glfwMakeContextCurrent(this->glfwWindow);

			if (this->vsync)
				glfwSwapInterval(1); // 0 = no vsync 1 = vsync
			else
				glfwSwapInterval(0);

			// Initialize glad
			// GLAD manages function pointers for OpenGL so we want to initialize GLAD before we call any OpenGL functions
			if (!gladLoadGL(glfwGetProcAddress))
			{
				SPDLOG_DEBUG("Window::init: Failed to initialize GLAD");
				return false;
			}
			else
			{
				// Associate this object with the window
				glfwSetWindowUserPointer(this->glfwWindow, this);

				// Set callback functions used by glfw (for when polling is unavailable or it makes better sense to use a callback)
				this->setCallbacks();

				// Set window input mode
				if (this->cursorHidden)
				{
					glfwSetInputMode(this->glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					glfwSetInputMode(this->glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
				}

				if (c.openglDebugContext)
				{
					int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
					if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
					{
						glEnable(GL_DEBUG_OUTPUT);
						glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
						glDebugMessageCallback(glDebugOutput, nullptr);
						glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

						SPDLOG_DEBUG("OpenGL debug context should be loaded");
					}
					else
					{
						SPDLOG_DEBUG("OpenGL debug context unable to load");
					}
				}

				// Set default viewport size
				glViewport(0, 0, this->windowSize.x, this->windowSize.y);


				return true;
			}
		}
	}

	glm::ivec2 Window::getResolution()
	{
		return this->resolution;
	}

	glm::ivec2 Window::getWindowSize()
	{
		return this->windowSize;
	}

	void Window::showMouseCursor()
	{
		glfwSetInputMode(this->glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	void Window::hideMouseCursor()
	{
		glfwSetInputMode(this->glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		//glfwFocusWindow(this->glfwWindow);
		//glfwSetInputMode(this->glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

    void Window::setTitle(const std::string& title)
    {
        glfwSetWindowTitle(this->glfwWindow, title.c_str());
    }

    void Window::setCallbacks() 
    {
		// Mouse Position
		glfwSetCursorPosCallback(this->glfwWindow, [](GLFWwindow* window, double xpos, double ypos)
		{
			auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));

			if (w->firstMouse) 
			{
				w->lastMouseX = xpos;
				w->lastMouseY = ypos;
				w->firstMouse = false;
				return;
			}

			double dx = xpos - w->lastMouseX;
			double dy = ypos - w->lastMouseY;

			w->lastMouseX = xpos;
			w->lastMouseY = ypos;

			// accumulate all motion since last frame
			w->mouseAccumDX += dx;
			w->mouseAccumDY += dy;
		});

		// Mouse Buttons
		glfwSetMouseButtonCallback(this->glfwWindow, [](GLFWwindow* window, int button, int action, int mods) {

			void* data = glfwGetWindowUserPointer(window);
			Window* w = static_cast<Window*>(data);

			bool pressed = (action == GLFW_PRESS);
			bool released = (action == GLFW_RELEASE);

			switch (button)
			{
			case GLFW_MOUSE_BUTTON_LEFT: w->inputState.mouseLeftButton = pressed; break;
			case GLFW_MOUSE_BUTTON_RIGHT: w->inputState.mouseRightButton = pressed; break;
			default: break;
			}

		});

		// Keys
		glfwSetKeyCallback(this->glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			void* data = glfwGetWindowUserPointer(window);
			Window* w = static_cast<Window*>(data);

			vel::VEL_KEY velKey;
			if (!glfwKeyToVelKey(key, velKey))
				return;

			if (action == GLFW_PRESS || action == GLFW_REPEAT)
				w->inputState.setKey(velKey, true);
			else if (action == GLFW_RELEASE)
				w->inputState.setKey(velKey, false);
		});

        // Scroll
        glfwSetScrollCallback(this->glfwWindow, [](GLFWwindow* window, double xoffset, double yoffset) {

            // get this from window
            void* data = glfwGetWindowUserPointer(window);
            Window* w = static_cast<Window*>(data);
            w->scroll = yoffset;

			//std::cout << std::to_string(w->scrollY) << std::endl;

        });

		// Focus
		glfwSetWindowFocusCallback(this->glfwWindow, [](GLFWwindow* window, int focused) {

			SPDLOG_DEBUG("Focused State: {}", focused);
		
		});

		// Window size updated
		glfwSetFramebufferSizeCallback(this->glfwWindow, [](GLFWwindow* window, int width, int height) {

			// get this from window
			void* data = glfwGetWindowUserPointer(window);
			Window* w = static_cast<Window*>(data);

			if (w->getLockResToWin())
			{
				w->setWindowSize(glm::ivec2(width, height));
				w->setResolution(glm::ivec2(width, height));
				w->setWindowSizeChanged(true);
			}

		});

    }

	void Window::setResolutionChanged(bool b)
	{
		this->resolutionChanged = b;
	}

	bool Window::getResolutionChanged()
	{
		return this->resolutionChanged;
	}

	bool Window::getWindowSizeChanged()
	{
		return this->windowSizeChanged;
	}

	void Window::setWindowSizeChanged(bool sc) 
	{
		this->windowSizeChanged = sc;
	}

	void Window::setWindowSize(glm::ivec2 ws)
	{
		this->windowSize = ws;
	}

	void Window::setResolution(glm::ivec2 res)
	{
		this->resolution = res;
	}

	bool Window::getLockResToWin()
	{
		return this->lockResToWin;
	}

	void Window::updateInputState()
	{
		this->inputState.updatePrevKeys();

		glfwPollEvents();
		setMouse();
		setScroll();
	}

    void Window::swapBuffers() 
    {
        // swap the color buffer (a large buffer that contains color values for each pixel in GLFW's window) 
        // that has been used to draw in during this iteration and show it as output to the screen
        glfwSwapBuffers(this->glfwWindow);
    }

    bool Window::shouldClose() 
    {
        return glfwWindowShouldClose(this->glfwWindow);
    }

    void Window::setMouse() 
    {
		//double mXPos;
		//double mYPos;

  //      glfwGetCursorPos(this->glfwWindow, &mXPos, &mYPos);

		//this->inputState.mouseXPos = (float)mXPos;
		//this->inputState.mouseYPos = (float)mYPos;

		// latch accumulated deltas for this frame
		this->inputState.mouseDX = (float)this->mouseAccumDX;
		this->inputState.mouseDY = (float)this->mouseAccumDY;
		this->mouseAccumDX = 0.0;
		this->mouseAccumDY = 0.0;

    }

    void Window::setScroll() 
    {
        this->inputState.scroll = this->scroll;
		this->scroll = 0;
    }

    const InputState* Window::getInputState() const
    {
        return &this->inputState;
    }

	bool Window::glfwKeyToVelKey(int glfwKey, VEL_KEY& out)
	{
		switch (glfwKey)
		{
		case GLFW_KEY_SPACE: out = VEL_KEY_SPACE; return true;
		case GLFW_KEY_APOSTROPHE: out = VEL_KEY_APOSTROPHE; return true;
		case GLFW_KEY_COMMA: out = VEL_KEY_COMMA; return true;
		case GLFW_KEY_MINUS: out = VEL_KEY_MINUS; return true;
		case GLFW_KEY_PERIOD: out = VEL_KEY_PERIOD; return true;
		case GLFW_KEY_SLASH: out = VEL_KEY_SLASH; return true;

		case GLFW_KEY_0: out = VEL_KEY_0; return true;
		case GLFW_KEY_1: out = VEL_KEY_1; return true;
		case GLFW_KEY_2: out = VEL_KEY_2; return true;
		case GLFW_KEY_3: out = VEL_KEY_3; return true;
		case GLFW_KEY_4: out = VEL_KEY_4; return true;
		case GLFW_KEY_5: out = VEL_KEY_5; return true;
		case GLFW_KEY_6: out = VEL_KEY_6; return true;
		case GLFW_KEY_7: out = VEL_KEY_7; return true;
		case GLFW_KEY_8: out = VEL_KEY_8; return true;
		case GLFW_KEY_9: out = VEL_KEY_9; return true;

		case GLFW_KEY_SEMICOLON: out = VEL_KEY_SEMICOLON; return true;
		case GLFW_KEY_EQUAL: out = VEL_KEY_EQUAL; return true;

		case GLFW_KEY_A: out = VEL_KEY_A; return true;
		case GLFW_KEY_B: out = VEL_KEY_B; return true;
		case GLFW_KEY_C: out = VEL_KEY_C; return true;
		case GLFW_KEY_D: out = VEL_KEY_D; return true;
		case GLFW_KEY_E: out = VEL_KEY_E; return true;
		case GLFW_KEY_F: out = VEL_KEY_F; return true;
		case GLFW_KEY_G: out = VEL_KEY_G; return true;
		case GLFW_KEY_H: out = VEL_KEY_H; return true;
		case GLFW_KEY_I: out = VEL_KEY_I; return true;
		case GLFW_KEY_J: out = VEL_KEY_J; return true;
		case GLFW_KEY_K: out = VEL_KEY_K; return true;
		case GLFW_KEY_L: out = VEL_KEY_L; return true;
		case GLFW_KEY_M: out = VEL_KEY_M; return true;
		case GLFW_KEY_N: out = VEL_KEY_N; return true;
		case GLFW_KEY_O: out = VEL_KEY_O; return true;
		case GLFW_KEY_P: out = VEL_KEY_P; return true;
		case GLFW_KEY_Q: out = VEL_KEY_Q; return true;
		case GLFW_KEY_R: out = VEL_KEY_R; return true;
		case GLFW_KEY_S: out = VEL_KEY_S; return true;
		case GLFW_KEY_T: out = VEL_KEY_T; return true;
		case GLFW_KEY_U: out = VEL_KEY_U; return true;
		case GLFW_KEY_V: out = VEL_KEY_V; return true;
		case GLFW_KEY_W: out = VEL_KEY_W; return true;
		case GLFW_KEY_X: out = VEL_KEY_X; return true;
		case GLFW_KEY_Y: out = VEL_KEY_Y; return true;
		case GLFW_KEY_Z: out = VEL_KEY_Z; return true;

		case GLFW_KEY_LEFT_BRACKET: out = VEL_KEY_LEFT_BRACKET; return true;
		case GLFW_KEY_RIGHT_BRACKET: out = VEL_KEY_RIGHT_BRACKET; return true;
		case GLFW_KEY_BACKSLASH: out = VEL_KEY_BACKSLASH; return true;
		case GLFW_KEY_GRAVE_ACCENT: out = VEL_KEY_GRAVE_ACCENT; return true;

		case GLFW_KEY_ESCAPE: out = VEL_KEY_ESCAPE; return true;
		case GLFW_KEY_ENTER: out = VEL_KEY_ENTER; return true;
		case GLFW_KEY_TAB: out = VEL_KEY_TAB; return true;
		case GLFW_KEY_BACKSPACE: out = VEL_KEY_BACKSPACE; return true;
		case GLFW_KEY_INSERT: out = VEL_KEY_INSERT; return true;
		case GLFW_KEY_DELETE: out = VEL_KEY_DELETE; return true;

		case GLFW_KEY_RIGHT: out = VEL_KEY_RIGHT; return true;
		case GLFW_KEY_LEFT: out = VEL_KEY_LEFT; return true;
		case GLFW_KEY_DOWN: out = VEL_KEY_DOWN; return true;
		case GLFW_KEY_UP: out = VEL_KEY_UP; return true;

		case GLFW_KEY_PAGE_UP: out = VEL_KEY_PAGE_UP; return true;
		case GLFW_KEY_PAGE_DOWN: out = VEL_KEY_PAGE_DOWN; return true;
		case GLFW_KEY_HOME: out = VEL_KEY_HOME; return true;
		case GLFW_KEY_END: out = VEL_KEY_END; return true;

		case GLFW_KEY_CAPS_LOCK: out = VEL_KEY_CAPS_LOCK; return true;
		case GLFW_KEY_SCROLL_LOCK: out = VEL_KEY_SCROLL_LOCK; return true;
		case GLFW_KEY_NUM_LOCK: out = VEL_KEY_NUM_LOCK; return true;
		case GLFW_KEY_PRINT_SCREEN: out = VEL_KEY_PRINT_SCREEN; return true;
		case GLFW_KEY_PAUSE: out = VEL_KEY_PAUSE; return true;

		case GLFW_KEY_F1: out = VEL_KEY_F1; return true;
		case GLFW_KEY_F2: out = VEL_KEY_F2; return true;
		case GLFW_KEY_F3: out = VEL_KEY_F3; return true;
		case GLFW_KEY_F4: out = VEL_KEY_F4; return true;
		case GLFW_KEY_F5: out = VEL_KEY_F5; return true;
		case GLFW_KEY_F6: out = VEL_KEY_F6; return true;
		case GLFW_KEY_F7: out = VEL_KEY_F7; return true;
		case GLFW_KEY_F8: out = VEL_KEY_F8; return true;
		case GLFW_KEY_F9: out = VEL_KEY_F9; return true;
		case GLFW_KEY_F10: out = VEL_KEY_F10; return true;
		case GLFW_KEY_F11: out = VEL_KEY_F11; return true;
		case GLFW_KEY_F12: out = VEL_KEY_F12; return true;

		case GLFW_KEY_KP_0: out = VEL_KEY_KP_0; return true;
		case GLFW_KEY_KP_1: out = VEL_KEY_KP_1; return true;
		case GLFW_KEY_KP_2: out = VEL_KEY_KP_2; return true;
		case GLFW_KEY_KP_3: out = VEL_KEY_KP_3; return true;
		case GLFW_KEY_KP_4: out = VEL_KEY_KP_4; return true;
		case GLFW_KEY_KP_5: out = VEL_KEY_KP_5; return true;
		case GLFW_KEY_KP_6: out = VEL_KEY_KP_6; return true;
		case GLFW_KEY_KP_7: out = VEL_KEY_KP_7; return true;
		case GLFW_KEY_KP_8: out = VEL_KEY_KP_8; return true;
		case GLFW_KEY_KP_9: out = VEL_KEY_KP_9; return true;

		case GLFW_KEY_KP_DECIMAL: out = VEL_KEY_KP_DECIMAL; return true;
		case GLFW_KEY_KP_DIVIDE: out = VEL_KEY_KP_DIVIDE; return true;
		case GLFW_KEY_KP_MULTIPLY: out = VEL_KEY_KP_MULTIPLY; return true;
		case GLFW_KEY_KP_SUBTRACT: out = VEL_KEY_KP_SUBTRACT; return true;
		case GLFW_KEY_KP_ADD: out = VEL_KEY_KP_ADD; return true;
		case GLFW_KEY_KP_ENTER: out = VEL_KEY_KP_ENTER; return true;
		case GLFW_KEY_KP_EQUAL: out = VEL_KEY_KP_EQUAL; return true;

		case GLFW_KEY_LEFT_SHIFT: out = VEL_KEY_LEFT_SHIFT; return true;
		case GLFW_KEY_LEFT_CONTROL: out = VEL_KEY_LEFT_CONTROL; return true;
		case GLFW_KEY_LEFT_ALT: out = VEL_KEY_LEFT_ALT; return true;
		case GLFW_KEY_LEFT_SUPER: out = VEL_KEY_LEFT_SUPER; return true;

		case GLFW_KEY_RIGHT_SHIFT: out = VEL_KEY_RIGHT_SHIFT; return true;
		case GLFW_KEY_RIGHT_CONTROL: out = VEL_KEY_RIGHT_CONTROL; return true;
		case GLFW_KEY_RIGHT_ALT: out = VEL_KEY_RIGHT_ALT; return true;
		case GLFW_KEY_RIGHT_SUPER: out = VEL_KEY_RIGHT_SUPER; return true;

		case GLFW_KEY_MENU: out = VEL_KEY_MENU; return true;

		default:
			return false;
		}
	}

}