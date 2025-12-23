#include "glfw_callbacks.h"
#include "app.h"
#include "scene/camera.h"

namespace kcShaders {

void escape_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app && action == GLFW_PRESS) {
        // Handle key presses
        if (key == GLFW_KEY_ESCAPE) {
            app->Stop();
        }
    }
}

} // namespace kcShaders