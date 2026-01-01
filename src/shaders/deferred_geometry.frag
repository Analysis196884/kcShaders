#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// G-Buffer outputs
layout(location = 0) out vec4 GAlbedo;      // RGB: albedo, A: unused
layout(location = 1) out vec4 GNormal;      // RGB: normal (view space), A: unused
layout(location = 2) out vec4 GPosition;    // RGB: position (world space), A: unused
layout(location = 3) out vec4 GMaterial;    // R: metallic, G: roughness, B: AO, A: unused

// Material properties
struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 emissive;
    float emissiveStrength;
    float opacity;
};

uniform Material material;
uniform vec3 viewPos;

// Texture samplers
uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;
uniform sampler2D aoMap;
uniform sampler2D emissiveMap;

// Flags to indicate if textures are bound
uniform bool hasAlbedoMap;
uniform bool hasMetallicMap;
uniform bool hasRoughnessMap;
uniform bool hasNormalMap;
uniform bool hasAOMap;
uniform bool hasEmissiveMap;

uniform mat4 uView;

// Simple normal mapping (without full TBN - using approximation)
vec3 getNormal()
{
    vec3 normal = normalize(Normal);
    
    if (hasNormalMap) {
        // Sample normal from normal map
        vec3 normalMap_sample = texture(normalMap, TexCoord).rgb;
        
        // Convert from [0,1] to [-1,1]
        normalMap_sample = normalMap_sample * 2.0 - 1.0;
        
        // Simple approximation: blend between geometric normal and map normal
        // Full TBN would be needed for proper results
        normal = normalize(normal + normalMap_sample * 0.5);
    }
    
    return normal;
}

void main()
{
    // Sample albedo
    vec3 albedo = material.albedo;
    if (hasAlbedoMap) {
        albedo = texture(albedoMap, TexCoord).rgb;
    }
    
    // Sample metallic
    float metallic = material.metallic;
    if (hasMetallicMap) {
        metallic = texture(metallicMap, TexCoord).r;
    }
    
    // Sample roughness
    float roughness = material.roughness;
    if (hasRoughnessMap) {
        roughness = texture(roughnessMap, TexCoord).r;
    }
    
    // Sample AO
    float ao = material.ao;
    if (hasAOMap) {
        ao = texture(aoMap, TexCoord).r;
    }
    
    // Get normal with normal mapping support
    vec3 normal = getNormal();
    
    // Transform normal to view space
    vec3 viewNormal = mat3(uView) * normal;
    
    // Store in G-Buffer
    GAlbedo = vec4(albedo, 1.0);
    GNormal = vec4(viewNormal, 1.0);
    GPosition = vec4(FragPos, 1.0);
    GMaterial = vec4(metallic, roughness, ao, 1.0);
}
