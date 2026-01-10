#include "app.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "graphics/renderer.h"
#include "scene/scene.h"
#include "scene/demo_scene.h"
#include "scene/camera.h"
#include "scene/light.h"
#include "gui/glfw_callbacks.h"

#ifdef ENABLE_USD_SUPPORT
#include "loaders/usd_loader.h"
#endif

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

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
    , current_scene_(nullptr)
    , camera_(nullptr)
    , camera_speed_(5.0f)
{
    // Initialize shader path arrays to empty strings
    vertex_shader_path_[0] = '\0';
    fragment_shader_path_[0] = '\0';
    
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
    // Explicitly use the global Renderer type to match the member declaration
    renderer_ = new Renderer(window_, width_, height_);
    if (!renderer_->initialize()) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }

    // Load default shaders
    std::string vpath(vertex_shader_path_);
    std::string fpath(fragment_shader_path_);
    if (!vpath.empty() && !fpath.empty()) {
        if (renderer_->use_shader(vpath, fpath)) {
            std::cout << "Default shaders loaded:\n";
            std::cout << "  Vertex: " << vpath << "\n";
            std::cout << "  Fragment: " << fpath << "\n";
        }
    }

    // Load demo scene by default
    LoadDemoScene();

    // Create camera
    int fb_w = renderer_->get_fb_width();
    int fb_h = renderer_->get_fb_height();
    float aspect_ratio = static_cast<float>(fb_w) / static_cast<float>(fb_h);
    camera_ = new Camera(45.0f, aspect_ratio, 0.1f, 100.0f);
    camera_->SetPosition(glm::vec3(5.0f, 5.0f, 5.0f));
    camera_->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));

    is_running_ = true;
    last_frame_time_ = static_cast<float>(glfwGetTime());

    return true;
}

void App::Shutdown()
{
    // Save config before cleanup
    SaveConfig();
    
    // Clean up scene
    ClearScene();
    
    // Clean up camera
    if (camera_) {
        delete camera_;
        camera_ = nullptr;
    }
    
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
    
    float coord_scale_x = static_cast<float>(fb_width) / static_cast<float>(win_width);
    float coord_scale_y = static_cast<float>(fb_height) / static_cast<float>(win_height);
    float rasterizer_density = (std::max)(coord_scale_x, coord_scale_y);
    
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

void App::ProcessKeyboardInput()
{
    if (!camera_) {
        return;
    }
    
    // Calculate movement speed based on delta time for frame-rate independent movement
    float move_speed = camera_speed_ * delta_time_;
    
    // WASD movement - these can all be pressed simultaneously
    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
        camera_->MoveForward(move_speed);
    }
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
        camera_->MoveBackward(move_speed);
    }
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) {
        camera_->MoveLeft(move_speed);
    }
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
        camera_->MoveRight(move_speed);
    }
    
    // Vertical movement
    if (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS) {
        camera_->MoveUp(move_speed);
    }
    if (glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS) {
        camera_->MoveDown(move_speed);
    }
    
    // Arrow keys for rotation (optional alternative to mouse)
    float rotate_speed = 30.0f * delta_time_; // degrees per second
    if (glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS) {
        camera_->RotateView(-rotate_speed, 0.0f);
    }
    if (glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        camera_->RotateView(rotate_speed, 0.0f);
    }
    if (glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS) {
        camera_->RotateView(0.0f, rotate_speed);
    }
    if (glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS) {
        camera_->RotateView(0.0f, -rotate_speed);
    }
}

