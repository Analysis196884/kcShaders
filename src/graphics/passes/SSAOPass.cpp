#include "SSAOPass.h"
#include "../gbuffer.h"
#include "../RenderContext.h"
#include "../../scene/camera.h"
#include <glm/gtc/random.hpp>
#include <iostream>

namespace kcShaders {

SSAOPass::SSAOPass(GBuffer* gbuffer, 
                   ShaderProgram* ssaoShader,
                   ShaderProgram* blurShader,
                   GLuint quadVAO, 
                   int width, 
                   int height)
    : gbuffer_(gbuffer)
    , ssaoShader_(ssaoShader)
    , blurShader_(blurShader)
    , quadVAO_(quadVAO)
    , width_(width)
    , height_(height)
{
}

SSAOPass::~SSAOPass()
{
    cleanup();
}

void SSAOPass::setup()
{   
    generateSampleKernel();
    generateNoiseTexture();
    createFramebuffers();
    
    initialized_ = true;
}

void SSAOPass::createFramebuffers()
{
    // Clean up existing framebuffers if any
    if (ssaoFBO_ != 0) {
        glDeleteFramebuffers(1, &ssaoFBO_);
        glDeleteTextures(1, &ssaoTexture_);
        glDeleteFramebuffers(1, &ssaoBlurFBO_);
        glDeleteTextures(1, &ssaoBlurTexture_);
    }
    
    // Create SSAO framebuffer and texture
    glGenFramebuffers(1, &ssaoFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO_);
    
    glGenTextures(1, &ssaoTexture_);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width_, height_, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture_, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[SSAOPass] SSAO framebuffer not complete!\n";
    }
    
    // Create blur framebuffer and texture
    glGenFramebuffers(1, &ssaoBlurFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO_);
    
    glGenTextures(1, &ssaoBlurTexture_);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width_, height_, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurTexture_, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[SSAOPass] SSAO blur framebuffer not complete!\n";
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // std::cout << "[SSAOPass] Framebuffers created (" << width_ << "x" << height_ << ")\n";
}

void SSAOPass::generateSampleKernel()
{
    ssaoKernel_.clear();
    ssaoKernel_.reserve(sampleCount_);
    
    for (int i = 0; i < sampleCount_; ++i) {
        // Generate random sample in hemisphere
        glm::vec3 sample(
            glm::linearRand(-1.0f, 1.0f),
            glm::linearRand(-1.0f, 1.0f),
            glm::linearRand(0.0f, 1.0f)  // Positive Z for hemisphere
        );
        sample = glm::normalize(sample);
        
        // Scale samples to be within unit hemisphere
        float scale = static_cast<float>(i) / static_cast<float>(sampleCount_);
        
        // Accelerating interpolation function to distribute samples more towards center
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        
        ssaoKernel_.push_back(sample);
    }
}

void SSAOPass::generateNoiseTexture()
{
    // Generate 4x4 noise texture for rotating sample kernel
    std::vector<glm::vec3> ssaoNoise;
    ssaoNoise.reserve(16);
    
    for (int i = 0; i < 16; ++i) {
        glm::vec3 noise(
            glm::linearRand(-1.0f, 1.0f),
            glm::linearRand(-1.0f, 1.0f),
            0.0f  // Rotate around Z axis
        );
        ssaoNoise.push_back(noise);
    }
    
    if (noiseTexture_ != 0) {
        glDeleteTextures(1, &noiseTexture_);
    }
    
    glGenTextures(1, &noiseTexture_);
    glBindTexture(GL_TEXTURE_2D, noiseTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void SSAOPass::bindGBufferTextures()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer_->getPositionTexture());
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer_->getNormalTexture());
}

void SSAOPass::execute(RenderContext& ctx)
{
    if (!ctx.isValid() || !ssaoShader_ || !blurShader_) {
        std::cerr << "[SSAOPass] Invalid context or shaders\n";
        return;
    }
    
    if (!initialized_) {
        setup();
    }
    
    // === Pass 1: Generate SSAO ===
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO_);
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Disable depth test and blend for fullscreen quad
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    ssaoShader_->use();
    
    // Bind G-Buffer textures
    bindGBufferTextures();
    
    // Bind noise texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture_);
    
    // Set uniforms
    ssaoShader_->setInt("gPosition", 0);
    ssaoShader_->setInt("gNormal", 1);
    ssaoShader_->setInt("texNoise", 2);
    
    // Upload sample kernel
    for (int i = 0; i < sampleCount_; ++i) {
        std::string uniformName = "samples[" + std::to_string(i) + "]";
        ssaoShader_->setVec3(uniformName, ssaoKernel_[i]);
    }
    
    // Camera matrices
    ssaoShader_->setMat4("projection", ctx.camera->GetProjectionMatrix());
    ssaoShader_->setMat4("view", ctx.camera->GetViewMatrix());
    
    // SSAO parameters
    ssaoShader_->setFloat("radius", radius_);
    ssaoShader_->setFloat("bias", bias_);
    ssaoShader_->setFloat("power", power_);
    ssaoShader_->setInt("kernelSize", sampleCount_);
    
    // Set noise scale (no setVec2 method, use direct uniform)
    GLint noiseScaleLoc = ssaoShader_->uniformLocation("noiseScale");
    if (noiseScaleLoc >= 0) {
        glUniform2f(noiseScaleLoc, 
            static_cast<float>(width_) / 4.0f,
            static_cast<float>(height_) / 4.0f);
    }
    
    // Draw fullscreen quad
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // === Pass 2: Blur SSAO ===
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO_);
    glClear(GL_COLOR_BUFFER_BIT);
    
    blurShader_->use();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture_);
    blurShader_->setInt("ssaoInput", 0);
    
    // Render fullscreen quad
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAOPass::resize(int width, int height)
{
    width_ = width;
    height_ = height;
    
    if (initialized_) {
        createFramebuffers();
    }
}

void SSAOPass::cleanup()
{
    if (ssaoFBO_ != 0) {
        glDeleteFramebuffers(1, &ssaoFBO_);
        glDeleteTextures(1, &ssaoTexture_);
        ssaoFBO_ = 0;
        ssaoTexture_ = 0;
    }
    
    if (ssaoBlurFBO_ != 0) {
        glDeleteFramebuffers(1, &ssaoBlurFBO_);
        glDeleteTextures(1, &ssaoBlurTexture_);
        ssaoBlurFBO_ = 0;
        ssaoBlurTexture_ = 0;
    }
    
    if (noiseTexture_ != 0) {
        glDeleteTextures(1, &noiseTexture_);
        noiseTexture_ = 0;
    }
    
    ssaoKernel_.clear();
    initialized_ = false;
}

} // namespace kcShaders
