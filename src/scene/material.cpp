#include "material.h"

namespace kcShaders {

Material::Material()
    : albedo(0.8f, 0.8f, 0.8f)
    , metallic(0.0f)
    , roughness(0.5f)
    , ao(1.0f)
    , emissive(0.0f)
    , emissiveStrength(0.0f)
    , opacity(1.0f)
    , albedoMap(0)
    , metallicMap(0)
    , roughnessMap(0)
    , normalMap(0)
    , aoMap(0)
    , emissiveMap(0)
    , name("Unnamed Material")
{
}

Material* Material::CreateDefault()
{
    Material* mat = new Material();
    mat->name = "Default Material";
    mat->albedo = glm::vec3(0.8f, 0.8f, 0.8f);
    mat->metallic = 0.0f;
    mat->roughness = 0.5f;
    return mat;
}

Material* Material::CreateMetal(const glm::vec3& color, float roughness)
{
    Material* mat = new Material();
    mat->name = "Metal Material";
    mat->albedo = color;
    mat->metallic = 1.0f;
    mat->roughness = roughness;
    return mat;
}

Material* Material::CreatePlastic(const glm::vec3& color, float roughness)
{
    Material* mat = new Material();
    mat->name = "Plastic Material";
    mat->albedo = color;
    mat->metallic = 0.0f;
    mat->roughness = roughness;
    return mat;
}

Material* Material::CreateGlass(const glm::vec3& color, float roughness)
{
    Material* mat = new Material();
    mat->name = "Glass Material";
    mat->albedo = color;
    mat->metallic = 0.0f;
    mat->roughness = roughness;
    mat->opacity = 0.1f; // Mostly transparent
    return mat;
}

Material* Material::CreateEmissive(const glm::vec3& color, float strength)
{
    Material* mat = new Material();
    mat->name = "Emissive Material";
    mat->albedo = glm::vec3(0.0f); // No base color
    mat->emissive = color;
    mat->emissiveStrength = strength;
    mat->metallic = 0.0f;
    mat->roughness = 1.0f;
    return mat;
}

} // namespace kcShaders