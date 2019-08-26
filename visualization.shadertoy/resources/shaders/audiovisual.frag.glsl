// Taken from https://www.shadertoy.com/view/MsBSzw#

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = (fragCoord.xy-.5*iResolution.xy)/min(iResolution.x,iResolution.y);			
    vec3 c = vec3(0.0);
    vec2 uv = fragCoord.xy / iResolution.xy;  
    float wave = texture( iChannel0, vec2(uv.x,0.75) ).x;

    for(int i = 1; i<20; i++)
    {
        float time = 2.*3.14*float(i)/20.* (iTime*.9);
        float x = sin(time)*1.8*smoothstep( 0.0, 0.15, abs(wave - uv.y));
        float y = sin(.5*time) *smoothstep( 0.0, 0.15, abs(wave - uv.y));
        y*=.5;
        vec2 o = .4*vec2(x*cos(iTime*.5),y*sin(iTime*.3));
        float red = fract(time);
        float green = 1.-red;
        c+=0.016/(length(p-o))*vec3(red,green,sin(iTime));
    }
    fragColor = vec4(c,1.0);
}
//2014 - Passion
//References  - https://www.shadertoy.com/view/Xds3Rr
//            - tokyodemofest.jp/2014/7lines/index.html
