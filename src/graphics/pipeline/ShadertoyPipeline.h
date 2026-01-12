#pragma once

#include <glad/glad.h>
#include "RenderPipeline.h"
#include <memory>
#include <string>

namespace kcShaders {

class ShaderProgram;

/**
 * @brief Shadertoy-style rendering pipeline
 * 
 * Renders fullscreen shader effects compatible with Shadertoy conventions.
 */
class ShadertoyPipeline : public RenderPipeline {
public:
    /**
     * @brief Construct shadertoy pipeline
     * @param fbo Framebuffer to render to
     * @param vao VAO for fullscreen triangle
     * @param width Initial viewport width
     * @param height Initial viewport height
     */
    ShadertoyPipeline(GLuint fbo, GLuint vao, int width, int height);
    ~ShadertoyPipeline() override;
    
    bool initialize() override;
    void execute(RenderContext& ctx) override;
    void resize(int width, int height) override;
    void cleanup() override;
    const char* getName() const override { return "ShadertoyPipeline"; }
    
    /**
     * @brief Load shadertoy shader
     * @param vertPath Vertex shader path (usually simple passthrough)
     * @param fragPath Fragment shader path (main shadertoy code)
     * @return true if shaders loaded successfully
     */
    bool loadShaders(const std::string& vertPath, const std::string& fragPath);

private:
    GLuint fbo_;
    GLuint vao_;
    int width_;
    int height_;
    
    std::unique_ptr<ShaderProgram> shader_;
};

} // namespace kcShaders
