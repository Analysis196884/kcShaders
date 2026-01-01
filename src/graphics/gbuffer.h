#pragma once

#include <glad/glad.h>

namespace kcShaders {

class GBuffer {
public:
    GBuffer();
    ~GBuffer();
    
    bool initialize(int width, int height);
    void resize(int width, int height);
    void bind();
    void bindForReading();
    void unbind();
    
    GLuint getAlbedoTexture() const { return albedoTexture_; }
    GLuint getNormalTexture() const { return normalTexture_; }
    GLuint getPositionTexture() const { return positionTexture_; }
    GLuint getMaterialTexture() const { return materialTexture_; }
    
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
private:
    void deleteBuffers();
    
private:
    GLuint FBO_;
    GLuint albedoTexture_;
    GLuint normalTexture_;
    GLuint positionTexture_;
    GLuint materialTexture_;
    GLuint depthTexture_;
    
    int width_;
    int height_;
};

} // namespace kcShaders
