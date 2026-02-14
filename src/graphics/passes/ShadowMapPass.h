#pragma once

#include "../RenderPass.h"
#include "../ShaderProgram.h"
#include <glad/glad.h>

namespace kcShaders {

/**
 * @brief Shadow Map Pass
 * Renders scene from light's perspective to generate shadow map
 * Currently supports directional light shadows
 */
class ShadowMapPass : public RenderPass {
public:
    /**
     * @brief Construct shadow map pass
     * @param shadowShader Depth-only shader for shadow mapping
     * @param shadowMapSize Resolution of shadow map (default 2048x2048)
     */
    ShadowMapPass(ShaderProgram* shadowShader, int shadowMapSize = 2048);
    ~ShadowMapPass() override;
    
    void execute(RenderContext& ctx) override;
    void resize(int width, int height) override;
    
    /**
     * @brief Get shadow map texture ID
     */
    GLuint getShadowMap() const { return shadowMap_; }
    
    /**
     * @brief Get light space matrix (for transforming world pos to light space)
     */
    glm::mat4 getLightSpaceMatrix() const { return lightSpaceMatrix_; }
    
    /**
     * @brief Setup shadow map framebuffer and textures
     */
    void setup();
    
    /**
     * @brief Cleanup shadow map resources
     */
    void cleanup();

private:
    ShaderProgram* shadowShader_;
    int shadowMapSize_;
    
    GLuint shadowFBO_;
    GLuint shadowMap_;  // Depth texture
    
    glm::mat4 lightSpaceMatrix_;  // Light's view-projection matrix
    
    /**
     * @brief Calculate light space matrix from directional light
     */
    glm::mat4 calculateLightSpaceMatrix(const glm::vec3& lightDir, 
                                         const glm::vec3& sceneCenter,
                                         float sceneRadius);
};

} // namespace kcShaders
