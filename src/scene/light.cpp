#include "light.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>

namespace kcShaders {

// ================= Base Light =================

Light::Light(LightType type)
    : type_(type)
    , color(1.0f)
    , intensity(1.0f)
    , enabled(true)
    , castShadows(true)
    , name("Light")
{
}

// ================= Directional Light =================

DirectionalLight::DirectionalLight()
    : Light(LightType::Directional)
    , direction(0.0f, -1.0f, 0.0f)
    , shadowDistance(100.0f)
    , shadowBias(0.005f)
{
    name = "Directional Light";
}

DirectionalLight* DirectionalLight::CreateSunlight(const glm::vec3& dir, const glm::vec3& color, float intensity)
{
    DirectionalLight* light = new DirectionalLight();
    light->name = "Sunlight";
    light->direction = glm::normalize(dir);
    light->color = color;
    light->intensity = intensity;
    return light;
}

// ================= Point Light =================

PointLight::PointLight()
    : Light(LightType::Point)
    , position(0.0f)
    , constant(1.0f)
    , linear(0.09f)
    , quadratic(0.032f)
    , radius(10.0f)
{
    name = "Point Light";
}

float PointLight::GetAttenuation(float distance) const
{
    if (distance > radius) {
        return 0.0f;
    }
    return 1.0f / (constant + linear * distance + quadratic * distance * distance);
}

PointLight* PointLight::CreateBulb(const glm::vec3& pos, const glm::vec3& color, float radius, float intensity)
{
    PointLight* light = new PointLight();
    light->name = "Bulb Light";
    light->position = pos;
    light->color = color;
    light->radius = radius;
    light->intensity = intensity;
    
    // Adjust attenuation based on radius
    light->constant = 1.0f;
    light->linear = 0.09f;
    light->quadratic = 0.032f;
    
    return light;
}

// ================= Spot Light =================

SpotLight::SpotLight()
    : Light(LightType::Spot)
    , position(0.0f)
    , direction(0.0f, -1.0f, 0.0f)
    , innerConeAngle(12.5f)
    , outerConeAngle(17.5f)
    , constant(1.0f)
    , linear(0.09f)
    , quadratic(0.032f)
    , range(20.0f)
{
    name = "Spot Light";
}

float SpotLight::GetAttenuation(float distance) const
{
    if (distance > range) {
        return 0.0f;
    }
    return 1.0f / (constant + linear * distance + quadratic * distance * distance);
}

float SpotLight::GetSpotFalloff(const glm::vec3& lightToFrag) const
{
    float theta = glm::dot(glm::normalize(lightToFrag), glm::normalize(direction));
    float epsilon = glm::cos(glm::radians(innerConeAngle)) - glm::cos(glm::radians(outerConeAngle));
    float spotEffect = glm::clamp((theta - glm::cos(glm::radians(outerConeAngle))) / epsilon, 0.0f, 1.0f);
    return spotEffect;
}

SpotLight* SpotLight::CreateFlashlight(const glm::vec3& pos, const glm::vec3& dir)
{
    SpotLight* light = new SpotLight();
    light->name = "Flashlight";
    light->position = pos;
    light->direction = glm::normalize(dir);
    light->innerConeAngle = 12.5f;
    light->outerConeAngle = 17.5f;
    light->range = 25.0f;
    light->intensity = 1.5f;
    return light;
}

// ================= Area Light =================

AreaLight::AreaLight()
    : Light(LightType::Area)
    , position(0.0f)
    , normal(0.0f, 0.0f, 1.0f)
    , tangent(1.0f, 0.0f, 0.0f)
    , width(2.0f)
    , height(2.0f)
    , twoSided(false)
{
    name = "Area Light";
}

AreaLight* AreaLight::CreatePanel(
    const glm::vec3& pos, const glm::vec3& normal,
    float width, float height, const glm::vec3& color, float intensity
) {
    AreaLight* light = new AreaLight();
    light->name = "Panel Light";
    light->position = pos;
    light->normal = glm::normalize(normal);
    light->width = width;
    light->height = height;
    light->intensity = intensity;
    light->color = color;
    
    // Calculate tangent perpendicular to normal
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(normal, up)) > 0.99f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    light->tangent = glm::normalize(glm::cross(up, normal));
    
    return light;
}

// ================= Ambient Light =================

AmbientLight::AmbientLight()
    : Light(LightType::Ambient)
{
    name = "Ambient Light";
    castShadows = false;  // Ambient light doesn't cast shadows
}

AmbientLight* AmbientLight::CreateDefault(const glm::vec3& color, float intensity)
{
    AmbientLight* light = new AmbientLight();
    light->name = "Ambient Light";
    light->color = color;
    light->intensity = intensity;
    return light;
}

} // namespace kcShaders