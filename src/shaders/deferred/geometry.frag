#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangent;
in vec3 Bitangent;

// G-Buffer outputs
layout(location = 0) out vec4 GAlbedo;      // RGB: albedo, A: unused
layout(location = 1) out vec4 GNormal;      // RGB: normal (world space), A: unused
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

// Proper normal mapping using TBN
vec3 getNormal()
{
    vec3 n = normalize(Normal);
    if (!hasNormalMap) return n;
    
    vec3 t = normalize(Tangent);
    vec3 b = normalize(Bitangent);
    mat3 TBN = mat3(t, b, n);
    
    vec3 sampleN = texture(normalMap, TexCoord).rgb;
    sampleN = normalize(sampleN * 2.0 - 1.0);
    return normalize(TBN * sampleN);
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
        metallic = texture(metallicMap, TexCoord).g;
    }
    
    // Sample roughness
    float roughness = material.roughness;
    if (hasRoughnessMap) {
        roughness = texture(roughnessMap, TexCoord).g;
    }
    
    // Sample AO
    float ao = material.ao;
    if (hasAOMap) {
        ao = texture(aoMap, TexCoord).r;
    }
    
    // Get normal with normal mapping support
    vec3 normal = getNormal();
    
    // Store in G-Buffer (world space normal)
    GAlbedo = vec4(albedo, 1.0);
    GNormal = vec4(normalize(normal), 1.0);
    GPosition = vec4(FragPos, 1.0);
    GMaterial = vec4(metallic, roughness, ao, 1.0);
}
