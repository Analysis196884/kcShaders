#include "mesh.h"

#include <cassert>

namespace kcShaders {

// ================= constructor =================
Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : vertices(vertices), indices(indices)
{
}

// ================= destructor =================
Mesh::~Mesh() {
    releaseGPU();
}

// ================= move semantics =================
Mesh::Mesh(Mesh&& other) noexcept {
    *this = std::move(other);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept 
{
    if (this != &other) 
    {
        releaseGPU();

        vertices = std::move(other.vertices);
        indices  = std::move(other.indices);
        name_ = std::move(other.name_);
        face_count_ = other.face_count_;

        vao = other.vao;
        vbo = other.vbo;
        ebo = other.ebo;
        uploaded = other.uploaded;

        other.vao = other.vbo = other.ebo = 0;
        other.uploaded = false;
        other.face_count_ = 0;
    }
    return *this;
}

// ================= data setup =================
void Mesh::setVertices(const std::vector<Vertex>& v) {
    vertices = v;
}

void Mesh::setIndices(const std::vector<uint32_t>& i) {
    indices = i;
}

// ================= compute tangents =================
void Mesh::computeTangents()
{
    if (indices.empty() || vertices.empty()) {
        return;
    }
    
    // Initialize tangent and bitangent to zero
    for (auto& v : vertices) {
        v.tangent = glm::vec3(0.0f);
        v.bitangent = glm::vec3(0.0f);
    }
    
    // Calculate tangents and bitangents per triangle
    // Reference: https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];
        
        glm::vec3& v0 = vertices[i0].position;
        glm::vec3& v1 = vertices[i1].position;
        glm::vec3& v2 = vertices[i2].position;
        
        glm::vec2& uv0 = vertices[i0].uv;
        glm::vec2& uv1 = vertices[i1].uv;
        glm::vec2& uv2 = vertices[i2].uv;
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;
        
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        
        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        
        glm::vec3 bitangent;
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        
        // Accumulate tangents and bitangents
        vertices[i0].tangent += tangent;
        vertices[i1].tangent += tangent;
        vertices[i2].tangent += tangent;
        
        vertices[i0].bitangent += bitangent;
        vertices[i1].bitangent += bitangent;
        vertices[i2].bitangent += bitangent;
    }
    
    // Normalize and orthogonalize tangents/bitangents
    for (auto& v : vertices) {
        // Gram-Schmidt orthogonalize
        v.tangent = glm::normalize(v.tangent - glm::dot(v.tangent, v.normal) * v.normal);
        v.bitangent = glm::normalize(v.bitangent);
    }
}

// ================= GPU upload =================
void Mesh::upload() 
{
    assert(!vertices.empty());

    if (uploaded) {
        return;
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 vertices.data(),
                 GL_STATIC_DRAW);

    if (!indices.empty()) 
    {
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(uint32_t),
                     indices.data(),
                     GL_STATIC_DRAW);
    }

    // layout(location = 0) position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, position));

    // layout(location = 1) normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // layout(location = 2) uv
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, uv));

    // layout(location = 3) tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    // layout(location = 4) bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

    glBindVertexArray(0);
    uploaded = true;
}

// ================= draw =================
void Mesh::draw() const 
{
    assert(uploaded);
    glBindVertexArray(vao);
    if (!indices.empty()) 
    {
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(indices.size()),
                       GL_UNSIGNED_INT,
                       nullptr);
    } else {
        glDrawArrays(GL_TRIANGLES,
                     0,
                     static_cast<GLsizei>(vertices.size()));
    }
    glBindVertexArray(0);
}

// ================= cleanup =================
void Mesh::releaseGPU() 
{
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    ebo = vbo = vao = 0;
    uploaded = false;
}

} // namespace kcShaders