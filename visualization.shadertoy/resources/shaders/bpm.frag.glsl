// Taken from https://www.shadertoy.com/view/ldB3D1

// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// An example showing the use of iChannelData[] in order to synchronize an aimation

#define BPM 128.0

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = -1.0+2.0*fragCoord.xy / iResolution.xy;
    uv.x *= iResolution.x/iResolution.y;		
    
    vec3 col = vec3(0.0);
	
    float h = fract( 0.25 + 0.5*iChannelTime[0]*BPM/60.0 );
    float f = 1.0-smoothstep( 0.0, 1.0, h );
    f *= smoothstep( 4.5, 4.51, iChannelTime[0] );
    float r = length(uv-0.0) + 0.2*cos(25.0*h)*exp(-4.0*h);
    f = pow(f,0.5)*(1.0-smoothstep( 0.5, 0.55, r) );
    float rn = r/0.55;
    col = mix( col, vec3(0.4+1.5*rn,0.1+rn*rn,0.50)*rn, f );
    

    col = mix( col, vec3(1.0), smoothstep(  0.0,  3.0, iChannelTime[0] )*exp( -1.00*max(0.0,iChannelTime[0]- 2.5)) );
    col = mix( col, vec3(1.0), smoothstep( 16.0, 18.0, iChannelTime[0] )*exp( -0.75*max(0.0,iChannelTime[0]-19.0)) );
    
    fragColor = vec4(col,1.0);
}
