#version 330 core

in vec2 TexCoord;
out float FragColor;

// G-Buffer inputs
uniform sampler2D gPosition;    // World-space position
uniform sampler2D gNormal;      // World-space normal
uniform sampler2D texNoise;     // Random rotation texture

// Sample kernel
uniform vec3 samples[64];       // Maximum 64 samples
uniform int kernelSize;

// Matrices
uniform mat4 projection;
uniform mat4 view;

// SSAO parameters
uniform float radius;           // Sampling radius
uniform float bias;             // Prevents self-occlusion artifacts
uniform float power;            // Power curve for darkening
uniform vec2 noiseScale;        // Screen dimensions / 4

void main()
{   
    // Get world-space position and normal from G-Buffer
    vec3 fragPosWorld = texture(gPosition, TexCoord).xyz;
    vec3 normalWorld = normalize(texture(gNormal, TexCoord).xyz);
    
    // Convert to view space
    vec3 fragPos = (view * vec4(fragPosWorld, 1.0)).xyz;
    vec3 normal = normalize(mat3(view) * normalWorld);
    
    // Get random rotation vector from noise texture (tiled)
    vec3 randomVec = normalize(texture(texNoise, TexCoord * noiseScale).xyz);
    
    // Create TBN matrix to transform samples from tangent to view space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // Sample points around fragment
    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i)
    {
        // Get sample position in view space
        vec3 samplePos = TBN * samples[i];     // Tangent to view space
        samplePos = fragPos + samplePos * radius;
        
        // Project sample position to screen space for sampling depth
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;           // View space to clip space
        offset.xyz /= offset.w;                 // Perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5;    // Transform to [0, 1] range
        
        // Get sample depth from G-Buffer (world space) and convert to view space
        vec3 samplePosWorld = texture(gPosition, offset.xy).xyz;
        float sampleDepth = (view * vec4(samplePosWorld, 1.0)).z;
        
        // Range check & accumulate occlusion
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        
        // If sample is behind the surface, it occludes
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    // Normalize and invert (1.0 = no occlusion, 0.0 = full occlusion)
    occlusion = 1.0 - (occlusion / float(kernelSize));
    
    // Apply power curve for artistic control
    FragColor = pow(occlusion, power);
    // FragColor = 0.5;
}
