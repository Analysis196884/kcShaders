#pragma once 

#include <glad/glad.h>
#include "RenderPipeline.h"
#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace kcShaders {

class ShaderProgram;

/**
 * @brief Ray Tracing Pipeline using OpenGL Compute Shaders
 * 
 * Renders scenes using ray tracing in a compute shader, outputs to a texture
 * which is then displayed on a fullscreen quad.
 */
class RayTracingPipeline : public RenderPipeline {
public:
    /**
     * @brief Construct ray tracing pipeline
     * @param fbo Framebuffer to render to
     * @param vao VAO for fullscreen triangle
     * @param width Initial viewport width
     * @param height Initial viewport height
     */
    RayTracingPipeline(GLuint fbo, GLuint vao, int width, int height);
    ~RayTracingPipeline() override;
    
    bool initialize() override;
    void execute(RenderContext& ctx) override;
    void resize(int width, int height) override;
    void cleanup() override;
    const char* getName() const override { return "RayTracingPipeline"; }
    
    /**
     * @brief Load compute shader for ray tracing
     * @param computePath Path to compute shader
     * @return true if shader loaded successfully
     */
    bool loadComputeShader(const std::string& computePath);
    
    /**
     * @brief Load display shader (vertex + fragment) for showing the ray traced image
     * @param vertPath Vertex shader path
     * @param fragPath Fragment shader path
     * @return true if shaders loaded successfully
     */
    bool loadDisplayShader(const std::string& vertPath, const std::string& fragPath);
    
    /**
     * @brief Set ray tracing parameters
     */
    void setMaxBounces(int bounces) { maxBounces_ = bounces; }
    void setSamplesPerPixel(int samples) { samplesPerPixel_ = samples; }
    
    /**
     * @brief Upload scene data to GPU
     * @param scene Scene to upload
     */
    void uploadScene(class Scene* scene);
    
private:
    void createOutputTexture();
    void deleteOutputTexture();
    void createSceneBuffers();
    void deleteSceneBuffers();
    
    GLuint fbo_;
    GLuint vao_;
    int width_;
    int height_;
    
    // Compute shader program ID (raw OpenGL handle)
    GLuint computeShaderProgram_;
    
    // Display shader for showing the ray traced result
    std::unique_ptr<ShaderProgram> displayShader_;
    
    // Output texture from compute shader
    GLuint outputTexture_;
    GLuint accumulationTexture_;  // Temporal accumulation texture
    
    // Camera state for detecting changes
    glm::vec3 lastCameraPosition_;
    glm::vec3 lastCameraFront_;
    bool cameraMovedThisFrame_;
    
    // Scene data SSBOs
    GLuint vertexBuffer_;
    GLuint triangleBuffer_;
    GLuint bvhBuffer_;
    GLuint materialBuffer_;
    bool sceneUploaded_;
    
    // Ray tracing parameters
    int maxBounces_;
    int samplesPerPixel_;
    int frameCount_;
};

} // namespace kcShaders