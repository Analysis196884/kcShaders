// =================================================================
//  Liquid Metal (Metaballs)
//  Concept: Signed Distance Fields (SDF) & Smooth Blending
// =================================================================

// 1. Smooth Minimum Function ( The Magic Sauce )
// This mixes two distances (d1, d2) smoothly.
// 'k' controls how "gooey" or liquid the blend is.
float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

// 2. Rotation Helper
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// 3. Map Function (Defines the Scene)
float map(vec3 p) {
    // --- Center Sphere ---
    // p: current point, 1.0: radius
    float sphereMain = length(p) - 1.0;
    
    // --- Orbiting Sphere 1 ---
    // Move position based on time
    vec3 p1 = p - vec3(sin(iTime)*1.5, cos(iTime)*1.5, sin(iTime)*0.5);
    float sphereSmall1 = length(p1) - 0.7;
    
    // --- Orbiting Sphere 2 ---
    // Move in a different axis/speed
    vec3 p2 = p - vec3(sin(iTime*1.2 + 2.0)*1.8, 0.0, cos(iTime*1.5)*1.8);
    float sphereSmall2 = length(p2) - 0.6;
    
    // --- Blending ---
    // Normally we use min(a, b) for hard union.
    // smin(a, b, k) creates the liquid effect.
    float d = smin(sphereMain, sphereSmall1, 0.2); // Blend main with small1
    d = smin(d, sphereSmall2, 0.2); // Blend result with small2
    
    return d;
}

// 4. Normal Calculation
// To light an object, we need to know which way the surface faces (Normal).
// In Ray Marching, we calculate this by sampling points slightly around the surface.
vec3 GetNormal(vec3 p) {
    float d = map(p);
    vec2 e = vec2(0.001, 0.0); // Small offset
    
    vec3 n = d - vec3(
        map(p - e.xyy),
        map(p - e.yxy),
        map(p - e.yyx)
    );
    return normalize(n);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalize UV
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    // --- Camera Setup ---
    vec3 ro = vec3(0.0, 0.0, -4.0); // Ray Origin (Camera is back 4 units)
    vec3 rd = normalize(vec3(uv, 1.0)); // Ray Direction
    
    // Slight mouse interaction (rotate camera)
    // if (iMouse.z > 0.0) {
    //    ro.yz *= rot((iMouse.y/iResolution.y - 0.5) * 3.0);
    //    ro.xz *= rot((iMouse.x/iResolution.x - 0.5) * 3.0);
    //    rd.yz *= rot((iMouse.y/iResolution.y - 0.5) * 3.0);
    //    rd.xz *= rot((iMouse.x/iResolution.x - 0.5) * 3.0);
    // }

    // --- Ray Marching Loop ---
    float t = 0.0; // Distance traveled
    float d = 0.0; // Distance to nearest object
    
    for(int i = 0; i < 80; i++) {
        vec3 p = ro + rd * t;
        d = map(p);
        t += d;
        // Optimization: Break if too close (hit) or too far (miss)
        if(d < 0.001 || t > 100.0) break;
    }

    // --- Lighting & Coloring ---
    vec3 col = vec3(0.0); // Background color (Black)
    
    // If we hit something (distance d is very small)
    if(d < 0.001) {
        vec3 p = ro + rd * t; // Position of the hit
        vec3 n = GetNormal(p); // Surface normal
        vec3 lightPos = vec3(2.0, 4.0, -3.0); // Light position
        vec3 l = normalize(lightPos - p); // Light direction
        
        // 1. Diffuse Light (Basic shading)
        float dif = clamp(dot(n, l), 0.0, 1.0);
        
        // 2. Specular Light (Shiny highlights)
        vec3 ref = reflect(-l, n); // Reflection vector
        float spec = pow(max(dot(ref, -rd), 0.0), 32.0); // 32.0 = shininess
        
        // 3. Environment Reflection (Fake "Chrome" look)
        // We reflect the background color based on normal
        vec3 env = vec3(0.1, 0.3, 0.5) * n.y // Blue from top
                 + vec3(0.2, 0.1, 0.1) * (1.0 - n.y); // Reddish from bottom
        
        // Combine Light
        vec3 materialColor = vec3(0.2, 0.5, 0.8); // Base blueish metal
        col = materialColor * dif + vec3(0.8) * spec + env * 0.5;
        
        // Add Fresnel (Edges get brighter)
        float fresnel = pow(1.0 + dot(rd, n), 4.0);
        col += vec3(0.5, 0.8, 1.0) * fresnel;
    }
    
    // Gamma Correction
    col = pow(col, vec3(0.4545));

    fragColor = vec4(col, 1.0);
}