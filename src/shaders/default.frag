#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

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

uniform Material material;

// Light properties
uniform vec3 lightPos = vec3(0.0, 0.0, 2.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 viewPos;

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

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, material.albedo, material.metallic);
    
    // Light calculation
    vec3 L = normalize(lightPos - FragPos);
    vec3 H = normalize(V + L);
    float dist = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.01 * dist * dist);
    vec3 radiance = lightColor * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, material.roughness);
    float G = geometrySmith(N, V, L, material.roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * material.albedo / PI + specular) * radiance * NdotL;
    
    // Ambient lighting (simple approximation)
    vec3 ambient = vec3(0.03) * material.albedo * material.ao;
    
    // Emissive
    vec3 emissive = material.emissive * material.emissiveStrength;
    
    vec3 color = ambient + Lo + emissive;
    
    // HDR tonemapping (simple Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, material.opacity);
}
