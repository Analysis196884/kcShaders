#pragma once

#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace kcShaders {

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    // Non-copyable
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // Load and compile shaders
    bool loadFromFiles(const std::string& vertPath, const std::string& fragPath, const std::string& geomPath = "");
    
    // Use this shader program
    void use() const;
    
    // Get uniform location (cached)
    GLint uniformLocation(const std::string& name);
    
    // Uniform setters
    void setInt(const std::string& name, int value);
    void setBool(const std::string& name, bool value);
    void setFloat(const std::string& name, float value);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setMat4(const std::string& name, const glm::mat4& value);
    
    GLuint id() const { return program_; }
    bool isValid() const { return program_ != 0; }

private:
    bool compileShader(GLuint& shader, GLenum type, const std::string& path);
    bool linkProgram(GLuint vertShader, GLuint fragShader, GLuint geomShader = 0);
    
    GLuint program_ = 0;
    std::unordered_map<std::string, GLint> locationCache_;
};

} // namespace kcShaders
