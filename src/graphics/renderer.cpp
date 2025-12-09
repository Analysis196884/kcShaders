#include "renderer.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace graphics {

Renderer::Renderer(GLFWwindow* window, int width, int height)
    : window_(window)
    , width_(width)
    , height_(height)
    , shader_program_(0)
    , vao_(0)
    , vbo_(0)
    , fbo_(0)
    , fbo_texture_(0)
    , rbo_(0)
    , fb_width_(800)
    , fb_height_(600)
{
}

Renderer::~Renderer()
{
    shutdown();
}

bool Renderer::initialize()
{
    // Create a fullscreen triangle for shader display
    float vertices[] = {
        // positions
        -1.0f, -1.0f, 0.0f,
         3.0f, -1.0f, 0.0f,
        -1.0f,  3.0f, 0.0f
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Load shaders
    if (!use_shader("../../../src/shaders/test.vert", "../../../src/shaders/test.frag")) {
        std::cerr << "Failed to load shaders\n";
        return false;
    }
    
    // Create framebuffer
    create_framebuffer();

    std::cout << "Renderer initialized successfully\n";
    return true;
}

void Renderer::shutdown()
{
    delete_framebuffer();
    
    if (vbo_ > 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ > 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (shader_program_ > 0) {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }
}

void Renderer::clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render_to_framebuffer()
{
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, fb_width_, fb_height_);
    
    // Clear framebuffer
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render scene
    if (shader_program_ > 0 && vao_ > 0) {
        glUseProgram(shader_program_);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::present()
{
    // This method is kept for compatibility but rendering now happens in render_to_framebuffer
    render_to_framebuffer();
}

bool Renderer::use_shader(const std::string& vertex_path, const std::string& fragment_path)
{
    std::string vertex_code = load_shader_source(vertex_path);
    std::string fragment_code = load_shader_source(fragment_path);

    if (vertex_code.empty() || fragment_code.empty()) {
        std::cerr << "Failed to load shader source files.\n";
        return false;
    }

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_code.c_str());
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_code.c_str());

    if (vertex_shader == 0 || fragment_shader == 0) {
        return false;
    }

    shader_program_ = link_program(vertex_shader, fragment_shader);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    if (shader_program_ == 0) {
        return false;
    }

    return true;
}

std::string Renderer::load_shader_source(const std::string& shaderpath)
{
    std::ifstream file(shaderpath, std::ios::in | std::ios::binary);
    if (!file)
        return std::string();

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Renderer::compile_shader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint Renderer::link_program(GLuint vs, GLuint fs)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void Renderer::create_framebuffer()
{
    // Delete existing framebuffer if any
    delete_framebuffer();
    
    // Create framebuffer
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    
    // Create color texture
    glGenTextures(1, &fbo_texture_);
    glBindTexture(GL_TEXTURE_2D, fbo_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fb_width_, fb_height_, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture_, 0);
    
    // Create renderbuffer for depth and stencil
    glGenRenderbuffers(1, &rbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fb_width_, fb_height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    std::cout << "Framebuffer created: " << fb_width_ << "x" << fb_height_ << std::endl;
}

void Renderer::delete_framebuffer()
{
    if (fbo_ > 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    if (fbo_texture_ > 0) {
        glDeleteTextures(1, &fbo_texture_);
        fbo_texture_ = 0;
    }
    if (rbo_ > 0) {
        glDeleteRenderbuffers(1, &rbo_);
        rbo_ = 0;
    }
}

void Renderer::resize_framebuffer(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    
    fb_width_ = width;
    fb_height_ = height;
    
    create_framebuffer();
}

} // namespace graphics