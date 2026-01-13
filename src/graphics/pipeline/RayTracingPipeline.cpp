#include "RayTracingPipeline.h"
#include "../ShaderProgram.h"
#include "../../scene/camera.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

// Helper function to check OpenGL errors
static void CheckGLError(const char* location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[RayTracingPipeline] OpenGL error at " << location << ": 0x" 
                  << std::hex << err << std::dec << std::endl;
    }
}

namespace kcShaders {

RayTracingPipeline::RayTracingPipeline(GLuint fbo, GLuint vao, int width, int height)
    : fbo_(fbo)
    , vao_(vao)
    , width_(width)
    , height_(height)
    , outputTexture_(0)
    , computeShaderProgram_(0)
    , maxBounces_(4)
    , samplesPerPixel_(1)
    , frameCount_(0)
{
}

RayTracingPipeline::~RayTracingPipeline()
{
    cleanup();
}

bool RayTracingPipeline::initialize()
{
    std::cout << "[RayTracingPipeline] Initializing...\n";
    
    // Create output texture for compute shader
    createOutputTexture();
    
    std::cout << "[RayTracingPipeline] Initialized\n";
    return true;
}

void RayTracingPipeline::createOutputTexture()
{
    if (outputTexture_ != 0) {
        deleteOutputTexture();
    }
    
    std::cout << "[RayTracingPipeline] Creating output texture: " << width_ << "x" << height_ << "\n";
    
    glGenTextures(1, &outputTexture_);
    CheckGLError("glGenTextures");
    
    glBindTexture(GL_TEXTURE_2D, outputTexture_);
    CheckGLError("glBindTexture");
    
    // RGBA32F for HDR rendering
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_RGBA, GL_FLOAT, nullptr);
    CheckGLError("glTexImage2D");
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckGLError("glTexParameteri");
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    std::cout << "[RayTracingPipeline] Output texture created successfully (ID: " << outputTexture_ << ")\n";
}

void RayTracingPipeline::deleteOutputTexture()
{
    if (outputTexture_ != 0) {
        glDeleteTextures(1, &outputTexture_);
        outputTexture_ = 0;
    }
}

bool RayTracingPipeline::loadComputeShader(const std::string& computePath)
{
    std::cout << "[RayTracingPipeline] Loading compute shader: " << computePath << "\n";
    
    // Read compute shader source
    std::ifstream file(computePath);
    if (!file.is_open()) {
        std::cerr << "[RayTracingPipeline] Failed to open compute shader: " << computePath << "\n";
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();
    
    // Compile compute shader
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);
    
    // Check compilation
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[RayTracingPipeline] Compute shader compilation failed:\n" << infoLog << "\n";
        glDeleteShader(shader);
        return false;
    }
    
    // Create program
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    
    // Check linking
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "[RayTracingPipeline] Compute shader program linking failed:\n" << infoLog << "\n";
        glDeleteShader(shader);
        glDeleteProgram(program);
        return false;
    }
    
    glDeleteShader(shader);
    
    // Create a simple wrapper for compute shader program
    // Store the program ID separately since ShaderProgram doesn't support compute shaders
    if (computeShaderProgram_ != 0) {
        glDeleteProgram(computeShaderProgram_);
    }
    
    // We'll use the raw program ID directly for compute shader
    computeShaderProgram_ = program;
    
    std::cout << "[RayTracingPipeline] Compute shader loaded successfully\n";
    return true;
}

bool RayTracingPipeline::loadDisplayShader(const std::string& vertPath, const std::string& fragPath)
{
    std::cout << "[RayTracingPipeline] Loading display shader: " << vertPath << " + " << fragPath << "\n";
    
    displayShader_ = std::make_unique<ShaderProgram>();
    if (!displayShader_->loadFromFiles(vertPath, fragPath)) {
        std::cerr << "[RayTracingPipeline] Failed to load display shaders\n";
        return false;
    }
    
    std::cout << "[RayTracingPipeline] Display shader loaded successfully\n";
    return true;
}

