// =================================================================
//  Cinematic Snowfall Shader
//  Based on Grid Tiling & Parallax Scrolling
// =================================================================

// Simple pseudo-random function
// Input: vec2 -> Output: float (0.0 - 1.0)
float N21(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

// Function to render a single layer of snow
// uv: normalized coordinates
// depth: how far away this layer is (0.0 = close/fast, 1.0 = far/slow)
float SnowLayer(vec2 uv, float depth) {
    // 1. Grid Setup
    // The further the layer (depth), the denser the grid (smaller flakes)
    float scale = mix(4.0, 20.0, depth); 
    
    // Create the grid coordinates and unique IDs
    vec2 uvGrid = fract(uv * scale) - 0.5;
    vec2 id = floor(uv * scale);
    
    // 2. Randomness
    // Get a random number for this specific grid cell
    float n = N21(id); 
    
    // 3. Position & Movement
    // Calculate the flake's position within the grid cell
    vec2 pos = vec2(0.0);
    
    // Horizontal variation (Random offset)
    pos.x = (n - 0.5) * 0.8;
    
    // Vertical variation (Random height offset to break lines)
    pos.y = (fract(n * 34.56) - 0.5) * 0.8;
    
    // Wind Flutter Simulation
    // Use sine waves to make the flake drift left/right slightly
    // We use the ID to make every flake flutter differently
    float wind = sin(iTime * 2.0 + id.x * 10.0) * 0.15; 
    pos.x += wind;
    
    // 4. Drawing the Flake
    float d = length(uvGrid - pos);
    
    // Size variation: Some flakes are tiny, some are fluffy
    float size = (0.03 + n * 0.04) * (1.0 - depth * 0.5);
    
    // Soft Glow Circle (Smoothstep for soft edges)
    // The divisor smooths the edge more for a "dreamy" look
    float shape = smoothstep(size, size * 0.2, d);
    
    // Brightness variation based on randomness
    return shape * (0.6 + 0.4 * n);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // 1. Normalize Coordinates (-0.5 to 0.5) and fix aspect ratio
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
    // 2. Background Gradient (Deep Winter Night)
    // Interpolate between dark blue-black and lighter midnight blue
    vec3 col = mix(vec3(0.02, 0.02, 0.1), vec3(0.05, 0.1, 0.25), uv.y + 0.5);
    
    // Add a Vignette (darkens corners) for focus
    float distFromCenter = length(uv);
    col *= smoothstep(1.2, 0.2, distFromCenter);

    // 3. Layer Loop (Parallax Effect)
    // We render multiple layers moving at different speeds
    float layers = 5.0; 
    float time = iTime * 0.3; // Global speed control
    
    for(float i = 0.0; i < 1.0; i += 1.0/layers) {
        // Calculate depth (0.0 to 1.0) cycling over time to create infinite fall
        float depth = fract(i + 0.1); // +0.1 to avoid sync issues
        
        // Movement Logic
        // Downward movement (y) and slight wind drift (x)
        vec2 layerUV = uv;
        layerUV.y += time * (1.0 - depth);  // Closer layers fall faster
        layerUV.x -= time * 0.1 * (1.0 - depth); // Slight constant wind to right
        
        // Apply rotation to the whole layer for more organic feel (optional)
        // float s = sin(iTime*0.01), c = cos(iTime*0.01);
        // layerUV *= mat2(c, -s, s, c);
        
        // Render the layer
        float snow = SnowLayer(layerUV, depth);
        
        // Accumulate Color
        // Fade out distant flakes (depth) to create fog effect
        float fade = smoothstep(1.0, 0.8, depth); 
        
        // Add snow color (slightly blueish white)
        col += snow * vec3(0.9, 0.95, 1.0) * fade;
    }
    
    // 4. Final Color Grading
    // Slight gamma correction for better contrast
    col = pow(col, vec3(0.9));

    fragColor = vec4(col, 1.0);
}