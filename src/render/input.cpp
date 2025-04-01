#include "input.hpp"
#include "vk/internal.hpp"

using namespace spock;

GLFWwindowfocusfun      prevCallbackWindowFocus;
GLFWcursorposfun        prevCallbackCursorPos;
GLFWcursorenterfun      prevCallbackCursorEnter;
GLFWmousebuttonfun      prevCallbackMousebutton;
GLFWscrollfun           prevCallbackScroll;
GLFWkeyfun              prevCallbackKey;
GLFWcharfun             prevCallbackChar;
GLFWmonitorfun          prevCallbackMonitor;
GLFWframebuffersizefun  prevCallbackFramebufferSize;

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    if (prevCallbackCursorPos) prevCallbackCursorPos(window, xpos, ypos);

    if (!input.firstMouse)
        input.firstMouse = true;
    else
        input.mouseRelativeMotion += glm::vec2{xpos - input.lastX, input.lastY - ypos};

    input.lastX     = xpos;
    input.lastY     = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (prevCallbackMousebutton) prevCallbackMousebutton(window, button, action, mods);
    input.mods = mods;
    if (action == GLFW_RELEASE) input.mouseButtonStates[button] = KEY_Off;
    else if (input.mouseButtonStates[button] == KEY_Off) input.mouseButtonStates[button] = KEY_Press;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (prevCallbackKey) prevCallbackKey(window, key, scancode, action, mods);
    input.mods = mods;
    if (action == GLFW_RELEASE) input.keyStates[key] = KEY_Off;
    else if (input.keyStates[key] == KEY_Off) input.keyStates[key] = KEY_Press;
}

void spock::init_input_callbacks()
{
    prevCallbackKey = glfwSetKeyCallback(ctx.window, key_callback);
    prevCallbackMousebutton = glfwSetMouseButtonCallback(ctx.window, mouse_button_callback);
    prevCallbackCursorPos = glfwSetCursorPosCallback(ctx.window, cursor_pos_callback);
}

void spock::update_input()
{
    input.mouseRelativeMotion = {0, 0};
    for (int i = 0; i < GLFW_KEY_LAST + 1; i++)
    {
        if (input.keyStates[i] == KEY_Press) input.keyStates[i] = KEY_Held;
        if (input.keyStates[i] == KEY_Release) input.keyStates[i] = KEY_Off;
    }

    for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST + 1; i++)
    {
        if (input.mouseButtonStates[i] == KEY_Press) input.mouseButtonStates[i] = KEY_Held;
        if (input.mouseButtonStates[i] == KEY_Release) input.mouseButtonStates[i] = KEY_Off;
    }
}
