#include "BVH.h"
#include <algorithm>
#include <iostream>

namespace kcShaders {

void BVHBuilder::build(const std::vector<GpuVertex>& vertices, 
                       const std::vector<GpuTriangle>& triangles)
{
    std::cout << "[BVH] Building BVH for " << triangles.size() << " triangles...\n";
    
    if (triangles.empty()) {
        std::cerr << "[BVH] No triangles to build BVH\n";
        return;
    }
    
    // Prepare triangle centroids
    triangleCentroids_.resize(triangles.size());
    triangleIndices_.resize(triangles.size());
    
    for (size_t i = 0; i < triangles.size(); i++) {
        const auto& tri = triangles[i];
        const auto& v0 = vertices[tri.v0].position;
        const auto& v1 = vertices[tri.v1].position;
        const auto& v2 = vertices[tri.v2].position;
        
        triangleCentroids_[i].index = static_cast<uint32_t>(i);
        triangleCentroids_[i].centroid = (v0 + v1 + v2) / 3.0f;
        
        triangleCentroids_[i].bounds.grow(v0);
        triangleCentroids_[i].bounds.grow(v1);
        triangleCentroids_[i].bounds.grow(v2);
        
        triangleIndices_[i] = static_cast<uint32_t>(i);
    }
    
    // Create root node
    nodes_.clear();
    nodes_.reserve(triangles.size() * 2);
    
    BVHNode root;
    root.leftFirst = 0;
    root.triCount = static_cast<uint32_t>(triangles.size());
    
    // Compute root bounds
    for (const auto& tc : triangleCentroids_) {
        root.boundsMin = glm::min(root.boundsMin, tc.bounds.min);
        root.boundsMax = glm::max(root.boundsMax, tc.bounds.max);
    }
    
    nodes_.push_back(root);
    
    // Recursively subdivide
    subdivide(0, vertices, triangles);
    
    std::cout << "[BVH] Built BVH with " << nodes_.size() << " nodes\n";
}

void BVHBuilder::subdivide(uint32_t nodeIdx, 
                           const std::vector<GpuVertex>& vertices,
                           const std::vector<GpuTriangle>& triangles)
{
    BVHNode& node = nodes_[nodeIdx];
    
    // Stop if too few triangles
    if (node.triCount <= 2) {
        return;
    }
    
    // Find best split
    int axis;
    float splitPos;
    float cost = findBestSplitPlane(nodeIdx, axis, splitPos);
    
    // Stop if no good split found
    float noSplitCost = node.triCount;
    if (cost >= noSplitCost) {
        return;
    }
    
    // Partition triangles
    uint32_t i = node.leftFirst;
    uint32_t j = i + node.triCount - 1;
    
    while (i <= j) {
        if (triangleCentroids_[triangleIndices_[i]].centroid[axis] < splitPos) {
            i++;
        } else {
            std::swap(triangleIndices_[i], triangleIndices_[j]);
            j--;
        }
    }
    
    // Create child nodes
    uint32_t leftCount = i - node.leftFirst;
    if (leftCount == 0 || leftCount == node.triCount) {
        return;  // Failed to split
    }
    
    // Create left child
    uint32_t leftChildIdx = static_cast<uint32_t>(nodes_.size());
    BVHNode leftChild;
    leftChild.leftFirst = node.leftFirst;
    leftChild.triCount = leftCount;
    leftChild.boundsMin = glm::vec3(1e30f);
    leftChild.boundsMax = glm::vec3(-1e30f);
    
    for (uint32_t k = 0; k < leftCount; k++) {
        uint32_t triIdx = triangleIndices_[leftChild.leftFirst + k];
        leftChild.boundsMin = glm::min(leftChild.boundsMin, triangleCentroids_[triIdx].bounds.min);
        leftChild.boundsMax = glm::max(leftChild.boundsMax, triangleCentroids_[triIdx].bounds.max);
    }
    
    // Create right child
    uint32_t rightChildIdx = leftChildIdx + 1;
    BVHNode rightChild;
    rightChild.leftFirst = i;
    rightChild.triCount = node.triCount - leftCount;
    rightChild.boundsMin = glm::vec3(1e30f);
    rightChild.boundsMax = glm::vec3(-1e30f);
    
    for (uint32_t k = 0; k < rightChild.triCount; k++) {
        uint32_t triIdx = triangleIndices_[rightChild.leftFirst + k];
        rightChild.boundsMin = glm::min(rightChild.boundsMin, triangleCentroids_[triIdx].bounds.min);
        rightChild.boundsMax = glm::max(rightChild.boundsMax, triangleCentroids_[triIdx].bounds.max);
    }
    
    // Update parent to be internal node
    node.leftFirst = leftChildIdx;
    node.triCount = 0;
    
    // Add children
    nodes_.push_back(leftChild);
    nodes_.push_back(rightChild);
    
    // Recursively subdivide children
    subdivide(leftChildIdx, vertices, triangles);
    subdivide(rightChildIdx, vertices, triangles);
}

float BVHBuilder::findBestSplitPlane(uint32_t nodeIdx, int& bestAxis, float& bestPos)
{
    const BVHNode& node = nodes_[nodeIdx];
    float bestCost = 1e30f;
    
    for (int axis = 0; axis < 3; axis++) {
        // Try several candidate positions
        for (int i = 1; i < 8; i++) {
            float candidatePos = node.boundsMin[axis] + 
                (node.boundsMax[axis] - node.boundsMin[axis]) * (i / 8.0f);
            
            // Calculate cost
            AABB leftBox, rightBox;
            int leftCount = 0, rightCount = 0;
            
            for (uint32_t j = 0; j < node.triCount; j++) {
                uint32_t triIdx = triangleIndices_[node.leftFirst + j];
                const auto& tc = triangleCentroids_[triIdx];
                
                if (tc.centroid[axis] < candidatePos) {
                    leftCount++;
                    leftBox.grow(tc.bounds);
                } else {
                    rightCount++;
                    rightBox.grow(tc.bounds);
                }
            }
            
            float cost = leftCount * leftBox.area() + rightCount * rightBox.area();
            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                bestPos = candidatePos;
            }
        }
    }
    
    return bestCost;
}

} // namespace kcShaders
