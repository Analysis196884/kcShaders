#pragma once

#ifdef ENABLE_USD_SUPPORT

#include <string>
#include <memory>
#include <vector>

// Forward declarations
namespace kcShaders {
    class Scene;
    class SceneNode;
    class TextureManager;
}

namespace kcShaders {

/**
 * @brief USD (Universal Scene Description) file loader
 * 
 * Loads USD/USDA/USDC files and converts them to the internal scene format.
 * Supports geometry, materials, transforms, and lights from USD scenes.
 */
class UsdLoader {
public:
    UsdLoader();
    ~UsdLoader();

    /**
     * @brief Load a USD file into a Scene
     * @param filepath Path to the USD/USDA/USDC file
     * @param outScene Target scene to populate
     * @return true if loading succeeded, false otherwise
     */
    bool LoadFromFile(const std::string& filepath, Scene* outScene);

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    const std::string& GetLastError() const { return last_error_; }

private:
    std::string last_error_;

    // Internal conversion methods
    bool ProcessPrim(void* prim, SceneNode* parentNode, Scene* scene);
    bool ProcessMesh(void* mesh, SceneNode* node);
    bool ProcessLight(void* light, Scene* scene);
    bool ProcessMaterial(void* material, SceneNode* node);
};

} // namespace kcShaders

#endif // ENABLE_USD_SUPPORT
