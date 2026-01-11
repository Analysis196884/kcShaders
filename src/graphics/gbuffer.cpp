#include "gbuffer.h"

#include <iostream>

namespace kcShaders {

GBuffer::GBuffer()
    : FBO_(0), albedoTexture_(0), normalTexture_(0), positionTexture_(0),
      materialTexture_(0), depthTexture_(0), width_(0), height_(0)
{
}

GBuffer::~GBuffer()
{
    deleteBuffers();
}

bool GBuffer::initialize(int width, int height)
{
    width_ = width;
    height_ = height;
    
    // Create FBO
    glGenFramebuffers(1, &FBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_);
    
    // Create textures
    // Albedo (RGB)
    glGenTextures(1, &albedoTexture_);
    glBindTexture(GL_TEXTURE_2D, albedoTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedoTexture_, 0);
    
    // Normal (RGB)
    glGenTextures(1, &normalTexture_);
    glBindTexture(GL_TEXTURE_2D, normalTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTexture_, 0);
    
    // Position (RGB)
    glGenTextures(1, &positionTexture_);
    glBindTexture(GL_TEXTURE_2D, positionTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, positionTexture_, 0);
    
    // Material (RGBA - metallic, roughness, AO, unused)
    glGenTextures(1, &materialTexture_);
    glBindTexture(GL_TEXTURE_2D, materialTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, materialTexture_, 0);
    
    // Depth texture
    glGenTextures(1, &depthTexture_);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture_, 0);
    
    // Tell OpenGL which color attachments we'll use for rendering
    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, attachments);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "GBuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        deleteBuffers();
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void GBuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GBuffer::bindForReading()
{
    // Only bind textures to texture units; do not touch framebuffer binding here.
    // Match actual uniform locations: Albedo:0 Material:1 Normal:2 Position:3
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoTexture_);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, materialTexture_);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTexture_);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, positionTexture_);
}

void GBuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBuffer::resize(int width, int height)
{
    if (width == width_ && height == height_) {
        return;
    }
    
    deleteBuffers();
    initialize(width, height);
}

void GBuffer::deleteBuffers()
{
    if (albedoTexture_) glDeleteTextures(1, &albedoTexture_);
    if (normalTexture_) glDeleteTextures(1, &normalTexture_);
    if (positionTexture_) glDeleteTextures(1, &positionTexture_);
    if (materialTexture_) glDeleteTextures(1, &materialTexture_);
    if (depthTexture_) glDeleteTextures(1, &depthTexture_);
    if (FBO_) glDeleteFramebuffers(1, &FBO_);
    
    albedoTexture_ = normalTexture_ = positionTexture_ = materialTexture_ = depthTexture_ = 0;
    FBO_ = 0;
}

} // namespace kcShaders
