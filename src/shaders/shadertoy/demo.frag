#version 330 core

out vec4 FragColor;

in vec2 vUV;

uniform vec3 iResolution;
uniform float iTime;

// Signed Distance Function for the box
float sdBox(vec3 p, vec3 b) // b: half size of the box
{
    vec3 d = abs(p) - b;
    return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

// rotation matrices
mat3 rotY(float a)
{
    float c = cos(a), s = sin(a);
    return mat3(
        c, 0.0, s,
        0.0, 1.0, 0.0,
       -s, 0.0, c
    );
}

mat3 rotX(float a)
{
    float c = cos(a), s = sin(a);
    return mat3(
        1.0, 0.0, 0.0,
        0.0, c, -s,
        0.0, s,  c
    );
}

// signed distance function for the scene
float map(vec3 p)
{
    // rotate the box over time
    p = rotY(iTime * 0.8) * rotX(iTime * 0.6) * p;

    return sdBox(p, vec3(0.5));
}

// normal calculation
vec3 calcNormal(vec3 p)
{
    float h = 0.0005;
    vec2 k = vec2(1, -1);
    return normalize(
        k.xyy * map(p + k.xyy * h) +
        k.yyx * map(p + k.yyx * h) +
        k.yxy * map(p + k.yxy * h) +
        k.xxx * map(p + k.xxx * h)
    );
}

void main()
{
    // screen coordinate to ray direction
    vec2 uv = (vUV * 2.0 - 1.0);
    uv.x *= iResolution.x / iResolution.y;

    vec3 ro = vec3(0.0, 0.0, 3.0); // camera position
    vec3 rd = normalize(vec3(uv, -1.5)); // ray direction

    // Ray marching
    float t = 0.0;
    float d;
    for (int i = 0; i < 100; i++)
    {
        vec3 p = ro + rd * t;
        d = map(p);
        if (d < 0.001) break;
        t += max(d, 0.001);
        if (t > 20.0) break;
    }

    if (t > 20.0)
    {
        FragColor = vec4(0.6, 0.8, 1.0, 1.0);
        return;
    }

    // hit record
    vec3 p = ro + rd * t;
    vec3 n = calcNormal(p);

    // simple lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(n, lightDir), 0.0);

    vec3 col = vec3(0.2, 0.6, 1.0) * diff + vec3(0.1);

    FragColor = vec4(col, 1.0);
}
