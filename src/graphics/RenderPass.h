#pragma once

#include "RenderContext.h"

namespace kcShaders {

/**
 * RenderPass: Base interface for all rendering passes
 * Each pass implements a specific stage of the rendering pipeline
 */
class RenderPass {
public:
    virtual ~RenderPass() = default;
    
    /**
     * One-time setup (load shaders, create FBOs, etc.)
     * Called during initialization or when switching pipelines
     */
    virtual void setup() {}
    
    /**
     * Handle viewport/framebuffer resize
     */
    virtual void resize(int width, int height) {}
    
    /**
     * Execute this rendering pass
     * Called every frame with current render context
     */
    virtual void execute(RenderContext& ctx) = 0;
    
    /**
     * Cleanup resources
     */
    virtual void cleanup() {}
};

} // namespace kcShaders
