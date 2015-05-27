// Taken from https://www.shadertoy.com/view/Msl3zr

const float PI = 3.14159;

// http://github.prideout.net/barrel-distortion/
vec2 distort(vec2 p, float power)
{
    float a  = atan(p.y, p.x);
    float r = length(p);
    r = pow(r, power);
    return vec2(r * cos(a), r*sin(a));
	//return vec2((a / PI), r*2.0-1.0);	// polar
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // create pixel coordinates
	vec2 uv = fragCoord.xy / iResolution.xy;
	
	// distort
	float bass = texture2D( iChannel0, vec2(0, 0) ).x;		
	uv = distort(uv*2.0-1.0, 0.5+bass)*0.5+0.5;
		
	// quantize coordinates
	const float bands = 20.0;
	const float segs = 20.0;
	vec2 p;
	p.x = floor(uv.x*bands)/bands;
	p.y = floor(uv.y*segs)/segs;
	
	// read frequency data from first row of texture
	float fft  = texture2D( iChannel0, vec2(p.x,0.0) ).x;	

	// led color
	vec3 color = mix(vec3(0.0, 2.0, 0.0), vec3(2.0, 0.0, 0.0), sqrt(uv.y));
	
	// mask for bar graph
	float mask = (p.y < fft) ? 1.0 : 0.0;
	
	// led shape
	vec2 d = fract((uv - p)*vec2(bands, segs)) - 0.5;
	float led = smoothstep(0.5, 0.3, abs(d.x)) *
		        smoothstep(0.5, 0.3, abs(d.y));
	vec3 ledColor = led*color*mask;

    // second texture row is the sound wave
	float wave = texture2D( iChannel0, vec2(uv.x, 0.75) ).x;
	vec3 waveColor = vec3(0.001, 0.01, 0.04) / abs(wave - uv.y);
		
	// output final color
	//fragColor = vec4(vec3(fft),1.0);
    //fragColor = vec4(d, 0.0, 1.0);
	//fragColor = vec4(ledColor, 1.0);
	//fragColor = vec4(waveColor, 1.0);
	fragColor = vec4(ledColor + waveColor, 1.0);
}

