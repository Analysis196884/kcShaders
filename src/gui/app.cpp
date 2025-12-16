#include "app.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "graphics/renderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

namespace kcShaders {

App::App(const std::string& title)
    : window_(nullptr)
    , is_running_(false)
    , width_(0)
    , height_(0)
    , renderer_(nullptr)
    , last_frame_time_(0.0f)
    , delta_time_(0.0f)
    , show_demo_(false)
    , show_metrics_window_(false)
    , clear_color_{0.1f, 0.1f, 0.12f, 1.0f}
    , ui_scale_(1.0f)
    , last_vertex_mod_time_(0)
    , last_fragment_mod_time_(0)
    , shader_check_timer_(0.0f)
    , regular_font_(nullptr)
    , mono_font_(nullptr)
{   
    // Load config (will override defaults if file exists)
    LoadConfig();
    
    if (!Initialize(title)) {
        throw std::runtime_error("Failed to initialize application");
    }
}

App::~App() 
{
    Shutdown();
}

bool App::Initialize(const std::string& title)
{
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    // Configure GLFW - Try OpenGL 3.3 first
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);  // Start maximized
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Get primary monitor's video mode for fullscreen dimensions
    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);
    
    if (mode) {
        width_ = mode->width;
        height_ = mode->height;
    } else {
        // Fallback to default if can't get monitor info
        width_ = 1920;
        height_ = 1080;
    }

    // Create maximized windowed mode (not true fullscreen)
    window_ = glfwCreateWindow(width_, height_, title.c_str(), nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window with OpenGL 3.3, trying 3.0...\n";
        
        // Try OpenGL 3.0 as fallback
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        
        window_ = glfwCreateWindow(width_, height_, title.c_str(), nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return false;
        }
    }

    // Make the OpenGL context current BEFORE initializing GLAD
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync

    // Set user pointer for callbacks
    glfwSetWindowUserPointer(window_, this);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window_, FramebufferSizeCallback);
    glfwSetKeyCallback(window_, KeyCallback);
    glfwSetMouseButtonCallback(window_, MouseButtonCallback);
    glfwSetScrollCallback(window_, ScrollCallback);
    glfwSetCursorPosCallback(window_, CursorPosCallback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window_);
        glfwTerminate();
        return false;
    }

    // Print OpenGL info
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Set DPI scale from system (before loading fonts)
    SetUIScaleFromSystemDPI();

    // Setup ImGui style and apply scaling
    ImGui::StyleColorsDark();
    
    // Apply UI scale to style BEFORE setting other style properties
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(ui_scale_);
    
    // When viewports are enabled, tweak WindowRounding
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends FIRST
    const char* glsl_version = "#version 330";
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Prepare fonts AFTER backend initialization
    PrepareImGuiFonts();

    // Configure OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize renderer with window
    renderer_ = new graphics::Renderer(window_, width_, height_);
    if (!renderer_->initialize()) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }

    is_running_ = true;
    last_frame_time_ = static_cast<float>(glfwGetTime());

    return true;
}

void App::Shutdown()
{
    // Save config before cleanup
    SaveConfig();
    
    if (renderer_) {
        delete renderer_;
        renderer_ = nullptr;
    }
    
    if (window_) {
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Cleanup GLFW
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}

void App::SetUIScaleFromSystemDPI()
{
    if (!window_) {
        return;
    }

    // Get the monitor's content scale
    float xscale = 1.0f, yscale = 1.0f;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        glfwGetMonitorContentScale(monitor, &xscale, &yscale);
    }
    
    // Use the average of x and y scale, or just x scale
    ui_scale_ = xscale;
    
    // Clamp the scale to reasonable values (between 1.0 and 3.0)
    if (ui_scale_ < 1.0f) ui_scale_ = 1.0f;
    if (ui_scale_ > 3.0f) ui_scale_ = 3.0f;
}

