#include "renderer.h"
#include "scene/scene.h"
#include "scene/mesh.h"
#include "scene/camera.h"
#include "scene/material.h"
#include "scene/light.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <iomanip>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB image write for screenshot
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace kcShaders {

Renderer::Renderer(GLFWwindow* window, int width, int height)
    : window_(window)
    , width_(width)
    , height_(height)
    , shader_program_(0)
    , vao_(0)
    , vbo_(0)
    , fbo_(0)
    , fbo_texture_(0)
    , rbo_(0)
    , fb_width_(800)
    , fb_height_(600)
    , vertex_count_(0)
{
}

Renderer::~Renderer()
{
    shutdown();
}

bool Renderer::initialize()
{
    // Create a fullscreen triangle for shader display
    float vertices[] = {
        // positions
        -1.0f, -1.0f, 0.0f,
         3.0f, -1.0f, 0.0f,
        -1.0f,  3.0f, 0.0f
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Calculate vertex count (number of vertices = total floats / components per vertex)
    vertex_count_ = static_cast<int>(sizeof(vertices) / (3 * sizeof(float)));
    
    // Create framebuffer
    create_framebuffer();

    return true;
}

void Renderer::shutdown()
{
    delete_framebuffer();
    
    if (vbo_ > 0) 
    {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ > 0) 
    {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (shader_program_ > 0) 
    {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }
}

void Renderer::clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render_shadertoy()
{
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, fb_width_, fb_height_);
    
    // Clear framebuffer
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render scene
    if (shader_program_ > 0 && vao_ > 0) 
    {
        glUseProgram(shader_program_);
        // Provide shadertoy-like uniforms 
        // iResolution (vec3)
        GLint locRes = glGetUniformLocation(shader_program_, "iResolution");
        if (locRes >= 0) {
            glUniform3f(static_cast<GLint>(locRes), (float)fb_width_, (float)fb_height_, 1.0f);
        }

        // iTime (float)
        GLint locTime = glGetUniformLocation(shader_program_, "iTime");
        if (locTime >= 0) 
        {
            float t = static_cast<float>(glfwGetTime());
            glUniform1f(locTime, t);
        }

        // iMouse (vec4) - x,y,current click x,y (simple mapping: z/w = 0)
        GLint locMouse = glGetUniformLocation(shader_program_, "iMouse");
        if (locMouse >= 0) 
        {
            double mx, my;
            glfwGetCursorPos(window_, &mx, &my);
            // Convert to pixels and invert Y to match shadertoy's origin (bottom-left)
            float mouse_x = static_cast<float>(mx);
            float mouse_y = static_cast<float>(fb_height_) - static_cast<float>(my);
            glUniform4f(locMouse, mouse_x, mouse_y, 0.0f, 0.0f);
        }
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
        glBindVertexArray(0);
    }
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::render_scene(Scene* scene, Camera* camera)
{
    if (!scene || !camera || shader_program_ == 0) {
        return;
    }

    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, fb_width_, fb_height_);
    
    // Clear framebuffer
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Use the shader program
    glUseProgram(shader_program_);
    
    // Get view and projection matrices from camera
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 proj = camera->GetProjectionMatrix();
    
    // Set view and projection uniforms 
    GLint locView = glGetUniformLocation(shader_program_, "uView");
    GLint locProj = glGetUniformLocation(shader_program_, "uProjection");
    
    if (locView >= 0) {
        glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);
    }
    
    if (locProj >= 0) {
        glUniformMatrix4fv(locProj, 1, GL_FALSE, &proj[0][0]);
    }
    
    // Set camera position for lighting calculations
    GLint locViewPos = glGetUniformLocation(shader_program_, "viewPos");
    if (locViewPos >= 0) {
        glm::vec3 camPos = camera->GetPosition();
        glUniform3fv(locViewPos, 1, &camPos[0]);
    }
    
    // Set up lighting uniforms
    int numDirLights = 0;
    int numPointLights = 0;
    int numSpotLights = 0;
    glm::vec3 ambientLight(0.0f);
    
    // Process all lights in the scene
    for (size_t i = 0; i < scene->lights.size(); ++i) {
        Light* light = scene->lights[i];
        if (!light || !light->enabled) continue;
        
        switch (light->GetType()) {
            case LightType::Directional: {
                if (numDirLights >= 4) break; // MAX_DIR_LIGHTS = 4
                DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
                
                std::string baseName = "dirLights[" + std::to_string(numDirLights) + "]";
                GLint loc;
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".direction").c_str());
                if (loc >= 0) glUniform3fv(loc, 1, &dirLight->direction[0]);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".color").c_str());
                if (loc >= 0) glUniform3fv(loc, 1, &dirLight->color[0]);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".intensity").c_str());
                if (loc >= 0) glUniform1f(loc, dirLight->intensity);
                
                numDirLights++;
                break;
            }
            
            case LightType::Point: {
                if (numPointLights >= 8) break; // MAX_POINT_LIGHTS = 8
                PointLight* pointLight = static_cast<PointLight*>(light);
                
                std::string baseName = "pointLights[" + std::to_string(numPointLights) + "]";
                GLint loc;
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".position").c_str());
                if (loc >= 0) glUniform3fv(loc, 1, &pointLight->position[0]);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".color").c_str());
                if (loc >= 0) glUniform3fv(loc, 1, &pointLight->color[0]);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".intensity").c_str());
                if (loc >= 0) glUniform1f(loc, pointLight->intensity);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".constant").c_str());
                if (loc >= 0) glUniform1f(loc, pointLight->constant);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".linear").c_str());
                if (loc >= 0) glUniform1f(loc, pointLight->linear);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".quadratic").c_str());
                if (loc >= 0) glUniform1f(loc, pointLight->quadratic);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".radius").c_str());
                if (loc >= 0) glUniform1f(loc, pointLight->radius);
                
                numPointLights++;
                break;
            }
            
            case LightType::Spot: {
                if (numSpotLights >= 4) break; // MAX_SPOT_LIGHTS = 4
                SpotLight* spotLight = static_cast<SpotLight*>(light);
                
                std::string baseName = "spotLights[" + std::to_string(numSpotLights) + "]";
                GLint loc;
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".position").c_str());
                if (loc >= 0) glUniform3fv(loc, 1, &spotLight->position[0]);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".direction").c_str());
                if (loc >= 0) glUniform3fv(loc, 1, &spotLight->direction[0]);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".color").c_str());
                if (loc >= 0) glUniform3fv(loc, 1, &spotLight->color[0]);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".intensity").c_str());
                if (loc >= 0) glUniform1f(loc, spotLight->intensity);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".innerConeAngle").c_str());
                if (loc >= 0) glUniform1f(loc, spotLight->innerConeAngle);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".outerConeAngle").c_str());
                if (loc >= 0) glUniform1f(loc, spotLight->outerConeAngle);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".constant").c_str());
                if (loc >= 0) glUniform1f(loc, spotLight->constant);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".linear").c_str());
                if (loc >= 0) glUniform1f(loc, spotLight->linear);
                
                loc = glGetUniformLocation(shader_program_, (baseName + ".quadratic").c_str());
                if (loc >= 0) glUniform1f(loc, spotLight->quadratic);
                
                numSpotLights++;
                break;
            }
            
            case LightType::Ambient: {
                AmbientLight* ambLight = static_cast<AmbientLight*>(light);
                ambientLight += ambLight->color * ambLight->intensity;
                break;
            }
            
            default:
                break;
        }
    }
    
    // Set light counts
    GLint locNumDirLights = glGetUniformLocation(shader_program_, "numDirLights");
    if (locNumDirLights >= 0) glUniform1i(locNumDirLights, numDirLights);
    
    GLint locNumPointLights = glGetUniformLocation(shader_program_, "numPointLights");
    if (locNumPointLights >= 0) glUniform1i(locNumPointLights, numPointLights);
    
    GLint locNumSpotLights = glGetUniformLocation(shader_program_, "numSpotLights");
    if (locNumSpotLights >= 0) glUniform1i(locNumSpotLights, numSpotLights);
    
    // Set ambient light
    GLint locAmbientLight = glGetUniformLocation(shader_program_, "ambientLight");
    if (locAmbientLight >= 0) glUniform3fv(locAmbientLight, 1, &ambientLight[0]);
    
    // Collect all render items from the scene
    std::vector<RenderItem> items;
    scene->collectRenderItems(items);
    
    // Render each item
    for (const auto& item : items) {
        if (item.mesh && item.mesh->isUploaded()) {
            // Set model matrix uniform
            GLint locModel = glGetUniformLocation(shader_program_, "uModel");
            if (locModel >= 0) {
                glUniformMatrix4fv(locModel, 1, GL_FALSE, &item.modelMatrix[0][0]);
            }
            
            // Set material properties
            if (item.material) {
                // Albedo/Base Color
                GLint locAlbedo = glGetUniformLocation(shader_program_, "material.albedo");
                if (locAlbedo >= 0) {
                    glUniform3fv(locAlbedo, 1, &item.material->albedo[0]);
                }
                
                // PBR properties
                GLint locMetallic = glGetUniformLocation(shader_program_, "material.metallic");
                if (locMetallic >= 0) {
                    glUniform1f(locMetallic, item.material->metallic);
                }
                
                GLint locRoughness = glGetUniformLocation(shader_program_, "material.roughness");
                if (locRoughness >= 0) {
                    glUniform1f(locRoughness, item.material->roughness);
                }
                
                GLint locAO = glGetUniformLocation(shader_program_, "material.ao");
                if (locAO >= 0) {
                    glUniform1f(locAO, item.material->ao);
                }
                
                // Emissive
                GLint locEmissive = glGetUniformLocation(shader_program_, "material.emissive");
                if (locEmissive >= 0) {
                    glUniform3fv(locEmissive, 1, &item.material->emissive[0]);
                }
                
                GLint locEmissiveStrength = glGetUniformLocation(shader_program_, "material.emissiveStrength");
                if (locEmissiveStrength >= 0) {
                    glUniform1f(locEmissiveStrength, item.material->emissiveStrength);
                }
                
                // Opacity
                GLint locOpacity = glGetUniformLocation(shader_program_, "material.opacity");
                if (locOpacity >= 0) {
                    glUniform1f(locOpacity, item.material->opacity);
                }
                
                // Set texture samplers (bind textures to texture units)
                // Texture unit 0: Albedo map
                GLint locAlbedoMap = glGetUniformLocation(shader_program_, "albedoMap");
                GLint locHasAlbedoMap = glGetUniformLocation(shader_program_, "hasAlbedoMap");
                if (locAlbedoMap >= 0) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, item.material->albedoMap);
                    glUniform1i(locAlbedoMap, 0);
                }
                if (locHasAlbedoMap >= 0) {
                    glUniform1i(locHasAlbedoMap, item.material->albedoMap != 0 ? 1 : 0);
                }
                
                // Texture unit 1: Metallic map
                GLint locMetallicMap = glGetUniformLocation(shader_program_, "metallicMap");
                GLint locHasMetallicMap = glGetUniformLocation(shader_program_, "hasMetallicMap");
                if (locMetallicMap >= 0) {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, item.material->metallicMap);
                    glUniform1i(locMetallicMap, 1);
                }
                if (locHasMetallicMap >= 0) {
                    glUniform1i(locHasMetallicMap, item.material->metallicMap != 0 ? 1 : 0);
                }
                
                // Texture unit 2: Roughness map
                GLint locRoughnessMap = glGetUniformLocation(shader_program_, "roughnessMap");
                GLint locHasRoughnessMap = glGetUniformLocation(shader_program_, "hasRoughnessMap");
                if (locRoughnessMap >= 0) {
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, item.material->roughnessMap);
                    glUniform1i(locRoughnessMap, 2);
                }
                if (locHasRoughnessMap >= 0) {
                    glUniform1i(locHasRoughnessMap, item.material->roughnessMap != 0 ? 1 : 0);
                }
                
                // Texture unit 3: Normal map
                GLint locNormalMap = glGetUniformLocation(shader_program_, "normalMap");
                GLint locHasNormalMap = glGetUniformLocation(shader_program_, "hasNormalMap");
                if (locNormalMap >= 0) {
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, item.material->normalMap);
                    glUniform1i(locNormalMap, 3);
                }
                if (locHasNormalMap >= 0) {
                    glUniform1i(locHasNormalMap, item.material->normalMap != 0 ? 1 : 0);
                }
                
                // Texture unit 4: AO map
                GLint locAOMap = glGetUniformLocation(shader_program_, "aoMap");
                GLint locHasAOMap = glGetUniformLocation(shader_program_, "hasAOMap");
                if (locAOMap >= 0) {
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, item.material->aoMap);
                    glUniform1i(locAOMap, 4);
                }
                if (locHasAOMap >= 0) {
                    glUniform1i(locHasAOMap, item.material->aoMap != 0 ? 1 : 0);
                }
                
                // Texture unit 5: Emissive map
                GLint locEmissiveMap = glGetUniformLocation(shader_program_, "emissiveMap");
                GLint locHasEmissiveMap = glGetUniformLocation(shader_program_, "hasEmissiveMap");
                if (locEmissiveMap >= 0) {
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, item.material->emissiveMap);
                    glUniform1i(locEmissiveMap, 5);
                }
                if (locHasEmissiveMap >= 0) {
                    glUniform1i(locHasEmissiveMap, item.material->emissiveMap != 0 ? 1 : 0);
                }
                
                // Reset active texture unit
                glActiveTexture(GL_TEXTURE0);
            } else {
                // Use default material values if no material is assigned
                GLint locAlbedo = glGetUniformLocation(shader_program_, "material.albedo");
                if (locAlbedo >= 0) {
                    glUniform3f(locAlbedo, 0.8f, 0.8f, 0.8f);
                }
                
                GLint locMetallic = glGetUniformLocation(shader_program_, "material.metallic");
                if (locMetallic >= 0) {
                    glUniform1f(locMetallic, 0.0f);
                }
                
                GLint locRoughness = glGetUniformLocation(shader_program_, "material.roughness");
                if (locRoughness >= 0) {
                    glUniform1f(locRoughness, 0.5f);
                }
                
                GLint locAO = glGetUniformLocation(shader_program_, "material.ao");
                if (locAO >= 0) {
                    glUniform1f(locAO, 1.0f);
                }
            }
            
            // Draw the mesh
            item.mesh->draw();
        }
    }
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Renderer::use_shader(const std::string& vertex_path, const std::string& fragment_path)
{
    std::string vertex_code;
    std::string fragment_code;

    // Load fragment shader if provided
    if (!fragment_path.empty()) {
        fragment_code = load_shader_source(fragment_path);
        if (fragment_code.empty()) {
            std::cerr << "Failed to load fragment shader source file: " << fragment_path << "\n";
            return false;
        }
    } else {
        std::cerr << "No fragment shader provided\n";
        return false;
    }

    // Load vertex shader
    if (!vertex_path.empty()) {
        vertex_code = load_shader_source(vertex_path);
        if (vertex_code.empty()) {
            std::cerr << "Failed to load vertex shader source file: " << vertex_path << "\n";
            return false;
        }
    } else {
        // ...
    }

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_code.c_str());
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_code.c_str());

    if (vertex_shader == 0 || fragment_shader == 0) {
        return false;
    }

    shader_program_ = link_program(vertex_shader, fragment_shader);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    if (shader_program_ == 0) {
        return false;
    }

    return true;
}

