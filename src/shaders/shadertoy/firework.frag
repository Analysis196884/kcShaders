// Fireworks: center→many-particles dispersal (Shadertoy)
// Paste into Shadertoy fragment shader (mainImage).
// Uniforms: iResolution, iTime
#ifdef GL_ES
precision highp float;
#endif

// ----------------- helpers -----------------
float hash1(vec2 p){
    return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453123);
}
vec2 hash2(vec2 p){
    return vec2(hash1(p), hash1(p + 41.7));
}
float rnd(float x){ return fract(sin(x)*43758.5453); }
vec2 rotate(vec2 v, float a){ float c=cos(a), s=sin(a); return vec2(v.x*c - v.y*s, v.x*s + v.y*c); }
vec3 palette(float t){
    return 0.5 + 0.5 * cos(6.28318 * (vec3(0.0,0.33,0.66) + t) + vec3(0.0,0.2,0.4));
}
vec3 tonemap(vec3 c){ return 1.0 - exp(-c); }

// screen coords centered and aspect-corrected
vec2 screenUV(vec2 fragCoord){
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 p = (uv - 0.5) * vec2(iResolution.x / iResolution.y, 1.0) * 2.0;
    return p;
}

// soft disc
float softDisc(vec2 p, float r){
    return smoothstep(r, r - 0.01, length(p));
}

// ----------------- params -----------------
const int FIREWORKS = 5;      // concurrent launchers
const int SPARKS = 96;        // particles per explosion (tune for perf)
const float GRAVITY = -0.9;
const float ROCKET_LIFETIME = 1.0;
const float SPARK_LIFETIME = 3.0;
const float LAUNCH_INTERVAL = 1.2;

