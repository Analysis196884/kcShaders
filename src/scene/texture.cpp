#include "texture.h"

#include <iostream>
#include <filesystem>

// Define stb_image implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace kcShaders {

// ================= Texture =================

Texture::~Texture() {
    release();
}

Texture::Texture(Texture&& other) noexcept {
    *this = std::move(other);
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        release();
        handle_ = other.handle_;
        width_ = other.width_;
        height_ = other.height_;
        channels_ = other.channels_;
        
        other.handle_ = 0;
        other.width_ = 0;
        other.height_ = 0;
        other.channels_ = 0;
    }
    return *this;
}

bool Texture::loadFromFile(const std::string& filepath) {
    // Check if file exists
    if (!std::filesystem::exists(filepath)) {
        std::cerr << "Texture file not found: " << filepath << std::endl;
        return false;
    }

    // Load image data
    unsigned char* data = stbi_load(filepath.c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "Failed to load texture: " << filepath << " - " << stbi_failure_reason() << std::endl;
        return false;
    }

    // Force RGBA format (4 channels)
    channels_ = 4;

    // Create OpenGL texture
    glGenTextures(1, &handle_);
    glBindTexture(GL_TEXTURE_2D, handle_);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    // std::cout << "Loaded texture: " << filepath << " (" << width_ << "x" << height_ << ")" << std::endl;
    return true;
}

void Texture::bind(GLuint unit) const {
    if (handle_ == 0) {
        std::cerr << "Attempting to bind unloaded texture" << std::endl;
        return;
    }
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, handle_);
}

void Texture::release() {
    if (handle_ != 0) {
        glDeleteTextures(1, &handle_);
        handle_ = 0;
    }
    width_ = height_ = channels_ = 0;
}

// ================= TextureManager =================

TextureManager::~TextureManager() {
    clear();
}

GLuint TextureManager::loadTexture(const std::string& filepath) {
    // Check if already cached
    auto it = cache_.find(filepath);
    if (it != cache_.end()) {
        return it->second->getHandle();
    }

    // Load new texture
    Texture* texture = new Texture();
    if (!texture->loadFromFile(filepath)) {
        delete texture;
        return 0;
    }

    GLuint handle = texture->getHandle();
    cache_[filepath] = texture;
    return handle;
}

Texture* TextureManager::getTexture(const std::string& filepath) {
    auto it = cache_.find(filepath);
    if (it != cache_.end()) {
        return it->second;
    }
    return nullptr;
}

void TextureManager::clear() {
    for (auto& pair : cache_) {
        delete pair.second;
    }
    cache_.clear();
}

} // namespace kcShaders
