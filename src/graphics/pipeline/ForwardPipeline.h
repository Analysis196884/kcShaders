#pragma once

#include <glad/glad.h>
#include "RenderPipeline.h"
#include <memory>
#include <string>

namespace kcShaders {

class ShaderProgram;

/**
 * @brief Forward rendering pipeline
 * 
 * Traditional single-pass rendering with direct lighting calculations.
 */
class ForwardPipeline : public RenderPipeline {
public:
    /**
     * @brief Construct forward pipeline
     * @param fbo Framebuffer to render to
     * @param width Initial viewport width
     * @param height Initial viewport height
     */
    ForwardPipeline(GLuint fbo, int width, int height);
    ~ForwardPipeline() override;
    
    bool initialize() override;
    void execute(RenderContext& ctx) override;
    void resize(int width, int height) override;
    void cleanup() override;
    const char* getName() const override { return "ForwardPipeline"; }
    
    /**
     * @brief Load shaders for the pipeline
     * @return true if shaders loaded successfully
     */
    bool loadShaders(const std::string& vertPath, const std::string& fragPath);

private:
    void renderScene(RenderContext& ctx);
    void setLightUniforms(RenderContext& ctx);
    
    GLuint fbo_;
    int width_;
    int height_;
    
    std::unique_ptr<ShaderProgram> shader_;
};

} // namespace kcShaders
