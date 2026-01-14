#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 Tangent;
out vec3 Bitangent;

void main()
{
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    
    // Transform normal, tangent, and bitangent to world space
    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    Normal = normalize(normalMatrix * aNormal);
    Tangent = normalize(normalMatrix * aTangent);
    Bitangent = normalize(normalMatrix * aBitangent);
    
    TexCoord = aTexCoord;
    
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}