// ----------------- main -----------------
void mainImage(out vec4 fragColor, in vec2 fragCoord){
    vec2 p = screenUV(fragCoord);
    float t = iTime;

    // night sky base
    vec3 sky = vec3(0.02, 0.02, 0.04);

    // faint static starfield
    {
        float s = 0.0;
        for(int y=-3;y<=3;y++){
            for(int x=-6;x<=6;x++){
                vec2 id = vec2(float(x), float(y));
                float h = hash1(id*13.7);
                vec2 pos = (id + 0.5) / vec2(13.0, 7.0);
                pos = (pos - 0.5) * vec2(iResolution.x/iResolution.y, 1.0) * 2.0;
                float d = length(p - pos);
                s += smoothstep(0.015 * (0.4 + 0.6*h), 0.0, d) * pow(h, 3.0);
            }
        }
        sky += vec3(0.7,0.8,1.0) * s * 0.04;
    }

    vec3 accum = vec3(0.0);

    // For each firework emitter
    for(int fw=0; fw<FIREWORKS; fw++){
        float fi = float(fw);
        // staggered launch times with small jitter
        float base = fi * 0.35;
        float cycles = floor((t - base) / LAUNCH_INTERVAL);
        float launchTime = base + cycles * LAUNCH_INTERVAL + (hash1(vec2(fi, 9.1)) - 0.5) * 0.15;
        if(launchTime < 0.0) launchTime = base;

        float u = t - launchTime; // time since this cycle's launch

        // random launch position along bottom half
        vec2 seed2 = hash2(vec2(fi, cycles));
        vec2 launchPos = vec2((seed2.x - 0.5) * 1.6, -1.15 + seed2.y * 0.25);

        // rocket ascent -> draw rocket and trail
        float rocketLifetime = ROCKET_LIFETIME * (0.8 + 0.5 * hash1(vec2(fi,7.0)));
        float vUp = 1.9 + 0.6 * hash1(vec2(fi,5.2));
        if(u > 0.0 && u < rocketLifetime){
            float wind = 0.35 * sin((t + fi)*0.6);
            vec2 rocketPos = launchPos + vec2(wind * 0.4, vUp * u + 0.5 * GRAVITY * u * u);
            float d = length(p - rocketPos);
            accum += palette(fract(hash1(vec2(fi,3.0)))) * smoothstep(0.08, 0.0, d) * 0.9;
            // short trail
            for(int k=1;k<=5;k++){
                float du = float(k) * 0.05;
                float tu = u - du;
                if(tu <= 0.0) break;
                vec2 prev = launchPos + vec2(wind * 0.4, vUp * tu + 0.5 * GRAVITY * tu * tu);
                float td = length(p - prev);
                accum += vec3(1.0,0.9,0.7) * smoothstep(0.06, 0.0, td) * (0.14/float(k));
            }
        }

        // explosion -> spawn many particles that truly disperse from center
        if(u >= rocketLifetime && u < rocketLifetime + SPARK_LIFETIME){
            float eu = u - rocketLifetime;
            // explosion center: final rocket pos
            float wind = 0.35 * sin((launchTime + rocketLifetime + fi)*0.6);
            vec2 center = launchPos + vec2(wind * 0.4, vUp * rocketLifetime + 0.5 * GRAVITY * rocketLifetime * rocketLifetime);

            // overall color for this explosion
            float cseed = hash1(vec2(fi*2.1, 3.3));
            vec3 baseColor = palette(cseed);

            // add a ring/shock at early time
            float ringT = smoothstep(0.0, 0.6, eu) * (1.0 - eu/SPARK_LIFETIME);
            float ringRadius = eu * 2.6; // controls ring expansion speed
            float ring = smoothstep(ringRadius + 0.03, ringRadius, length(p - center)) * 1.2 * ringT;
            accum += baseColor * ring * 0.9;

            // smoke/puff (soft low-frequency)
            float smoke = exp(-length(p - center)*4.0) * smoothstep(0.0, 1.0, 1.0 - eu/SPARK_LIFETIME) * 0.25;
            accum += vec3(0.12,0.08,0.05) * smoke;

            // PARTICLES: true dispersal from center with branching & clustering
            for(int s=0; s<SPARKS; s++){
                float si = float(s);
                // deterministic randomness per spark
                float a = fract(hash1(vec2(fi, si*1.17)))*6.2831853;             // angle
                float rnorm = fract(hash1(vec2(fi, si*3.33)));                  // 0..1
                // bias radial distribution so many particles near center initially but spread
                float radialBias = pow(rnorm, 0.55);                            // controls density
                // base speed: wide range
                float minSpd = 0.7;
                float maxSpd = 3.2;
                float spd = mix(minSpd, maxSpd, radialBias);

                // add a small tangential (rotational) component to create petals/arms
                float tangential = 0.25 * (hash1(vec2(fi, si*4.4)) - 0.5);

                // allow some sparks to be "fragment" (split into secondary directions)
                float fragChance = hash1(vec2(fi, si*5.6));
                float fragAngle = (fract(hash1(vec2(fi, si*6.7))*2.0) - 1.0) * 1.2;

                // radial & tangential velocity vector
                vec2 dir = vec2(cos(a), sin(a));
                vec2 vel = dir * spd + rotate(dir, 1.5708) * tangential; // rotate 90deg for tangential

                // if fragment, modify vel (gives branching)
                if(fragChance > 0.82){
                    float f = smoothstep(0.82, 1.0, fragChance);
                    vel += rotate(dir, fragAngle) * (0.8 * f);
                    // also lengthen lifetime slightly for fragments
                }

                // introduce a small time-dependent spread (gives organic expansion)
                float wobble = 0.25 * sin( eu * (3.0 + 6.0 * hash1(vec2(fi, si*7.7))) + si * 0.5 );
                vel += rotate(dir, wobble) * 0.08;

                // particle position with gravity on y
                vec2 pos = center + vel * eu + vec2(0.0, 0.5 * GRAVITY * eu * eu);

                // particle size evolving: start small, grow slightly, then shrink
                float life = clamp(1.0 - eu / SPARK_LIFETIME, 0.0, 1.0);
                float size = 0.012 + 0.02 * (1.0 - life) * (0.6 + 0.8 * radialBias);

                // distance to pixel
                float d = length(p - pos);

                // core intensity and tail: sample backward along vel to produce streaks
                float core = smoothstep(size, 0.0, d) * (0.9 * life);
                float trail = 0.0;
                // 3 trail samples
                for(int tr=1; tr<=3; tr++){
                    float backT = float(tr) * 0.06;
                    vec2 backPos = pos - vel * backT;
                    float td = length(p - backPos);
                    trail += smoothstep(size*1.6 + 0.01*float(tr), 0.0, td) * (0.28 / float(tr)) * life;
                }

                // flicker / brightness variance
                float flick = 0.6 + 0.4 * sin( (eu + si*0.07) * (8.0 + 6.0*hash1(vec2(fi,si*2.2))) );

                // color shifting per particle (warmer centers, cooler edges)
                float cshift = hash1(vec2(fi, si*2.9));
                vec3 col = mix(baseColor, vec3(1.0,0.94,0.8), 0.3 + 0.7 * cshift);

                // distance attenuation + life
                float brightness = (core * 1.6 + trail * 1.2) * flick * life;
                accum += col * brightness * 0.95;

                // subtle glow around particles
                float glow = exp(-d*d*900.0*(0.6 + radialBias*0.8)) * life * 0.6;
                accum += col * glow * 0.7;
            }
        }
    }

    // slight vignette to keep focus
    float vig = 1.0 - smoothstep(1.2, 1.7, length(p));
    accum *= 1.0 + 0.12 * vig;

    // final composite and tone map
    vec3 finalCol = tonemap(sky + accum * 1.6);
    finalCol = pow(finalCol, vec3(0.9)); // gamma
    fragColor = vec4(finalCol, 1.0);
}
