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

// USD includes
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdLux/lightAPI.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/sphereLight.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3f.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <iostream>
#include <fstream>
#include <cstdlib>

PXR_NAMESPACE_USING_DIRECTIVE

namespace kcShaders {

UsdLoader::UsdLoader() {
    // 
}

UsdLoader::~UsdLoader() {}

bool UsdLoader::LoadFromFile(const std::string& filepath, Scene* outScene) {
    if (!outScene) {
        last_error_ = "Output scene is null";
        return false;
    }

    // Check if file exists
    std::ifstream testFile(filepath);
    if (!testFile.good()) {
        last_error_ = "File does not exist or cannot be accessed: " + filepath;
        std::cerr << "  Error: Cannot access file" << std::endl;
        return false;
    }
    testFile.close();

    // Normalize path separators for USD (use forward slashes)
    std::string normalizedPath = filepath;
    for (char& c : normalizedPath) {
        if (c == '\\') {
            c = '/';
        }
    }
    
    // Open the USD stage
    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(normalizedPath);
    
    if (!stage) {
        last_error_ = "Failed to open USD stage: " + filepath;
        std::cerr << "  UsdStage::Open failed" << std::endl;
        return false;
    }

    // Get the root prim
    pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
    if (!rootPrim.IsValid()) {
        last_error_ = "Invalid root prim in USD file";
        return false;
    }

    // Traverse the stage and process prims
    for (const pxr::UsdPrim& child : rootPrim.GetChildren()) {
        if (!child.IsValid()) continue;
        
        // Create a root scene node for this prim
        auto rootNode = std::make_unique<SceneNode>();
        rootNode->name = child.GetName().GetString();
        
        // Process this prim and its children
        ProcessPrim(const_cast<pxr::UsdPrim*>(&child), rootNode.get(), outScene);
        
        // Add to scene
        outScene->roots.push_back(std::move(rootNode));
    }

    std::cout << "USD file loaded successfully" << std::endl;
    return true;
}

bool UsdLoader::ProcessPrim(void* primPtr, SceneNode* parentNode, Scene* scene) {
    pxr::UsdPrim* prim = static_cast<pxr::UsdPrim*>(primPtr);
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
                glmTransform[i][j] = static_cast<float>(transform[i][j]);
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

    // Handle mesh geometry - attach directly to this node
    if (prim->IsA<UsdGeomMesh>()) {
        UsdGeomMesh mesh(*prim);
        ProcessMesh(&mesh, parentNode);
    }

    // Handle lights
    if (prim->IsA<UsdLuxDistantLight>() || prim->IsA<UsdLuxSphereLight>() || prim->IsA<UsdLuxRectLight>()) {
        ProcessLight(prim, scene);
    }

    // Process children
    for (const pxr::UsdPrim& child : prim->GetChildren()) {
        if (!child.IsValid()) continue;
        
        auto childNode = std::make_unique<SceneNode>();
        childNode->name = child.GetName().GetString();
        childNode->parent = parentNode;
        
        ProcessPrim(const_cast<pxr::UsdPrim*>(&child), childNode.get(), scene);
        
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
    
    // Check if mesh has valid geometry
    if (points.empty()) {
        std::cout << "  Warning: Mesh has no vertices" << std::endl;
        return false;
    }
    
    // Get face vertex indices
    VtArray<int> faceVertexIndices;
    usdMesh->GetFaceVertexIndicesAttr().Get(&faceVertexIndices);
    
    // Get face vertex counts
    VtArray<int> faceVertexCounts;
    usdMesh->GetFaceVertexCountsAttr().Get(&faceVertexCounts);

    // Convert to our mesh format
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Get texture coordinates if available
    VtArray<GfVec2f> texCoords;
    bool hasTexCoords = false;
    UsdGeomPrimvar texCoordsPrimvar = UsdGeomPrimvarsAPI(usdMesh->GetPrim()).GetPrimvar(TfToken("st"));
    if (texCoordsPrimvar) {
        hasTexCoords = texCoordsPrimvar.Get(&texCoords) && !texCoords.empty();
    }

    // Build vertex data - initialize with default values, normals will be computed from faces
    for (size_t i = 0; i < points.size(); i++) 
    {
        Vertex v;
        
        // Position
        v.position = glm::vec3(points[i][0], points[i][1], points[i][2]);
        
        // Normal will be computed from face geometry
        v.normal = glm::vec3(0.0f); // Will accumulate face normals
        
        // Texture coordinates
        if (hasTexCoords && i < texCoords.size()) {
            v.uv = glm::vec2(texCoords[i][0], texCoords[i][1]);
        } else {
            v.uv = glm::vec2(0.0f, 0.0f);
        }
        
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
            // Quad - triangulate into 2 triangles
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 0]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 1]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 2]));
            
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 0]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 2]));
            indices.push_back(static_cast<uint32_t>(faceVertexIndices[faceStart + 3]));
        } else {
            // N-gon - fan triangulation
            uint32_t idx0 = static_cast<uint32_t>(faceVertexIndices[faceStart + 0]);
            for (int i = 1; i < vertCount - 1; i++) {
                uint32_t idx1 = static_cast<uint32_t>(faceVertexIndices[faceStart + i]);
                uint32_t idx2 = static_cast<uint32_t>(faceVertexIndices[faceStart + i + 1]);
                
                indices.push_back(idx0);
                indices.push_back(idx1);
                indices.push_back(idx2);
            }
        }
        
        faceStart += vertCount;
    }

    // Check if we have valid indices
    if (indices.empty()) {
        std::cout << "  Warning: Mesh has no indices after triangulation" << std::endl;
        return false;
    }
    
    // Reset normals
    for (auto& v : vertices) {
        v.normal = glm::vec3(0.0f);
    }
    
    // Accumulate face normals to vertices
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];
        
        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));
        
        vertices[i0].normal += faceNormal;
        vertices[i1].normal += faceNormal;
        vertices[i2].normal += faceNormal;
    }
    
    // Normalize vertex normals
    for (auto& v : vertices) {
        v.normal = glm::normalize(v.normal);
    }

    // Create mesh object
    node->mesh = new Mesh(vertices, indices);

    // Process material if attached to this mesh
    pxr::UsdPrim materialPrim = usdMesh->GetPrim();
    if (!ProcessMaterial(&materialPrim, node)) {
        // Create default material if none exists
        node->material = Material::CreatePlastic(glm::vec3(0.8f, 0.8f, 0.8f));
    }

    return true;
}

