#include "DeferredPipeline.h"
#include "../ShaderProgram.h"
#include "../passes/GBufferPass.h"
#include "../passes/LightingPass.h"
#include "../passes/SSAOPass.h"
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
    , ssaoPass_(nullptr)
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
    const std::string& lightFrag,
    const std::string& ssaoVert,
    const std::string& ssaoFrag,
    const std::string& ssaoBlurVert,
    const std::string& ssaoBlurFrag
)
{
    std::cout << "[DeferredPipeline] Loading shaders...\n";
    std::cout << "  Geometry: " << geomVert << " + " << geomFrag << "\n";
    std::cout << "  Lighting: " << lightVert << " + " << lightFrag << "\n";
    
    // Create temporary shader objects
    auto tempGeometryShader = std::make_unique<ShaderProgram>();
    if (!tempGeometryShader->loadFromFiles(geomVert, geomFrag)) {
        std::cerr << "[DeferredPipeline] Failed to load geometry shader\n";
        // Keep old shaders if they exist, don't update anything
        return false;
    }
    
    auto tempLightingShader = std::make_unique<ShaderProgram>();
    if (!tempLightingShader->loadFromFiles(lightVert, lightFrag)) {
        std::cerr << "[DeferredPipeline] Failed to load lighting shader\n";
        // Keep old shaders if they exist, don't update anything
        return false;
    }
    
    // Load SSAO shaders if provided
    std::unique_ptr<ShaderProgram> tempSsaoShader;
    std::unique_ptr<ShaderProgram> tempSsaoBlurShader;
    bool hasSSAO = !ssaoVert.empty() && !ssaoFrag.empty() && !ssaoBlurVert.empty() && !ssaoBlurFrag.empty();
    
    if (hasSSAO) {
        std::cout << "  SSAO: " << ssaoVert << " + " << ssaoFrag << "\n";
        std::cout << "  SSAO Blur: " << ssaoBlurVert << " + " << ssaoBlurFrag << "\n";
        
        tempSsaoShader = std::make_unique<ShaderProgram>();
        if (!tempSsaoShader->loadFromFiles(ssaoVert, ssaoFrag)) {
            std::cerr << "[DeferredPipeline] Failed to load SSAO shader\n";
            return false;
        }
        
        tempSsaoBlurShader = std::make_unique<ShaderProgram>();
        if (!tempSsaoBlurShader->loadFromFiles(ssaoBlurVert, ssaoBlurFrag)) {
            std::cerr << "[DeferredPipeline] Failed to load SSAO blur shader\n";
            return false;
        }
    }
    
    // Only update if all required shaders loaded successfully
    geometryShader_ = std::move(tempGeometryShader);
    lightingShader_ = std::move(tempLightingShader);
    
    if (hasSSAO) {
        ssaoShader_ = std::move(tempSsaoShader);
        ssaoBlurShader_ = std::move(tempSsaoBlurShader);
    }
    
    // Create passes with the new valid shaders
    auto gbufferPass = std::make_unique<GBufferPass>(gbuffer_, geometryShader_.get());
    
    // Create SSAO pass if shaders are loaded
    std::unique_ptr<SSAOPass> ssaoPass;
    if (hasSSAO && ssaoShader_ && ssaoBlurShader_) {
        ssaoPass = std::make_unique<SSAOPass>(
            gbuffer_,
            ssaoShader_.get(),
            ssaoBlurShader_.get(),
            quadVAO_,
            width_,
            height_
        );
        ssaoPass_ = ssaoPass.get();
    } else {
        ssaoPass_ = nullptr;
    }
    
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
    
    // Add SSAO pass if available (will be used based on ssaoEnabled_ flag)
    if (ssaoPass) {
        passes_.push_back(std::move(ssaoPass));
        ssaoEnabled_ = true;  // Enable SSAO by default if shaders are loaded
    }
    
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
    
    // Safety check: ensure shaders are loaded
    if (!geometryShader_ || !lightingShader_) {
        std::cerr << "[DeferredPipeline] Shaders not loaded, cannot execute\n";
        return;
    }
    
    // Safety check: ensure passes are created
    if (passes_.empty()) {
        std::cerr << "[DeferredPipeline] No render passes configured\n";
        return;
    }
    
    // Execute all passes in sequence
    // Order: GBuffer -> (optional) SSAO -> Lighting
    for (auto& pass : passes_) {
        if (pass) {  // Check pass pointer is valid
            pass->execute(ctx);
            
            // After SSAO pass executes, set the SSAO texture for lighting pass
            if (dynamic_cast<SSAOPass*>(pass.get()) && ssaoPass_ && lightingPass_) {
                if (ssaoEnabled_) {
                    GLuint ssaoTex = ssaoPass_->getSSAOTexture();
                    lightingPass_->setSSAOTexture(ssaoTex);
                } else {
                    lightingPass_->setSSAOTexture(0);
                }
            }
        }
    }
}

void DeferredPipeline::enableSSAO(bool enable)
{
    if (!ssaoPass_) {
        std::cerr << "[DeferredPipeline] SSAO pass not available\n";
        return;
    }
    
    ssaoEnabled_ = enable;
    std::cout << "[DeferredPipeline] SSAO " << (enable ? "enabled" : "disabled") << "\n";
}

void DeferredPipeline::resize(int width, int height)
{
    width_ = width;
    height_ = height;
    
    // Resize all passes
    for (auto& pass : passes_) {
        pass->resize(width, height);
    }
}

void DeferredPipeline::cleanup()
{
    passes_.clear();
    geometryShader_.reset();
    lightingShader_.reset();
    ssaoShader_.reset();
    ssaoBlurShader_.reset();
    gbufferPass_ = nullptr;
    lightingPass_ = nullptr;
    ssaoPass_ = nullptr;
}

} // namespace kcShaders
