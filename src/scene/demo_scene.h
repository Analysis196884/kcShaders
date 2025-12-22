#pragma once

#include "scene.h"
#include "geometry.h"
#include "mesh.h"

namespace kcShaders {

// Create demo scene with simple objects: a cube above a plane
inline Scene* create_demo_scene()
{
    Scene* scene = new Scene();
    
    // Create a node with a cube mesh
    SceneNode* cube_root = scene->createRoot();
    cube_root->mesh = create_cube(1.0f);

    // Create a root node with a plane mesh
    SceneNode* plane_node = scene->createRoot();
    plane_node->mesh = create_plane(10.0f, 10.0f, 10, 10);
    plane_node->transform.position = glm::vec3(0.0f, -1.0f, 0.0f);
    
    return scene;
}

} // namespace kcShaders