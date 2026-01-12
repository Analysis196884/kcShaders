#pragma once

#include "../RenderPass.h"
#include "../ShaderProgram.h"
#include <memory>

namespace kcShaders {

class GBuffer;

/**
 * GBufferPass: Geometry pass for deferred rendering
 * Renders scene geometry into G-Buffer (albedo, normal, position, material)
 */
class GBufferPass : public RenderPass {
public:
    GBufferPass(GBuffer* gbuffer, ShaderProgram* geometryShader);
    ~GBufferPass() override = default;
    
    void execute(RenderContext& ctx) override;
    void resize(int width, int height) override;

private:
    GBuffer* gbuffer_;
    ShaderProgram* geometryShader_;
    bool firstFrame_ = true;
};

} // namespace kcShaders
