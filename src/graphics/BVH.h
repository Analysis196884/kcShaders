#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace kcShaders {

// GPU-friendly vertex data (std430 layout compatible)
// Note: In std430, vec3 is 12 bytes but the next member aligns to 16 bytes
struct GpuVertex {
    glm::vec3 position;
    float _pad0;           // Padding to align normal to 16 bytes
    glm::vec3 normal;
    float _pad1;           // Padding to align uv to 16 bytes  
    glm::vec2 uv;
    float _pad2[2];        // Padding to align struct size to 16 bytes
};

// GPU-friendly triangle data
struct GpuTriangle {
    uint32_t v0, v1, v2;  // Vertex indices
    uint32_t materialId;   // Material index
};

// GPU-friendly material data (std430 layout compatible)
struct GpuMaterial {
    glm::vec3 albedo;
    float metallic;
    glm::vec3 emissive;
    float roughness;
    float ao;
    float opacity;
    float emissiveStrength;
    float _pad0;  // Padding for alignment
};

// GPU-friendly BVH node (std430 layout compatible)
struct BVHNode {
    glm::vec3 boundsMin;
    uint32_t leftFirst;    // Left child index (internal) or first triangle (leaf)
    glm::vec3 boundsMax;
    uint32_t triCount;     // 0 = internal node, >0 = leaf node with triangle count
};

// AABB for BVH construction
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB() : min(1e30f), max(-1e30f) {}
    
    void grow(const glm::vec3& p) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }
    
    void grow(const AABB& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }
    
    float area() const {
        glm::vec3 e = max - min;
        return e.x * e.y + e.y * e.z + e.z * e.x;
    }
    
    glm::vec3 center() const {
        return (min + max) * 0.5f;
    }
};

// Triangle with centroid for BVH construction
struct TriangleCentroid {
    uint32_t index;
    glm::vec3 centroid;
    AABB bounds;
};

// BVH Builder
class BVHBuilder {
public:
    BVHBuilder() = default;
    
    // Build BVH from triangles
    void build(const std::vector<GpuVertex>& vertices, 
               const std::vector<GpuTriangle>& triangles);
    
    // Get built BVH nodes
    const std::vector<BVHNode>& getNodes() const { return nodes_; }
    
    // Get triangle indices (reordered for better cache coherency)
    const std::vector<uint32_t>& getTriangleIndices() const { return triangleIndices_; }
    
private:
    void subdivide(uint32_t nodeIdx, 
                   const std::vector<GpuVertex>& vertices,
                   const std::vector<GpuTriangle>& triangles);
    
    float findBestSplitPlane(uint32_t nodeIdx, int& axis, float& splitPos);
    
    std::vector<BVHNode> nodes_;
    std::vector<TriangleCentroid> triangleCentroids_;
    std::vector<uint32_t> triangleIndices_;
};

} // namespace kcShaders
