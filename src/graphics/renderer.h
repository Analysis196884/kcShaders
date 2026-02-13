#pragma once 

// Include GLAD before any OpenGL/GLFW headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <memory>

namespace kcShaders {
class Scene;
class Camera;
class GBuffer;
class RenderPipeline;
class ForwardPipeline;
class DeferredPipeline;
class ShadertoyPipeline;
class RayTracingPipeline;

class Renderer {
  public:
    Renderer(GLFWwindow* window, int width, int height);
    ~Renderer();

    bool initialize();
    void shutdown();

    void clear(float r, float g, float b, float a);
    
    // Framebuffer methods
    void resize_framebuffer(int width, int height);
    void render_shadertoy();
    void render_forward(kcShaders::Scene* scene, kcShaders::Camera* camera);
    void render_deferred(kcShaders::Scene* scene, kcShaders::Camera* camera);
    void render_raytracing(kcShaders::Scene* scene, kcShaders::Camera* camera);
    
    // Ray tracing scene management
    void uploadRayTracingScene(kcShaders::Scene* scene);
    
    GLuint get_framebuffer_texture() const { return fbo_texture_; }
    int get_fb_width() const { return fb_width_; }
    int get_fb_height() const { return fb_height_; }
    
    // Screenshot
    bool take_screenshot(const std::string& filename);

    // Unified shader loading interface
    bool loadForwardShaders(const std::string& vertex_path, const std::string& fragment_path);
    bool loadDeferredShaders(
        const std::string& geom_vert = "../../src/shaders/deferred/geometry.vert",
        const std::string& geom_frag = "../../src/shaders/deferred/geometry.frag",
        const std::string& light_vert = "../../src/shaders/deferred/lighting.vert",
        const std::string& light_frag = "../../src/shaders/deferred/lighting.frag",
        const std::string& ssao_vert = "../../src/shaders/deferred/ssao.vert",
        const std::string& ssao_frag = "../../src/shaders/deferred/ssao.frag",
        const std::string& ssao_blur_vert = "../../src/shaders/deferred/ssao_blur.vert",
        const std::string& ssao_blur_frag = "../../src/shaders/deferred/ssao_blur.frag"
    );
    bool loadShadertoyShaders(const std::string& vertex_path, const std::string& fragment_path);
    bool loadRayTracingShaders(const std::string& compute_path, const std::string& display_vert, const std::string& display_frag);

    // API for setting parameters of rendering pipelines
    void setRayTracingParameters(int max_bounces, int samples_per_pixel);
    void enableDeferredSSAO(bool enable);

  private:
    void create_framebuffer();
    void delete_framebuffer();
    
    // Pipeline setup
    void setupFullscreenQuad();
    void cleanupFullscreenQuad();

  private:
    int width_;
    int height_;    
    GLFWwindow* window_;
    Camera* camera_;

    // OpenGL resources
    GLuint vao_;
    GLuint vbo_;
    int vertex_count_;
    
    // Framebuffer objects (for final output)
    GLuint fbo_;
    GLuint fbo_texture_;
    GLuint rbo_;
    int fb_width_;
    int fb_height_;
    
    // Deferred rendering resources
    GBuffer* gbuffer_;
    
    // Rendering pipelines
    std::unique_ptr<ForwardPipeline> forwardPipeline_;
    std::unique_ptr<DeferredPipeline> deferredPipeline_;
    std::unique_ptr<ShadertoyPipeline> shadertoyPipeline_;
    std::unique_ptr<RayTracingPipeline> raytracingPipeline_;
    RenderPipeline* activePipeline_;  // Non-owning pointer to active pipeline
    
    // Fullscreen quad for deferred rendering
    GLuint quad_vao_;
    GLuint quad_vbo_;
};

} // namespace kcShaders