std::string Renderer::load_shader_source(const std::string& shaderpath)
{
    std::ifstream file(shaderpath, std::ios::in | std::ios::binary);
    if (!file)
        return std::string();

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Renderer::compile_shader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint Renderer::link_program(GLuint vs, GLuint fs)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void Renderer::create_framebuffer()
{
    // Delete existing framebuffer if any
    delete_framebuffer();
    
    // Create framebuffer
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    
    // Create color texture
    glGenTextures(1, &fbo_texture_);
    glBindTexture(GL_TEXTURE_2D, fbo_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fb_width_, fb_height_, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture_, 0);
    
    // Create renderbuffer for depth and stencil
    glGenRenderbuffers(1, &rbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fb_width_, fb_height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::delete_framebuffer()
{
    if (fbo_ > 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    if (fbo_texture_ > 0) {
        glDeleteTextures(1, &fbo_texture_);
        fbo_texture_ = 0;
    }
    if (rbo_ > 0) {
        glDeleteRenderbuffers(1, &rbo_);
        rbo_ = 0;
    }
}

void Renderer::resize_framebuffer(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    
    fb_width_ = width;
    fb_height_ = height;
    
    create_framebuffer();
}

bool Renderer::take_screenshot(const std::string& filename)
{
    // Set pixel alignment for proper row reading
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    
    // Allocate memory for pixel data
    std::vector<unsigned char> pixels(fb_width_ * fb_height_ * 3);
    
    // Bind the framebuffer for reading
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    
    // Check what we're reading from
    GLint readFBO;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);
    
    // Read the pixels from bottom to top (OpenGL origin is bottom-left)
    // We read directly in the correct order to avoid manual flipping
    for (int y = 0; y < fb_height_; ++y) {
        glReadPixels(0, fb_height_ - 1 - y, fb_width_, 1, GL_RGB, GL_UNSIGNED_BYTE, 
                     pixels.data() + (y * fb_width_ * 3));
    }
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    
    // Reset pixel alignment
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    
    // Write to file
    int result = stbi_write_png(filename.c_str(), fb_width_, fb_height_, 3, pixels.data(), fb_width_ * 3);
    
    if (result) {
        std::cout << "Screenshot saved: " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Failed to save screenshot: " << filename << std::endl;
        return false;
    }
}

} // namespace kcShaders