void RayTracingPipeline::execute(RenderContext& ctx)
{
    if (computeShaderProgram_ == 0) {
        std::cerr << "[RayTracingPipeline] Compute shader not loaded\n";
        return;
    }
    
    if (!displayShader_) {
        std::cerr << "[RayTracingPipeline] Display shader not loaded\n";
        return;
    }
    
    if (outputTexture_ == 0) {
        std::cerr << "[RayTracingPipeline] Output texture not created\n";
        return;
    }
    
    // === Step 1: Run compute shader for ray tracing ===
    glUseProgram(computeShaderProgram_);
    CheckGLError("glUseProgram(compute)");
    
    // Bind output texture as image for compute shader write
    glBindImageTexture(0, outputTexture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    CheckGLError("glBindImageTexture");
    
    // Set uniforms manually for compute shader
    GLint loc;
    
    loc = glGetUniformLocation(computeShaderProgram_, "iResolution");
    if (loc >= 0) {
        glUniform3f(loc, (float)width_, (float)height_, 0.0f);
        CheckGLError("iResolution");
    }
    
    loc = glGetUniformLocation(computeShaderProgram_, "iTime");
    if (loc >= 0) {
        glUniform1f(loc, ctx.totalTime);
        CheckGLError("iTime");
    }
    
    loc = glGetUniformLocation(computeShaderProgram_, "iFrame");
    if (loc >= 0) {
        glUniform1i(loc, frameCount_++);
        CheckGLError("iFrame");
    }
    
    loc = glGetUniformLocation(computeShaderProgram_, "maxBounces");
    if (loc >= 0) {
        glUniform1i(loc, maxBounces_);
        CheckGLError("maxBounces");
    }
    
    loc = glGetUniformLocation(computeShaderProgram_, "samplesPerPixel");
    if (loc >= 0) {
        glUniform1i(loc, samplesPerPixel_);
        CheckGLError("samplesPerPixel");
    }
    
    // Camera uniforms (if camera exists)
    if (ctx.camera) {
        glm::vec3 camPos = ctx.camera->GetPosition();
        glm::vec3 camFront = ctx.camera->GetFront();
        glm::vec3 camUp = ctx.camera->GetUp();
        glm::vec3 camRight = ctx.camera->GetRight();
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraPosition");
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(camPos));
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraFront");
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(camFront));
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraUp");
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(camUp));
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraRight");
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(camRight));
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraFov");
        if (loc >= 0) glUniform1f(loc, 45.0f);
        
        CheckGLError("camera uniforms");
    }
    
    // Dispatch compute shader (one thread per pixel, organized in 16x16 local groups)
    GLuint numGroupsX = (width_ + 15) / 16;
    GLuint numGroupsY = (height_ + 15) / 16;
    
    // Only log first frame to avoid spam
    static bool firstFrame = true;
    if (firstFrame) {
        std::cout << "[RayTracingPipeline] Dispatching compute: " << numGroupsX << "x" << numGroupsY 
                  << " groups for " << width_ << "x" << height_ << " pixels\n";
        firstFrame = false;
    }
    
    glDispatchCompute(numGroupsX, numGroupsY, 1);
    CheckGLError("glDispatchCompute");
    
    // Wait for compute shader to finish
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    CheckGLError("glMemoryBarrier");
    
    // === Step 2: Display the ray traced image on fullscreen quad ===
    
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    CheckGLError("glBindFramebuffer");
    
    glViewport(0, 0, width_, height_);
    CheckGLError("glViewport");
    
    // Clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckGLError("glClear");
    
    // Disable depth test
    glDisable(GL_DEPTH_TEST);
    
    // Use display shader
    displayShader_->use();
    CheckGLError("displayShader->use");
    
    // Bind output texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outputTexture_);
    CheckGLError("glBindTexture(output)");
    
    displayShader_->setInt("screenTexture", 0);
    CheckGLError("setInt(screenTexture)");
    
    // Draw fullscreen triangle
    glBindVertexArray(vao_);
    CheckGLError("glBindVertexArray");
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    CheckGLError("glDrawArrays");
    
    glBindVertexArray(0);
    
    // Re-enable depth test
    glEnable(GL_DEPTH_TEST);
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RayTracingPipeline::resize(int width, int height)
{
    width_ = width;
    height_ = height;
    
    // Recreate output texture with new size
    createOutputTexture();
    
    // Reset frame count for progressive rendering
    frameCount_ = 0;
    
    std::cout << "[RayTracingPipeline] Resized to " << width << "x" << height << "\n";
}

void RayTracingPipeline::cleanup()
{
    deleteOutputTexture();
    
    if (computeShaderProgram_ != 0) {
        glDeleteProgram(computeShaderProgram_);
        computeShaderProgram_ = 0;
    }
    
    displayShader_.reset();
}

} // namespace kcShaders
