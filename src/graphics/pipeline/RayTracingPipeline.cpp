#include "RayTracingPipeline.h"
#include "../ShaderProgram.h"
#include "../BVH.h"
#include "../../scene/camera.h"
#include "../../scene/scene.h"
#include "../../scene/mesh.h"
#include "../../scene/material.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <glm/gtc/type_ptr.hpp>

// Helper function to check OpenGL errors
static void CheckGLError(const char* location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[RayTracingPipeline] OpenGL error at " << location << ": 0x" 
                  << std::hex << err << std::dec << std::endl;
    }
}

namespace kcShaders {

RayTracingPipeline::RayTracingPipeline(GLuint fbo, GLuint vao, int width, int height)
    : fbo_(fbo)
    , vao_(vao)
    , width_(width)
    , height_(height)
    , outputTexture_(0)
    , accumulationTexture_(0)
    , lastCameraPosition_(0.0f)
    , lastCameraFront_(0.0f, 1.0f, 0.0f)
    , cameraMovedThisFrame_(false)
    , computeShaderProgram_(0)
    , vertexBuffer_(0)
    , triangleBuffer_(0)
    , bvhBuffer_(0)
    , materialBuffer_(0)
    , sceneUploaded_(false)
    , maxBounces_(4)
    , samplesPerPixel_(1)
    , frameCount_(0)
{
}

RayTracingPipeline::~RayTracingPipeline()
{
    cleanup();
}

bool RayTracingPipeline::initialize()
{   
    // Create output texture for compute shader
    createOutputTexture();
    
    // Create scene buffers
    createSceneBuffers();
    
    std::cout << "[RayTracingPipeline] Initialized\n";
    return true;
}

void RayTracingPipeline::createOutputTexture()
{
    if (outputTexture_ != 0) {
        deleteOutputTexture();
    }
    
    // Create output texture
    glGenTextures(1, &outputTexture_);
    CheckGLError("glGenTextures output");
    
    glBindTexture(GL_TEXTURE_2D, outputTexture_);
    CheckGLError("glBindTexture output");
    
    // RGBA32F for HDR rendering
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_RGBA, GL_FLOAT, nullptr);
    CheckGLError("glTexImage2D output");
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckGLError("glTexParameteri output");
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Create accumulation texture
    glGenTextures(1, &accumulationTexture_);
    CheckGLError("glGenTextures accumulation");
    
    glBindTexture(GL_TEXTURE_2D, accumulationTexture_);
    CheckGLError("glBindTexture accumulation");
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_RGBA, GL_FLOAT, nullptr);
    CheckGLError("glTexImage2D accumulation");
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckGLError("glTexParameteri accumulation");
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RayTracingPipeline::deleteOutputTexture()
{
    if (outputTexture_ != 0) {
        glDeleteTextures(1, &outputTexture_);
        outputTexture_ = 0;
    }
    if (accumulationTexture_ != 0) {
        glDeleteTextures(1, &accumulationTexture_);
        accumulationTexture_ = 0;
    }
}

bool RayTracingPipeline::loadComputeShader(const std::string& computePath)
{
    std::cout << "[RayTracingPipeline] Loading compute shader: " << computePath << "\n";
    
    // Read compute shader source
    std::ifstream file(computePath);
    if (!file.is_open()) {
        std::cerr << "[RayTracingPipeline] Failed to open compute shader: " << computePath << "\n";
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();
    
    // Compile compute shader
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);
    
    // Check compilation
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[RayTracingPipeline] Compute shader compilation failed:\n" << infoLog << "\n";
        glDeleteShader(shader);
        return false;
    }
    
    // Create program
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    
    // Check linking
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "[RayTracingPipeline] Compute shader program linking failed:\n" << infoLog << "\n";
        glDeleteShader(shader);
        glDeleteProgram(program);
        return false;
    }
    
    glDeleteShader(shader);
    
    // Create a simple wrapper for compute shader program
    // Store the program ID separately since ShaderProgram doesn't support compute shaders
    if (computeShaderProgram_ != 0) {
        glDeleteProgram(computeShaderProgram_);
    }
    
    // We'll use the raw program ID directly for compute shader
    computeShaderProgram_ = program;
    
    std::cout << "[RayTracingPipeline] Compute shader loaded successfully\n";
    return true;
}

