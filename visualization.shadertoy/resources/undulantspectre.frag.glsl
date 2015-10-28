// Taken from https://www.shadertoy.com/view/XsjGz3#

static const vec3 COLOR1 = vec3(0.0, 0.0, 0.3);
static const vec3 COLOR2 = vec3(0.5, 0.0, 0.0);
float BLOCK_WIDTH = 0.01;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
	
	vec3 final_color = vec3(1.0,1.0,1.0);
	vec3 bg_color = vec3(0.0,0.0,0.0);
	vec3 wave_color = vec3(0.0,0.0,0.0);
	
	bg_color = mix(COLOR1, COLOR2, uv.x) + texture2D(iChannel0, vec2(uv.x/8., abs(.5 - uv.y))).rgb - .7;
	
	float wave_width;
	uv  = -1.0 + 2.0 * uv;
	uv.y += 0.1;
	uv.x *= 2.;
	const float n = 10.;
	float prev = 0.;
	float curr = .6;
	float next = texture2D(iChannel0, vec2(0.,0.)).r;
	
	for(float i = 0.0; i < n; i++) {
		prev = curr;
		curr = next;
		next = texture2D(iChannel0, vec2((i+1.)/n,0.)).r;
		
		float a = max(0., curr * 2. - prev - next);
		
		uv.y += (1. * cos(mod(uv.x * 2. * i / n * 10. + iGlobalTime * i, 2.*3.14)) * a * a);
		uv.x += 0.1;
		
		wave_width = abs(1.0 / (200.0 * uv.y));
		
		wave_color += vec3(wave_width * 1.9, wave_width, wave_width * 1.5) * 5. / n;
	}
	
	final_color = bg_color + wave_color;
	
	
	fragColor = vec4(final_color, 1.0);
}
