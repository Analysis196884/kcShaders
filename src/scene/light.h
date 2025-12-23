#pragma once 

#include <glm/glm.hpp>
#include <string>

namespace kcShaders {

// Light type enumeration for efficient type checking
enum class LightType {
    Directional,
    Point,
    Spot,
    Area,
    Ambient
};

// Base light class
class Light {
public:
    Light(LightType type);
    virtual ~Light() = default;

    // Common properties
    glm::vec3 color = glm::vec3(1.0f);       // Light color
    float intensity = 1.0f;                   // Light intensity/brightness
    bool enabled = true;                      // Whether light is active
    bool castShadows = true;                  // Whether light casts shadows
    
    std::string name = "Light";
    
    // Get light type
    LightType GetType() const { return type_; }
    
    // Virtual method for getting attenuation at a distance
    virtual float GetAttenuation(float distance) const { return 1.0f; }

protected:
    LightType type_;
};

// Directional light (sun-like, parallel rays)
class DirectionalLight : public Light {
public:
    DirectionalLight();
    ~DirectionalLight() = default;

    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);  // Light direction
    
    // Shadow properties for directional lights
    float shadowDistance = 100.0f;            // Max shadow distance
    float shadowBias = 0.005f;                // Shadow bias to prevent acne
    
    static DirectionalLight* CreateSunlight(const glm::vec3& dir = glm::vec3(-0.3f, -1.0f, -0.5f));
};

// Point light (omnidirectional, like a bulb)
class PointLight : public Light {
public:
    PointLight();
    ~PointLight() = default;

    glm::vec3 position = glm::vec3(0.0f);     // Light position in world space
    
    // Attenuation parameters (distance falloff)
    float constant = 1.0f;                    // Constant attenuation
    float linear = 0.09f;                     // Linear attenuation
    float quadratic = 0.032f;                 // Quadratic attenuation
    float radius = 10.0f;                     // Maximum effective radius
    
    float GetAttenuation(float distance) const override;
    
    static PointLight* CreateBulb(const glm::vec3& pos, const glm::vec3& color = glm::vec3(1.0f), float radius = 10.0f);
};

// Spot light (cone-shaped, like a flashlight)
class SpotLight : public Light {
public:
    SpotLight();
    ~SpotLight() = default;

    glm::vec3 position = glm::vec3(0.0f);     // Light position
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);  // Cone direction
    
    // Cone angles (in degrees)
    float innerConeAngle = 12.5f;             // Inner cone (full intensity)
    float outerConeAngle = 17.5f;             // Outer cone (falloff to zero)
    
    // Attenuation
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float range = 20.0f;                      // Maximum range
    
    float GetAttenuation(float distance) const override;
    float GetSpotFalloff(const glm::vec3& lightToFrag) const;
    
    static SpotLight* CreateFlashlight(const glm::vec3& pos, const glm::vec3& dir);
};

// Area light (rectangular, soft lighting)
class AreaLight : public Light {
public:
    AreaLight();
    ~AreaLight() = default;

    glm::vec3 position = glm::vec3(0.0f);     // Center position
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);  // Surface normal
    glm::vec3 tangent = glm::vec3(1.0f, 0.0f, 0.0f); // Tangent (width direction)
    
    float width = 2.0f;                       // Width of the light panel
    float height = 2.0f;                      // Height of the light panel
    
    // Two-sided emission
    bool twoSided = false;
    
    static AreaLight* CreatePanel(const glm::vec3& pos, const glm::vec3& normal, float width, float height);
};

// Ambient light (global illumination approximation)
class AmbientLight : public Light {
public:
    AmbientLight();
    ~AmbientLight() = default;

    // Ambient light is typically just color * intensity applied globally
    // No position or direction needed
    
    static AmbientLight* CreateDefault(const glm::vec3& color = glm::vec3(0.1f), float intensity = 1.0f);
};

} // namespace kcShaders