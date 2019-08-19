// Taken from https://www.shadertoy.com/view/ldsXWn#

// ---- change here kaleidoscope size and toggle on/off----
const float USE_KALEIDOSCOPE = 1.0;
const float NUM_SIDES = 8.0;

// ---- Change the amount at which the visualizer reacts to music (per song different settings work best) ----
const float MIN_SIZE = 0.01;
const float REACT_INTENSITY = 0.3;
const float BASS_INTENSITY = 1.1;
const float BASS_BASE_HEIGHT = .0;
const float ZOOM_FACTOR = 0.5;

// math const
const float PI = 3.14159265359;
const float DEG_TO_RAD = PI / 180.0;

// -4/9(r/R)^6 + (17/9)(r/R)^4 - (22/9)(r/R)^2 + 1.0
float field( vec2 p, vec2 center, float r ) {
    float d = length( p - center ) / r;
    
    float t   = d  * d;
    float tt  = t  * d;
    float ttt = tt * d;
    
    float v =
	( - 10.0 / 9.0 ) * ttt +
	(  17.0 / 9.0 ) * tt +
	( -22.0 / 9.0 ) * t +
	1.0;
    
    return clamp( v, 0.0, 1.0 );
}

vec2 Kaleidoscope( vec2 uv, float n, float bias ) {
    float angle = PI / n;
    
    float r = length( uv*.5 );
    float a = atan( uv.y, uv.x ) / angle;
    
    a = mix( fract( a ), 1.0 - fract( a ), mod( floor( a ), 2.0 ) ) * angle;
    
    return vec2( cos( a ), sin( a ) ) * r;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 ratio = iResolution.xy / min( iResolution.x, iResolution.y );
    vec2 uv = ( fragCoord.xy * 2.0 - iResolution.xy ) / min( iResolution.x, iResolution.y );
    
    // --- Kaleidoscope ---
    uv = mix( uv, Kaleidoscope( uv, NUM_SIDES, iChannelTime[1]*10. ), USE_KALEIDOSCOPE ); 
    
    uv *= ZOOM_FACTOR+(BASS_INTENSITY)* (texture(iChannel1, vec2(.01,0)).r);
    
    vec3 final_color = vec3( 0.0 );
    float final_density = 0.0;
    for ( int i = 0; i < 128; i++ ) {
	vec4 noise  = texture( iChannel0, vec2( float( i ) + 0.5, 0.5 ) / 256.0 );
	vec4 noise2 = texture( iChannel0, vec2( float( i ) + 0.5, 9.5 ) / 256.0 );
	
	
	//sound loudness to intensity
	float velintensity = texture(iChannel1, vec2(1.0+uv.x,0)).r;
	
	// velocity
	vec2 vel = -abs(noise.xy) * 3.0 +vec2( .003*-velintensity )-vec2(2.0);
	vel*=-.5;
	
	// center
	vec2 pos = noise.xy;
	pos += iChannelTime[1] * vel * 0.2;
	pos = mix( fract( pos ),fract( pos ), mod( floor( pos ), 2.0 ) );
	
	//sound loudness to intensity
	float intensity = texture(iChannel1, vec2(pos.y,0)).r;
	
	// remap to screen
	pos = ( pos * 2.0 - 1.0 ) * ratio;
	
	
	
	// radius
	float radius = clamp( noise.w, 0.3, 0.8 );
	radius *= 3.*intensity;
	//radius += 0.5;
	radius *= radius * 0.4;
	radius = clamp(radius, MIN_SIZE*abs(-pos.x), REACT_INTENSITY);
	
	// color
	vec3 color = noise2.xyz;
	
	// density
	float density = field( uv, pos, radius );

	// accumulate
	final_density += density;		
	final_color += density * color;
    }

    final_density = clamp( final_density - 0.1, 0.0, 1.0 );
    final_density = pow( final_density, 3.0 );

    fragColor = vec4( final_color * final_density, final_density );
}
