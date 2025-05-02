#pragma once

#include <GLFW/glfw3.h>
#include <string.h>
#include <glm/vec2.hpp>

namespace spock {
    enum ButtonState {
        KEY_Off = 0,
        KEY_Press = 0b0001,
        KEY_Held = 0b0010,
        KEY_Release = 0b0100,

        KEY_PressORHeld = 0b0011
    };

    inline struct InputData {
        int keyStates[GLFW_KEY_LAST + 1];
        int mouseStates[GLFW_MOUSE_BUTTON_LAST + 1];
        int mods;
        float lastX;
        float lastY;
        glm::dvec2 mouseRelativeMotion;
        bool firstMouse = false;
        InputData() { memset(keyStates, KEY_Off, sizeof(int)*(GLFW_KEY_LAST+1));
                      memset(mouseStates, KEY_Off, sizeof(int)*(GLFW_MOUSE_BUTTON_LAST+1)); }
    } input;
    void init_input_callbacks();
    //needs to be run before GlfwPollEvents
    void update_input();
}