void App::Update(float delta_time) 
{
    ProcessKeyboardInput();
    
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

    // Render scene if available
    if (current_scene_ && camera_) {
        renderer_->render_scene(current_scene_, camera_);
    } else {
        renderer_->render_shadertoy();
    }

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
#ifdef ENABLE_USD_SUPPORT
            if (ImGui::MenuItem("Open USD File...", "Ctrl+O")) {
                OpenFileDialog();
            }
            ImGui::Separator();
#endif
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                is_running_ = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Load Demo Scene")) {
                LoadDemoScene();
            }
            if (ImGui::MenuItem("Clear Scene")) {
                ClearScene();
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

    // Camera info and controls
    if (camera_) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Camera");
        
        glm::vec3 cam_pos = camera_->GetPosition();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", cam_pos.x, cam_pos.y, cam_pos.z);
        
        ImGui::SliderFloat("Speed", &camera_speed_, 1.0f, 20.0f, "%.1f units/s");
    }

    ImGui::Spacing();

    // Screenshot button
    if (ImGui::Button("Take Screenshot", ImVec2(180, 0))) {
        // Generate timestamped filename
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream filename_stream;
        filename_stream << "screenshots/screenshot_" 
                       << std::put_time(&tm, "%Y%m%d_%H%M%S") 
                       << ".png";
        std::string filename = filename_stream.str();
        
        // Create screenshots directory if it doesn't exist
        std::system("if not exist screenshots mkdir screenshots");
        
        renderer_->take_screenshot(filename);
    }
    
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
    
    ImGui::Text("Scene Management");
    ImGui::Separator();
    
    // Display current scene status
    if (current_scene_) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Scene Loaded");
        
        // Count scene nodes
        int root_count = static_cast<int>(current_scene_->roots.size());
        ImGui::Text("Root Nodes: %d", root_count);
        
        // Collect render items to show mesh count
        std::vector<RenderItem> items;
        current_scene_->collectRenderItems(items);
        ImGui::Text("Render Items: %d", static_cast<int>(items.size()));
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No Scene Loaded");
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Scene loading buttons
    if (ImGui::Button("Load Demo Scene", ImVec2(180, 0))) {
        LoadDemoScene();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Clear Scene", ImVec2(150, 0))) {
        ClearScene();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Scene hierarchy tree (if scene exists)
    if (current_scene_) {
        ImGui::Text("Hierarchy:");
        for (size_t i = 0; i < current_scene_->roots.size(); ++i) {
            const auto& root = current_scene_->roots[i];
            DisplaySceneNodeTree(root.get(), static_cast<int>(i));
        }
        
        // Lights section
        if (!current_scene_->lights.empty() || true) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Lights (%zu):", current_scene_->lights.size());
            
            // Add light button
            if (ImGui::Button("+ Add Light", ImVec2(150, 36))) {
                ImGui::OpenPopup("AddLightPopup");
            }
            
            // Light type selection popup
            if (ImGui::BeginPopup("AddLightPopup")) {
                if (ImGui::MenuItem("Directional Light")) {
                    AddLight(LightType::Directional);
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Point Light")) {
                    AddLight(LightType::Point);
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Spot Light")) {
                    AddLight(LightType::Spot);
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Area Light")) {
                    AddLight(LightType::Area);
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Ambient Light")) {
                    AddLight(LightType::Ambient);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            
            // Display existing lights
            for (size_t i = 0; i < current_scene_->lights.size(); ++i) {
                Light* light = current_scene_->lights[i];
                ImGui::PushID(1000 + static_cast<int>(i));
                
                // Get light type name
                const char* typeName = "Unknown";
                switch (light->GetType()) {
                    case LightType::Directional: typeName = "Directional"; break;
                    case LightType::Point: typeName = "Point"; break;
                    case LightType::Spot: typeName = "Spot"; break;
                    case LightType::Area: typeName = "Area"; break;
                    case LightType::Ambient: typeName = "Ambient"; break;
                }
                
                bool light_open = ImGui::TreeNode("Light", "%s: %s", typeName, light->name.c_str());
                if (light_open) {
                    ImGui::Checkbox("Enabled", &light->enabled);
                    ImGui::ColorEdit3("Color", &light->color[0], ImGuiColorEditFlags_NoInputs);
                    ImGui::SliderFloat("Intensity", &light->intensity, 0.0f, 5.0f);
                    
                    // Type-specific properties
                    if (light->GetType() == LightType::Point) {
                        PointLight* plight = static_cast<PointLight*>(light);
                        ImGui::DragFloat3("Position##point", &plight->position[0], 0.1f);
                        ImGui::DragFloat("Radius##point", &plight->radius, 0.1f, 0.1f, 100.0f);
                        ImGui::Text("Attenuation: Const=%.2f, Linear=%.4f, Quad=%.6f",
                            plight->constant, plight->linear, plight->quadratic);
                    } else if (light->GetType() == LightType::Directional) {
                        DirectionalLight* dlight = static_cast<DirectionalLight*>(light);
                        ImGui::DragFloat3("Direction##dir", &dlight->direction[0], 0.01f, -1.0f, 1.0f);
                        // Normalize direction after editing
                        dlight->direction = glm::normalize(dlight->direction);
                    } else if (light->GetType() == LightType::Spot) {
                        SpotLight* slight = static_cast<SpotLight*>(light);
                        ImGui::DragFloat3("Position##spot", &slight->position[0], 0.1f);
                        ImGui::DragFloat3("Direction##spot", &slight->direction[0], 0.01f, -1.0f, 1.0f);
                        // Normalize direction after editing
                        slight->direction = glm::normalize(slight->direction);
                        ImGui::SliderFloat("Inner Cone Angle", &slight->innerConeAngle, 0.0f, slight->outerConeAngle);
                        ImGui::SliderFloat("Outer Cone Angle", &slight->outerConeAngle, slight->innerConeAngle, 180.0f);
                        ImGui::Text("Attenuation: Const=%.2f, Linear=%.4f, Quad=%.6f",
                            slight->constant, slight->linear, slight->quadratic);
                    } else if (light->GetType() == LightType::Area) {
                        AreaLight* alight = static_cast<AreaLight*>(light);
                        ImGui::DragFloat3("Position##area", &alight->position[0], 0.1f);
                        ImGui::DragFloat3("Normal##area", &alight->normal[0], 0.01f, -1.0f, 1.0f);
                        // Normalize normal after editing
                        alight->normal = glm::normalize(alight->normal);
                        ImGui::DragFloat("Width##area", &alight->width, 0.1f, 0.1f, 100.0f);
                        ImGui::DragFloat("Height##area", &alight->height, 0.1f, 0.1f, 100.0f);
                    } else if (light->GetType() == LightType::Ambient) {
                        // Ambient lights don't have position/direction
                        ImGui::Text("Ambient light properties:");
                        ImGui::Text("  No position/direction (affects entire scene)");
                    }
                    
                    ImGui::TreePop();
                }
                
                ImGui::PopID();
            }
        }
    }
    
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
    // Ctrl + O to open file dialog
    if (key == GLFW_KEY_O && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
        App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (app) {
            app->OpenFileDialog();
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
            // std::cerr << "Failed to reload shader\n";
        }
    }
}

void App::LoadDemoScene()
{
    ClearScene();
    current_scene_ = create_demo_scene();
    std::cout << "Demo scene loaded\n";
}

void App::ClearScene()
{
    if (current_scene_) {
        delete current_scene_;
        current_scene_ = nullptr;
        std::cout << "Scene cleared\n";
    }
}

void App::AddLight(LightType type)
{
    if (!current_scene_) {
        std::cout << "No scene loaded, creating new scene\n";
        current_scene_ = new Scene();
    }

    Light* light = nullptr;
    
    switch (type) {
        case LightType::Directional: {
            light = DirectionalLight::CreateSunlight(glm::vec3(0.0f, 0.0f, -1.0f));
            break;
        }
        case LightType::Point: {
            light = PointLight::CreateBulb(glm::vec3(5.0f, 0.0f, 5.0f));
            break;
        }
        case LightType::Spot: {
            light = SpotLight::CreateFlashlight(glm::vec3(5.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f));
            break;
        }
        case LightType::Area: {
            light = AreaLight::CreatePanel(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 2.0f, 2.0f);
            break;
        }
        case LightType::Ambient: {
            light = AmbientLight::CreateDefault(glm::vec3(0.2f, 0.2f, 0.2f), 0.5f);
            break;
        }
    }
    
    if (light) {
        current_scene_->addLight(light);
        std::cout << "Light added to scene\n";
    }
}

void App::DisplaySceneNodeTree(SceneNode* node, int nodeIndex)
{
    if (!node) return;
    
    // Skip trivial group nodes (no mesh, single child with different name pattern)
    // This helps reduce noise from intermediate transform nodes
    if (node->mesh == nullptr && node->children.size() == 1) {
        SceneNode* child = node->children[0].get();
        // Only skip if this is an intermediate node (names don't match exactly)
        if (node->name != child->name && node->children[0]->mesh != nullptr) {
            // Directly display the child mesh without the intermediate group
            DisplaySceneNodeTree(child, nodeIndex);
            return;
        }
    }
    
    ImGui::PushID(nodeIndex);
    
    // Determine if this node should be displayed as a tree node
    bool hasChildren = !node->children.empty();
    bool hasMesh = node->mesh != nullptr;
    bool hasContent = hasChildren || hasMesh;
    
    // Node name and type indicator
    std::string nodeLabel = node->name;
    if (hasMesh) {
        nodeLabel += " [Mesh]";
    }
    if (hasChildren && !hasMesh) {
        nodeLabel += " [Group]";
    }
    
    bool node_open = false;
    if (hasContent) {
        node_open = ImGui::TreeNode("SceneNode", "%s", nodeLabel.c_str());
    } else {
        ImGui::Bullet();
        ImGui::Text("%s", nodeLabel.c_str());
    }
    
    if (node_open || !hasContent) {
        // Display transform (only if non-identity)
        if (node->transform.position != glm::vec3(0.0f) || 
            node->transform.scale != glm::vec3(1.0f) ||
            // if not identity rotation
            node->transform.rotation != glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", 
                node->transform.position.x, 
                node->transform.position.y, 
                node->transform.position.z);
            
            if (node->transform.scale != glm::vec3(1.0f)) {
                ImGui::Text("Scale: (%.2f, %.2f, %.2f)", 
                    node->transform.scale.x, 
                    node->transform.scale.y, 
                    node->transform.scale.z);
            }
        }
        
        // Display mesh info
        if (node->mesh) {
            ImGui::Text("Mesh: %s", node->mesh->name().c_str());
            uint32_t faceCount = node->mesh->GetFaceCount();
            if (faceCount > 0) {
                // Show original face count
                ImGui::Text("  Vertices: %zu, Faces: %u", 
                    node->mesh->GetVertexCount(), 
                    faceCount);
            } else {
                // Fallback to triangulated face count if original not set
                size_t triangulatedFaces = node->mesh->GetIndexCount() / 3;
                ImGui::Text("  Vertices: %zu, Faces: %zu (triangulated)", 
                    node->mesh->GetVertexCount(), 
                    triangulatedFaces);
            }
        }
        
        // Display material info
        if (node->material) {
            bool material_open = ImGui::TreeNode("Material", "Material: %s", node->material->name.c_str());
            if (material_open) {
                ImGui::Indent();
                
                // Display albedo (color or texture)
                if (node->material->albedoMap != 0) {
                    ImGui::Text("Albedo: Texture (ID: %u)", node->material->albedoMap);
                    // Show texture preview (64x64 pixels)
                    ImGui::Image((void*)(intptr_t)node->material->albedoMap, ImVec2(64, 64));
                } else {
                    ImGui::ColorEdit3("Albedo##mat", &node->material->albedo[0], ImGuiColorEditFlags_NoInputs);
                }
                
                // Display metallic (value or texture)
                if (node->material->metallicMap != 0) {
                    ImGui::Text("Metallic: Texture (ID: %u)", node->material->metallicMap);
                    ImGui::Image((void*)(intptr_t)node->material->metallicMap, ImVec2(64, 64));
                } else {
                    ImGui::Text("Metallic: %.2f", node->material->metallic);
                }
                
                // Display roughness (value or texture)
                if (node->material->roughnessMap != 0) {
                    ImGui::Text("Roughness: Texture (ID: %u)", node->material->roughnessMap);
                    ImGui::Image((void*)(intptr_t)node->material->roughnessMap, ImVec2(64, 64));
                } else {
                    ImGui::Text("Roughness: %.2f", node->material->roughness);
                }
                
                // Display other texture maps if present
                if (node->material->normalMap != 0) {
                    ImGui::Text("Normal Map: Texture (ID: %u)", node->material->normalMap);
                    ImGui::Image((void*)(intptr_t)node->material->normalMap, ImVec2(64, 64));
                }
                if (node->material->aoMap != 0) {
                    ImGui::Text("AO Map: Texture (ID: %u)", node->material->aoMap);
                    ImGui::Image((void*)(intptr_t)node->material->aoMap, ImVec2(64, 64));
                }
                if (node->material->emissiveMap != 0) {
                    ImGui::Text("Emissive Map: Texture (ID: %u)", node->material->emissiveMap);
                    ImGui::Image((void*)(intptr_t)node->material->emissiveMap, ImVec2(64, 64));
                }
                
                ImGui::Unindent();
                ImGui::TreePop();
            }
        }
        
        // Display children recursively
        if (!node->children.empty()) {
            for (size_t i = 0; i < node->children.size(); ++i) {
                DisplaySceneNodeTree(node->children[i].get(), static_cast<int>(nodeIndex * 1000 + i));
            }
        }
        
        if (hasContent) {
            ImGui::TreePop();
        }
    }
    
    ImGui::PopID();
}


