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
    void render_scene(kcShaders::Scene* scene, kcShaders::Camera* camera);
    GLuint get_framebuffer_texture() const { return fbo_texture_; }
    int get_fb_width() const { return fb_width_; }
    int get_fb_height() const { return fb_height_; }
    
    // Screenshot
    bool take_screenshot(const std::string& filename);
    
    // Rendering mode control
    void setDeferredRendering(bool enabled) { use_deferred_ = enabled; }
    bool isDeferredRendering() const { return use_deferred_; }

    // Unified shader loading interface
    bool loadForwardShaders(const std::string& vertex_path, const std::string& fragment_path);
    bool loadDeferredShaders(
        const std::string& geom_vert = "../../src/shaders/deferred_geometry.vert",
        const std::string& geom_frag = "../../src/shaders/deferred_geometry.frag",
        const std::string& light_vert = "../../src/shaders/deferred_lighting.vert",
        const std::string& light_frag = "../../src/shaders/deferred_lighting.frag"
    );
    
    // Legacy compatibility (deprecated, use loadForwardShaders instead)
    bool use_shader(const std::string& vertex_path, const std::string& fragment_path) {
        return loadForwardShaders(vertex_path, fragment_path);
    }
    
    // Shader utility methods
    std::string load_shader_source(const std::string& shaderpath);
    static GLuint compile_shader(GLenum type, const char* src);
    static GLuint link_program(GLuint vs, GLuint fs);

  private:
    void create_framebuffer();
    void delete_framebuffer();
    
    // Deferred rendering methods
    void renderGeometryPass(Scene* scene, Camera* camera);
    void renderLightingPass(Scene* scene, Camera* camera);
    void setupFullscreenQuad();
    void cleanupFullscreenQuad();

  private:
    int width_;
    int height_;    
    GLFWwindow* window_;
    Camera* camera_;  // Current camera for deferred rendering

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
    
    // Rendering mode
    bool use_deferred_;
    
    // Forward rendering shader
    GLuint forward_shader_;
    
    // Deferred rendering resources
    GBuffer* gbuffer_;
    GLuint geometry_shader_;  // Deferred geometry pass shader
    GLuint lighting_shader_;  // Deferred lighting pass shader
    
    // Fullscreen quad for lighting pass
    GLuint quad_vao_;
    GLuint quad_vbo_;
    
    bool shadertoy_mode_ = true; // Enable shadertoy-like uniforms by default
};

} // namespace graphics