bool UsdLoader::ProcessLight(void* lightPtr, Scene* scene) {
    pxr::UsdPrim* prim = static_cast<pxr::UsdPrim*>(lightPtr);
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
    pxr::UsdGeomXformable xformable(*prim);
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
        glm::vec3 direction(0.0f, -1.0f, 0.0f);
        
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

bool UsdLoader::ProcessMaterial(void* primPtr, SceneNode* node) 
{
    pxr::UsdPrim* prim = static_cast<pxr::UsdPrim*>(primPtr);
    if (!prim || !prim->IsValid()) {
        return false;
    }

    // Try to find bound material
    UsdShadeMaterial material = UsdShadeMaterialBindingAPI(*prim).ComputeBoundMaterial();
    if (!material.GetPrim().IsValid()) {
        // No material bound, use default
        return false;
    }

    // Create a new material object with proper initialization
    Material* newMaterial = new Material();
    newMaterial->name = material.GetPrim().GetName().GetString();
    // Initialize with default PBR values
    newMaterial->albedo = glm::vec3(0.8f, 0.8f, 0.8f);
    newMaterial->metallic = 0.0f;
    newMaterial->roughness = 0.5f;
    newMaterial->ao = 1.0f;
    newMaterial->emissive = glm::vec3(0.0f);
    newMaterial->emissiveStrength = 0.0f;
    newMaterial->opacity = 1.0f;

    // Get material surface shader
    UsdShadeShader surfaceShader = material.ComputeSurfaceSource();
    if (surfaceShader.GetPrim().IsValid()) {
        // Try to extract parameters from the surface shader
        
        // Base color / Albedo - try multiple common names
        if (UsdAttribute baseColorAttr = surfaceShader.GetInput(TfToken("diffuseColor"))) {
            GfVec3f baseColor;
            // Try to get color value (not a texture)
            if (baseColorAttr.Get(&baseColor)) {
                newMaterial->albedo = glm::vec3(baseColor[0], baseColor[1], baseColor[2]);
            }
        } else if (UsdAttribute baseColorAttr = surfaceShader.GetInput(TfToken("base_color"))) {
            GfVec3f baseColor;
            if (baseColorAttr.Get(&baseColor)) {
                newMaterial->albedo = glm::vec3(baseColor[0], baseColor[1], baseColor[2]);
            }
        }

        // Metallic
        if (UsdAttribute metallicAttr = surfaceShader.GetInput(TfToken("metallic"))) {
            float metallic = 0.0f;
            if (metallicAttr.Get(&metallic)) {
                newMaterial->metallic = metallic;
            }
        }

        // Roughness
        if (UsdAttribute roughnessAttr = surfaceShader.GetInput(TfToken("roughness"))) {
            float roughness = 0.5f;
            if (roughnessAttr.Get(&roughness)) {
                newMaterial->roughness = roughness;
            }
        }

        // Ambient Occlusion
        if (UsdAttribute aoAttr = surfaceShader.GetInput(TfToken("occlusion"))) {
            float ao = 1.0f;
            if (aoAttr.Get(&ao)) {
                newMaterial->ao = ao;
            }
        }

        // Emissive color
        if (UsdAttribute emissiveAttr = surfaceShader.GetInput(TfToken("emissive"))) {
            GfVec3f emissive;
            if (emissiveAttr.Get(&emissive)) {
                newMaterial->emissive = glm::vec3(emissive[0], emissive[1], emissive[2]);
            }
        }

        // Emissive strength / intensity
        if (UsdAttribute emissiveStrengthAttr = surfaceShader.GetInput(TfToken("emissive_strength"))) {
            float strength = 0.0f;
            if (emissiveStrengthAttr.Get(&strength)) {
                newMaterial->emissiveStrength = strength;
            }
        }

        // Opacity
        if (UsdAttribute opacityAttr = surfaceShader.GetInput(TfToken("opacity"))) {
            float opacity = 1.0f;
            if (opacityAttr.Get(&opacity)) {
                newMaterial->opacity = opacity;
            }
        }
    }

    // Assign material to node
    if (node->material) {
        delete node->material;
    }
    node->material = newMaterial;
    return true;
}

} // namespace kcShaders

#endif // ENABLE_USD_SUPPORT
