// Taken from https://www.shadertoy.com/view/MsBSzw#

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = (fragCoord.xy-.5*iResolution.xy)/min(iResolution.x,iResolution.y);			
    vec3 c = vec3(0.0,0.0,0.0);
    vec2 uv = fragCoord.xy / iResolution.xy;  
    float wave = texture2D( iChannel0, vec2(uv.x,0.75) ).x;

    float time = 0.0;
    float alpha = 2.*3.14/20.* (iGlobalTime*.9);
    float cs = cos(iGlobalTime*.5);
    float sn = sin(iGlobalTime*.3);
    float sn2 = sin(iGlobalTime);
    float sm=smoothstep( 0.0, 0.15, abs(wave - uv.y));
    for(int i = 1; i<10; i++)
    {
        time += alpha;
        float x = sin(time)*1.8*sm;
        float y = sin(.5*time)*0.5*sm;
        vec2 o = .4*vec2(x*cs,y*sn);
        float red = fract(time);
        float green = 1.-red;
        c+=0.016/(length(p-o))*vec3(red,green,sn2);
    }
    fragColor = vec4(c,1.0);
}
//2014 - Passion
//References  - https://www.shadertoy.com/view/Xds3Rr
//            - tokyodemofest.jp/2014/7lines/index.html


