#include "usd_loader.h"

#ifdef ENABLE_USD_SUPPORT

#include "scene/scene.h"
#include "scene/mesh.h"
#include "scene/material.h"
#include "scene/light.h"

// Define this before including USD headers to avoid Windows.h conflicts
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Disable Python bindings to avoid linking to boost::python
#define PXR_PYTHON_SUPPORT_ENABLED 0

// USD includes
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdLux/lightAPI.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/sphereLight.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3f.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtx/matrix_decompose.hpp>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace kcShaders {

UsdLoader::UsdLoader() {}

UsdLoader::~UsdLoader() {}

bool UsdLoader::LoadFromFile(const std::string& filepath, Scene* outScene) {
    if (!outScene) {
        last_error_ = "Output scene is null";
        return false;
    }

    // Open the USD stage
    UsdStageRefPtr stage = UsdStage::Open(filepath);
    if (!stage) {
        last_error_ = "Failed to open USD file: " + filepath;
        return false;
    }

    std::cout << "Successfully opened USD stage: " << filepath << std::endl;

    // Get the root prim
    UsdPrim rootPrim = stage->GetPseudoRoot();
    if (!rootPrim.IsValid()) {
        last_error_ = "Invalid root prim in USD file";
        return false;
    }

    // Traverse the stage and process prims
    for (const UsdPrim& child : rootPrim.GetChildren()) {
        if (!child.IsValid()) continue;
        
        // Create a root scene node for this prim
        auto rootNode = std::make_unique<SceneNode>();
        rootNode->name = child.GetName().GetString();
        
        // Process this prim and its children
        ProcessPrim(const_cast<UsdPrim*>(&child), rootNode.get(), outScene);
        
        // Add to scene
        outScene->roots.push_back(std::move(rootNode));
    }

    std::cout << "USD file loaded successfully" << std::endl;
    return true;
}

bool UsdLoader::ProcessPrim(void* primPtr, SceneNode* parentNode, Scene* scene) {
    UsdPrim* prim = static_cast<UsdPrim*>(primPtr);
    if (!prim || !prim->IsValid()) return false;

    std::cout << "Processing prim: " << prim->GetPath().GetString() << 
                 " (Type: " << prim->GetTypeName() << ")" << std::endl;

    // Handle transform
    if (UsdGeomXformable xformable = UsdGeomXformable(*prim)) {
        GfMatrix4d transform;
        bool resetsXformStack;
        xformable.GetLocalTransformation(&transform, &resetsXformStack);
        
        // Convert USD matrix to GLM
        glm::mat4 glmTransform;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                glmTransform[j][i] = static_cast<float>(transform[i][j]);
            }
        }
        
        // Extract position, rotation, scale from matrix
        glm::vec3 position, scale, skew;
        glm::vec4 perspective;
        glm::quat rotation;
        glm::decompose(glmTransform, scale, rotation, position, skew, perspective);
        
        parentNode->transform.position = position;
        parentNode->transform.scale = scale;
        parentNode->transform.rotation = glm::degrees(glm::eulerAngles(rotation));
    }

    // Handle mesh geometry
    if (prim->IsA<UsdGeomMesh>()) {
        UsdGeomMesh mesh(*prim);
        ProcessMesh(&mesh, parentNode);
    }

    // Handle lights
    if (prim->IsA<UsdLuxDistantLight>() || prim->IsA<UsdLuxSphereLight>() || prim->IsA<UsdLuxRectLight>()) {
        ProcessLight(prim, scene);
    }

    // Process children
    for (const UsdPrim& child : prim->GetChildren()) {
        if (!child.IsValid()) continue;
        
        auto childNode = std::make_unique<SceneNode>();
        childNode->name = child.GetName().GetString();
        childNode->parent = parentNode;
        
        ProcessPrim(const_cast<UsdPrim*>(&child), childNode.get(), scene);
        
        parentNode->children.push_back(std::move(childNode));
    }

    return true;
}

