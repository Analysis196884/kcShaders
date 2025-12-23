#pragma once

#include "scene.h"
#include "geometry.h"
#include "mesh.h"
#include "material.h"
#include "light.h"

namespace kcShaders {

// Create demo scene with simple objects: a cube above a plane
inline Scene* create_demo_scene()
{
    Scene* scene = new Scene();
    
    // Create a node with a cube mesh (gold metal material)
    SceneNode* cube_root = scene->createRoot();
    cube_root->mesh = create_cube(2.0f);
    cube_root->material = Material::CreateMetal(glm::vec3(1.0f, 0.84f, 0.0f), 0.2f); // Gold
    cube_root->material->name = "Gold Metal";

    // Create a root node with a plane mesh (rough plastic material)
    SceneNode* plane_node = scene->createRoot();
    plane_node->mesh = create_plane(10.0f, 10.0f, 10, 10);
    plane_node->transform.position = glm::vec3(0.0f, 0.0f, -1.0f);
    plane_node->material = Material::CreatePlastic(glm::vec3(0.3f, 0.5f, 0.8f), 0.7f); // Blue plastic
    plane_node->material->name = "Blue Plastic";
    
    // Add lights to the scene
    // Main directional light (sun)
    DirectionalLight* sunlight = DirectionalLight::CreateSunlight(glm::vec3(-0.3f, -1.0f, -0.5f));
    sunlight->intensity = 0.8f;
    scene->addLight(sunlight);
    
    // Key point light (warm, above the scene)
    PointLight* keyLight = PointLight::CreateBulb(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(1.0f, 0.9f, 0.7f), 15.0f);
    keyLight->name = "Key Light";
    keyLight->intensity = 2.0f;
    scene->addLight(keyLight);
    
    // Fill light (cool, side)
    PointLight* fillLight = PointLight::CreateBulb(glm::vec3(-2.0f, 2.0f, 2.0f), glm::vec3(0.7f, 0.8f, 1.0f), 12.0f);
    fillLight->name = "Fill Light";
    fillLight->intensity = 1.0f;
    scene->addLight(fillLight);
    
    // Ambient light for base illumination
    AmbientLight* ambient = AmbientLight::CreateDefault(glm::vec3(0.15f, 0.15f, 0.2f), 0.3f);
    scene->addLight(ambient);
    
    return scene;
}

} // namespace kcShaders