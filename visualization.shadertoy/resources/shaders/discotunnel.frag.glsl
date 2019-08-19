// Taken from https://www.shadertoy.com/view/lsBSRR

float tex(vec2 uv, float s)
{    
    float col;
    float ntiles = 10.0;
    
    // Calculate the shape of the LEDs
    vec2 r = mod(uv * ntiles, 1.0) - vec2(0.5);
    col = 1.0 - dot(r, r);
    
    // Add some light to the tunnel
    vec4 n = texture(iChannel1, floor(uv * ntiles) / ntiles);
    col *= mod(n.r * n.g * n.b * s + iTime * 0.1 + clamp(s, 0.0, 0.6), 1.0);    
    return clamp(col, 0.0, 1.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 uv1 = uv * 2.0 - 1.0;
    
    uv1.x *= iResolution.x / iResolution.y;
    
    // Calculate new UV coordinates
    vec2 center = vec2(0.0, 0.0) + 
        vec2(0.075*(0.5 + 0.5 * sin(iTime*4.0)),
             0.05*(0.5 + 0.5 * sin(iTime*0.01 + 1.7)));
    vec2 p = uv1 - center;
    float r = length(p);
    float a = atan(p.y, p.x) * 3.0 / 3.14;
    vec2 uv2= vec2(1.0 / r + iTime*0.25, a);

    // Read the sound texture
    float sound = texture(iChannel2, vec2(0.01, 1.0 - r)).r;
    sound = pow(sound, 1.5);
    
    // Calculate the colors
    vec3 c1 = vec3(0.02, 0.1, 0.02);
    vec3 c2 = mix( vec3(1.0, 0.6, 0.6), vec3(0.6, 0.6, 1.0), vec3(0.5 + 0.5 * sin(iTime*0.1)));
    vec3 c  = mix(c1, c2, r);
    vec3 coltunnel =  sound * c * tex(uv2, sound) + ( 0.15 * texture(iChannel0, uv2).rgb);
    vec3 colback   = vec3(0.05,0.05,0.05);

    // Mix the colors
    fragColor = vec4(r * coltunnel + (1.0 - r) * colback, 1.0);
}
