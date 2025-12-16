#pragma once 

namespace kcShaders {

class Light {
public:
    enum class Type 
    {
        DIRECTIONAL,
        POINT,
        SPOT
    };

    Light(Type type);
    ~Light();

    void set_position(float x, float y, float z);
    void set_color(float r, float g, float b);
    void set_intensity(float intensity);

    Type get_type() const { return type_; }
    void get_position(float& x, float& y, float& z) const;
    void get_color(float& r, float& g, float& b) const;
    float get_intensity() const { return intensity_; }

private:
    Type type_;
    float position_[3];
    float color_[3];
    float intensity_;
};

} // namespace kcShaders