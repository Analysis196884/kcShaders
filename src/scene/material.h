#pragma once 

#include <glm/glm.hpp>
#include <string>

namespace kcShaders {

// Material properties for Physically Based Rendering (PBR)
class Material {
public:
    Material();
    ~Material() = default;

    // Base color (albedo)
    glm::vec3 albedo = glm::vec3(0.8f, 0.8f, 0.8f);
    
    // PBR properties
    float metallic = 0.0f;     // 0.0 = dielectric, 1.0 = metal
    float roughness = 0.5f;    // 0.0 = smooth, 1.0 = rough
    float ao = 1.0f;           // Ambient occlusion (0.0 = fully occluded, 1.0 = no occlusion)
    
    // Emissive properties
    glm::vec3 emissive = glm::vec3(0.0f);
    float emissiveStrength = 0.0f;
    
    // Transparency
    float opacity = 1.0f;      // 0.0 = fully transparent, 1.0 = fully opaque
    
    // Texture IDs (0 = no texture)
    unsigned int albedoMap = 0;
    unsigned int metallicMap = 0;
    unsigned int roughnessMap = 0;
    unsigned int normalMap = 0;
    unsigned int aoMap = 0;
    unsigned int emissiveMap = 0;
    
    // Material name (optional, for debugging)
    std::string name = "Unnamed Material";
    
    // Helper functions to create common materials
    static Material* CreateDefault();
    static Material* CreateMetal(const glm::vec3& color, float roughness = 0.3f);
    static Material* CreatePlastic(const glm::vec3& color, float roughness = 0.5f);
    static Material* CreateGlass(const glm::vec3& color, float roughness = 0.0f);
    static Material* CreateEmissive(const glm::vec3& color, float strength = 1.0f);
};

} // namespace kcShaders