bool RayTracingPipeline::loadDisplayShader(const std::string& vertPath, const std::string& fragPath)
{
    // std::cout << "[RayTracingPipeline] Loading display shader: " << vertPath << " + " << fragPath << "\n";
    
    displayShader_ = std::make_unique<ShaderProgram>();
    if (!displayShader_->loadFromFiles(vertPath, fragPath)) {
        std::cerr << "[RayTracingPipeline] Failed to load display shaders\n";
        return false;
    }
    
    // std::cout << "[RayTracingPipeline] Display shader loaded successfully\n";
    return true;
}

void RayTracingPipeline::execute(RenderContext& ctx)
{
    if (computeShaderProgram_ == 0) {
        std::cerr << "[RayTracingPipeline] Compute shader not loaded\n";
        return;
    }
    
    if (!displayShader_) {
        std::cerr << "[RayTracingPipeline] Display shader not loaded\n";
        return;
    }
    
    if (outputTexture_ == 0) {
        std::cerr << "[RayTracingPipeline] Output texture not created\n";
        return;
    }
    
    // Detect camera movement
    cameraMovedThisFrame_ = false;
    if (ctx.camera) {
        glm::vec3 camPos = ctx.camera->GetPosition();
        glm::vec3 camFront = ctx.camera->GetFront();
        
        float posDelta = glm::length(camPos - lastCameraPosition_);
        float frontDelta = glm::length(camFront - lastCameraFront_);
        
        if (posDelta > 0.0001f || frontDelta > 0.0001f) {
            cameraMovedThisFrame_ = true;
            frameCount_ = 0;  // Reset accumulation
            lastCameraPosition_ = camPos;
            lastCameraFront_ = camFront;
        }
    }
    
    // === Step 1: Run compute shader for ray tracing ===
    glUseProgram(computeShaderProgram_);
    CheckGLError("glUseProgram(compute)");
    
    // Bind output texture as image for compute shader write
    glBindImageTexture(0, outputTexture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    CheckGLError("glBindImageTexture output");
    
    // Bind accumulation texture for read/write
    glBindImageTexture(1, accumulationTexture_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    CheckGLError("glBindImageTexture accumulation");
    
    // Set uniforms manually for compute shader
    GLint loc;
    
    loc = glGetUniformLocation(computeShaderProgram_, "iResolution");
    if (loc >= 0) {
        glUniform3f(loc, (float)width_, (float)height_, 0.0f);
        CheckGLError("iResolution");
    }
    
    loc = glGetUniformLocation(computeShaderProgram_, "iTime");
    if (loc >= 0) {
        glUniform1f(loc, ctx.totalTime);
        CheckGLError("iTime");
    }
    
    loc = glGetUniformLocation(computeShaderProgram_, "iFrame");
    if (loc >= 0) {
        glUniform1i(loc, frameCount_);
        CheckGLError("iFrame");
    }
    
    // Increment frame count after setting uniform
    frameCount_++;
    
    loc = glGetUniformLocation(computeShaderProgram_, "maxBounces");
    if (loc >= 0) {
        glUniform1i(loc, maxBounces_);
        CheckGLError("maxBounces");
    }
    
    loc = glGetUniformLocation(computeShaderProgram_, "samplesPerPixel");
    if (loc >= 0) {
        glUniform1i(loc, samplesPerPixel_);
        CheckGLError("samplesPerPixel");
    }
    
    // Camera uniforms (if camera exists)
    if (ctx.camera) {
        glm::vec3 camPos = ctx.camera->GetPosition();
        glm::vec3 camFront = ctx.camera->GetFront();
        glm::vec3 camRight = ctx.camera->GetRight();
        glm::vec3 camUp = glm::cross(camRight, camFront);
        float camFov = ctx.camera->GetFov();
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraPosition");
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(camPos));
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraFront");
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(camFront));
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraUp");
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(camUp));
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraRight");
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(camRight));
        
        loc = glGetUniformLocation(computeShaderProgram_, "cameraFov");
        if (loc >= 0) glUniform1f(loc, camFov);
        
        CheckGLError("camera uniforms");
    }
    
    // Dispatch compute shader (one thread per pixel, organized in 16x16 local groups)
    GLuint numGroupsX = (width_ + 15) / 16;
    GLuint numGroupsY = (height_ + 15) / 16;
    
    // Only log first frame to avoid spam
    // static bool firstFrame = true;
    // if (firstFrame) {
    //     std::cout << "[RayTracingPipeline] Dispatching compute: " << numGroupsX << "x" << numGroupsY 
    //               << " groups for " << width_ << "x" << height_ << " pixels\n";
    //     firstFrame = false;
    // }
    
    glDispatchCompute(numGroupsX, numGroupsY, 1);
    CheckGLError("glDispatchCompute");
    
    // Wait for compute shader to finish
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    CheckGLError("glMemoryBarrier");
    
    // === Step 2: Display the ray traced image on fullscreen quad ===
    
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    CheckGLError("glBindFramebuffer");
    
    glViewport(0, 0, width_, height_);
    CheckGLError("glViewport");
    
    // Clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckGLError("glClear");
    
    // Disable depth test
    glDisable(GL_DEPTH_TEST);
    
    // Use display shader
    displayShader_->use();
    CheckGLError("displayShader->use");
    
    // Bind output texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outputTexture_);
    CheckGLError("glBindTexture(output)");
    
    displayShader_->setInt("screenTexture", 0);
    CheckGLError("setInt(screenTexture)");
    
    // Draw fullscreen triangle
    glBindVertexArray(vao_);
    CheckGLError("glBindVertexArray");
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    CheckGLError("glDrawArrays");
    
    glBindVertexArray(0);
    
    // Re-enable depth test
    glEnable(GL_DEPTH_TEST);
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RayTracingPipeline::resize(int width, int height)
{
    width_ = width;
    height_ = height;
    
    // Recreate output texture with new size
    createOutputTexture();
    
    // Reset frame count for progressive rendering
    frameCount_ = 0;
}

