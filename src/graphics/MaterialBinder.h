#pragma once

#include <glad/glad.h>

namespace kcShaders {

// Forward declarations
class ShaderProgram;
struct Material;

/**
 * MaterialBinder: Centralized material and texture binding
 * Eliminates duplicate binding code in forward and deferred geometry passes
 */
class MaterialBinder {
public:
    /**
     * Bind material properties and textures to a shader
     * Uses standardized texture unit assignments:
     *   Unit 0: albedoMap
     *   Unit 1: metallicMap
     *   Unit 2: roughnessMap
     *   Unit 3: normalMap
     *   Unit 4: aoMap
     *   Unit 5: emissiveMap
     */
    static void bind(ShaderProgram& shader, const Material* material);
    
    /**
     * Unbind all texture units (cleanup after rendering)
     */
    static void unbindTextures();

private:
    // Texture unit indices (matching both forward and deferred shaders)
    enum TextureUnit {
        Albedo = 0,
        Metallic = 1,
        Roughness = 2,
        Normal = 3,
        AO = 4,
        Emissive = 5
    };
};

} // namespace kcShaders
