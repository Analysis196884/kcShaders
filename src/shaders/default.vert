#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out mat3 TBN;

void main()
{
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    TexCoord = aTexCoord;
    
    // Compute simplified TBN matrix for normal mapping
    // For proper normal mapping, tangent/bitangent should be vertex attributes
    // For now, we use a simplified approximation
    vec3 N = normalize(Normal);
    
    // Create perpendicular vectors (simplified approach)
    vec3 T;
    if (abs(N.x) > 0.9) {
        T = normalize(cross(N, vec3(0.0, 1.0, 0.0)));
    } else {
        T = normalize(cross(N, vec3(1.0, 0.0, 0.0)));
    }
    vec3 B = cross(N, T);
    
    TBN = mat3(T, B, N);
    
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}
