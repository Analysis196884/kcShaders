#pragma once 

#include <string>
#include <vector>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace kcShaders {

// Forward declarations
class Mesh;
class Material;

// ================= Transform =================
struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 localMatrix() const;
};


// ================= RenderItem =================
struct RenderItem {
    Mesh* mesh = nullptr;
    Material* material = nullptr;
    glm::mat4 modelMatrix{1.0f};
};


// ================= SceneNode =================
class SceneNode {
public:
    Transform transform;

    Mesh* mesh = nullptr; // nullptr if not renderable
    Material* material = nullptr; // nullptr allowed (use default)

    SceneNode* parent = nullptr;
    std::vector<std::unique_ptr<SceneNode>> children;

    // hierarchy
    SceneNode* createChild();

    // transforms
    glm::mat4 worldMatrix() const;

    // traversal
    void collectRenderItems(std::vector<RenderItem>& out) const;
};

// ================= Scene =================
class Scene {
public:
    std::vector<std::unique_ptr<SceneNode>> roots;

    SceneNode* createRoot();

    void collectRenderItems(std::vector<RenderItem>& out) const;
};

} // namespace kcShaders