#include "scene.h"
#include "mesh.h"
#include "material.h"

#include <glm/gtc/matrix_transform.hpp>

namespace kcShaders {

glm::mat4 Transform::localMatrix() const 
{
    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 R = glm::mat4_cast(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
    return T * R * S;
}

// ================= SceneNode =================

SceneNode::~SceneNode()
{
    // Clean up mesh if owned
    if (mesh) {
        delete mesh;
        mesh = nullptr;
    }
    
    // Clean up material if owned
    if (material) {
        delete material;
        material = nullptr;
    }
}

SceneNode* SceneNode::createChild() 
{
    children.emplace_back(std::make_unique<SceneNode>());
    SceneNode* child = children.back().get();
    child->parent = this;
    return child;
}


glm::mat4 SceneNode::worldMatrix() const 
{
    if (parent) {
        return parent->worldMatrix() * transform.localMatrix();
    }
    return transform.localMatrix();
}


void SceneNode::collectRenderItems(std::vector<RenderItem>& out) const 
{
    if (mesh) 
    {
        RenderItem item;
        item.mesh = mesh;
        item.material = material;
        item.modelMatrix = worldMatrix();
        out.push_back(item);
    }

    for (const auto& c : children) {
        c->collectRenderItems(out);
    }
}

// ================= Scene =================

Scene::~Scene()
{
    // unique_ptr will automatically clean up SceneNodes
    // SceneNode destructor will clean up meshes and materials
}

SceneNode* Scene::createRoot() 
{
    roots.emplace_back(std::make_unique<SceneNode>());
    return roots.back().get();
}


void Scene::collectRenderItems(std::vector<RenderItem>& out) const 
{
    for (const auto& r : roots) {
        r->collectRenderItems(out);
    }
}


} // namespace kcShaders