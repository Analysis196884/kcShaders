#include "renderer.h"
#include "scene/scene.h"
#include "scene/mesh.h"
#include "scene/camera.h"
#include "scene/material.h"
#include "scene/light.h"
#include "gbuffer.h"
#include "RenderContext.h"
#include "pipeline/RenderPipeline.h"
#include "pipeline/ForwardPipeline.h"
#include "pipeline/DeferredPipeline.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <iomanip>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB image write for screenshot
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace kcShaders {

Renderer::Renderer(GLFWwindow* window, int width, int height)
    : window_(window)
    , width_(width)
    , height_(height)
    , camera_(nullptr)
    , vao_(0)
    , vbo_(0)
    , fbo_(0)
    , fbo_texture_(0)
    , rbo_(0)
    , fb_width_(800)
    , fb_height_(600)
    , vertex_count_(0)
    , use_deferred_(true)  // Enable deferred rendering by default
    , gbuffer_(nullptr)
    , activePipeline_(nullptr)
    , quad_vao_(0)
    , quad_vbo_(0)
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

    // Calculate vertex count (number of vertices = total floats / components per vertex)
    vertex_count_ = static_cast<int>(sizeof(vertices) / (3 * sizeof(float)));
    
    // Create framebuffer
    create_framebuffer();
    
    // Initialize G-Buffer for deferred rendering
    if (use_deferred_) {
        gbuffer_ = new GBuffer();
        if (!gbuffer_->initialize(fb_width_, fb_height_)) {
            std::cerr << "Failed to initialize G-Buffer\n";
            delete gbuffer_;
            gbuffer_ = nullptr;
            use_deferred_ = false;
        } else {
            setupFullscreenQuad();
            
            // Create deferred pipeline
            deferredPipeline_ = std::make_unique<DeferredPipeline>(
                gbuffer_, fbo_, quad_vao_, fb_width_, fb_height_
            );
            if (!deferredPipeline_->initialize()) {
                std::cerr << "Failed to initialize deferred pipeline\n";
                deferredPipeline_.reset();
                delete gbuffer_;
                gbuffer_ = nullptr;
                cleanupFullscreenQuad();
                use_deferred_ = false;
            } else {
                // Load deferred rendering shaders
                if (!loadDeferredShaders()) {
                    std::cerr << "Failed to load deferred shaders, falling back to forward rendering\n";
                    deferredPipeline_.reset();
                    delete gbuffer_;
                    gbuffer_ = nullptr;
                    cleanupFullscreenQuad();
                    use_deferred_ = false;
                } else {
                    activePipeline_ = deferredPipeline_.get();
                }
            }
        }
    }
    
    // Create forward pipeline (always available as fallback)
    forwardPipeline_ = std::make_unique<ForwardPipeline>(fbo_, fb_width_, fb_height_);
    if (!forwardPipeline_->initialize()) {
        std::cerr << "Failed to initialize forward pipeline\n";
        return false;
    }
    
    // If deferred pipeline failed, use forward as active
    if (!activePipeline_) {
        activePipeline_ = forwardPipeline_.get();
        use_deferred_ = false;
    }

    return true;
}