void App::PrepareImGuiFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    
    // Get framebuffer size and window size for rasterizer density calculation
    int fb_width, fb_height;
    glfwGetFramebufferSize(window_, &fb_width, &fb_height);
    
    int win_width, win_height;
    glfwGetWindowSize(window_, &win_width, &win_height);
    
    // Calculate coordinate scale (similar to polyscope)
    float coord_scale_x = static_cast<float>(fb_width) / static_cast<float>(win_width);
    float coord_scale_y = static_cast<float>(fb_height) / static_cast<float>(win_height);
    float rasterizer_density = std::max(coord_scale_x, coord_scale_y);
    
    // Clear existing fonts
    io.Fonts->Clear();
    
    // Windows system font paths
    const char* windows_fonts_path = "C:\\Windows\\Fonts\\";
    
    // Configure font loading with proper size
    ImFontConfig config;
    config.RasterizerDensity = rasterizer_density;
    config.OversampleH = 2;
    config.OversampleV = 1;
    
    // Try to load Segoe UI (Windows default UI font)
    std::string segoe_ui_path = std::string(windows_fonts_path) + "segoeui.ttf";
    regular_font_ = io.Fonts->AddFontFromFileTTF(
        segoe_ui_path.c_str(), 
        18.0f * ui_scale_, 
        &config
    );
    
    // Fallback to default if Segoe UI not found
    if (!regular_font_) {
        std::cout << "Segoe UI not found, trying Arial...\n";
        std::string arial_path = std::string(windows_fonts_path) + "arial.ttf";
        regular_font_ = io.Fonts->AddFontFromFileTTF(
            arial_path.c_str(), 
            18.0f * ui_scale_, 
            &config
        );
    }
    
    // Final fallback to ImGui default font
    if (!regular_font_) {
        std::cout << "System fonts not found, using ImGui default font\n";
        config.SizePixels = 18.0f * ui_scale_;
        regular_font_ = io.Fonts->AddFontDefault(&config);
    }
    
    // Load mono font (Consolas - Windows monospace font)
    ImFontConfig mono_config;
    mono_config.RasterizerDensity = rasterizer_density;
    mono_config.OversampleH = 2;
    mono_config.OversampleV = 1;
    
    std::string consolas_path = std::string(windows_fonts_path) + "consola.ttf";
    mono_font_ = io.Fonts->AddFontFromFileTTF(
        consolas_path.c_str(), 
        16.0f * ui_scale_, 
        &mono_config
    );
    
    // Fallback for mono font
    if (!mono_font_) {
        std::cout << "Consolas not found, trying Courier New...\n";
        std::string courier_path = std::string(windows_fonts_path) + "cour.ttf";
        mono_font_ = io.Fonts->AddFontFromFileTTF(
            courier_path.c_str(), 
            16.0f * ui_scale_, 
            &mono_config
        );
    }
    
    // Final fallback for mono font
    if (!mono_font_) 
    {
        std::cout << "Monospace fonts not found, using default\n";
        mono_config.SizePixels = 16.0f * ui_scale_;
        mono_font_ = io.Fonts->AddFontDefault(&mono_config);
    }
}

void App::Run() 
{
    while (is_running_ && !glfwWindowShouldClose(window_)) 
    {
        // Calculate delta time
        float current_frame = static_cast<float>(glfwGetTime());
        delta_time_ = current_frame - last_frame_time_;
        last_frame_time_ = current_frame;

        ProcessEvents();
        Update(delta_time_);
        Render();
    }
}

void App::ProcessEvents() 
{
    glfwPollEvents();
}

void App::Update(float delta_time) 
{
    // Check shader file changes every 0.5 seconds
    shader_check_timer_ += delta_time;
    if (shader_check_timer_ >= 0.5f) {
        shader_check_timer_ = 0.0f;
        CheckShaderFileChanges();
    }
    
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Update application logic here
    RenderUI();
}

void App::Render() 
{
    // Rendering
    ImGui::Render();
    
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    
    glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], clear_color_[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render your 3D scene here
    renderer_->present();

    // Render ImGui
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(window_);
}

void App::RenderUI()
{
    // Create main dockspace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Menu Bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                is_running_ = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Metrics", nullptr, &show_metrics_window_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // Show about dialog
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }

    ImGui::End();

    // Show metrics window
    if (show_metrics_window_) {
        ImGui::ShowMetricsWindow(&show_metrics_window_);
    }

    // Control panel
    ImGui::Begin("Control Panel");
    
    ImGui::Text("Application Info");
    ImGui::Separator();
    ImGui::Text("FPS: %.1f (%.3f ms/frame)", 1.0f / delta_time_, delta_time_ * 1000.0f);
    
    ImGui::End();

    // Shader editor panel
    ImGui::Begin("Shader Editor");
    
    ImGui::Text("Shader Paths");
    ImGui::Separator();
    
    // Vertex shader path input
    ImGui::Text("Vertex Shader:");
    ImGui::SameLine();
    ImGui::InputText("##VertexShader", vertex_shader_path_, sizeof(vertex_shader_path_));
    
    // Fragment shader path input
    ImGui::Text("Fragment Shader:");
    ImGui::SameLine();
    ImGui::InputText("##FragmentShader", fragment_shader_path_, sizeof(fragment_shader_path_));
    
    // Load/Reload shader button
    if (ImGui::Button("Load Shaders", ImVec2(140, 0))) 
    {
        std::string vpath(vertex_shader_path_);
        std::string fpath(fragment_shader_path_);
        if (renderer_->use_shader(vpath, fpath)) {
            std::cout << "Shaders loaded successfully:\n";
            std::cout << "  Vertex: " << vpath << "\n";
            std::cout << "  Fragment: " << fpath << "\n";
        } else {
            // std::cerr << "Failed to load shaders" << std::endl;
        }
    }
    
    ImGui::Separator();
    
    // Preview shader code
    ImGui::Text("Shader Preview (example)");
    
    // Use mono font for shader code
    if (mono_font_) {
        ImGui::PushFont(mono_font_);
    }
    
    ImGui::Text("// Vertex Shader Example");
    ImGui::Text("#version 330 core");
    ImGui::Text("layout (location = 0) in vec3 aPos;");
    ImGui::Text("void main() {");
    ImGui::Text("    gl_Position = vec4(aPos, 1.0);");
    ImGui::Text("}");
    
    if (mono_font_) {
        ImGui::PopFont();
    }
    
    ImGui::End();

    // Viewport window - Display 3D scene from framebuffer
    ImGui::Begin("Viewport");
    
    // Get available content region
    ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    
    // Resize framebuffer if needed
    if (viewport_size.x > 0 && viewport_size.y > 0) {
        int fb_w = renderer_->get_fb_width();
        int fb_h = renderer_->get_fb_height();
        
        if (fb_w != (int)viewport_size.x || fb_h != (int)viewport_size.y) {
            renderer_->resize_framebuffer((int)viewport_size.x, (int)viewport_size.y);
        }
        
        // Display the framebuffer texture
        GLuint texture_id = renderer_->get_framebuffer_texture();
        ImGui::Image((void*)(intptr_t)texture_id, viewport_size, ImVec2(0, 1), ImVec2(1, 0));
    }
    
    ImGui::End();

    // Scene hierarchy
    ImGui::Begin("Scene");
    ImGui::Text("Scene hierarchy coming soon...");
    ImGui::End();
}

