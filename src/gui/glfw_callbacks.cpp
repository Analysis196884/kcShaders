#include "glfw_callbacks.h"

namespace kcShaders {

void wasd_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Example WASD key handling
    if (action == GLFW_PRESS || action == GLFW_REPEAT) 
    {
        switch (key) {
            case GLFW_KEY_W:
                // Move forward
                break;
            case GLFW_KEY_A:
                // Move left
                break;
            case GLFW_KEY_S:
                // Move backward
                break;
            case GLFW_KEY_D:
                // Move right
                break;
            default:
                break;
        }
    }
}

} // namespace kcShaders