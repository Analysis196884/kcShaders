void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = fragCoord;
    vec2 center = iResolution.xy*0.5;
    
    float dist = sqrt(pow((pos.x-center.x),2.0) + pow((pos.y-center.y),2.0));
    
    float radius = mod(iTime, 3.0) * 200.0;
    
    vec3 color;
    
    if (dist>radius && dist<radius*1.1)
        color = vec3(0.5,1.0,0.5);

    fragColor = vec4(color,1.0);
}