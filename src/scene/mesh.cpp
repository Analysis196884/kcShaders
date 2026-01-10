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