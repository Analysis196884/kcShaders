// =================================================================
//  Mini Solar System Simulation
//  Technique: Ray Marching with Material IDs & Point Lighting
// =================================================================

// Rotation matrix helper
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// --- SDF Primitives ---
float sdSphere(vec3 p, float s) {
    return length(p) - s;
}

// Torus for Saturn's rings
// t.x = radius, t.y = thickness
float sdTorus(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

// --- Scene Mapping ---
// Returns vec2: x = distance, y = material ID
vec2 map(vec3 p) {
    vec2 res = vec2(100.0, 0.0); // Init result (dist, id)
    
    // 1. The Sun (ID 1.0)
    // Static at the center (0,0,0)
    float sun = sdSphere(p, 1.5); 
    if(sun < res.x) res = vec2(sun, 1.0);
    
    // Global Time for orbit speed
    float t = iTime;
    
    // 2. Mercury (ID 2.0)
    // Fast orbit, close to sun
    vec3 pMerc = p - vec3(cos(t * 1.5) * 2.5, 0.0, sin(t * 1.5) * 2.5);
    float merc = sdSphere(pMerc, 0.2);
    if(merc < res.x) res = vec2(merc, 2.0);
    
    // 3. Venus (ID 3.0)
    vec3 pVenus = p - vec3(cos(t * 1.1) * 3.5, 0.0, sin(t * 1.1) * 3.5);
    float venus = sdSphere(pVenus, 0.3);
    if(venus < res.x) res = vec2(venus, 3.0);
    
    // 4. Earth (ID 4.0)
    vec3 pEarth = p - vec3(cos(t * 0.8) * 5.0, 0.0, sin(t * 0.8) * 5.0);
    float earth = sdSphere(pEarth, 0.35);
    if(earth < res.x) res = vec2(earth, 4.0);
    
    // Moon (Orbiting Earth) - Just for fun, let's make it part of Earth ID for simplicity
    // or give it ID 4.1. Let's keep it simple.
    vec3 pMoon = pEarth - vec3(cos(t * 5.0) * 0.6, 0.1, sin(t * 5.0) * 0.6);
    float moon = sdSphere(pMoon, 0.1);
    // Combine Moon with Earth (min)
    if(moon < res.x) res = vec2(moon, 4.1);

    // 5. Mars (ID 5.0)
    vec3 pMars = p - vec3(cos(t * 0.5) * 6.5, 0.0, sin(t * 0.5) * 6.5);
    float mars = sdSphere(pMars, 0.28);
    if(mars < res.x) res = vec2(mars, 5.0);
    
    // 6. Jupiter (ID 6.0)
    // Big, slow
    vec3 pJup = p - vec3(cos(t * 0.3) * 8.0, 0.0, sin(t * 0.3) * 8.0);
    float jup = sdSphere(pJup, 0.9);
    if(jup < res.x) res = vec2(jup, 6.0);
    
    // 7. Saturn (ID 7.0) with Rings
    vec3 pSatOffset = vec3(cos(t * 0.2) * 10.0, 0.0, sin(t * 0.2) * 10.0);
    vec3 pSat = p - pSatOffset;
    
    // Tilt Saturn
    pSat.xy *= rot(0.4); 
    
    float satBody = sdSphere(pSat, 0.7);
    // Ring: Radius 1.2, Thickness 0.05
    float satRing = sdTorus(pSat, vec2(1.1, 0.05)); 
    
    // Combine Body and Ring
    float saturn = min(satBody, satRing);
    
    if(saturn < res.x) res = vec2(saturn, 7.0);

    return res;
}

// Calc Normal
vec3 calcNormal(vec3 p) {
    vec2 e = vec2(0.001, 0.0);
    return normalize(vec3(
        map(p + e.xyy).x - map(p - e.xyy).x,
        map(p + e.yxy).x - map(p - e.yxy).x,
        map(p + e.yyx).x - map(p - e.yyx).x
    ));
}

// Get Color based on Material ID
vec3 getMaterialColor(float id, vec3 p) {
    if(id < 1.5) return vec3(1.0, 0.9, 0.2); // Sun (Yellow/Orange)
    if(id < 2.5) return vec3(0.6, 0.6, 0.6); // Mercury (Grey)
    if(id < 3.5) return vec3(0.9, 0.7, 0.3); // Venus (Orange/White)
    if(id < 4.05) return vec3(0.1, 0.4, 0.8); // Earth (Blue)
    if(id < 4.5) return vec3(0.5, 0.5, 0.5); // Moon (Grey)
    if(id < 5.5) return vec3(0.8, 0.3, 0.1); // Mars (Red)
    if(id < 6.5) return vec3(0.8, 0.6, 0.4); // Jupiter (Beige/Brown)
    if(id < 7.5) return vec3(0.9, 0.8, 0.6); // Saturn (Pale Gold)
    return vec3(1.0);
}

// =================================================================
//  High-Quality Galaxy Background (Ported from your reference)
// =================================================================

#define DUST_OPACITY 0.15       
#define GLOW_INTENSITY 0.5
#define GALAXY_TILT 2.7       
#define GAL_PI 3.14159265359

// --- Helper Functions (Renamed with gal_ prefix to avoid conflicts) ---

vec2 gal_hash( vec2 p ) {
	p = vec2( dot(p,vec2(127.1,311.7)),
			  dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0 * fract(sin(p)*43758.5453123);
}

float gal_noise( in vec2 p ) {
    vec2 i = floor( p );
    vec2 f = fract( p );
	vec2 u = f*f*(3.0-2.0*f); 
    return mix( mix( dot( gal_hash( i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ), 
                     dot( gal_hash( i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
                mix( dot( gal_hash( i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ), 
                     dot( gal_hash( i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
}

float gal_fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 8.0;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * gal_noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

float gal_rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 gal_randColor(vec2 seed) {
    return vec3(gal_rand(seed + 13.1), gal_rand(seed + 37.7), gal_rand(seed + 91.3));
}

vec3 gal_renderStreak(float streakId, vec2 p, float time) {
    const float speed = 2.0;
    const float travelTime = 1.0 / speed;

    vec2 seed_id = vec2(streakId);
    float timeOffset = gal_rand(seed_id + 17.0) * 200.0;
    float cyclePeriod = travelTime + 0.5 + gal_rand(seed_id + 29.0) * 15.;

    float localTime = time + timeOffset;
    float timeInCycle = mod(localTime, cyclePeriod);

    if (timeInCycle > travelTime) {
        return vec3(0.0);
    }
    
    float cycleID = floor(localTime / cyclePeriod);
    vec2 run_seed = vec2(streakId, cycleID);
    
    float a = timeInCycle / travelTime;

    float ang = gal_rand(run_seed) * 6.283;
    vec2 dir  = vec2(cos(ang), sin(ang));
    vec2 offset = vec2(-dir.y, dir.x) * (gal_rand(run_seed * 9.8) * 2.0 - 1.0) * 0.8;
    
    vec2 center = dir * 1.4 * (a * 2.0 - 1.0) + offset;
    
    const float segLen = 0.2;
    float clampedProj = clamp(dot(p - center, dir), -segLen, segLen);
    float line = smoothstep(0.001, 0.0, length((p - center) - dir * clampedProj));

    float streakAlpha = line 
                      * (smoothstep(0.0, 0.15, a) * smoothstep(1.0, 0.85, a))
                      * ((clampedProj + segLen) / (2.0 * segLen));

    return gal_randColor(run_seed) * streakAlpha;
}

// --- MAIN STARS FUNCTION ---
// Modified to accept fragCoord for pixel-perfect stars
vec3 stars(vec2 uv, vec2 fragCoord) {
    
    // --- PART 1: GALAXY BACKGROUND ---

    // 1. ROTATION
    float angle = GALAXY_TILT;
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    vec2 rotated_uv = uv * rotation + vec2(0.5, 0.2); // Slight offset

    // 2. BASIC SHAPE
    float bandShape = pow(1.0 - abs(rotated_uv.y), 3.0) * 0.9;
    float coreGlow = 1.0 - smoothstep(0.0, 1.0, length(rotated_uv * vec2(0.5, 1.0)));
    coreGlow = pow(coreGlow, 5.0) * GLOW_INTENSITY;
    float milkyWay = bandShape + coreGlow;

    // 3. CLOUDY TEXTURES
    
    // Glowing Gas Clouds
    vec2 gas_uv = rotated_uv * 12.0 + vec2(123.45, 678.9); 
    float gasFBM = gal_fbm(gas_uv, 1);
    gasFBM = (gasFBM + 1.0) * 0.5; 
    milkyWay += gasFBM * bandShape * 0.5;
    
    // Dark Dust Lanes
    vec2 dust_uv = rotated_uv * 6.0 + vec2(456.7, 890.12);
    vec2 dust_distort = vec2(gal_fbm(dust_uv + 15.5, 4), gal_fbm(dust_uv + 33.3, 4)) * 0.3;
    float dustFBM = gal_fbm(dust_uv + dust_distort, 7);
    dustFBM = (dustFBM + 1.0) * 0.1;
    float dustMask = smoothstep(0.45, 0.7, dustFBM);

    // 4. COMBINE
    milkyWay *= (1.0 - dustMask * DUST_OPACITY);
    milkyWay = max(0.0, milkyWay);
    
    // 5. COLORING
    vec3 galaxyColor1 = 0.5 * vec3(0.1, 0.2, 0.4);
    vec3 galaxyColor2 = 0.5 * vec3(0.3, 0.4, 0.5);
    vec3 galaxyColor3 = 0.5 * vec3(0.3, 0.4, 0.5);
    
    vec3 galaxyBaseColor = mix(galaxyColor1, galaxyColor2, smoothstep(0.0, 0.1, milkyWay));
    galaxyBaseColor = mix(galaxyBaseColor, galaxyColor3, smoothstep(0.3, 0.9, milkyWay));
    
    vec3 skyColor = vec3(0.01, 0.02, 0.05); // Deep space background
    vec3 galaxyFinalColor = 0.15 * mix(skyColor, galaxyBaseColor, milkyWay);

    // --- PART 2: STARS & SHOOTING STARS ---
    
    vec3 starsAndStreaksColor = vec3(0.0);

    // Grid-based twinkling stars (Needs fragCoord!)
    float starSize = 15.0 * iResolution.y / 720.0;
    vec2 gridPos = floor(fragCoord / starSize);
    float starValue = gal_rand(gridPos);
    float prob = 0.99;
    
    if (starValue > prob) {
        vec2 center = starSize * gridPos + starSize * 0.5;
        float twinkleSpeed = 1.0 + gal_rand(gridPos + 42.0) * 4.0;
        float phaseOffset = (starValue - prob) / (1.0 - prob) * GAL_PI * 2.0;
        float t = 0.9 + 0.2 * sin(iTime * twinkleSpeed + phaseOffset);
        float d = distance(fragCoord, center);
        float base = max(0.0, 1.0 - d / (starSize * 0.5));
        base *= t*t / ((abs(fragCoord.y - center.y) + 0.5) * (abs(fragCoord.x - center.x) + 0.5));
        vec3 starTint = mix(vec3(2.0), gal_randColor(gridPos), gal_rand(gridPos + 123.4) * 0.7);
        starsAndStreaksColor += base * starTint;
    }
    // Random "salt and pepper" stars
    else if (gal_rand(fragCoord / iResolution.xy) > 0.996) {
        float r = gal_rand(fragCoord);
        float base = r * (0.25 * sin(iTime * (r * 5.0) + 720.0 * r) + 0.75);
        vec3 starTint = mix(vec3(1.0), gal_randColor(fragCoord), gal_rand(fragCoord * 0.13) * 0.8);
        starsAndStreaksColor += base * starTint;
    }

    // Shooting Streaks
    const int NUM_STREAKS = 5;
    for (int i = 0; i < NUM_STREAKS; i++) {
        starsAndStreaksColor += gal_renderStreak(float(i), uv, iTime);
    }
    
    return galaxyFinalColor + starsAndStreaksColor;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    // --- Camera Setup ---
    // Looking down from an angle to see orbits
    vec3 ro = vec3(0.0, 8.0, -20.0); 
    vec3 rd = normalize(vec3(uv.x, uv.y - 0.4, 1.0)); // Tilted camera
    
    // Background (Stars)
    vec3 col = stars(uv, fragCoord);

    // --- Ray Marching ---
    float t = 0.0;
    float id = 0.0;
    float d = 0.0;
    
    for(int i = 0; i < 100; i++) {
        vec3 p = ro + rd * t;
        vec2 res = map(p);
        d = res.x;
        id = res.y;
        
        t += d;
        if(d < 0.01 || t > 50.0) break;
    }
    
    // --- Lighting ---
    if(d < 0.01) {
        vec3 p = ro + rd * t;
        vec3 n = calcNormal(p);
        vec3 materialCol = getMaterialColor(id, p);
        
        // SUN LOGIC (ID 1.0)
        if(id < 1.5) {
            // Sun is self-illuminated
            col = materialCol;
            // Add a bit of surface variation for the sun
            col += sin(p.x*10.0 + iTime)*0.1; 
        } 
        else {
            // PLANET LOGIC
            // Light comes from the Sun (0,0,0)
            vec3 lightPos = vec3(0.0, 0.0, 0.0);
            vec3 l = normalize(lightPos - p);
            
            // Diffuse lighting (N dot L)
            // Clamp ensures the dark side is black
            float dif = clamp(dot(n, l), 0.05, 1.0); // 0.05 for slight ambient
            
            // Shadows? 
            // Ray March towards the sun to check for shadows is expensive, 
            // but we can fake "Self-Shadowing" easily with the dot product.
            
            col = materialCol * dif;
            
            // Rim lighting (Atmosphere effect)
            float fresnel = pow(1.0 + dot(rd, n), 3.0);
            if(id > 3.9 && id < 4.1) { // Earth Atmosphere
                col += vec3(0.4, 0.6, 1.0) * fresnel * 0.5;
            }
        }
    }
    
    // --- Post Processing ---
    
    // Sun Glow (Bloom)
    // Even if we missed the sun, looking near center adds glow
    // We calculate screen-space distance to sun center (projected) or simpler:
    // Just map the sun again to get distance field glow
    // But simplistic way:
    vec2 sunPosScreen = vec2(0.0, 0.3); // Rough approximation for this camera angle
    // Better: Accumulate glow based on SDF
    // If t is large (missed), we can still use the last 'd' to estimate proximity?
    // Let's just use a simple center glow for visual flair
    // (Since sun is at 0,0,0 and camera looks at it)
    
    // Glow trick from Ray Marching loop:
    // This is a cheap way to add glow based on how close rays passed to objects
    // Note: To do this properly requires modifying the loop, but here is a simple overlay:
    col += vec3(1.0, 0.8, 0.5) * exp(-length(uv - vec2(0.0, 0.0)) * 10.0) * 0.1;

    // Gamma correction
    col = pow(col, vec3(0.4545));

    fragColor = vec4(col, 1.0);
}