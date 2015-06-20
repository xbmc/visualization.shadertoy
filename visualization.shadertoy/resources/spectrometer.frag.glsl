// Created by inigo quilez - iq/2013
// Modified by Javier Barandiaran
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // create pixel coordinates
	vec2 uv = fragCoord.xy / iResolution.xy;

	// first texture row is frequency data
	float fft  = texture2D( iChannel0, vec2(uv.x,0.25) ).x; 
	
    // second texture row is the sound wave
	float wave = texture2D( iChannel0, vec2(uv.x,0.75) ).x;
	
	// convert frequency to colors
	vec3 col = vec3( 1.5*(1.0-fft), 4.0*fft*(1.0-fft), fft )*fft;

    // add wave form on top	
	col *= 1.0 -  smoothstep( 0.0, 0.6, abs(wave - uv.y) );
    col += 0.1*(1.0 -  smoothstep( 0.0, 0.3, abs(wave - uv.y) ));
    col += 0.1*(1.0 -  smoothstep( 0.5, 0.1, abs(wave - uv.y) ));
    col += 0.5*(-0.5+uv.y)*(1.0 -  smoothstep( 0.0, 0.08, abs(wave - uv.y) ));
	
	// output final color
	fragColor = vec4(col,1.0);
}
