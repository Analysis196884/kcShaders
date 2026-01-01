#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

// G-Buffer inputs
uniform sampler2D GAlbedo;      // Albedo
uniform sampler2D GNormal;      // View-space normal
uniform sampler2D GPosition;    // World-space position
uniform sampler2D GMaterial;    // Metallic, roughness, AO

uniform vec3 viewPos;
uniform mat4 uView;

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
    
    return nom / max(denom, 0.0000001);
}

// Geometry function (Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, 0.0000001);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// PBR calculation
vec3 calculatePBR(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness)
{
    vec3 H = normalize(V + L);
    
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    
    // Calculate F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL;
    vec3 specular = numerator / max(denominator, 0.001);
    
    return (kD * albedo / PI + specular) * radiance * max(NdotL, 0.0);
}

// Calculate attenuation
float calculateAttenuation(float distance, float constant, float linear, float quadratic)
{
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);
    return attenuation;
}

void main()
{
    // Read from G-Buffer
    vec3 albedo = texture(GAlbedo, TexCoord).rgb;
    vec3 normal = normalize(texture(GNormal, TexCoord).rgb);
    vec3 position = texture(GPosition, TexCoord).rgb;
    vec4 material = texture(GMaterial, TexCoord);
    float metallic = material.r;
    float roughness = max(material.g, 0.05);  // Clamp roughness
    float ao = material.b;
    
    // Calculate view direction
    vec3 V = normalize(viewPos - position);
    
    vec3 Lo = vec3(0.0);
    
    // Directional lights
    for (int i = 0; i < numDirLights; ++i) {
        vec3 L = normalize(-dirLights[i].direction);
        vec3 radiance = dirLights[i].color * dirLights[i].intensity;
        
        Lo += calculatePBR(normal, V, L, radiance, albedo, metallic, roughness);
    }
    
    // Point lights
    for (int i = 0; i < numPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position - position);
        float distance = length(pointLights[i].position - position);
        float attenuation = calculateAttenuation(distance, pointLights[i].constant, pointLights[i].linear, pointLights[i].quadratic);
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;
        
        Lo += calculatePBR(normal, V, L, radiance, albedo, metallic, roughness);
    }
    
    // Spot lights
    for (int i = 0; i < numSpotLights; ++i) {
        vec3 L = normalize(spotLights[i].position - position);
        float distance = length(spotLights[i].position - position);
        float attenuation = calculateAttenuation(distance, spotLights[i].constant, spotLights[i].linear, spotLights[i].quadratic);
        
        float theta = dot(L, normalize(-spotLights[i].direction));
        float innerAngle = radians(spotLights[i].innerConeAngle);
        float outerAngle = radians(spotLights[i].outerConeAngle);
        float epsilon = cos(innerAngle) - cos(outerAngle);
        float intensity = clamp((theta - cos(outerAngle)) / epsilon, 0.0, 1.0);
        
        vec3 radiance = spotLights[i].color * spotLights[i].intensity * attenuation * intensity;
        
        Lo += calculatePBR(normal, V, L, radiance, albedo, metallic, roughness);
    }
    
    // Ambient lighting
    vec3 ambient = ambientLight * albedo * ao;
    
    vec3 color = ambient + Lo;
    
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
}
