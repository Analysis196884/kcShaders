#pragma once 

// Include GLAD before any OpenGL/GLFW headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <memory>

namespace graphics {

class Renderer {
  public:
    Renderer(GLFWwindow* window, int width, int height);
    ~Renderer();

    bool initialize();
    void shutdown();

    void clear(float r, float g, float b, float a);
    void present();
    
    // Framebuffer methods
    void resize_framebuffer(int width, int height);
    void render_to_framebuffer();
    GLuint get_framebuffer_texture() const { return fbo_texture_; }
    int get_fb_width() const { return fb_width_; }
    int get_fb_height() const { return fb_height_; }

    // shader compilation and management 
    bool use_shader(const std::string& vertex_path, const std::string& fragment_path);
    std::string load_shader_source(const std::string& shaderpath);
    static GLuint compile_shader(GLenum type, const char* src);
    static GLuint link_program(GLuint vs, GLuint fs);

  private:
    void create_framebuffer();
    void delete_framebuffer();

  private:
    int width_;
    int height_;    
    GLFWwindow* window_;

    // OpenGL resources
    GLuint shader_program_;
    GLuint vao_;
    GLuint vbo_;
    
    // Framebuffer objects
    GLuint fbo_;
    GLuint fbo_texture_;
    GLuint rbo_;
    int fb_width_;
    int fb_height_;
};

} // namespace graphics