/*
Electric pulse - https://www.shadertoy.com/view/llj3WV
Based on Electric Dream by: mu6k - 21st March, 2014 https://www.shadertoy.com/view/ld23Wd#
Electric pulse by: uNiversal - 27th May, 2015
Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
*/

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 tuv = fragCoord.xy / iResolution.xy;
    vec2 uv = fragCoord.xy / iResolution.yy-vec2(0.9,0.5);

    float acc = .0;
    float best = .0;
    float best_acc = .0;

    for (float i = .0; i<0.5; i+=.008)
    {
        acc+=texture(iChannel0,vec2(i,1.0)).x-.5;
        if (acc>best_acc)
        {
            best_acc = acc;
            best = i;
        }
    }

    vec3 colorize = vec3(.5);

    for (float i = .0; i< 1.0; i+=.05)
    {
        colorize[int(i*3.0)]+=texture(iChannel0,vec2(i,9.1)).x*pow(i+.5,.9);
    }

    colorize = normalize(colorize);

    float offset = best;

    float osc = texture(iChannel0,vec2(offset+tuv.x*.4 +.1,1.0)).x-.5;
    osc += texture(iChannel0,vec2(offset+tuv.x*.4 -.01,2.0)).x-.5;
    osc += texture(iChannel0,vec2(offset+tuv.x*.4 +.01,5.0)).x-.5;
    osc*=.333;
    float boost = texture(iChannel0,vec2(1.0)).x;
    float power = pow(boost,9.0);

    vec3 color = vec3(.01);

    color += colorize*vec3((power*.9+.1)*0.02/(abs((power*1.4+.5)*osc-uv.y)));
    color += colorize*.2*((.9-power)*.5+.1);

    vec2 buv = uv*(1.0+power*power*power*.2);
    buv += vec2(pow(power,12.0)*5.1,iTime*.5);
    
    color +=
    color -= length(uv)*1.1;

    color += texture(iChannel0,fragCoord.xy/256.0).xyz*.01;
    color = pow(max(vec3(.05),color),vec3(1.5));

    fragColor = vec4(color,0.5);
}
