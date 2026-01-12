#include "GBufferPass.h"
#include "../RenderContext.h"
#include "../MaterialBinder.h"
#include "../gbuffer.h"
#include "../../scene/scene.h"
#include "../../scene/camera.h"
#include "../../scene/mesh.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace kcShaders {

GBufferPass::GBufferPass(GBuffer* gbuffer, ShaderProgram* geometryShader)
    : gbuffer_(gbuffer)
    , geometryShader_(geometryShader)
{
}

void GBufferPass::execute(RenderContext& ctx) {
    if (!ctx.isValid() || !gbuffer_ || !geometryShader_) {
        return;
    }
    
    // Bind G-Buffer
    gbuffer_->bind();
    
    // Set viewport
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
    
    // Clear all attachments
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    
    // Use geometry shader
    geometryShader_->use();
    
    // Set view and projection matrices
    geometryShader_->setMat4("uView", ctx.camera->GetViewMatrix());
    geometryShader_->setMat4("uProjection", ctx.camera->GetProjectionMatrix());
    
    // Collect render items
    std::vector<RenderItem> items;
    ctx.scene->collectRenderItems(items);
    
    // Render all meshes
    for (const auto& item : items) {
        if (!item.mesh) continue;
        
        // Set model matrix
        geometryShader_->setMat4("uModel", item.modelMatrix);
        
        // Bind material
        MaterialBinder::bind(*geometryShader_, item.material);
        
        // Draw mesh
        item.mesh->draw();
    }
    
    // // Debug: Check first pixel of G-Buffer
    // if (firstFrame_) {
    //     glReadBuffer(GL_COLOR_ATTACHMENT0);
    //     float albedo[4];
    //     glReadPixels(ctx.viewportWidth/2, ctx.viewportHeight/2, 1, 1, GL_RGBA, GL_FLOAT, albedo);
    //     std::cout << "[GBufferPass] Center pixel albedo: (" 
    //               << albedo[0] << ", " << albedo[1] << ", " << albedo[2] << ", " << albedo[3] << ")\n";
        
    //     glReadBuffer(GL_COLOR_ATTACHMENT1);
    //     float normal[4];
    //     glReadPixels(ctx.viewportWidth/2, ctx.viewportHeight/2, 1, 1, GL_RGBA, GL_FLOAT, normal);
    //     std::cout << "[GBufferPass] Center pixel normal: (" 
    //               << normal[0] << ", " << normal[1] << ", " << normal[2] << ", " << normal[3] << ")\n";
        
    //     glReadBuffer(GL_COLOR_ATTACHMENT2);
    //     float position[4];
    //     glReadPixels(ctx.viewportWidth/2, ctx.viewportHeight/2, 1, 1, GL_RGBA, GL_FLOAT, position);
    //     std::cout << "[GBufferPass] Center pixel position: (" 
    //               << position[0] << ", " << position[1] << ", " << position[2] << ", " << position[3] << ")\n";
        
    //     glReadBuffer(GL_COLOR_ATTACHMENT3);
    //     float material[4];
    //     glReadPixels(ctx.viewportWidth/2, ctx.viewportHeight/2, 1, 1, GL_RGBA, GL_FLOAT, material);
    //     std::cout << "[GBufferPass] Center pixel material (M/R/AO): (" 
    //               << material[0] << ", " << material[1] << ", " << material[2] << ", " << material[3] << ")\n";
        
    //     firstFrame_ = false;
    // }
    
    // Unbind G-Buffer
    gbuffer_->unbind();
}

void GBufferPass::resize(int width, int height) {
    if (gbuffer_) {
        gbuffer_->resize(width, height);
    }
}

} // namespace kcShaders
