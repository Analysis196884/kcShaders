#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>

namespace kcShaders {

/**
 * @brief Represents a loaded texture on the GPU
 */
class Texture {
public:
    Texture() = default;
    ~Texture();

    // non-copyable (GPU resource)
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // movable
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    /**
     * @brief Load texture from file
     * @param filepath Path to image file (PNG, JPG, etc.)
     * @return true if load succeeded
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * @brief Get OpenGL texture ID
     */
    GLuint getHandle() const { return handle_; }

    /**
     * @brief Check if texture is loaded
     */
    bool isLoaded() const { return handle_ != 0; }

    /**
     * @brief Get texture dimensions
     */
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    /**
     * @brief Bind texture to a specific unit
     */
    void bind(GLuint unit = 0) const;

private:
    void release();

private:
    GLuint handle_ = 0;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
};

/**
 * @brief Manages texture loading and caching
 */
class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager();

    /**
     * @brief Load or get cached texture
     * @param filepath Path to image file
     * @return Texture ID (GLuint), or 0 if failed
     */
    GLuint loadTexture(const std::string& filepath);

    /**
     * @brief Get texture by path
     */
    Texture* getTexture(const std::string& filepath);

    /**
     * @brief Clear all cached textures
     */
    void clear();

private:
    std::unordered_map<std::string, Texture*> cache_;
};

} // namespace kcShaders
