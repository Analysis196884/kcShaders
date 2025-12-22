#include "geometry.h"
#include "mesh.h"
#include <memory>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace kcShaders {

// ================= compute normals =================
void compute_normals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) 
{
    // Reset normals
    for (auto& v : vertices) {
        v.normal = glm::vec3(0.0f);
    }

    // Accumulate face normals
    for (size_t i = 0; i < indices.size(); i += 3) 
    {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 face_normal = glm::normalize(glm::cross(edge1, edge2));

        vertices[i0].normal += face_normal;
        vertices[i1].normal += face_normal;
        vertices[i2].normal += face_normal;
    }

    // Normalize vertex normals
    for (auto& v : vertices) {
        v.normal = glm::normalize(v.normal);
    }
}

// ================= Plane =================
Mesh* create_plane(float width, float height, int segments_w, int segments_h) 
{
    Mesh* mesh = new Mesh();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float dx = width / segments_w;
    float dy = height / segments_h;

    for (int j = 0; j <= segments_h; ++j) 
    {
        for (int i = 0; i <= segments_w; ++i) 
        {
            float x = -width / 2.0f + i * dx;
            float y = -height / 2.0f + j * dy;
            float u = static_cast<float>(i) / segments_w;
            float v = static_cast<float>(j) / segments_h;

            vertices.push_back({
                {x, y, 0.0f},   // position
                {0.0f, 0.0f, 1.0f}, // normal
                {u, v}          // uv
            });
        }
    }

    for (int j = 0; j < segments_h; ++j) 
    {
        for (int i = 0; i < segments_w; ++i) 
        {
            int row1 = j * (segments_w + 1);
            int row2 = (j + 1) * (segments_w + 1);

            indices.push_back(row1 + i);
            indices.push_back(row2 + i);
            indices.push_back(row2 + i + 1);

            indices.push_back(row1 + i);
            indices.push_back(row2 + i + 1);
            indices.push_back(row1 + i + 1);
        }
    }

    mesh->setVertices(vertices);
    mesh->setIndices(indices);
    mesh->upload();
    return mesh;
}

// ================= Cube =================
Mesh* create_cube(float size) 
{
    Mesh* mesh = new Mesh();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float h = size / 2.0f;

    // 8 corners
    glm::vec3 positions[8] = {
        {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h}, // front
        {-h, -h, -h}, { h, -h, -h}, { h,  h, -h}, {-h,  h, -h}  // back
    };

    // Each face: 4 vertices with normals and UV
    struct Face { int a,b,c,d; glm::vec3 normal; };
    Face faces[6] = {
        {0,1,2,3, {0,0,1}}, // front
        {5,4,7,6, {0,0,-1}}, // back
        {4,0,3,7, {-1,0,0}}, // left
        {1,5,6,2, {1,0,0}}, // right
        {3,2,6,7, {0,1,0}}, // top
        {4,5,1,0, {0,-1,0}} // bottom
    };

    for (auto& f : faces) 
    {
        glm::vec2 uvs[4] = {{0,0},{1,0},{1,1},{0,1}};
        int startIndex = vertices.size();
        vertices.push_back({positions[f.a], f.normal, uvs[0]});
        vertices.push_back({positions[f.b], f.normal, uvs[1]});
        vertices.push_back({positions[f.c], f.normal, uvs[2]});
        vertices.push_back({positions[f.d], f.normal, uvs[3]});

        indices.push_back(startIndex + 0);
        indices.push_back(startIndex + 1);
        indices.push_back(startIndex + 2);

        indices.push_back(startIndex + 0);
        indices.push_back(startIndex + 2);
        indices.push_back(startIndex + 3);
    }

    mesh->setVertices(vertices);
    mesh->setIndices(indices);
    mesh->upload();
    return mesh;
}

// ================= Sphere =================
Mesh* create_sphere(float radius, int segments_lat, int segments_lon) 
{
    Mesh* mesh = new Mesh();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int lat = 0; lat <= segments_lat; ++lat) 
    {
        float theta = lat * M_PI / segments_lat;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int lon = 0; lon <= segments_lon; ++lon) 
        {
            float phi = lon * 2.0f * M_PI / segments_lon;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            glm::vec3 normal = {cosPhi * sinTheta, cosTheta, sinPhi * sinTheta};
            glm::vec3 position = radius * normal;
            glm::vec2 uv = {static_cast<float>(lon) / segments_lon,
                            static_cast<float>(lat) / segments_lat};

            vertices.push_back({position, glm::normalize(normal), uv});
        }
    }

    for (int lat = 0; lat < segments_lat; ++lat) 
    {
        for (int lon = 0; lon < segments_lon; ++lon) 
        {
            int first = lat * (segments_lon + 1) + lon;
            int second = first + segments_lon + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    mesh->setVertices(vertices);
    mesh->setIndices(indices);
    mesh->upload();
    return mesh;
}

} // namespace kcShaders