#ifdef ENABLE_USD_SUPPORT
void App::LoadUsdFile(const std::string& filepath)
{
    if (filepath.empty()) {
        std::cerr << "Empty USD file path\n";
        return;
    }
    
    ClearScene();

    std::cout << "Loading USD file: " << filepath << "\n";
    
    current_scene_ = new Scene();
    
    UsdLoader loader;
    if (loader.LoadFromFile(filepath, current_scene_)) {
        
        // Reset camera to a good viewing position
        if (camera_) {
            camera_->SetPosition(glm::vec3(5.0f, 5.0f, 5.0f));
            camera_->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
        }
    } else {
        std::cerr << "Failed to load USD file: " << loader.GetLastError() << "\n";
    }
#else
    std::cerr << "USD support not enabled\n";
#endif
}

#ifdef ENABLE_USD_SUPPORT
void App::OpenFileDialog()
{
#ifdef _WIN32
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(window_);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"USD Files\0*.usd;*.usda;*.usdc\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameW(&ofn) == TRUE) {
        // Convert wide char to UTF-8
        int size = WideCharToMultiByte(CP_UTF8, 0, szFile, -1, nullptr, 0, nullptr, nullptr);
        if (size > 0) {
            std::string utf8Path(size - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, szFile, -1, &utf8Path[0], size, nullptr, nullptr);
            LoadUsdFile(utf8Path);
        }
    }
#else
    std::cerr << "File dialog not implemented for this platform\n";
#endif
}
#endif // ENABLE_USD_SUPPORT

} // namespace kcShaders