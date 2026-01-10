#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangent;
in vec3 Bitangent;

out vec4 FragColor;

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

// Light structure definitions
struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float innerConeAngle;
    float outerConeAngle;
    float constant;
    float linear;
    float quadratic;
};

uniform Material material;
uniform vec3 viewPos;

// Texture samplers (0 = no texture)
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

// Light arrays
#define MAX_DIR_LIGHTS 4
#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS 4

uniform int numDirLights;
uniform int numPointLights;
uniform int numSpotLights;

uniform DirectionalLight dirLights[MAX_DIR_LIGHTS];
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

uniform vec3 ambientLight;

const float PI = 3.14159265359;

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / max(denom, 0.0001);
}

// Geometry function (Smith's method)
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, 0.0001);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Calculate lighting contribution for a single light
vec3 calculateLighting(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 F0, float roughness, float metallic, vec3 albedo)
{
    vec3 H = normalize(V + L);
    
    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// Calculate directional light contribution
vec3 calcDirectionalLight(DirectionalLight light, vec3 N, vec3 V, vec3 F0, float roughness, float metallic, vec3 albedo)
{
    vec3 L = normalize(-light.direction);
    vec3 radiance = light.color * light.intensity;
    return calculateLighting(L, radiance, N, V, F0, roughness, metallic, albedo);
}

// Calculate point light contribution
vec3 calcPointLight(PointLight light, vec3 N, vec3 V, vec3 F0, float roughness, float metallic, vec3 albedo)
{
    vec3 L = normalize(light.position - FragPos);
    float dist = length(light.position - FragPos);
    
    // Distance attenuation
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    
    // Radius cutoff for smooth falloff
    if (light.radius > 0.0) {
        float smoothFactor = 1.0 - smoothstep(light.radius * 0.8, light.radius, dist);
        attenuation *= smoothFactor;
    }
    
    vec3 radiance = light.color * light.intensity * attenuation;
    return calculateLighting(L, radiance, N, V, F0, roughness, metallic, albedo);
}

// Calculate spot light contribution
vec3 calcSpotLight(SpotLight light, vec3 N, vec3 V, vec3 F0, float roughness, float metallic, vec3 albedo)
{
    vec3 L = normalize(light.position - FragPos);
    float dist = length(light.position - FragPos);
    
    // Distance attenuation
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    
    // Spot cone attenuation
    vec3 lightDir = normalize(light.direction);
    float theta = dot(L, -lightDir);
    float innerCos = cos(radians(light.innerConeAngle));
    float outerCos = cos(radians(light.outerConeAngle));
    float epsilon = innerCos - outerCos;
    float spotIntensity = clamp((theta - outerCos) / epsilon, 0.0, 1.0);
    
    attenuation *= spotIntensity;
    
    vec3 radiance = light.color * light.intensity * attenuation;
    return calculateLighting(L, radiance, N, V, F0, roughness, metallic, albedo);
}

// Normal mapping helper function
vec3 applyNormalMapping(vec3 N, vec3 normalSample)
{
    // Normalize the sampled normal (from [0,1] to [-1,1])
    normalSample = normalize(normalSample * 2.0 - 1.0);
    
    // Normalize the T, B, N vectors received from vertex shader
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);
    vec3 Nlocal = normalize(Normal);
    
    // Construct TBN matrix (transforms from tangent space to world space)
    mat3 TBN = mat3(T, B, Nlocal);
    
    // Transform normal from tangent space to world space
    vec3 mappedNormal = normalize(TBN * normalSample);
    
    return mappedNormal;
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    // Apply normal mapping if available
    if (hasNormalMap) {
        vec3 normalSample = texture(normalMap, TexCoord).rgb;
        N = applyNormalMapping(N, normalSample);
    }
    
    // Sample textures and override material properties
    vec3 albedo = material.albedo;
    float metallic = material.metallic;
    float roughness = material.roughness;
    float ao = material.ao;
    vec3 emissive = material.emissive;
    float emissiveStrength = material.emissiveStrength;
    
    // Albedo texture (RGB)
    if (hasAlbedoMap) {
        vec4 albedoSample = texture(albedoMap, TexCoord);
        albedo = albedoSample.rgb;
    }
    
    // Metallic texture (R channel, grayscale)
    if (hasMetallicMap) {
        vec4 metallicSample = texture(metallicMap, TexCoord);
        metallic = metallicSample.r;
    }
    
    // Roughness texture (R channel, grayscale)
    if (hasRoughnessMap) {
        vec4 roughnessSample = texture(roughnessMap, TexCoord);
        roughness = roughnessSample.r;
    }
    
    // AO texture (R channel, grayscale)
    if (hasAOMap) {
        vec4 aoSample = texture(aoMap, TexCoord);
        ao = aoSample.r;
    }
    
    // Emissive texture (RGB)
    if (hasEmissiveMap) {
        vec4 emissiveSample = texture(emissiveMap, TexCoord);
        emissive = emissiveSample.rgb;
    }
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    
    // Accumulate lighting from all sources
    vec3 Lo = vec3(0.0);
    
    // Directional lights
    for (int i = 0; i < numDirLights && i < MAX_DIR_LIGHTS; i++) {
        Lo += calcDirectionalLight(dirLights[i], N, V, F0, roughness, metallic, albedo);
    }
    
    // Point lights
    for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; i++) {
        Lo += calcPointLight(pointLights[i], N, V, F0, roughness, metallic, albedo);
    }
    
    // Spot lights
    for (int i = 0; i < numSpotLights && i < MAX_SPOT_LIGHTS; i++) {
        Lo += calcSpotLight(spotLights[i], N, V, F0, roughness, metallic, albedo);
    }
    
    // Ambient lighting
    vec3 ambient = ambientLight * albedo * ao;
    
    // Emissive is already sampled above
    vec3 finalEmissive = emissive * emissiveStrength;
    
    vec3 color = ambient + Lo + finalEmissive;
    
    // HDR tonemapping (simple Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, material.opacity);
}
