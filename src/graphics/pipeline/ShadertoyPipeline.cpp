#include "ShadertoyPipeline.h"
#include "../ShaderProgram.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

namespace kcShaders {

ShadertoyPipeline::ShadertoyPipeline(GLuint fbo, GLuint vao, int width, int height)
    : fbo_(fbo)
    , vao_(vao)
    , width_(width)
    , height_(height)
{
}

ShadertoyPipeline::~ShadertoyPipeline()
{
    cleanup();
}

bool ShadertoyPipeline::initialize()
{
    std::cout << "[ShadertoyPipeline] Initialized\n";
    return true;
}

bool ShadertoyPipeline::loadShaders(const std::string& vertPath, const std::string& fragPath)
{
    std::cout << "[ShadertoyPipeline] Loading shaders: " << vertPath << " + " << fragPath << "\n";
    
    // Read vertex shader
    std::ifstream vertFile(vertPath);
    if (!vertFile.is_open()) {
        std::cerr << "[ShadertoyPipeline] Failed to open vertex shader: " << vertPath << "\n";
        return false;
    }
    std::stringstream vertStream;
    vertStream << vertFile.rdbuf();
    std::string vertSource = vertStream.str();
    vertFile.close();
    
    // Read fragment shader
    std::ifstream fragFile(fragPath);
    if (!fragFile.is_open()) {
        std::cerr << "[ShadertoyPipeline] Failed to open fragment shader: " << fragPath << "\n";
        return false;
    }
    std::stringstream fragStream;
    fragStream << fragFile.rdbuf();
    std::string fragSource = fragStream.str();
    fragFile.close();
    
    // Wrap Shadertoy fragment shader code
    // Standard Shadertoy shaders use mainImage(out vec4 fragColor, in vec2 fragCoord)
    // We need to wrap it with a main() function that calls mainImage
    std::string wrappedFragSource = R"(
#version 330 core

out vec4 FragColor;
in vec2 vUV;

// Shadertoy standard uniforms
uniform vec3 iResolution;
uniform float iTime;
uniform float iTimeDelta;
uniform int iFrame;
uniform vec4 iMouse;

)";
    
    // Add the user's Shadertoy code (which contains mainImage function)
    wrappedFragSource += fragSource;
    
    // Add the main function that calls mainImage
    wrappedFragSource += R"(

void main()
{
    // Convert UV coordinates to fragCoord (pixel coordinates)
    vec2 fragCoord = vUV * iResolution.xy;
    mainImage(FragColor, fragCoord);
}
)";
    
    // Create shader program
    shader_ = std::make_unique<ShaderProgram>();
    if (!shader_->loadFromSource(vertSource, wrappedFragSource)) {
        std::cerr << "[ShadertoyPipeline] Failed to compile shaders\n";
        return false;
    }
    
    std::cout << "[ShadertoyPipeline] Shaders loaded successfully\n";
    return true;
}

void ShadertoyPipeline::execute(RenderContext& ctx)
{
    if (!shader_) {
        std::cerr << "[ShadertoyPipeline] No shader loaded\n";
        return;
    }
    
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    
    // Clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Disable depth test for fullscreen quad
    glDisable(GL_DEPTH_TEST);
    
    // Use shader
    shader_->use();
    
    // Set Shadertoy standard uniforms
    // iResolution: viewport resolution (in pixels)
    shader_->setVec3("iResolution", glm::vec3(width_, height_, 0.0f));
    
    // iTime: shader playback time (in seconds)
    shader_->setFloat("iTime", ctx.totalTime);
    
    // iTimeDelta: render time (in seconds)
    shader_->setFloat("iTimeDelta", ctx.deltaTime);
    
    // iFrame: shader playback frame
    static int frame = 0;
    shader_->setInt("iFrame", frame++);
    
    // iMouse: mouse pixel coords. xy: current (if mouse button down), zw: click
    // For now, just pass zero - can be enhanced later
    shader_->setVec4("iMouse", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    
    // Draw fullscreen triangle
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    
    // Re-enable depth test
    glEnable(GL_DEPTH_TEST);
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadertoyPipeline::resize(int width, int height)
{
    width_ = width;
    height_ = height;
}

void ShadertoyPipeline::cleanup()
{
    shader_.reset();
}

} // namespace kcShaders
