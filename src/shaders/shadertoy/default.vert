#version 330 core

// output to fragment shader
out vec2 vUV;

void main()
{
    // full-screen triangle vertices as in shadertoy
    vec2 pos = vec2(
        (gl_VertexID == 2) ?  3.0 : -1.0,
        (gl_VertexID == 1) ?  3.0 : -1.0
    );

    vUV = pos * 0.5 + 0.5;   // transform from [-1,1] to [0,1]
    gl_Position = vec4(pos, 0.0, 1.0);
}
