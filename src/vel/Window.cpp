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
		this->windowMode = c.WINDOW_MODE;
		this->windowSize = glm::ivec2(c.WINDOW_WIDTH, c.WINDOW_HEIGHT);
		this->lockResToWin = c.LOCK_RES_TO_WIN;
		this->resolution = c.LOCK_RES_TO_WIN ? glm::ivec2(c.WINDOW_WIDTH, c.WINDOW_HEIGHT) : glm::ivec2(c.RESOLUTION_X, c.RESOLUTION_Y);
		this->cursorHidden = c.CURSOR_HIDDEN;
		this->vsync = c.VSYNC;

		this->inputState.mouseSensitivity = c.MOUSE_SENSITIVITY;


#ifdef WINDOWS_BUILD
		initNvidiaApplicationProfile(c.APP_EXE_NAME, c.APP_NAME);
#endif

		// Initialize GLFW
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); //for borderless windowed
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		//glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

		if (c.OPENGL_DEBUG_CONTEXT)
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

		glfwSetErrorCallback([](int error, const char* description) {
			SPDLOG_DEBUG("Window::init::glfwSetErrorCallback: {}", description);
		});

		if (this->windowMode)
		{
			if (!this->lockResToWin)
				glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

			this->glfwWindow = glfwCreateWindow(this->windowSize.x, this->windowSize.y, c.APP_NAME.c_str(), NULL, NULL);
		}
		else
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);

			this->windowSize = glm::ivec2(mode->width, mode->height);

			this->glfwWindow = glfwCreateWindow(mode->width, mode->height, c.APP_NAME.c_str(), monitor, NULL);
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

				if (c.OPENGL_DEBUG_CONTEXT)
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
		glfwSetKeyCallback(this->glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {

			void* data = glfwGetWindowUserPointer(window);
			Window* w = static_cast<Window*>(data);

			bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);

			switch (key)
			{
			case GLFW_KEY_SPACE: w->inputState.keySpace = pressed; break;
			case GLFW_KEY_APOSTROPHE: w->inputState.keyApostrophe = pressed; break;
			case GLFW_KEY_COMMA: w->inputState.keyComma = pressed; break;
			case GLFW_KEY_MINUS: w->inputState.keyMinus = pressed; break;
			case GLFW_KEY_PERIOD: w->inputState.keyPeriod = pressed; break;
			case GLFW_KEY_SLASH: w->inputState.keySlash = pressed; break;
			case GLFW_KEY_0: w->inputState.key0 = pressed; break;
			case GLFW_KEY_1: w->inputState.key1 = pressed; break;
			case GLFW_KEY_2: w->inputState.key2 = pressed; break;
			case GLFW_KEY_3: w->inputState.key3 = pressed; break;
			case GLFW_KEY_4: w->inputState.key4 = pressed; break;
			case GLFW_KEY_5: w->inputState.key5 = pressed; break;
			case GLFW_KEY_6: w->inputState.key6 = pressed; break;
			case GLFW_KEY_7: w->inputState.key7 = pressed; break;
			case GLFW_KEY_8: w->inputState.key8 = pressed; break;
			case GLFW_KEY_9: w->inputState.key9 = pressed; break;
			case GLFW_KEY_SEMICOLON: w->inputState.keySemicolon = pressed; break;
			case GLFW_KEY_EQUAL: w->inputState.keyEqual = pressed; break;
			case GLFW_KEY_A: w->inputState.keyA = pressed; break;
			case GLFW_KEY_B: w->inputState.keyB = pressed; break;
			case GLFW_KEY_C: w->inputState.keyC = pressed; break;
			case GLFW_KEY_D: w->inputState.keyD = pressed; break;
			case GLFW_KEY_E: w->inputState.keyE = pressed; break;
			case GLFW_KEY_F: w->inputState.keyF = pressed; break;
			case GLFW_KEY_G: w->inputState.keyG = pressed; break;
			case GLFW_KEY_H: w->inputState.keyH = pressed; break;
			case GLFW_KEY_I: w->inputState.keyI = pressed; break;
			case GLFW_KEY_J: w->inputState.keyJ = pressed; break;
			case GLFW_KEY_K: w->inputState.keyK = pressed; break;
			case GLFW_KEY_L: w->inputState.keyL = pressed; break;
			case GLFW_KEY_M: w->inputState.keyM = pressed; break;
			case GLFW_KEY_N: w->inputState.keyN = pressed; break;
			case GLFW_KEY_O: w->inputState.keyO = pressed; break;
			case GLFW_KEY_P: w->inputState.keyP = pressed; break;
			case GLFW_KEY_Q: w->inputState.keyQ = pressed; break;
			case GLFW_KEY_R: w->inputState.keyR = pressed; break;
			case GLFW_KEY_S: w->inputState.keyS = pressed; break;
			case GLFW_KEY_T: w->inputState.keyT = pressed; break;
			case GLFW_KEY_U: w->inputState.keyU = pressed; break;
			case GLFW_KEY_V: w->inputState.keyV = pressed; break;
			case GLFW_KEY_W: w->inputState.keyW = pressed; break;
			case GLFW_KEY_X: w->inputState.keyX = pressed; break;
			case GLFW_KEY_Y: w->inputState.keyY = pressed; break;
			case GLFW_KEY_Z: w->inputState.keyZ = pressed; break;
			case GLFW_KEY_LEFT_BRACKET: w->inputState.keyLeftBracket = pressed; break;
			case GLFW_KEY_RIGHT_BRACKET: w->inputState.keyRightBracket = pressed; break;
			case GLFW_KEY_BACKSLASH: w->inputState.keyBackslash = pressed; break;
			case GLFW_KEY_GRAVE_ACCENT: w->inputState.keyGraveAccent = pressed; break;
			case GLFW_KEY_ESCAPE: w->inputState.keyEscape = pressed; break;
			case GLFW_KEY_ENTER: w->inputState.keyEnter = pressed; break;
			case GLFW_KEY_TAB: w->inputState.keyTab = pressed; break;
			case GLFW_KEY_BACKSPACE: w->inputState.keyBackspace = pressed; break;
			case GLFW_KEY_INSERT: w->inputState.keyInsert = pressed; break;
			case GLFW_KEY_DELETE: w->inputState.keyDelete = pressed; break;
			case GLFW_KEY_RIGHT: w->inputState.keyRight = pressed; break;
			case GLFW_KEY_LEFT: w->inputState.keyLeft = pressed; break;
			case GLFW_KEY_DOWN: w->inputState.keyDown = pressed; break;
			case GLFW_KEY_UP: w->inputState.keyUp = pressed; break;
			case GLFW_KEY_PAGE_UP: w->inputState.keyPageUp = pressed; break;
			case GLFW_KEY_PAGE_DOWN: w->inputState.keyPageDown = pressed; break;
			case GLFW_KEY_HOME: w->inputState.keyHome = pressed; break;
			case GLFW_KEY_END: w->inputState.keyEnd = pressed; break;
			case GLFW_KEY_CAPS_LOCK: w->inputState.keyCapsLock = pressed; break;
			case GLFW_KEY_SCROLL_LOCK: w->inputState.keyScrollLock = pressed; break;
			case GLFW_KEY_NUM_LOCK: w->inputState.keyNumLock = pressed; break;
			case GLFW_KEY_PRINT_SCREEN: w->inputState.keyPrintScreen = pressed; break;
			case GLFW_KEY_PAUSE: w->inputState.keyPause = pressed; break;
			case GLFW_KEY_F1: w->inputState.keyF1 = pressed; break;
			case GLFW_KEY_F2: w->inputState.keyF2 = pressed; break;
			case GLFW_KEY_F3: w->inputState.keyF3 = pressed; break;
			case GLFW_KEY_F4: w->inputState.keyF4 = pressed; break;
			case GLFW_KEY_F5: w->inputState.keyF5 = pressed; break;
			case GLFW_KEY_F6: w->inputState.keyF6 = pressed; break;
			case GLFW_KEY_F7: w->inputState.keyF7 = pressed; break;
			case GLFW_KEY_F8: w->inputState.keyF8 = pressed; break;
			case GLFW_KEY_F9: w->inputState.keyF9 = pressed; break;
			case GLFW_KEY_F10: w->inputState.keyF10 = pressed; break;
			case GLFW_KEY_F11: w->inputState.keyF11 = pressed; break;
			case GLFW_KEY_F12: w->inputState.keyF12 = pressed; break;
			case GLFW_KEY_KP_0: w->inputState.keypad0 = pressed; break;
			case GLFW_KEY_KP_1: w->inputState.keypad1 = pressed; break;
			case GLFW_KEY_KP_2: w->inputState.keypad2 = pressed; break;
			case GLFW_KEY_KP_3: w->inputState.keypad3 = pressed; break;
			case GLFW_KEY_KP_4: w->inputState.keypad4 = pressed; break;
			case GLFW_KEY_KP_5: w->inputState.keypad5 = pressed; break;
			case GLFW_KEY_KP_6: w->inputState.keypad6 = pressed; break;
			case GLFW_KEY_KP_7: w->inputState.keypad7 = pressed; break;
			case GLFW_KEY_KP_8: w->inputState.keypad8 = pressed; break;
			case GLFW_KEY_KP_9: w->inputState.keypad9 = pressed; break;
			case GLFW_KEY_KP_DECIMAL: w->inputState.keypadDecimal = pressed; break;
			case GLFW_KEY_KP_DIVIDE: w->inputState.keypadDivide = pressed; break;
			case GLFW_KEY_KP_MULTIPLY: w->inputState.keypadMultiply = pressed; break;
			case GLFW_KEY_KP_SUBTRACT: w->inputState.keypadSubtract = pressed; break;
			case GLFW_KEY_KP_ADD: w->inputState.keypadAdd = pressed; break;
			case GLFW_KEY_KP_ENTER: w->inputState.keypadEnter = pressed; break;
			case GLFW_KEY_KP_EQUAL: w->inputState.keypadEqual = pressed; break;
			case GLFW_KEY_LEFT_SHIFT: w->inputState.keyLeftShift = pressed; break;
			case GLFW_KEY_LEFT_CONTROL: w->inputState.keyLeftControl = pressed; break;
			case GLFW_KEY_LEFT_ALT: w->inputState.keyLeftAlt = pressed; break;
			case GLFW_KEY_LEFT_SUPER: w->inputState.keyLeftSuper = pressed; break;
			case GLFW_KEY_RIGHT_SHIFT: w->inputState.keyRightShift = pressed; break;
			case GLFW_KEY_RIGHT_CONTROL: w->inputState.keyRightControl = pressed; break;
			case GLFW_KEY_RIGHT_ALT: w->inputState.keyRightAlt = pressed; break;
			case GLFW_KEY_RIGHT_SUPER: w->inputState.keyRightSuper = pressed; break;
			case GLFW_KEY_MENU: w->inputState.keyMenu = pressed; break;
			default: break;
			}
			
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

    void Window::setToClose() 
    {
        glfwSetWindowShouldClose(this->glfwWindow, true);
    }

    const InputState* Window::getInputState() const
    {
        return &this->inputState;
    }


}