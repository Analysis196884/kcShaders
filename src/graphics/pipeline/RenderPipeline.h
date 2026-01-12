#pragma once

#include "../RenderContext.h"
#include "../RenderPass.h"
#include <vector>
#include <memory>

namespace kcShaders {

/**
 * @brief Base class for rendering pipelines
 * 
 * A RenderPipeline manages a sequence of RenderPass objects
 * and executes them in order to produce the final image.
 */
class RenderPipeline {
public:
    virtual ~RenderPipeline() = default;
    
    /**
     * @brief Initialize the pipeline
     * @return true if initialization succeeded
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Execute all passes in the pipeline
     * @param ctx Rendering context containing scene, camera, and other state
     */
    virtual void execute(RenderContext& ctx) = 0;
    
    /**
     * @brief Resize all passes in the pipeline
     * @param width New viewport width
     * @param height New viewport height
     */
    virtual void resize(int width, int height) = 0;
    
    /**
     * @brief Clean up pipeline resources
     */
    virtual void cleanup() = 0;
    
    /**
     * @brief Get the pipeline name for debugging
     */
    virtual const char* getName() const = 0;

protected:
    std::vector<std::unique_ptr<RenderPass>> passes_;
};

} // namespace kcShaders
