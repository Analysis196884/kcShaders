#include "DeferredPipeline.h"
#include "../ShaderProgram.h"
#include "../passes/GBufferPass.h"
#include "../passes/LightingPass.h"
#include "../gbuffer.h"
#include <iostream>

namespace kcShaders {

DeferredPipeline::DeferredPipeline(GBuffer* gbuffer, GLuint fbo, GLuint quadVAO, int width, int height)
    : gbuffer_(gbuffer)
    , fbo_(fbo)
    , quadVAO_(quadVAO)
    , width_(width)
    , height_(height)
    , gbufferPass_(nullptr)
    , lightingPass_(nullptr)
{
}

DeferredPipeline::~DeferredPipeline()
{
    cleanup();
}

bool DeferredPipeline::initialize()
{
    if (!gbuffer_) {
        std::cerr << "[DeferredPipeline] Error: G-Buffer is null\n";
        return false;
    }
    
    std::cout << "[DeferredPipeline] Initialized\n";
    return true;
}

bool DeferredPipeline::loadShaders(
    const std::string& geomVert,
    const std::string& geomFrag,
    const std::string& lightVert,
    const std::string& lightFrag
)
{
    std::cout << "[DeferredPipeline] Loading shaders...\n";
    std::cout << "  Geometry: " << geomVert << " + " << geomFrag << "\n";
    std::cout << "  Lighting: " << lightVert << " + " << lightFrag << "\n";
    
    // Create geometry shader
    geometryShader_ = std::make_unique<ShaderProgram>();
    if (!geometryShader_->loadFromFiles(geomVert, geomFrag)) {
        std::cerr << "[DeferredPipeline] Failed to load geometry shader\n";
        return false;
    }
    
    // Create lighting shader
    lightingShader_ = std::make_unique<ShaderProgram>();
    if (!lightingShader_->loadFromFiles(lightVert, lightFrag)) {
        std::cerr << "[DeferredPipeline] Failed to load lighting shader\n";
        return false;
    }
    
    // Create passes
    auto gbufferPass = std::make_unique<GBufferPass>(gbuffer_, geometryShader_.get());
    auto lightingPass = std::make_unique<LightingPass>(
        gbuffer_,
        lightingShader_.get(),
        fbo_,
        quadVAO_,
        width_,
        height_
    );
    
    // Store non-owning pointers for direct access
    gbufferPass_ = gbufferPass.get();
    lightingPass_ = lightingPass.get();
    
    // Transfer ownership to passes_ vector
    passes_.clear();
    passes_.push_back(std::move(gbufferPass));
    passes_.push_back(std::move(lightingPass));
    
    std::cout << "[DeferredPipeline] Shaders loaded successfully\n";
    return true;
}

void DeferredPipeline::execute(RenderContext& ctx)
{
    if (!ctx.isValid()) {
        std::cerr << "[DeferredPipeline] Invalid render context\n";
        return;
    }
    
    // Execute all passes in sequence
    for (auto& pass : passes_) {
        pass->execute(ctx);
    }
}

void DeferredPipeline::resize(int width, int height)
{
    width_ = width;
    height_ = height;
    
    // Resize all passes
    for (auto& pass : passes_) {
        pass->resize(width, height);
    }
    
    std::cout << "[DeferredPipeline] Resized to " << width << "x" << height << "\n";
}

void DeferredPipeline::cleanup()
{
    passes_.clear();
    geometryShader_.reset();
    lightingShader_.reset();
    gbufferPass_ = nullptr;
    lightingPass_ = nullptr;
}

} // namespace kcShaders
