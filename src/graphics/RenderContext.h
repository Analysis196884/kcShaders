#pragma once

namespace kcShaders {

// Forward declarations
class Scene;
class Camera;
class GBuffer;

/**
 * RenderContext: Unified context passed to all render passes
 * Centralizes frame-level rendering state to avoid scattered parameters
 */
struct RenderContext {
    // Scene data
    Scene* scene = nullptr;
    Camera* camera = nullptr;
    
    // Viewport dimensions
    int viewportWidth = 0;
    int viewportHeight = 0;
    
    // G-Buffer (for deferred rendering)
    GBuffer* gbuffer = nullptr;
    
    // Frame time (for animations, can be extended later)
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
    
    // Validation
    bool isValid() const {
        return scene != nullptr && camera != nullptr && 
               viewportWidth > 0 && viewportHeight > 0;
    }
};

} // namespace kcShaders
