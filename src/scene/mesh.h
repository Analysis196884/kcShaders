#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace kcShaders {

// ================= Vertex =================
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

// ================= Mesh =================
// Represents a GPU-resident triangle mesh.
// - Owns CPU-side vertex/index data (optional after upload)
// - Owns GPU buffers (VAO/VBO/EBO)
// - Does NOT store transform or material (which is scene-level responsibility)
class Mesh {
public:
    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh();

    // non-copyable (GPU resource)
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // movable
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // data setup
    void setVertices(const std::vector<Vertex>& vertices);
    void setIndices(const std::vector<uint32_t>& indices);
    void setName(const std::string& name) { this->name_ = name; }
    void setFaceCount(uint32_t count) { this->face_count_ = count; }

    // Compute tangent and bitangent vectors for normal mapping
    void computeTangents();

    // GPU upload
    void upload();

    // draw call
    void draw() const;

    // query
    bool isUploaded() const { return uploaded; }
    const std::vector<Vertex>& GetVertices() const { return vertices; }
    const std::vector<uint32_t>& GetIndices() const { return indices; }
    const Vertex& GetVertex(size_t index) const { return vertices[index]; }
    uint32_t indexCount() const { return static_cast<uint32_t>(indices.size()); }
    uint32_t GetIndexCount() const { return indexCount(); }
    uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
    uint32_t GetFaceCount() const { return face_count_; }
    std::string name() const { return this->name_; };

private:
    void releaseGPU();

private:
    // CPU-side data
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::string name_ = "Unnamed Mesh";
    uint32_t face_count_ = 0;  // Original face count before triangulation

    // GPU-side objects
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    bool uploaded = false;
};

} // namespace kcShaders