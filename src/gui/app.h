#pragma once

#include <string>
#include "imgui.h"
#include <ctime>

// Forward declarations
struct GLFWwindow;

namespace kcShaders {

// Rendering mode enumeration
enum class RenderMode {
    ForwardRendering,
    DeferredRendering,
    Shadertoy,
    RayTracing  // Not yet implemented
};

class Scene;
class Renderer;
class Camera;
class SceneNode;
enum class LightType;

class App {
public:
    App(const std::string& title = "kcShaders");
    ~App();

    // Disable copy and move
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    void Run();
    
    // Getters
    bool IsRunning() const { return is_running_; }
    void Stop() { is_running_ = false; }
    GLFWwindow* GetWindow() const { return window_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    Camera* GetCamera() const { return camera_; }

private:
    bool Initialize(const std::string& title);
    void Shutdown();
    void SetUIScaleFromSystemDPI();
    void PrepareImGuiFonts();
    void LoadConfig();
    void SaveConfig();
    void CheckShaderFileChanges();
    time_t GetFileModTime(const std::string& path);
    
    // Scene management
    void LoadDemoScene();
    void ClearScene();
    void AddLight(LightType type);
#ifdef ENABLE_USD_SUPPORT
    void LoadUsdFile(const std::string& filepath);
    void OpenFileDialog();
#endif
    
    void ProcessEvents();
    void ProcessKeyboardInput();
    void Update(float delta_time);
    void Render();
    void RenderUI();
    
    // UI Panel rendering methods (extracted from RenderUI)
    void RenderMenuBar();
    void RenderControlPanel();
    void RenderShaderEditorPanel();
    void RenderViewportPanel();
    void RenderScenePanel();
    void RenderLightsSection();
    
    // Helper method for rendering scene node tree
    void DisplaySceneNodeTree(SceneNode* node, int nodeIndex);
    
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

    kcShaders::Renderer* renderer_;
    
    // Timing
    float last_frame_time_;
    float delta_time_;
    
    // UI state
    bool show_demo_;
    bool show_metrics_window_;
    float clear_color_[4];
    float ui_scale_;
    RenderMode render_mode_;
    
    // Shader paths
    char vertex_shader_path_[256];
    char fragment_shader_path_[256];
    
    // Deferred rendering shader paths
    char geom_vert_shader_path_[256];
    char geom_frag_shader_path_[256];
    char light_vert_shader_path_[256];
    char light_frag_shader_path_[256];
    
    // Shadertoy shader paths
    char shadertoy_vert_shader_path_[256];
    char shadertoy_frag_shader_path_[256];
    
    // Shader file watch
    time_t last_vertex_mod_time_;
    time_t last_fragment_mod_time_;
    
    // Deferred shader file watch
    time_t last_geom_vert_mod_time_;
    time_t last_geom_frag_mod_time_;
    time_t last_light_vert_mod_time_;
    time_t last_light_frag_mod_time_;
    
    // Shadertoy shader file watch
    time_t last_shadertoy_vert_mod_time_;
    time_t last_shadertoy_frag_mod_time_;
    
    float shader_check_timer_;
    
    // Fonts
    ImFont* regular_font_;
    ImFont* mono_font_;
    
    // Scene
    Scene* current_scene_;
    
    // Camera
    Camera* camera_;
    float camera_speed_;
};

}  // namespace kcShaders