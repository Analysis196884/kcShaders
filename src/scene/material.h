#pragma once 

namespace kcShaders {

class Material {
public:
    Material() noexcept = default;
    ~Material() noexcept = default;

    // non-copyable
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    // movable
    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;
};

} // namespace kcShaders