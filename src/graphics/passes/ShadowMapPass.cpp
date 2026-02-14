#include "ShadowMapPass.h"
#include "../RenderContext.h"
#include "../../scene/scene.h"
#include "../../scene/light.h"
#include "../../scene/mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace kcShaders {

ShadowMapPass::ShadowMapPass(ShaderProgram* shadowShader, int shadowMapSize)
    : shadowShader_(shadowShader)
    , shadowMapSize_(shadowMapSize)
    , shadowFBO_(0)
    , shadowMap_(0)
    , lightSpaceMatrix_(1.0f)
{
}

ShadowMapPass::~ShadowMapPass() {
    cleanup();
}

void ShadowMapPass::setup() {
    // Create framebuffer for shadow map
    glGenFramebuffers(1, &shadowFBO_);
    
    // Create depth texture for shadow map
    glGenTextures(1, &shadowMap_);
    glBindTexture(GL_TEXTURE_2D, shadowMap_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                 shadowMapSize_, shadowMapSize_, 0, 
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                          GL_TEXTURE_2D, shadowMap_, 0);
    glDrawBuffer(GL_NONE);  // No color buffer
    glReadBuffer(GL_NONE);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow map framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapPass::cleanup() {
    if (shadowMap_ != 0) {
        glDeleteTextures(1, &shadowMap_);
        shadowMap_ = 0;
    }
    if (shadowFBO_ != 0) {
        glDeleteFramebuffers(1, &shadowFBO_);
        shadowFBO_ = 0;
    }
}

glm::mat4 ShadowMapPass::calculateLightSpaceMatrix(
    const glm::vec3& lightDir, 
    const glm::vec3& sceneCenter,
    float sceneRadius) 
{
    // Position camera at scene center + offset in light direction
    glm::vec3 lightPos = sceneCenter - glm::normalize(lightDir) * sceneRadius * 2.0f;
    
    // Light view matrix (looking from light towards scene)
    glm::mat4 lightView = glm::lookAt(
        lightPos,
        sceneCenter,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // Orthographic projection for directional light
    float orthoSize = sceneRadius * 1.5f;
    glm::mat4 lightProj = glm::ortho(
        -orthoSize, orthoSize,
        -orthoSize, orthoSize,
        0.1f, sceneRadius * 4.0f
    );
    
    return lightProj * lightView;
}

void ShadowMapPass::execute(RenderContext& ctx) {
    if (!shadowShader_ || shadowFBO_ == 0) {
        return;
    }
    
    Scene* scene = ctx.scene;
    if (!scene) {
        return;
    }
    
    // Find first directional light that casts shadows
    DirectionalLight* shadowLight = nullptr;
    for (Light* light : scene->lights) {
        if (light->GetType() == LightType::Directional && 
            light->castShadows && 
            light->enabled) {
            shadowLight = static_cast<DirectionalLight*>(light);
            break;
        }
    }
    
    if (!shadowLight) {
        return;  // No shadow-casting directional light
    }
    
    // Calculate light space matrix
    // Use scene bounds (simplified: assume scene center at origin with radius 20)
    glm::vec3 sceneCenter(0.0f, 0.0f, 0.0f);
    float sceneRadius = 20.0f;
    lightSpaceMatrix_ = calculateLightSpaceMatrix(
        shadowLight->direction, 
        sceneCenter, 
        sceneRadius
    );
    
    // Render to shadow map
    glViewport(0, 0, shadowMapSize_, shadowMapSize_);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO_);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Enable depth test, disable blending
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    // Optional: Enable front-face culling to reduce shadow acne
    glCullFace(GL_FRONT);
    
    shadowShader_->use();
    shadowShader_->setMat4("lightSpaceMatrix", lightSpaceMatrix_);
    
    // Collect render items
    std::vector<RenderItem> items;
    scene->collectRenderItems(items);
    
    // Render all meshes from light's perspective
    for (const auto& item : items) {
        if (!item.mesh) continue;
        
        // Use the model matrix from render item
        shadowShader_->setMat4("model", item.modelMatrix);
        
        item.mesh->draw();
    }
    
    // Restore culling
    glCullFace(GL_BACK);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapPass::resize(int width, int height) {
    // Shadow map size is independent of screen resolution
    // No action needed
}

} // namespace kcShaders