void Renderer::shutdown()
{
    delete_framebuffer();
    
    // Clean up pipelines
    activePipeline_ = nullptr;
    forwardPipeline_.reset();
    deferredPipeline_.reset();
    
    // Clean up deferred rendering resources
    if (gbuffer_) {
        delete gbuffer_;
        gbuffer_ = nullptr;
    }
    
    cleanupFullscreenQuad();
    
    if (vbo_ > 0) 
    {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ > 0) 
    {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}

void Renderer::clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render_shadertoy()
{
    // TODO: Implement shadertoy rendering through pipeline system
    // This function needs to be refactored to use a dedicated ShaderToy pipeline
    
    /*
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, fb_width_, fb_height_);
    
    // Clear framebuffer
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render scene with shadertoy uniforms
    if (vao_ > 0) 
    {
        // Use appropriate shader and set uniforms
        // iResolution, iTime, iMouse, etc.
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
        glBindVertexArray(0);
    }
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    */
}

void Renderer::render_scene(Scene* scene, Camera* camera)
{
    if (!scene || !camera) {
        return;
    }
    
    camera_ = camera;
    
    // Use active pipeline
    if (activePipeline_) {
        RenderContext ctx;
        ctx.scene = scene;
        ctx.camera = camera;
        ctx.viewportWidth = fb_width_;
        ctx.viewportHeight = fb_height_;
        ctx.gbuffer = gbuffer_;
        ctx.deltaTime = 0.0f;
        ctx.totalTime = 0.0f;
        
        activePipeline_->execute(ctx);
        return;
    }
    
    std::cerr << "[Renderer] No active pipeline available\n";
}

bool Renderer::loadForwardShaders(const std::string& vertex_path, const std::string& fragment_path)
{
    if (!forwardPipeline_) {
        std::cerr << "[Renderer] Forward pipeline not initialized\n";
        return false;
    }
    
    return forwardPipeline_->loadShaders(vertex_path, fragment_path);
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
    
    // Resize G-Buffer if deferred rendering is enabled
    if (gbuffer_) {
        gbuffer_->resize(width, height);
    }
    
    // Notify pipelines about resize
    if (forwardPipeline_) {
        forwardPipeline_->resize(width, height);
    }
    if (deferredPipeline_) {
        deferredPipeline_->resize(width, height);
    }
}

bool Renderer::take_screenshot(const std::string& filename)
{
    if (fbo_ == 0 || fbo_texture_ == 0) {
        std::cerr << "ERROR: Framebuffer not initialized" << std::endl;
        return false;
    }
    
    if (fb_width_ <= 0 || fb_height_ <= 0) {
        std::cerr << "ERROR: Invalid framebuffer dimensions: " << fb_width_ << "x" << fb_height_ << std::endl;
        return false;
    }
    
    // Set pixel alignment for proper row reading
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    
    // Allocate memory for pixel data
    std::vector<unsigned char> pixels(fb_width_ * fb_height_ * 3);
    
    // Bind the framebuffer for reading
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    
    // Check OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error before reading pixels: " << error << std::endl;
    }
    
    // Read the pixels from bottom to top (OpenGL origin is bottom-left)
    // We read directly in the correct order to avoid manual flipping
    for (int y = 0; y < fb_height_; ++y) {
        glReadPixels(0, fb_height_ - 1 - y, fb_width_, 1, GL_RGB, GL_UNSIGNED_BYTE, 
                     pixels.data() + (y * fb_width_ * 3));
    }
    
    // Check OpenGL errors
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after reading pixels: " << error << std::endl;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        return false;
    }
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    
    // Reset pixel alignment
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    
    // Write to file
    int result = stbi_write_png(filename.c_str(), fb_width_, fb_height_, 3, pixels.data(), fb_width_ * 3);
    
    if (result == 0) {
        std::cerr << "stbi_write_png failed for: " << filename << std::endl;
        std::cerr << "  Dimensions: " << fb_width_ << "x" << fb_height_ << std::endl;
        std::cerr << "  Data size: " << pixels.size() << " bytes" << std::endl;
        return false;
    }
    
    return true;
}

void Renderer::setupFullscreenQuad()
{
    // Fullscreen triangle vertices (covers screen with single triangle)
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -3.0f,  0.0f, -1.0f,
         3.0f,  1.0f,  2.0f, 1.0f
    };

    glGenVertexArrays(1, &quad_vao_);
    glGenBuffers(1, &quad_vbo_);
    
    glBindVertexArray(quad_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

void Renderer::cleanupFullscreenQuad()
{
    if (quad_vbo_ > 0) {
        glDeleteBuffers(1, &quad_vbo_);
        quad_vbo_ = 0;
    }
    if (quad_vao_ > 0) {
        glDeleteVertexArrays(1, &quad_vao_);
        quad_vao_ = 0;
    }
}

bool Renderer::loadDeferredShaders(
    const std::string& geom_vert,
    const std::string& geom_frag,
    const std::string& light_vert,
    const std::string& light_frag
)
{   
    if (!deferredPipeline_) {
        std::cerr << "[Renderer] Deferred pipeline not initialized\n";
        return false;
    }
    
    return deferredPipeline_->loadShaders(geom_vert, geom_frag, light_vert, light_frag);
}

void Renderer::setDeferredRendering(bool enabled)
{
    if (enabled == use_deferred_) {
        return; // Already in desired mode
    }
    
    if (enabled && deferredPipeline_) {
        activePipeline_ = deferredPipeline_.get();
        use_deferred_ = true;
        std::cout << "[Renderer] Switched to deferred rendering\n";
    } else if (!enabled && forwardPipeline_) {
        activePipeline_ = forwardPipeline_.get();
        use_deferred_ = false;
        std::cout << "[Renderer] Switched to forward rendering\n";
    } else {
        std::cerr << "[Renderer] Cannot switch rendering mode - pipeline not available\n";
    }
}

} // namespace kcShaders