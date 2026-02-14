#include "LightingPass.h"
#include "../RenderContext.h"
#include "../gbuffer.h"
#include "../../scene/scene.h"
#include "../../scene/camera.h"
#include "../../scene/light.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace kcShaders {

LightingPass::LightingPass(GBuffer* gbuffer, ShaderProgram* lightingShader,
                           GLuint fbo, GLuint quadVAO, int fbWidth, int fbHeight)
    : gbuffer_(gbuffer)
    , lightingShader_(lightingShader)
    , fbo_(fbo)
    , quadVAO_(quadVAO)
    , fbWidth_(fbWidth)
    , fbHeight_(fbHeight)
{
}

void LightingPass::execute(RenderContext& ctx) {
    if (!ctx.isValid() || !gbuffer_ || !lightingShader_) {
        return;
    }
    
    // Bind final framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    
    // Set viewport
    glViewport(0, 0, fbWidth_, fbHeight_);
    
    // Clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Disable depth test for fullscreen quad
    glDisable(GL_DEPTH_TEST);
    
    // Use lighting shader
    lightingShader_->use();
    
    // Bind G-Buffer textures
    bindGBufferTextures();
    
    // Set camera uniforms
    lightingShader_->setVec3("viewPos", ctx.camera->GetPosition());
    lightingShader_->setMat4("uView", ctx.camera->GetViewMatrix());
    
    // Set light uniforms
    setLightUniforms(ctx);
    
    // Render fullscreen quad
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    
    // Check for errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR && firstFrame_) {
        std::cout << "[LightingPass] OpenGL Error: " << err << "\n";
    }
    
    // Debug output
    // if (firstFrame_) {
    //     glReadBuffer(GL_COLOR_ATTACHMENT0);
    //     float pixel[4];
    //     glReadPixels(fbWidth_/2, fbHeight_/2, 1, 1, GL_RGBA, GL_FLOAT, pixel);
    //     std::cout << "[LightingPass] Center pixel output: (" 
    //               << pixel[0] << ", " << pixel[1] << ", " << pixel[2] << ", " << pixel[3] << ")\n";
    //     std::cout << "[LightingPass] FBO: " << fbo_ << "\n";
    //     firstFrame_ = false;
    // }
    
    // Re-enable depth test
    glEnable(GL_DEPTH_TEST);
    
    // Unbind textures (including SSAO at unit 4 and shadow map at unit 5)
    for (int i = 0; i < 6; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glFlush();
    
    firstFrame_ = false;
}

void LightingPass::resize(int width, int height) {
    fbWidth_ = width;
    fbHeight_ = height;
}

void LightingPass::bindGBufferTextures() {
    // Bind G-Buffer textures to texture units 0-3
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer_->getPositionTexture());
    lightingShader_->setInt("GPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer_->getNormalTexture());
    lightingShader_->setInt("GNormal", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gbuffer_->getAlbedoTexture());
    lightingShader_->setInt("GAlbedo", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gbuffer_->getMaterialTexture());
    lightingShader_->setInt("GMaterial", 3);
    
    // Bind SSAO texture if available
    if (ssaoTexture_ != 0) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ssaoTexture_);
        lightingShader_->setInt("GSSAO", 4);
        lightingShader_->setInt("useSSAO", 1);
    } else {
        lightingShader_->setInt("useSSAO", 0);
    }
    
    // Bind shadow map texture if available
    if (shadowMapTexture_ != 0) {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMapTexture_);
        lightingShader_->setInt("shadowMap", 5);
        lightingShader_->setInt("useShadows", 1);
        lightingShader_->setMat4("lightSpaceMatrix", lightSpaceMatrix_);
    } else {
        lightingShader_->setInt("useShadows", 0);
    }
}

void LightingPass::setLightUniforms(RenderContext& ctx) {
    int numDirLights = 0;
    int numPointLights = 0;
    int numSpotLights = 0;
    int numAreaLights = 0;
    glm::vec3 ambientLight(0.0f);
    
    // Process scene lights
    for (Light* light : ctx.scene->lights) {
        if (!light || !light->enabled) continue;
        
        switch (light->GetType()) {
            case LightType::Directional: {
                if (numDirLights >= 4) break;
                DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
                std::string base = "dirLights[" + std::to_string(numDirLights) + "]";
                
                lightingShader_->setVec3(base + ".direction", dirLight->direction);
                lightingShader_->setVec3(base + ".color", dirLight->color);
                lightingShader_->setFloat(base + ".intensity", dirLight->intensity);
                numDirLights++;
                break;
            }
            
            case LightType::Point: {
                if (numPointLights >= 8) break;
                PointLight* pointLight = static_cast<PointLight*>(light);
                std::string base = "pointLights[" + std::to_string(numPointLights) + "]";
                
                lightingShader_->setVec3(base + ".position", pointLight->position);
                lightingShader_->setVec3(base + ".color", pointLight->color);
                lightingShader_->setFloat(base + ".intensity", pointLight->intensity);
                lightingShader_->setFloat(base + ".radius", pointLight->radius);
                lightingShader_->setFloat(base + ".constant", pointLight->constant);
                lightingShader_->setFloat(base + ".linear", pointLight->linear);
                lightingShader_->setFloat(base + ".quadratic", pointLight->quadratic);
                numPointLights++;
                break;
            }
            
            case LightType::Spot: {
                if (numSpotLights >= 4) break;
                SpotLight* spotLight = static_cast<SpotLight*>(light);
                std::string base = "spotLights[" + std::to_string(numSpotLights) + "]";
                
                lightingShader_->setVec3(base + ".position", spotLight->position);
                lightingShader_->setVec3(base + ".direction", spotLight->direction);
                lightingShader_->setVec3(base + ".color", spotLight->color);
                lightingShader_->setFloat(base + ".intensity", spotLight->intensity);
                lightingShader_->setFloat(base + ".innerConeAngle", spotLight->innerConeAngle);
                lightingShader_->setFloat(base + ".outerConeAngle", spotLight->outerConeAngle);
                lightingShader_->setFloat(base + ".constant", spotLight->constant);
                lightingShader_->setFloat(base + ".linear", spotLight->linear);
                lightingShader_->setFloat(base + ".quadratic", spotLight->quadratic);
                numSpotLights++;
                break;
            }
            
            case LightType::Ambient: {
                AmbientLight* ambLight = static_cast<AmbientLight*>(light);
                ambientLight = ambLight->color * ambLight->intensity;
                break;
            }
            
            default:
                break;
        }
    }
    
    // Set light counts
    lightingShader_->setInt("numDirLights", numDirLights);
    lightingShader_->setInt("numPointLights", numPointLights);
    lightingShader_->setInt("numSpotLights", numSpotLights);
    lightingShader_->setInt("numAreaLights", numAreaLights);
    lightingShader_->setVec3("ambientLight", ambientLight);
}

} // namespace kcShaders
