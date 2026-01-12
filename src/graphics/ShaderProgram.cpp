#include "ShaderProgram.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace kcShaders {

ShaderProgram::~ShaderProgram() {
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

bool ShaderProgram::loadFromFiles(const std::string& vertPath, const std::string& fragPath, const std::string& geomPath) {
    GLuint vertShader = 0, fragShader = 0, geomShader = 0;
    
    if (!compileShader(vertShader, GL_VERTEX_SHADER, vertPath)) {
        return false;
    }
    
    if (!compileShader(fragShader, GL_FRAGMENT_SHADER, fragPath)) {
        glDeleteShader(vertShader);
        return false;
    }
    
    if (!geomPath.empty()) {
        if (!compileShader(geomShader, GL_GEOMETRY_SHADER, geomPath)) {
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);
            return false;
        }
    }
    
    bool success = linkProgram(vertShader, fragShader, geomShader);
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    if (geomShader != 0) {
        glDeleteShader(geomShader);
    }
    
    return success;
}

bool ShaderProgram::loadFromSource(const std::string& vertSource, const std::string& fragSource, const std::string& geomSource) {
    GLuint vertShader = 0, fragShader = 0, geomShader = 0;
    
    if (!compileShaderFromSource(vertShader, GL_VERTEX_SHADER, vertSource, "vertex")) {
        return false;
    }
    
    if (!compileShaderFromSource(fragShader, GL_FRAGMENT_SHADER, fragSource, "fragment")) {
        glDeleteShader(vertShader);
        return false;
    }
    
    if (!geomSource.empty()) {
        if (!compileShaderFromSource(geomShader, GL_GEOMETRY_SHADER, geomSource, "geometry")) {
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);
            return false;
        }
    }
    
    bool success = linkProgram(vertShader, fragShader, geomShader);
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    if (geomShader != 0) {
        glDeleteShader(geomShader);
    }
    
    return success;
}

void ShaderProgram::use() const {
    glUseProgram(program_);
}

GLint ShaderProgram::uniformLocation(const std::string& name) {
    auto it = locationCache_.find(name);
    if (it != locationCache_.end()) {
        return it->second;
    }
    
    GLint loc = glGetUniformLocation(program_, name.c_str());
    locationCache_[name] = loc;
    return loc;
}

void ShaderProgram::setInt(const std::string& name, int value) {
    GLint loc = uniformLocation(name);
    if (loc >= 0) glUniform1i(loc, value);
}

void ShaderProgram::setBool(const std::string& name, bool value) {
    setInt(name, value ? 1 : 0);
}

void ShaderProgram::setFloat(const std::string& name, float value) {
    GLint loc = uniformLocation(name);
    if (loc >= 0) glUniform1f(loc, value);
}

void ShaderProgram::setVec3(const std::string& name, const glm::vec3& value) {
    GLint loc = uniformLocation(name);
    if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(value));
}

void ShaderProgram::setVec4(const std::string& name, const glm::vec4& value) {
    GLint loc = uniformLocation(name);
    if (loc >= 0) glUniform4fv(loc, 1, glm::value_ptr(value));
}

void ShaderProgram::setMat4(const std::string& name, const glm::mat4& value) {
    GLint loc = uniformLocation(name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

bool ShaderProgram::compileShader(GLuint& shader, GLenum type, const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceStr = buffer.str();
    const char* source = sourceStr.c_str();
    
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed (" << path << "):\n" << infoLog << std::endl;
        glDeleteShader(shader);
        shader = 0;
        return false;
    }
    
    return true;
}

bool ShaderProgram::compileShaderFromSource(GLuint& shader, GLenum type, const std::string& source, const std::string& label) {
    const char* sourcePtr = source.c_str();
    
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed (" << label << "):\n" << infoLog << std::endl;
        glDeleteShader(shader);
        shader = 0;
        return false;
    }
    
    return true;
}

bool ShaderProgram::linkProgram(GLuint vertShader, GLuint fragShader, GLuint geomShader) {
    program_ = glCreateProgram();
    glAttachShader(program_, vertShader);
    glAttachShader(program_, fragShader);
    if (geomShader != 0) {
        glAttachShader(program_, geomShader);
    }
    
    glLinkProgram(program_);
    
    GLint success;
    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program_, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }
    
    return true;
}

} // namespace kcShaders