void RayTracingPipeline::cleanup()
{
    deleteOutputTexture();
    deleteSceneBuffers();
    
    if (computeShaderProgram_ != 0) {
        glDeleteProgram(computeShaderProgram_);
        computeShaderProgram_ = 0;
    }
    
    displayShader_.reset();
}

void RayTracingPipeline::createSceneBuffers()
{   
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &triangleBuffer_);
    glGenBuffers(1, &bvhBuffer_);
    glGenBuffers(1, &materialBuffer_);
    
    CheckGLError("createSceneBuffers");
}

void RayTracingPipeline::deleteSceneBuffers()
{
    if (vertexBuffer_ != 0) {
        glDeleteBuffers(1, &vertexBuffer_);
        vertexBuffer_ = 0;
    }
    if (triangleBuffer_ != 0) {
        glDeleteBuffers(1, &triangleBuffer_);
        triangleBuffer_ = 0;
    }
    if (bvhBuffer_ != 0) {
        glDeleteBuffers(1, &bvhBuffer_);
        bvhBuffer_ = 0;
    }
    if (materialBuffer_ != 0) {
        glDeleteBuffers(1, &materialBuffer_);
        materialBuffer_ = 0;
    }
}

void RayTracingPipeline::uploadScene(Scene* scene)
{
    if (!scene) {
        std::cerr << "[RayTracingPipeline] Cannot upload null scene\n";
        return;
    }
    
    // std::cout << "[RayTracingPipeline] Uploading scene to GPU...\n";
    
    // Collect all render items (mesh + transform)
    std::vector<RenderItem> renderItems;
    scene->collectRenderItems(renderItems);
    
    if (renderItems.empty()) {
        std::cout << "[RayTracingPipeline] Scene has no meshes to render\n";
        return;
    }
    
    // Temporary storage for all triangles
    std::vector<GpuVertex> allVertices;
    std::vector<GpuTriangle> allTriangles;
    std::vector<GpuMaterial> allMaterials;
    
    // Material index map (Material* -> GPU index)
    std::map<Material*, uint32_t> materialIndexMap;
    
    // Add default material at index 0
    GpuMaterial defaultMat;
    defaultMat.albedo = glm::vec3(0.8f, 0.8f, 0.8f);
    defaultMat.metallic = 0.0f;
    defaultMat.roughness = 0.5f;
    defaultMat.ao = 1.0f;
    defaultMat.opacity = 1.0f;
    defaultMat.emissive = glm::vec3(0.0f);
    defaultMat.emissiveStrength = 0.0f;
    defaultMat._pad0 = 0.0f;
    allMaterials.push_back(defaultMat);
    
    // Process each render item
    for (const auto& item : renderItems) {
        if (!item.mesh) continue;
        
        Mesh* mesh = item.mesh;
        Material* material = item.material;
        glm::mat4 modelMatrix = item.modelMatrix;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
        
        // Get or create material index
        uint32_t materialIndex = 0;  // Default material
        if (material) {
            auto it = materialIndexMap.find(material);
            if (it != materialIndexMap.end()) {
                materialIndex = it->second;
            } else {
                // Add new material
                materialIndex = static_cast<uint32_t>(allMaterials.size());
                materialIndexMap[material] = materialIndex;
                
                GpuMaterial gpuMat;
                gpuMat.albedo = material->albedo;
                gpuMat.metallic = material->metallic;
                gpuMat.roughness = material->roughness;
                gpuMat.ao = material->ao;
                gpuMat.opacity = material->opacity;
                gpuMat.emissive = material->emissive;
                gpuMat.emissiveStrength = material->emissiveStrength;
                gpuMat._pad0 = 0.0f;
                allMaterials.push_back(gpuMat);
            }
        }
        
        uint32_t baseVertex = static_cast<uint32_t>(allVertices.size());
        
        // Add vertices (transformed to world space)
        for (size_t i = 0; i < mesh->GetVertexCount(); i++) {
            GpuVertex gpuVert;
            
            // Transform position to world space
            glm::vec4 worldPos = modelMatrix * glm::vec4(mesh->GetVertex(i).position, 1.0f);
            gpuVert.position = glm::vec3(worldPos);
            gpuVert._pad0 = 0.0f;
            
            // Transform normal to world space
            if (i < mesh->GetVertexCount()) {
                glm::vec3 normal = mesh->GetVertex(i).normal;
                // Ensure input normal is normalized
                if (glm::length(normal) > 0.001f) {
                    gpuVert.normal = glm::normalize(normalMatrix * normal);
                } else {
                    // Fallback to up vector if normal is zero
                    gpuVert.normal = glm::vec3(0.0f, 0.0f, 1.0f);
                }
            } else {
                gpuVert.normal = glm::vec3(0.0f, 0.0f, 1.0f);
            }
            gpuVert._pad1 = 0.0f;
            
            // UV coordinates
            if (i < mesh->GetIndexCount()) {
                gpuVert.uv = mesh->GetVertex(i).uv;
            } else {
                gpuVert.uv = glm::vec2(0.0f);
            }
            gpuVert._pad2[0] = 0.0f;
            gpuVert._pad2[1] = 0.0f;
            
            allVertices.push_back(gpuVert);
        }
        
        // Add triangles
        const auto& meshIndices = mesh->GetIndices();
        for (size_t i = 0; i < meshIndices.size(); i += 3) {
            if (i + 2 >= meshIndices.size()) break;
            
            uint32_t i0 = baseVertex + meshIndices[i];
            uint32_t i1 = baseVertex + meshIndices[i + 1];
            uint32_t i2 = baseVertex + meshIndices[i + 2];
            
            GpuTriangle tri;
            tri.v0 = i0;
            tri.v1 = i1;
            tri.v2 = i2;
            tri.materialId = materialIndex;  // Use actual material index
            
            allTriangles.push_back(tri);
        }
    }
    
    if (allTriangles.empty()) {
        std::cout << "[RayTracingPipeline] No triangles to render\n";
        return;
    }
    
    // std::cout << "[RayTracingPipeline] Collected " << allVertices.size() 
    //           << " vertices, " << allTriangles.size() << " triangles, "
    //           << allMaterials.size() << " materials\n";
    
    // Build BVH
    // std::cout << "[RayTracingPipeline] Building BVH...\n";
    BVHBuilder bvhBuilder;
    bvhBuilder.build(allVertices, allTriangles);
    
    const auto& bvhNodes = bvhBuilder.getNodes();
    std::cout << "[RayTracingPipeline] BVH built with " << bvhNodes.size() << " nodes\n";
    
    // Reorder triangles based on BVH
    const auto& triIndices = bvhBuilder.getTriangleIndices();
    std::vector<GpuTriangle> reorderedTriangles(allTriangles.size());
    for (size_t i = 0; i < triIndices.size(); i++) {
        reorderedTriangles[i] = allTriangles[triIndices[i]];
    }
    
    // Upload to GPU
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, allVertices.size() * sizeof(GpuVertex), 
                 allVertices.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vertexBuffer_);
    CheckGLError("upload vertices");
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleBuffer_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, reorderedTriangles.size() * sizeof(GpuTriangle), 
                 reorderedTriangles.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, triangleBuffer_);
    CheckGLError("upload triangles");
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhBuffer_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bvhNodes.size() * sizeof(BVHNode), 
                 bvhNodes.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bvhBuffer_);
    CheckGLError("upload BVH");
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialBuffer_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, allMaterials.size() * sizeof(GpuMaterial), 
                 allMaterials.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, materialBuffer_);
    CheckGLError("upload materials");
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    sceneUploaded_ = true;
    // std::cout << "[RayTracingPipeline] Scene data uploaded to GPU successfully\n";
}

} // namespace kcShaders