#pragma once

#include <string>
#include "imgui.h"

// Forward declarations
struct GLFWwindow;

namespace gui {

class App {
public:
    App(const std::string& title = "kcShaders", int width = 1280, int height = 720);
    ~App();

    // Disable copy and move
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    void Run();
    
    // Getters
    bool IsRunning() const { return is_running_; }
    GLFWwindow* GetWindow() const { return window_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

private:
    bool Initialize(const std::string& title, int width, int height);
    void Shutdown();
    void SetUIScaleFromSystemDPI();
    void PrepareImGuiFonts();
    
    void ProcessEvents();
    void Update(float delta_time);
    void Render();
    void RenderUI();
    
    // GLFW callbacks
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);

private:
    GLFWwindow* window_;
    bool is_running_;
    int width_;
    int height_;
    
    // Timing
    float last_frame_time_;
    float delta_time_;
    
    // UI state
    bool show_demo_window_;
    bool show_metrics_window_;
    float clear_color_[4];
    float ui_scale_;
    
    // Fonts
    ImFont* regular_font_;
    ImFont* mono_font_;
};

}  // namespace gui