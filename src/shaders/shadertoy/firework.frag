// =================================================================
//  Cinematic New Year Fireworks (Pure Sky Version)
//  Features: Physics-based particles & Dreamy Glow
//  Removed: Water Reflection & Horizon
// =================================================================

#define NUM_PARTICLES 50.0 // Number of particles per firework
#define NUM_EXPLOSIONS 5.0 // Number of simultaneous fireworks

// Helper: Generate random vector 2D based on a float
vec2 Hash12(float t) {
    float x = fract(sin(t * 674.3) * 453.2);
    float y = fract(sin(t * 2654.2) * 267.3);
    return vec2(x, y);
}

// Helper: Generate random number 0.0 - 1.0
float Hash11(float t) {
    return fract(sin(t * 1234.5) * 5678.9);
}

// Helper: Palette function for beautiful colors
vec3 GetColor(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263, 0.416, 0.557);
    return a + b * cos(6.28318 * (c * t + d));
}

// Function to render a single firework explosion
vec3 Firework(vec2 uv, float t, float seed) {
    
    vec3 col = vec3(0.0);
    
    // Friction mimics air resistance (particles slow down)
    float friction = pow(0.001, t);
    
    // Randomize explosion characteristics based on seed
    float brightness = 0.0;
    vec3 color = GetColor(seed);
    
    // Loop through particles for this specific firework
    for(float i = 0.0; i < NUM_PARTICLES; i++) {
        
        // Generate random direction for this particle
        vec2 dir = Hash12(i + seed * 100.0) - 0.5; // -0.5 to 0.5
        dir = normalize(dir); // Make it a unit vector
        
        // Random spread speed
        float speed = (Hash11(i * 33.0) * 0.5 + 0.5) * 0.4;
        
        // 1. Calculate Particle Position
        vec2 particlePos = dir * t * speed; // Expanding out
        
        // 2. Apply Gravity (y = v*t - 0.5*g*t^2)
        particlePos.y -= 0.2 * t * t; 
        
        // 3. Draw the particle (Light intensity)
        // Distance from current pixel to particle
        float d = length(uv - particlePos);
        
        // Physics-based light decay (Glow effect)
        // Intensity is inversely proportional to distance (1/d)
        float sparkle = (0.0005 / d); 
        
        // Twinkle effect (particles flash)
        float twinkle = sin(t * 20.0 + i) * 0.5 + 0.5;
        
        // Fade out at the end
        float lifeFade = smoothstep(1.0, 0.7, t); 
        
        brightness += sparkle * twinkle * lifeFade;
    }
    
    return color * brightness;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // 1. Normalize UV
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
    // Optional: Shift camera slightly to center the fireworks better
    // since we removed the water at the bottom
    uv.y -= 0.05; 
    
    // 2. Setup Background (Deep Night Sky)
    vec3 col = vec3(0.02, 0.02, 0.05);
    col -= uv.y * 0.1; // Vertical gradient (darker at top, lighter at bottom)
    
    // 3. Render Multiple Fireworks
    for(float i = 0.0; i < NUM_EXPLOSIONS; i++) {
        
        // Create a time offset for each explosion
        float t = iTime + i * (10.0 / NUM_EXPLOSIONS);
        
        float id = floor(t); 
        float localTime = fract(t);
        
        // Generate a random position
        vec2 offset = Hash12(id + i * 43.2) - 0.5;
        offset.x *= 1.5; // Spread horizontally
        
        // Adjusted height range since we don't have water anymore
        offset.y = offset.y * 0.5 + 0.1; 
        
        // Render the firework
        vec3 explosion = Firework(uv - offset, localTime, id + i);
        
        // Accumulate the light
        col += explosion;
    }

    // 4. Gamma correction
    col = pow(col, vec3(0.8)); 

    fragColor = vec4(col, 1.0);
}