bool UsdLoader::ProcessMesh(void* meshPtr, SceneNode* node) {
    UsdGeomMesh* usdMesh = static_cast<UsdGeomMesh*>(meshPtr);
    if (!usdMesh) return false;

    // Get vertices
    VtArray<GfVec3f> points;
    usdMesh->GetPointsAttr().Get(&points);
    
    // Get face vertex indices
    VtArray<int> faceVertexIndices;
    usdMesh->GetFaceVertexIndicesAttr().Get(&faceVertexIndices);
    
    // Get face vertex counts
    VtArray<int> faceVertexCounts;
    usdMesh->GetFaceVertexCountsAttr().Get(&faceVertexCounts);

    std::cout << "  Mesh has " << points.size() << " vertices, " 
              << faceVertexCounts.size() << " faces" << std::endl;

    // Convert to our mesh format
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Get normals if available
    VtArray<GfVec3f> normals;
    bool hasNormals = false;
    UsdAttribute normalsAttr = usdMesh->GetNormalsAttr();
    if (normalsAttr) {
        hasNormals = normalsAttr.Get(&normals);
    }

    // Build vertex data
    for (size_t i = 0; i < points.size(); i++) {
        Vertex v;
        
        // Position
        v.position = glm::vec3(points[i][0], points[i][1], points[i][2]);
        
        // Normal (use existing or default)
        if (hasNormals && i < normals.size()) {
            v.normal = glm::vec3(normals[i][0], normals[i][1], normals[i][2]);
        } else {
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        
        // Texture coordinates (placeholder)
        v.uv = glm::vec2(0.0f, 0.0f);
        
        vertices.push_back(v);
    }

    // Build indices - triangulate if needed
    size_t faceStart = 0;
    for (size_t faceIdx = 0; faceIdx < faceVertexCounts.size(); faceIdx++) {
        int vertCount = faceVertexCounts[faceIdx];
        
        if (vertCount == 3) {
            // Triangle - add directly
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 0]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 1]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 2]));
        } else if (vertCount == 4) {
            // Quad - triangulate
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 0]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 1]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 2]));
            
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 0]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 2]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 3]));
        } else {
            // N-gon - simple fan triangulation
            for (int i = 1; i < vertCount - 1; i++) {
                indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 0]));
                indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + i]));
                indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + i + 1]));
            }
        }
        
        faceStart += vertCount;
    }

    // Create mesh object
    node->mesh = new Mesh(vertices, indices);

    // Create default material if none exists
    if (!node->material) {
        node->material = Material::CreatePlastic(glm::vec3(0.8f, 0.8f, 0.8f));
    }

    return true;
}

bool UsdLoader::ProcessLight(void* lightPtr, Scene* scene) {
    UsdPrim* prim = static_cast<UsdPrim*>(lightPtr);
    if (!prim || !prim->IsValid()) return false;

    // Get light properties using LightAPI
    UsdLuxLightAPI lightAPI(*prim);
    
    GfVec3f colorAttr(1.0f);
    float intensity = 1.0f;
    
    if (UsdAttribute attr = lightAPI.GetColorAttr()) {
        attr.Get(&colorAttr);
    }
    if (UsdAttribute attr = lightAPI.GetIntensityAttr()) {
        attr.Get(&intensity);
    }

    glm::vec3 color(colorAttr[0], colorAttr[1], colorAttr[2]);

    // Get transform for position
    UsdGeomXformable xformable(*prim);
    GfMatrix4d transform;
    bool resetsXformStack;
    xformable.GetLocalTransformation(&transform, &resetsXformStack);
    
    glm::vec3 position(
        static_cast<float>(transform[3][0]),
        static_cast<float>(transform[3][1]),
        static_cast<float>(transform[3][2])
    );

    Light* light = nullptr;

    // Create appropriate light type
    if (prim->IsA<UsdLuxDistantLight>()) {
        // Directional light
        glm::vec3 direction(0.0f, -1.0f, 0.0f);  // Default direction
        
        // USD distant lights point in -Z direction by default
        // Manually convert GfMatrix4d to glm::mat4
        glm::mat4 glmTransform;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                glmTransform[j][i] = static_cast<float>(transform[i][j]);
            }
        }
        glm::vec4 dir4 = glmTransform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
        direction = glm::normalize(glm::vec3(dir4));
        
        light = DirectionalLight::CreateSunlight(direction);
        std::cout << "  Created directional light" << std::endl;
    }
    else if (prim->IsA<UsdLuxSphereLight>()) {
        // Point light
        UsdLuxSphereLight sphereLight(*prim);
        float radius = 10.0f;
        if (UsdAttribute attr = sphereLight.GetRadiusAttr()) {
            attr.Get(&radius);
        }
        
        light = PointLight::CreateBulb(position, color, radius);
        std::cout << "  Created point light at (" << position.x << ", " 
                  << position.y << ", " << position.z << ")" << std::endl;
    }
    else if (prim->IsA<UsdLuxRectLight>()) {
        // Area light
        UsdLuxRectLight rectLight(*prim);
        float width = 1.0f, height = 1.0f;
        
        if (UsdAttribute attr = rectLight.GetWidthAttr()) {
            attr.Get(&width);
        }
        if (UsdAttribute attr = rectLight.GetHeightAttr()) {
            attr.Get(&height);
        }
        
        glm::vec3 normal(0.0f, 0.0f, 1.0f);
        // Manually convert GfMatrix4d to glm::mat4
        glm::mat4 glmTransform;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                glmTransform[j][i] = static_cast<float>(transform[i][j]);
            }
        }
        glm::vec4 n4 = glmTransform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        normal = glm::normalize(glm::vec3(n4));
        
        light = AreaLight::CreatePanel(position, normal, width, height, color, intensity);
        std::cout << "  Created area light" << std::endl;
    }

    if (light) {
        light->name = prim->GetName().GetString();
        scene->addLight(light);
        return true;
    }

    return false;
}

bool UsdLoader::ProcessMaterial(void* materialPtr, SceneNode* node) {
    // Material processing can be expanded later
    // For now, USD materials are complex and would require shader network traversal
    return true;
}

} // namespace kcShaders

#endif // ENABLE_USD_SUPPORT
