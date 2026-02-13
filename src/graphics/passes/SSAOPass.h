#pragma once

#include "../RenderPass.h"
#include "../ShaderProgram.h"
#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>

namespace kcShaders {

class GBuffer;

/**
 * SSAOPass: Screen-Space Ambient Occlusion pass
 * Generates SSAO texture from G-Buffer depth and normal information
 */
class SSAOPass : public RenderPass {
public:
    /**
     * @brief Construct SSAO pass
     * @param gbuffer G-Buffer containing position and normal data
     * @param ssaoShader Shader for computing SSAO
     * @param blurShader Shader for blurring SSAO result
     * @param quadVAO VAO for fullscreen quad
     * @param width Viewport width
     * @param height Viewport height
     */
    SSAOPass(GBuffer* gbuffer, 
             ShaderProgram* ssaoShader,
             ShaderProgram* blurShader,
             GLuint quadVAO, 
             int width, 
             int height);
    
    ~SSAOPass() override;
    
    void setup() override;
    void execute(RenderContext& ctx) override;
    void resize(int width, int height) override;
    void cleanup() override;
    
    /**
     * @brief Get the final SSAO texture (after blur)
     * @return SSAO texture ID
     */
    GLuint getSSAOTexture() const { return ssaoBlurTexture_; }
    
    /**
     * @brief Set SSAO parameters
     * @param radius Sampling radius in view space
     * @param bias Bias to prevent self-occlusion
     * @param power Power curve for darkening effect
     */
    void setParameters(float radius, float bias, float power) {
        radius_ = radius;
        bias_ = bias;
        power_ = power;
    }
    
    /**
     * @brief Set sample count (must call setup() again after changing)
     * @param count Number of samples (recommended: 8-64)
     */
    void setSampleCount(int count) {
        sampleCount_ = count;
    }

private:
    void createFramebuffers();
    void generateSampleKernel();
    void generateNoiseTexture();
    void bindGBufferTextures();
    
    GBuffer* gbuffer_;
    ShaderProgram* ssaoShader_;
    ShaderProgram* blurShader_;
    GLuint quadVAO_;
    int width_;
    int height_;
    
    // SSAO parameters
    float radius_ = 0.5f;
    float bias_ = 0.025f;
    float power_ = 2.0f;
    int sampleCount_ = 32;
    
    // SSAO textures and framebuffers
    GLuint ssaoFBO_ = 0;
    GLuint ssaoTexture_ = 0;         // Raw SSAO output
    GLuint ssaoBlurFBO_ = 0;
    GLuint ssaoBlurTexture_ = 0;     // Blurred SSAO output
    
    // Noise texture for sample rotation
    GLuint noiseTexture_ = 0;
    
    // Sample kernel
    std::vector<glm::vec3> ssaoKernel_;
    
    bool firstFrame_ = true;
    bool initialized_ = false;
};

} // namespace kcShaders
