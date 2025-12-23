#pragma once

#include "scene.h"
#include "geometry.h"
#include "mesh.h"
#include "material.h"

namespace kcShaders {

// Create demo scene with simple objects: a cube above a plane
inline Scene* create_demo_scene()
{
    Scene* scene = new Scene();
    
    // Create a node with a cube mesh (gold metal material)
    SceneNode* cube_root = scene->createRoot();
    cube_root->mesh = create_cube(1.0f);
    cube_root->material = Material::CreateMetal(glm::vec3(1.0f, 0.84f, 0.0f), 0.2f); // Gold
    cube_root->material->name = "Gold Metal";

    // Create a root node with a plane mesh (rough plastic material)
    SceneNode* plane_node = scene->createRoot();
    plane_node->mesh = create_plane(10.0f, 10.0f, 10, 10);
    plane_node->transform.position = glm::vec3(0.0f, -1.0f, 0.0f);
    plane_node->material = Material::CreatePlastic(glm::vec3(0.3f, 0.5f, 0.8f), 0.7f); // Blue plastic
    plane_node->material->name = "Blue Plastic";
    
    return scene;
}

} // namespace kcShaders