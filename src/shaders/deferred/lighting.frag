#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

// G-Buffer inputs (order matters for default binding!)
uniform sampler2D GPosition;    // Texture unit 0 - World-space position
uniform sampler2D GNormal;      // Texture unit 1 - World-space normal
uniform sampler2D GAlbedo;      // Texture unit 2 - Albedo
uniform sampler2D GMaterial;    // Texture unit 3 - Metallic, roughness, AO
uniform sampler2D GSSAO;        // Texture unit 4 - SSAO (optional)
uniform sampler2D shadowMap;    // Texture unit 5 - Shadow map (optional)

uniform vec3 viewPos;
uniform mat4 uView;
uniform int useSSAO;            // 1 if SSAO is enabled, 0 otherwise
uniform int useShadows;         // 1 if shadows are enabled, 0 otherwise
uniform mat4 lightSpaceMatrix;  // Transform to light space for shadow mapping

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
    
    return nom / max(denom, 0.0001);
}

// Geometry function (Schlick-GGX)
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
    vec3 specular = numerator / max(denominator, 0.0001);
    
    return (kD * albedo / PI + specular) * radiance * max(NdotL, 0.0);
}

// Shadow calculation with PCF (Percentage Closer Filtering)
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Outside shadow map bounds = no shadow
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }
    
    // Get closest depth value from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    // Debug: If closestDepth is 1.0 (border color), shadow map might not be rendering
    // if (closestDepth >= 0.9999) return 0.0;  // Uncomment to test
    
    // Calculate bias based on surface angle to light
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    // PCF (Percentage Closer Filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

// Unified lighting helper (matches forward shader behavior)
vec3 calculateLighting(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 F0, float roughness, float metallic, vec3 albedo)
{
    vec3 H = normalize(V + L);
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// Calculate attenuation
float calculateAttenuation(float dist, float constant, float linear, float quadratic)
{
    float attenuation = 1.0 / (constant + linear * dist + quadratic * dist * dist);
    return attenuation;
}

void main()
{
    // Sample G-Buffer
    vec3 FragPos = texture(GPosition, TexCoord).rgb;
    vec3 Normal = texture(GNormal, TexCoord).rgb;
    vec3 Albedo = texture(GAlbedo, TexCoord).rgb;
    vec4 MaterialData = texture(GMaterial, TexCoord);
    
    float metallic = MaterialData.r;
    float roughness = MaterialData.g;
    float ao = MaterialData.b;
    
    // Sample SSAO if enabled
    float ssao = 1.0;
    if (useSSAO == 1) {
        ssao = texture(GSSAO, TexCoord).r;
    }

    // Basic validity guard
    if (length(Normal) < 1e-4) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
    ao = clamp(ao, 0.0, 1.0);

    // Reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, Albedo, metallic);

    vec3 Lo = vec3(0.0);

    // Directional lights
    float shadow = 0.0;
    for (int i = 0; i < numDirLights && i < MAX_DIR_LIGHTS; ++i) {
        vec3 L = normalize(-dirLights[i].direction);
        vec3 radiance = dirLights[i].color * dirLights[i].intensity;
        
        // Calculate shadow if enabled (only for first directional light)
        shadow = 0.0;
        if (useShadows == 1) {
            vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
            shadow = calculateShadow(fragPosLightSpace, N, L);
        }
        
        Lo += calculateLighting(L, radiance, N, V, F0, roughness, metallic, Albedo) * (1.0 - shadow);
    }

    // Point lights
    for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; ++i) {
        vec3 Lvec = pointLights[i].position - FragPos;
        float dist = length(Lvec);
        vec3 L = Lvec / max(dist, 1e-4);

        float c = max(pointLights[i].constant, 0.0001);
        float l = max(pointLights[i].linear, 0.0);
        float q = max(pointLights[i].quadratic, 0.0);
        float attenuation = 1.0 / (c + l * dist + q * dist * dist);

        // Smooth radius falloff like forward shader
        if (pointLights[i].radius > 0.0) {
            float smoothFactor = 1.0 - smoothstep(pointLights[i].radius * 0.8, pointLights[i].radius, dist);
            attenuation *= smoothFactor;
        }

        attenuation = clamp(attenuation, 0.0, 1.0);
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;
        Lo += calculateLighting(L, radiance, N, V, F0, roughness, metallic, Albedo);
    }

    // Spot lights
    for (int i = 0; i < numSpotLights && i < MAX_SPOT_LIGHTS; ++i) {
        vec3 Lvec = spotLights[i].position - FragPos;
        float dist = length(Lvec);
        vec3 L = Lvec / max(dist, 1e-4);

        float c = max(spotLights[i].constant, 0.0001);
        float l = max(spotLights[i].linear, 0.0);
        float q = max(spotLights[i].quadratic, 0.0);
        float attenuation = 1.0 / (c + l * dist + q * dist * dist);

        // Spot cone attenuation (angles in degrees, match forward shader)
        float theta = dot(L, normalize(-spotLights[i].direction));
        float innerCos = cos(radians(spotLights[i].innerConeAngle));
        float outerCos = cos(radians(spotLights[i].outerConeAngle));
        float epsilon = innerCos - outerCos;
        float spotIntensity = clamp((theta - outerCos) / max(epsilon, 1e-4), 0.0, 1.0);

        attenuation *= spotIntensity;
        attenuation = clamp(attenuation, 0.0, 1.0);

        vec3 radiance = spotLights[i].color * spotLights[i].intensity * attenuation;
        Lo += calculateLighting(L, radiance, N, V, F0, roughness, metallic, Albedo);
    }

    // Apply SSAO to ambient term
    vec3 ambient = ambientLight * Albedo * ao * ssao;
    vec3 color = ambient + Lo;

    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
