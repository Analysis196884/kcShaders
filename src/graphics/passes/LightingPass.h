#pragma once

#include "../RenderPass.h"
#include "../ShaderProgram.h"
#include <glm/glm.hpp>

namespace kcShaders {

class GBuffer;

/**
 * LightingPass: Deferred lighting pass
 * Samples G-Buffer and computes lighting, renders fullscreen quad to FBO
 */
class LightingPass : public RenderPass {
public:
    LightingPass(GBuffer* gbuffer, ShaderProgram* lightingShader, 
                 GLuint fbo, GLuint quadVAO, int fbWidth, int fbHeight);
    ~LightingPass() override = default;
    
    void execute(RenderContext& ctx) override;
    void resize(int width, int height) override;
    
    // Update FBO reference (called when renderer resizes)
    void setFBO(GLuint fbo, int width, int height) {
        fbo_ = fbo;
        fbWidth_ = width;
        fbHeight_ = height;
    }
    
    // Set SSAO texture (0 to disable)
    void setSSAOTexture(GLuint texture) {
        ssaoTexture_ = texture;
    }
    
    // Set shadow map texture (0 to disable) and light space matrix
    void setShadowMap(GLuint texture, const glm::mat4& lightSpaceMatrix) {
        shadowMapTexture_ = texture;
        lightSpaceMatrix_ = lightSpaceMatrix;
    }

private:
    void bindGBufferTextures();
    void setLightUniforms(RenderContext& ctx);
    
    GBuffer* gbuffer_;
    ShaderProgram* lightingShader_;
    GLuint fbo_;
    GLuint quadVAO_;
    int fbWidth_;
    int fbHeight_;
    GLuint ssaoTexture_ = 0;  // SSAO texture (0 = disabled)
    GLuint shadowMapTexture_ = 0;  // Shadow map texture (0 = disabled)
    glm::mat4 lightSpaceMatrix_;   // Light space transform matrix
    bool firstFrame_ = true;
};

} // namespace kcShaders
