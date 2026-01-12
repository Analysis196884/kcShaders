#pragma once

#include <glad/glad.h>
#include "RenderPipeline.h"
#include <memory>
#include <string>

namespace kcShaders {

class GBuffer;
class ShaderProgram;
class GBufferPass;
class LightingPass;

/**
 * @brief Deferred rendering pipeline
 * 
 * Executes geometry pass followed by lighting pass using G-Buffer.
 */
class DeferredPipeline : public RenderPipeline {
public:
    /**
     * @brief Construct deferred pipeline
     * @param gbuffer G-Buffer for storing geometry data
     * @param fbo Final framebuffer to render to
     * @param quadVAO VAO for fullscreen quad
     * @param width Initial viewport width
     * @param height Initial viewport height
     */
    DeferredPipeline(GBuffer* gbuffer, GLuint fbo, GLuint quadVAO, int width, int height);
    ~DeferredPipeline() override;
    
    bool initialize() override;
    void execute(RenderContext& ctx) override;
    void resize(int width, int height) override;
    void cleanup() override;
    const char* getName() const override { return "DeferredPipeline"; }
    
    /**
     * @brief Load shaders for the pipeline
     * @return true if shaders loaded successfully
     */
    bool loadShaders(
        const std::string& geomVert,
        const std::string& geomFrag,
        const std::string& lightVert,
        const std::string& lightFrag
    );

private:
    GBuffer* gbuffer_;
    GLuint fbo_;
    GLuint quadVAO_;
    int width_;
    int height_;
    
    std::unique_ptr<ShaderProgram> geometryShader_;
    std::unique_ptr<ShaderProgram> lightingShader_;
    
    GBufferPass* gbufferPass_;      // Non-owning pointer (owned by passes_)
    LightingPass* lightingPass_;    // Non-owning pointer (owned by passes_)
};

} // namespace kcShaders
