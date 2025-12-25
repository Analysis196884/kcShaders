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

    // GPU upload
    void upload();

    // draw call
    void draw() const;

    // query
    bool isUploaded() const { return uploaded; }
    uint32_t indexCount() const { return static_cast<uint32_t>(indices.size()); }
    uint32_t GetIndexCount() const { return indexCount(); }
    uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
    std::string name() const { return this->name_; };

private:
    void releaseGPU();

private:
    // CPU-side data
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::string name_ = "Unnamed Mesh";

    // GPU-side objects
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    bool uploaded = false;
};

} // namespace kcShaders