// GLFW Callbacks
void App::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->width_ = width;
        app->height_ = height;
        glViewport(0, 0, width, height);
    }
}

void App::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app && action == GLFW_PRESS) {
        // Handle key presses
        if (key == GLFW_KEY_ESCAPE) {
            app->is_running_ = false;
        }
    }
}

void App::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Handle mouse button events
}

void App::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Handle scroll events
}

void App::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Handle cursor position events
}

void App::LoadConfig()
{
    std::ifstream config_file("shader_config.ini");
    if (config_file.is_open()) {
        std::string line;
        while (std::getline(config_file, line)) 
        {
            if (line.find("vertex_shader=") == 0) 
            {
                std::string path = line.substr(14);
                if (!path.empty() && path.length() < sizeof(vertex_shader_path_)) {
                    strcpy_s(vertex_shader_path_, sizeof(vertex_shader_path_), path.c_str());
                }
            } else if (line.find("fragment_shader=") == 0) 
            {
                std::string path = line.substr(16);
                if (!path.empty() && path.length() < sizeof(fragment_shader_path_)) {
                    strcpy_s(fragment_shader_path_, sizeof(fragment_shader_path_), path.c_str());
                }
            }
        }
        config_file.close();
    } else {
        // std::cout << "No shader config file found, using defaults\n";
    }
}

void App::SaveConfig()
{
    std::ofstream config_file("shader_config.ini");
    if (config_file.is_open()) {
        config_file << "vertex_shader=" << vertex_shader_path_ << "\n";
        config_file << "fragment_shader=" << fragment_shader_path_ << "\n";
        config_file.close();
    }
}

std::time_t App::GetFileModTime(const std::string& path)
{
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) == 0) {
        return file_stat.st_mtime;
    }
    return 0;
}

void App::CheckShaderFileChanges()
{
    if (!renderer_) return;
    
    std::string vertex_path(vertex_shader_path_);
    std::string fragment_path(fragment_shader_path_);
    // If both paths are empty, nothing to watch
    if (vertex_path.empty() && fragment_path.empty()) {
        return;
    }

    std::time_t vertex_mod = vertex_path.empty() ? 0 : GetFileModTime(vertex_path);
    std::time_t fragment_mod = fragment_path.empty() ? 0 : GetFileModTime(fragment_path);

    bool vertex_changed = (!vertex_path.empty() && vertex_mod != 0 && vertex_mod != last_vertex_mod_time_);
    bool fragment_changed = (!fragment_path.empty() && fragment_mod != 0 && fragment_mod != last_fragment_mod_time_);

    // Initialize on first check for provided paths
    if (( !vertex_path.empty() && last_vertex_mod_time_ == 0) || ( !fragment_path.empty() && last_fragment_mod_time_ == 0)) 
    {
        if (!vertex_path.empty()) last_vertex_mod_time_ = vertex_mod;
        if (!fragment_path.empty()) last_fragment_mod_time_ = fragment_mod;
        return;
    }

    if (vertex_changed || fragment_changed) 
    {
        std::cout << "Shader file changed, reloading...\n";

        if (renderer_->use_shader(vertex_path, fragment_path)) 
        {
            std::cout << "Shader reloaded successfully!\n";
            if (vertex_changed) last_vertex_mod_time_ = vertex_mod;
            if (fragment_changed) last_fragment_mod_time_ = fragment_mod;
        } else {
            std::cerr << "Failed to reload shader\n";
        }
    }
}

} // namespace kcShaders