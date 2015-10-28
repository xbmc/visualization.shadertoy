// Taken from https://www.shadertoy.com/view/Xsf3WB

const float tau = 6.28318530717958647692;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = (fragCoord.xy - iResolution.xy*.5)/iResolution.x;

	uv = vec2(abs(atan(uv.x,uv.y)/(.5*tau)),length(uv));

	// adjust frequency to look pretty	
	uv.x *= 1.0/2.0;
	
	float seperation = 0.06*(1.0-iMouse.x/iResolution.x);

	vec3 wave = vec3(0.0,0.0,0.0);
	const int n = 9;
	for ( int i=0; i < n; i++ )
	{
/*		float u = uv.x*255.0;
		float f = fract(u);
		f = f*f*(3.0-2.0*f);
		u = floor(u);
		float sound = mix( texture2D( iChannel0, vec2((u+.5)/256.0,.75) ).x, texture2D( iChannel0, vec2((u+1.5)/256.0,.75) ).x, f );*/
		float sound = texture2D( iChannel0, vec2(uv.x,.75) ).x;
		
		// choose colour from spectrum
		float a = .9*float(i)*tau/float(n)-.6;
		vec3 phase = smoothstep(-1.0,.5,vec3(cos(a),cos(a-tau/3.0),cos(a-tau*2.0/3.0)));
		
		wave += phase*smoothstep(4.0/640.0, 0.0, abs(uv.y - sound*.3));
		uv.x += seperation/float(n);
	}
	wave *= 3.0/float(n);
		
	vec3 col = vec3(0,0,0);
	col.z  += texture2D( iChannel0, vec2(.000,.25) ).x;
	col.zy += texture2D( iChannel0, vec2(.125,.25) ).xx*vec2(1.5,.5);
	col.zy += texture2D( iChannel0, vec2(.250,.25) ).xx;
	col.zy += texture2D( iChannel0, vec2(.375,.25) ).xx*vec2(.5,1.5);
	col.y  += texture2D( iChannel0, vec2(.500,.25) ).x;
	col.yx += texture2D( iChannel0, vec2(.625,.25) ).xx*vec2(1.5,.5);
	col.yx += texture2D( iChannel0, vec2(.750,.25) ).xx;
	col.yx += texture2D( iChannel0, vec2(.875,.25) ).xx*vec2(.5,1.5);
	col.x  += texture2D( iChannel0, vec2(1.00,.25) ).x;
	col /= vec3(4.0,7.0,4.0);
	
	// vignetting
	col *= smoothstep( 1.2, 0.0, uv.y );
	
	fragColor = vec4(wave+col,1);
}
