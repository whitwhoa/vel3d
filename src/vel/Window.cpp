#include <iostream>


#include "glad/gl.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "vel/Window.h"
#include "vel/nvapi.hpp"

// stutter caused when not in fullscreen mode: https://stackoverflow.com/a/21663076/1609485
// https://www.reddit.com/r/opengl/comments/8754el/stuttering_with_learnopengl_tutorials/dwbp7ta?utm_source=share&utm_medium=web2x
// could be because of multiple monitors all running different refresh rates

//TODO: Need to refactor this to use Log.h

namespace vel
{
	void APIENTRY glDebugOutput(GLenum source,
		GLenum type,
		unsigned int id,
		GLenum severity,
		GLsizei length,
		const char *message,
		const void *userParam)
	{
		// ignore non-significant error/warning codes
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

		std::cout << "---------------" << std::endl;
		std::cout << "Debug message (" << id << "): " << message << std::endl;

		switch (source)
		{
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
		} std::cout << std::endl;

		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
		} std::cout << std::endl;

		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
		} std::cout << std::endl;
		std::cout << std::endl;
	}


    Window::Window(Config c) :
		windowMode(c.WINDOW_MODE),
        windowSize(glm::ivec2(c.WINDOW_WIDTH, c.WINDOW_HEIGHT)),
		resolution(glm::ivec2(c.RESOLUTION_X, c.RESOLUTION_Y)),
		cursorHidden(c.CURSOR_HIDDEN),
		useImGui(c.USE_IMGUI),
		vsync(c.VSYNC),
		scrollX(0.0f),
		scrollY(0.0f),
		imguiFrameOpen(false)
    {
		this->inputState.mouseSensitivity = c.MOUSE_SENSITIVITY;

		// should we include nvidia api so we can set application profile?
#ifdef WINDOWS_BUILD
		initNvidiaApplicationProfile(c.APP_EXE_NAME, c.APP_NAME);
#endif


        // Initialize GLFW. This is the library that creates our cross platform (kinda since
        // apple decided to ditch opengl support for metal only) window object
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); //for borderless windowed
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		//glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
        
		if (c.OPENGL_DEBUG_CONTEXT)
		{
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
		}

        
        

        glfwSetErrorCallback([](int error, const char* description) {
            std::cout << description << "\n";
            std::cin.get();
        });

        if (this->windowMode) 
		{
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
            glfwTerminate();
            std::cout << "Failed to create GLFW window\n";
            std::cin.get();
            exit(EXIT_FAILURE);
        }
        else 
        {

            glfwMakeContextCurrent(this->glfwWindow);

			if(this->vsync)
				glfwSwapInterval(1); // 0 = no vsync 1 = vsync
			else
				glfwSwapInterval(0);

			

            // Initialize glad. Glad is a .c file which is included in our project.
            // GLAD manages function pointers for OpenGL so we want to initialize GLAD before we call any OpenGL function
            //if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
			if (!gladLoadGL(glfwGetProcAddress))
            {
                std::cout << "Failed to initialize GLAD\n";
                std::cin.get();
                exit(EXIT_FAILURE);
            }
            else 
            {
                // Associate this object with the window
                glfwSetWindowUserPointer(this->glfwWindow, this);

                // Set callback functions used by glfw (for when polling is unavailable or it makes better sense
                // to use a callback)
                this->setCallbacks();

				//glfwSetInputMode(this->glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

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
						std::cout << "OpenGL debug context should be loaded" << std::endl;
					}
					else
					{
						std::cout << "OpenGL debug context unable to load" << std::endl;
						std::cin.get();
					}
				}


                // Set default viewport size loaded from config file
                glViewport(0, 0, this->windowSize.x, this->windowSize.y);


				//glfwFocusWindow(this->glfwWindow);

				if (this->useImGui)
				{
					// Setup Dear ImGui context
					IMGUI_CHECKVERSION();
					ImGui::CreateContext();
					ImGuiIO& io = ImGui::GetIO(); (void)io;
					//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
					//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

					// get pointer to default font
					this->imguiFonts["default"] = io.Fonts->AddFontDefault();

					// create fonts
					for (auto& f : c.imguiFonts)
					{
						this->imguiFonts[f.key] = io.Fonts->AddFontFromFileTTF(f.path.c_str(), f.pixels);
						//std::cout << this->imguiFonts[f.key] << std::endl;
					}
						

					// Setup Dear ImGui style
					ImGui::StyleColorsDark();
					//ImGui::StyleColorsClassic();

					

					// Setup Platform/Renderer bindings
					ImGui_ImplGlfw_InitForOpenGL(this->glfwWindow, true);
					ImGui_ImplOpenGL3_Init("#version 450 core");
				}

            }

        }


    }
    Window::~Window() 
    {
        glfwDestroyWindow(this->glfwWindow);

        // Terminate GLFW application process
        glfwTerminate();
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

	ImFont* Window::getImguiFont(std::string key) const
	{
		return this->imguiFonts.at(key);
	}

    void Window::setTitle(std::string title)
    {
        glfwSetWindowTitle(this->glfwWindow, title.c_str());
    }

    void Window::setCallbacks() 
    {
		// Mouse Buttons
		glfwSetMouseButtonCallback(this->glfwWindow, [](GLFWwindow* window, int button, int action, int mods) {

			void* data = glfwGetWindowUserPointer(window);
			Window* w = static_cast<Window*>(data);

			bool pressed = (action == GLFW_PRESS);
			bool released = (action == GLFW_RELEASE);

			switch (button)
			{
			case GLFW_MOUSE_BUTTON_LEFT: w->inputState.mouseLeftButton = pressed; w->inputState.mouseLeftButton_Released = released; break;
			case GLFW_MOUSE_BUTTON_RIGHT: w->inputState.mouseRightButton = pressed; w->inputState.mouseRightButton_Released = released; break;
			default: break;
			}

		});

		// Keys
		glfwSetKeyCallback(this->glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {

			void* data = glfwGetWindowUserPointer(window);
			Window* w = static_cast<Window*>(data);

			bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
			bool released = (action == GLFW_RELEASE);

			switch (key)
			{
			case GLFW_KEY_SPACE: w->inputState.keySpace = pressed; w->inputState.keySpace_Released = released; break;
			case GLFW_KEY_APOSTROPHE: w->inputState.keyApostrophe = pressed; w->inputState.keyApostrophe_Released = released; break;
			case GLFW_KEY_COMMA: w->inputState.keyComma = pressed; w->inputState.keyComma_Released = released; break;
			case GLFW_KEY_MINUS: w->inputState.keyMinus = pressed; w->inputState.keyMinus_Released = released; break;
			case GLFW_KEY_PERIOD: w->inputState.keyPeriod = pressed; w->inputState.keyPeriod_Released = released; break;
			case GLFW_KEY_SLASH: w->inputState.keySlash = pressed; w->inputState.keySlash_Released = released; break;
			case GLFW_KEY_0: w->inputState.key0 = pressed; w->inputState.key0_Released = released; break;
			case GLFW_KEY_1: w->inputState.key1 = pressed; w->inputState.key1_Released = released; break;
			case GLFW_KEY_2: w->inputState.key2 = pressed; w->inputState.key2_Released = released; break;
			case GLFW_KEY_3: w->inputState.key3 = pressed; w->inputState.key3_Released = released; break;
			case GLFW_KEY_4: w->inputState.key4 = pressed; w->inputState.key4_Released = released; break;
			case GLFW_KEY_5: w->inputState.key5 = pressed; w->inputState.key5_Released = released; break;
			case GLFW_KEY_6: w->inputState.key6 = pressed; w->inputState.key6_Released = released; break;
			case GLFW_KEY_7: w->inputState.key7 = pressed; w->inputState.key7_Released = released; break;
			case GLFW_KEY_8: w->inputState.key8 = pressed; w->inputState.key8_Released = released; break;
			case GLFW_KEY_9: w->inputState.key9 = pressed; w->inputState.key9_Released = released; break;
			case GLFW_KEY_SEMICOLON: w->inputState.keySemicolon = pressed; w->inputState.keySemicolon_Released = released; break;
			case GLFW_KEY_EQUAL: w->inputState.keyEqual = pressed; w->inputState.keyEqual_Released = released; break;
			case GLFW_KEY_A: w->inputState.keyA = pressed; w->inputState.keyA_Released = released; break;
			case GLFW_KEY_B: w->inputState.keyB = pressed; w->inputState.keyB_Released = released; break;
			case GLFW_KEY_C: w->inputState.keyC = pressed; w->inputState.keyC_Released = released; break;
			case GLFW_KEY_D: w->inputState.keyD = pressed; w->inputState.keyD_Released = released; break;
			case GLFW_KEY_E: w->inputState.keyE = pressed; w->inputState.keyE_Released = released; break;
			case GLFW_KEY_F: w->inputState.keyF = pressed; w->inputState.keyF_Released = released; break;
			case GLFW_KEY_G: w->inputState.keyG = pressed; w->inputState.keyG_Released = released; break;
			case GLFW_KEY_H: w->inputState.keyH = pressed; w->inputState.keyH_Released = released; break;
			case GLFW_KEY_I: w->inputState.keyI = pressed; w->inputState.keyI_Released = released; break;
			case GLFW_KEY_J: w->inputState.keyJ = pressed; w->inputState.keyJ_Released = released; break;
			case GLFW_KEY_K: w->inputState.keyK = pressed; w->inputState.keyK_Released = released; break;
			case GLFW_KEY_L: w->inputState.keyL = pressed; w->inputState.keyL_Released = released; break;
			case GLFW_KEY_M: w->inputState.keyM = pressed; w->inputState.keyM_Released = released; break;
			case GLFW_KEY_N: w->inputState.keyN = pressed; w->inputState.keyN_Released = released; break;
			case GLFW_KEY_O: w->inputState.keyO = pressed; w->inputState.keyO_Released = released; break;
			case GLFW_KEY_P: w->inputState.keyP = pressed; w->inputState.keyP_Released = released; break;
			case GLFW_KEY_Q: w->inputState.keyQ = pressed; w->inputState.keyQ_Released = released; break;
			case GLFW_KEY_R: w->inputState.keyR = pressed; w->inputState.keyR_Released = released; break;
			case GLFW_KEY_S: w->inputState.keyS = pressed; w->inputState.keyS_Released = released; break;
			case GLFW_KEY_T: w->inputState.keyT = pressed; w->inputState.keyT_Released = released; break;
			case GLFW_KEY_U: w->inputState.keyU = pressed; w->inputState.keyU_Released = released; break;
			case GLFW_KEY_V: w->inputState.keyV = pressed; w->inputState.keyV_Released = released; break;
			case GLFW_KEY_W: w->inputState.keyW = pressed; w->inputState.keyW_Released = released; break;
			case GLFW_KEY_X: w->inputState.keyX = pressed; w->inputState.keyX_Released = released; break;
			case GLFW_KEY_Y: w->inputState.keyY = pressed; w->inputState.keyY_Released = released; break;
			case GLFW_KEY_Z: w->inputState.keyZ = pressed; w->inputState.keyZ_Released = released; break;
			case GLFW_KEY_LEFT_BRACKET: w->inputState.keyLeftBracket = pressed; w->inputState.keyLeftBracket_Released = released; break;
			case GLFW_KEY_RIGHT_BRACKET: w->inputState.keyRightBracket = pressed; w->inputState.keyRightBracket_Released = released; break;
			case GLFW_KEY_BACKSLASH: w->inputState.keyBackslash = pressed; w->inputState.keyBackslash_Released = released; break;
			case GLFW_KEY_GRAVE_ACCENT: w->inputState.keyGraveAccent = pressed; w->inputState.keyGraveAccent_Released = released; break;
			case GLFW_KEY_ESCAPE: w->inputState.keyEscape = pressed; w->inputState.keyEscape_Released = released; break;
			case GLFW_KEY_ENTER: w->inputState.keyEnter = pressed; w->inputState.keyEnter_Released = released; break;
			case GLFW_KEY_TAB: w->inputState.keyTab = pressed; w->inputState.keyTab_Released = released; break;
			case GLFW_KEY_BACKSPACE: w->inputState.keyBackspace = pressed; w->inputState.keyBackspace_Released = released; break;
			case GLFW_KEY_INSERT: w->inputState.keyInsert = pressed; w->inputState.keyInsert_Released = released; break;
			case GLFW_KEY_DELETE: w->inputState.keyDelete = pressed; w->inputState.keyDelete_Released = released; break;
			case GLFW_KEY_RIGHT: w->inputState.keyRight = pressed; w->inputState.keyRight_Released = released; break;
			case GLFW_KEY_LEFT: w->inputState.keyLeft = pressed; w->inputState.keyLeft_Released = released; break;
			case GLFW_KEY_DOWN: w->inputState.keyDown = pressed; w->inputState.keyDown_Released = released; break;
			case GLFW_KEY_UP: w->inputState.keyUp = pressed; w->inputState.keyUp_Released = released; break;
			case GLFW_KEY_PAGE_UP: w->inputState.keyPageUp = pressed; w->inputState.keyPageUp_Released = released; break;
			case GLFW_KEY_PAGE_DOWN: w->inputState.keyPageDown = pressed; w->inputState.keyPageDown_Released = released; break;
			case GLFW_KEY_HOME: w->inputState.keyHome = pressed; w->inputState.keyHome_Released = released; break;
			case GLFW_KEY_END: w->inputState.keyEnd = pressed; w->inputState.keyEnd_Released = released; break;
			case GLFW_KEY_CAPS_LOCK: w->inputState.keyCapsLock = pressed; w->inputState.keyCapsLock_Released = released; break;
			case GLFW_KEY_SCROLL_LOCK: w->inputState.keyScrollLock = pressed; w->inputState.keyScrollLock_Released = released; break;
			case GLFW_KEY_NUM_LOCK: w->inputState.keyNumLock = pressed; w->inputState.keyNumLock_Released = released; break;
			case GLFW_KEY_PRINT_SCREEN: w->inputState.keyPrintScreen = pressed; w->inputState.keyPrintScreen_Released = released; break;
			case GLFW_KEY_PAUSE: w->inputState.keyPause = pressed; w->inputState.keyPause_Released = released; break;
			case GLFW_KEY_F1: w->inputState.keyF1 = pressed; w->inputState.keyF1_Released = released; break;
			case GLFW_KEY_F2: w->inputState.keyF2 = pressed; w->inputState.keyF2_Released = released; break;
			case GLFW_KEY_F3: w->inputState.keyF3 = pressed; w->inputState.keyF3_Released = released; break;
			case GLFW_KEY_F4: w->inputState.keyF4 = pressed; w->inputState.keyF4_Released = released; break;
			case GLFW_KEY_F5: w->inputState.keyF5 = pressed; w->inputState.keyF5_Released = released; break;
			case GLFW_KEY_F6: w->inputState.keyF6 = pressed; w->inputState.keyF6_Released = released; break;
			case GLFW_KEY_F7: w->inputState.keyF7 = pressed; w->inputState.keyF7_Released = released; break;
			case GLFW_KEY_F8: w->inputState.keyF8 = pressed; w->inputState.keyF8_Released = released; break;
			case GLFW_KEY_F9: w->inputState.keyF9 = pressed; w->inputState.keyF9_Released = released; break;
			case GLFW_KEY_F10: w->inputState.keyF10 = pressed; w->inputState.keyF10_Released = released; break;
			case GLFW_KEY_F11: w->inputState.keyF11 = pressed; w->inputState.keyF11_Released = released; break;
			case GLFW_KEY_F12: w->inputState.keyF12 = pressed; w->inputState.keyF12_Released = released; break;
			case GLFW_KEY_KP_0: w->inputState.keypad0 = pressed; w->inputState.keypad0_Released = released; break;
			case GLFW_KEY_KP_1: w->inputState.keypad1 = pressed; w->inputState.keypad1_Released = released; break;
			case GLFW_KEY_KP_2: w->inputState.keypad2 = pressed; w->inputState.keypad2_Released = released; break;
			case GLFW_KEY_KP_3: w->inputState.keypad3 = pressed; w->inputState.keypad3_Released = released; break;
			case GLFW_KEY_KP_4: w->inputState.keypad4 = pressed; w->inputState.keypad4_Released = released; break;
			case GLFW_KEY_KP_5: w->inputState.keypad5 = pressed; w->inputState.keypad5_Released = released; break;
			case GLFW_KEY_KP_6: w->inputState.keypad6 = pressed; w->inputState.keypad6_Released = released; break;
			case GLFW_KEY_KP_7: w->inputState.keypad7 = pressed; w->inputState.keypad7_Released = released; break;
			case GLFW_KEY_KP_8: w->inputState.keypad8 = pressed; w->inputState.keypad8_Released = released; break;
			case GLFW_KEY_KP_9: w->inputState.keypad9 = pressed; w->inputState.keypad9_Released = released; break;
			case GLFW_KEY_KP_DECIMAL: w->inputState.keypadDecimal = pressed; w->inputState.keypadDecimal_Released = released; break;
			case GLFW_KEY_KP_DIVIDE: w->inputState.keypadDivide = pressed; w->inputState.keypadDivide_Released = released; break;
			case GLFW_KEY_KP_MULTIPLY: w->inputState.keypadMultiply = pressed; w->inputState.keypadMultiply_Released = released; break;
			case GLFW_KEY_KP_SUBTRACT: w->inputState.keypadSubtract = pressed; w->inputState.keypadSubtract_Released = released; break;
			case GLFW_KEY_KP_ADD: w->inputState.keypadAdd = pressed; w->inputState.keypadAdd_Released = released; break;
			case GLFW_KEY_KP_ENTER: w->inputState.keypadEnter = pressed; w->inputState.keypadEnter_Released = released; break;
			case GLFW_KEY_KP_EQUAL: w->inputState.keypadEqual = pressed; w->inputState.keypadEqual_Released = released; break;
			case GLFW_KEY_LEFT_SHIFT: w->inputState.keyLeftShift = pressed; w->inputState.keyLeftShift_Released = released; break;
			case GLFW_KEY_LEFT_CONTROL: w->inputState.keyLeftControl = pressed; w->inputState.keyLeftControl_Released = released; break;
			case GLFW_KEY_LEFT_ALT: w->inputState.keyLeftAlt = pressed; w->inputState.keyLeftAlt_Released = released; break;
			case GLFW_KEY_LEFT_SUPER: w->inputState.keyLeftSuper = pressed; w->inputState.keyLeftSuper_Released = released; break;
			case GLFW_KEY_RIGHT_SHIFT: w->inputState.keyRightShift = pressed; w->inputState.keyRightShift_Released = released; break;
			case GLFW_KEY_RIGHT_CONTROL: w->inputState.keyRightControl = pressed; w->inputState.keyRightControl_Released = released; break;
			case GLFW_KEY_RIGHT_ALT: w->inputState.keyRightAlt = pressed; w->inputState.keyRightAlt_Released = released; break;
			case GLFW_KEY_RIGHT_SUPER: w->inputState.keyRightSuper = pressed; w->inputState.keyRightSuper_Released = released; break;
			case GLFW_KEY_MENU: w->inputState.keyMenu = pressed; w->inputState.keyMenu_Released = released; break;
			default: break;
			}
			
		});

        // Scroll
        glfwSetScrollCallback(this->glfwWindow, [](GLFWwindow* window, double xoffset, double yoffset) {

            // get this from window
            void* data = glfwGetWindowUserPointer(window);
            Window* w = static_cast<Window*>(data);
            w->scrollX += xoffset;
            w->scrollY += yoffset;

			//std::cout << std::to_string(w->scrollY) << std::endl;

        });

		// Focus
		glfwSetWindowFocusCallback(this->glfwWindow, [](GLFWwindow* window, int focused) {
		
			if (focused)
				std::cout << "window focused\n";
			else
				std::cout << "window NOT focused\n";
		
		});

    }

	bool Window::getImguiFrameOpen()
	{
		return this->imguiFrameOpen;
	}

	void Window::renderGui()
	{
		if (this->useImGui)
		{
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			this->imguiFrameOpen = false;
		}
	}

	void Window::updateInputState()
	{
		glfwPollEvents();
		setMouse();
		setScroll();
	}

    void Window::update() 
    {
		if (this->useImGui)
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			this->imguiFrameOpen = true;
		}
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
		double mXPos;
		double mYPos;

        glfwGetCursorPos(this->glfwWindow, &mXPos, &mYPos);

		//std::cout << "setMouse: " << mXPos << "\n";

		this->inputState.mouseXPos = (float)mXPos;
		this->inputState.mouseYPos = (float)mYPos;
    }

    void Window::setScroll() 
    {
        this->inputState.scrollX = (float)this->scrollX;
        this->inputState.scrollY = (float)this->scrollY;
        //this->scrollX = 0;
        //this->scrollY = 0;
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