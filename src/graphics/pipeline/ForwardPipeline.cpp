#include "ForwardPipeline.h"
#include "../ShaderProgram.h"
#include "../MaterialBinder.h"
#include "../../scene/scene.h"
#include "../../scene/camera.h"
#include "../../scene/light.h"
#include "../../scene/mesh.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace kcShaders {

ForwardPipeline::ForwardPipeline(GLuint fbo, int width, int height)
    : fbo_(fbo)
    , width_(width)
    , height_(height)
{
}

ForwardPipeline::~ForwardPipeline()
{
    cleanup();
}

bool ForwardPipeline::initialize()
{
    std::cout << "[ForwardPipeline] Initialized\n";
    return true;
}

bool ForwardPipeline::loadShaders(const std::string& vertPath, const std::string& fragPath)
{
    std::cout << "[ForwardPipeline] Loading shaders: " << vertPath << " + " << fragPath << "\n";
    
    shader_ = std::make_unique<ShaderProgram>();
    if (!shader_->loadFromFiles(vertPath, fragPath)) {
        std::cerr << "[ForwardPipeline] Failed to load shaders\n";
        return false;
    }
    
    std::cout << "[ForwardPipeline] Shaders loaded successfully\n";
    return true;
}

void ForwardPipeline::execute(RenderContext& ctx)
{
    if (!ctx.isValid() || !shader_) {
        std::cerr << "[ForwardPipeline] Invalid context or shader\n";
        return;
    }
    
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    
    // Clear framebuffer
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    
    // Use shader
    shader_->use();
    
    // Set camera uniforms
    glm::mat4 view = ctx.camera->GetViewMatrix();
    glm::mat4 proj = ctx.camera->GetProjectionMatrix();
    glm::vec3 camPos = ctx.camera->GetPosition();
    
    shader_->setMat4("uView", view);
    shader_->setMat4("uProjection", proj);
    shader_->setVec3("viewPos", camPos);
    
    // Set lighting uniforms
    setLightUniforms(ctx);
    
    // Render scene
    renderScene(ctx);
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ForwardPipeline::renderScene(RenderContext& ctx)
{
    // Collect all render items from the scene
    std::vector<RenderItem> items;
    ctx.scene->collectRenderItems(items);
    
    // Render each item
    for (const auto& item : items) {
        if (item.mesh && item.mesh->isUploaded()) {
            // Set model matrix
            shader_->setMat4("uModel", item.modelMatrix);
            
            // Bind material
            if (item.material) {
                MaterialBinder::bind(*shader_, item.material);
            }
            
            // Draw mesh
            item.mesh->draw();
        }
    }
}

void ForwardPipeline::setLightUniforms(RenderContext& ctx)
{
    int numDirLights = 0;
    int numPointLights = 0;
    int numSpotLights = 0;
    glm::vec3 ambientLight(0.0f);
    
    // Process all lights in the scene
    for (size_t i = 0; i < ctx.scene->lights.size(); ++i) {
        Light* light = ctx.scene->lights[i];
        if (!light || !light->enabled) continue;
        
        switch (light->GetType()) {
            case LightType::Directional: {
                if (numDirLights >= 4) break; // MAX_DIR_LIGHTS = 4
                DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
                
                std::string base = "dirLights[" + std::to_string(numDirLights) + "]";
                shader_->setVec3(base + ".direction", dirLight->direction);
                shader_->setVec3(base + ".color", dirLight->color);
                shader_->setFloat(base + ".intensity", dirLight->intensity);
                
                numDirLights++;
                break;
            }
            
            case LightType::Point: {
                if (numPointLights >= 8) break; // MAX_POINT_LIGHTS = 8
                PointLight* pointLight = static_cast<PointLight*>(light);
                
                std::string base = "pointLights[" + std::to_string(numPointLights) + "]";
                shader_->setVec3(base + ".position", pointLight->position);
                shader_->setVec3(base + ".color", pointLight->color);
                shader_->setFloat(base + ".intensity", pointLight->intensity);
                shader_->setFloat(base + ".constant", pointLight->constant);
                shader_->setFloat(base + ".linear", pointLight->linear);
                shader_->setFloat(base + ".quadratic", pointLight->quadratic);
                shader_->setFloat(base + ".radius", pointLight->radius);
                
                numPointLights++;
                break;
            }
            
            case LightType::Spot: {
                if (numSpotLights >= 4) break; // MAX_SPOT_LIGHTS = 4
                SpotLight* spotLight = static_cast<SpotLight*>(light);
                
                std::string base = "spotLights[" + std::to_string(numSpotLights) + "]";
                shader_->setVec3(base + ".position", spotLight->position);
                shader_->setVec3(base + ".direction", spotLight->direction);
                shader_->setVec3(base + ".color", spotLight->color);
                shader_->setFloat(base + ".intensity", spotLight->intensity);
                shader_->setFloat(base + ".innerConeAngle", spotLight->innerConeAngle);
                shader_->setFloat(base + ".outerConeAngle", spotLight->outerConeAngle);
                shader_->setFloat(base + ".constant", spotLight->constant);
                shader_->setFloat(base + ".linear", spotLight->linear);
                shader_->setFloat(base + ".quadratic", spotLight->quadratic);
                
                numSpotLights++;
                break;
            }
            
            case LightType::Ambient: {
                AmbientLight* ambLight = static_cast<AmbientLight*>(light);
                ambientLight += ambLight->color * ambLight->intensity;
                break;
            }
            
            default:
                break;
        }
    }
    
    // Set light counts
    shader_->setInt("numDirLights", numDirLights);
    shader_->setInt("numPointLights", numPointLights);
    shader_->setInt("numSpotLights", numSpotLights);
    shader_->setVec3("ambientLight", ambientLight);
}

void ForwardPipeline::resize(int width, int height)
{
    width_ = width;
    height_ = height;
    std::cout << "[ForwardPipeline] Resized to " << width << "x" << height << "\n";
}

void ForwardPipeline::cleanup()
{
    shader_.reset();
}

} // namespace kcShaders
