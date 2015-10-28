// Taken from https://www.shadertoy.com/view/4d23Ww

static const float Pi = 3.14159;
static float beat = 0.;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	float ct = iGlobalTime;
	float beat = 0.3*texture2D(iChannel0,vec2(0.4,0.0)).x;
	float scale = iResolution.y / 50.0;
	float ring = 20.0;
	float radius = iResolution.x*1.0;
	float gap = scale*.5;
	vec2 pos = fragCoord.xy - iResolution.xy*.5;
	
	float d = length(pos);
	
	// Create the wiggle
	d += beat*2.0*(sin(pos.y*0.25/scale+iGlobalTime*cos(ct))*sin(pos.x*0.25/scale+iGlobalTime*.5*cos(ct)))*scale*5.0;
	
	// Compute the distance to the closest ring
	float v = mod(d + radius/(ring*2.0), radius/ring);
	v = abs(v - radius/(ring*2.0));
	
	v = clamp(v-gap, 0.0, 1.0);
	
	d /= radius;
	vec3 m = fract((d-1.0)*vec3(ring*-.5, -ring, ring*.25)*0.5);
	
	fragColor = vec4(m*v, 1.0);
}
