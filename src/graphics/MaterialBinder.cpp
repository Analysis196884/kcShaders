#include "MaterialBinder.h"
#include "ShaderProgram.h"
#include "../scene/material.h"
#include <glm/glm.hpp>

namespace kcShaders {

void MaterialBinder::bind(ShaderProgram& shader, const Material* material) {
    if (!material) {
        // Set default material
        shader.setVec3("material.albedo", glm::vec3(0.8f));
        shader.setFloat("material.metallic", 0.0f);
        shader.setFloat("material.roughness", 0.5f);
        shader.setFloat("material.ao", 1.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));
        shader.setFloat("material.emissiveStrength", 0.0f);
        shader.setFloat("material.opacity", 1.0f);
        
        // Set "no texture" flags
        shader.setBool("hasAlbedoMap", false);
        shader.setBool("hasMetallicMap", false);
        shader.setBool("hasRoughnessMap", false);
        shader.setBool("hasNormalMap", false);
        shader.setBool("hasAOMap", false);
        shader.setBool("hasEmissiveMap", false);
        return;
    }
    
    // Set material properties
    shader.setVec3("material.albedo", material->albedo);
    shader.setFloat("material.metallic", material->metallic);
    shader.setFloat("material.roughness", material->roughness);
    shader.setFloat("material.ao", material->ao);
    shader.setVec3("material.emissive", material->emissive);
    shader.setFloat("material.emissiveStrength", material->emissiveStrength);
    shader.setFloat("material.opacity", material->opacity);
    
    // Bind albedo texture
    if (material->albedoMap != 0) {
        glActiveTexture(GL_TEXTURE0 + TextureUnit::Albedo);
        glBindTexture(GL_TEXTURE_2D, material->albedoMap);
        shader.setInt("albedoMap", TextureUnit::Albedo);
        shader.setBool("hasAlbedoMap", true);
    } else {
        shader.setBool("hasAlbedoMap", false);
    }
    
    // Bind metallic texture
    if (material->metallicMap != 0) {
        glActiveTexture(GL_TEXTURE0 + TextureUnit::Metallic);
        glBindTexture(GL_TEXTURE_2D, material->metallicMap);
        shader.setInt("metallicMap", TextureUnit::Metallic);
        shader.setBool("hasMetallicMap", true);
    } else {
        shader.setBool("hasMetallicMap", false);
    }
    
    // Bind roughness texture
    if (material->roughnessMap != 0) {
        glActiveTexture(GL_TEXTURE0 + TextureUnit::Roughness);
        glBindTexture(GL_TEXTURE_2D, material->roughnessMap);
        shader.setInt("roughnessMap", TextureUnit::Roughness);
        shader.setBool("hasRoughnessMap", true);
    } else {
        shader.setBool("hasRoughnessMap", false);
    }
    
    // Bind normal map
    if (material->normalMap != 0) {
        glActiveTexture(GL_TEXTURE0 + TextureUnit::Normal);
        glBindTexture(GL_TEXTURE_2D, material->normalMap);
        shader.setInt("normalMap", TextureUnit::Normal);
        shader.setBool("hasNormalMap", true);
    } else {
        shader.setBool("hasNormalMap", false);
    }
    
    // Bind AO texture
    if (material->aoMap != 0) {
        glActiveTexture(GL_TEXTURE0 + TextureUnit::AO);
        glBindTexture(GL_TEXTURE_2D, material->aoMap);
        shader.setInt("aoMap", TextureUnit::AO);
        shader.setBool("hasAOMap", true);
    } else {
        shader.setBool("hasAOMap", false);
    }
    
    // Bind emissive texture
    if (material->emissiveMap != 0) {
        glActiveTexture(GL_TEXTURE0 + TextureUnit::Emissive);
        glBindTexture(GL_TEXTURE_2D, material->emissiveMap);
        shader.setInt("emissiveMap", TextureUnit::Emissive);
        shader.setBool("hasEmissiveMap", true);
    } else {
        shader.setBool("hasEmissiveMap", false);
    }
}

void MaterialBinder::unbindTextures() {
    for (int i = 0; i <= TextureUnit::Emissive; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0); // Reset to default
}

} // namespace kcShaders
