void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = (fragCoord.xy-.5*iResolution.xy)/min(iResolution.x,iResolution.y);			
    vec3 c = vec3(0.0);
    vec2 uv = fragCoord.xy / iResolution.xy;  
    float wave = texture2D( iChannel0, vec2(uv.x,0.75) ).x;
    float s = smoothstep( 0.0, 0.15, abs(wave - uv.y));
    float t = .2826 * iGlobalTime;

    for(int i = 1; i<10; i++)
    {
	float time = t * float(i);
        float x = sin(time)*1.8*s;
        float y = sin(.5*time)*0.5*s;
        vec2 o = .4*vec2(x*cos(iGlobalTime*.5),y*sin(iGlobalTime*.3));
        float red = fract(time);
        float green = 1.-red;
        c+=0.016/(length(p-o))*vec3(red,green,sin(iGlobalTime));
    }
    fragColor = vec4(c,1.0);
}
//2014 - Passion
//References  - https://www.shadertoy.com/view/Xds3Rr
//            - tokyodemofest.jp/2014/7lines/index.html


