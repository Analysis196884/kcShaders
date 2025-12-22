// Functions for creating geometry objects

#pragma once 

#include <vector>
#include <cstdint>

namespace kcShaders {

class Mesh;
struct Vertex;

// auto compute normals
void compute_normals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

kcShaders::Mesh* create_plane(float width, float height, int segments_w, int segments_h);
kcShaders::Mesh* create_cube(float size);
kcShaders::Mesh* create_sphere(float radius, int segments_lat, int segments_lon);

} // namespace kcShaders