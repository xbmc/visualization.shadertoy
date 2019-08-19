// Taken from https://www.shadertoy.com/view/4dXGWH

/*by mu6k, Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
 
 I started animating the nyancat image. I looked up the original colors 
 for rainbow and background. I kept adding stuff until I ended up with 
 something I could watch for 10 hours. Nah 10 hours is too long... or is it? 

 Feel free to tweak the quality parameter... if you have a monster pc.

 quality 1 - no AA, extremely noisy blur.
 quality 2 - 2xAA, decent
 quality 3 - 3xAA, slow here on fullscreen
 quality 4 - 4xAA, doesn't even compile here

 Enjoy!

 04/05/2013:
 - published

 muuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuusk!*/

#define quality 2

float hash(float x)
{
	return fract(sin(x*.0127863)*17143.321);
}

float hash(vec2 x)
{
	return fract(sin(dot(x,vec2(1.4,52.14)))*17143.321);
}


float hashmix(float x0, float x1, float interp)
{
	x0 = hash(x0);
	x1 = hash(x1);
	#ifdef noise_use_smoothstep
	interp = smoothstep(0.0,1.0,interp);
	#endif
	return mix(x0,x1,interp);
}

float hashmix(vec2 p0, vec2 p1, vec2 interp)
{
	float v0 = hashmix(p0[0]+p0[1]*128.0,p1[0]+p0[1]*128.0,interp[0]);
	float v1 = hashmix(p0[0]+p1[1]*128.0,p1[0]+p1[1]*128.0,interp[0]);
	#ifdef noise_use_smoothstep
	interp = smoothstep(vec2(0.0),vec2(1.0),interp);
	#endif
	return mix(v0,v1,interp[1]);
}

float hashmix(vec3 p0, vec3 p1, vec3 interp)
{
	float v0 = hashmix(p0.xy+vec2(p0.z*43.0,0.0),p1.xy+vec2(p0.z*43.0,0.0),interp.xy);
	float v1 = hashmix(p0.xy+vec2(p1.z*43.0,0.0),p1.xy+vec2(p1.z*43.0,0.0),interp.xy);
	#ifdef noise_use_smoothstep
	interp = smoothstep(vec3(0.0),vec3(1.0),interp);
	#endif
	return mix(v0,v1,interp[2]);
}

float hashmix(vec4 p0, vec4 p1, vec4 interp)
{
	float v0 = hashmix(p0.xyz+vec3(p0.w*17.0,0.0,0.0),p1.xyz+vec3(p0.w*17.0,0.0,0.0),interp.xyz);
	float v1 = hashmix(p0.xyz+vec3(p1.w*17.0,0.0,0.0),p1.xyz+vec3(p1.w*17.0,0.0,0.0),interp.xyz);
	#ifdef noise_use_smoothstep
	interp = smoothstep(vec4(0.0),vec4(1.0),interp);
	#endif
	return mix(v0,v1,interp[3]);
}

float noise(float p)
{
	float pm = mod(p,1.0);
	float pd = p-pm;
	return hashmix(pd,pd+1.0,pm);
}

float noise(vec2 p)
{
	vec2 pm = mod(p,1.0);
	vec2 pd = p-pm;
	return hashmix(pd,(pd+vec2(1.0,1.0)), pm);
}

float noise(vec3 p)
{
	vec3 pm = mod(p,1.0);
	vec3 pd = p-pm;
	return hashmix(pd,(pd+vec3(1.0,1.0,1.0)), pm);
}

float noise(vec4 p)
{
	vec4 pm = mod(p,1.0);
	vec4 pd = p-pm;
	return hashmix(pd,(pd+vec4(1.0,1.0,1.0,1.0)), pm);
}

vec3 background(vec2 p)
{
	vec2 pm = mod(p,vec2(0.5));
	float q = pm.x+pm.y;
	vec3 color = vec3(13,66,121)/255.0;
	
	vec2 visuv = p*0.5;
	vec2 visuvm = mod(visuv,vec2(0.05,0.05));
	visuv-=visuvm;
	float vis = texture2D(iChannel0,vec2((visuv.x+1.0)*.5,0.0)).y*2.0-visuv.y;
	
	if (vis>1.0&&visuvm.x<0.04&&visuvm.y<0.04)
		color.xyz *=0.85;
	
	vec2 p2;
	float stars;
	for (int i=0; i<5; i++)
	{
		float s = float(i)*0.2+1.0;
		//float ts
		p2=p*s+vec2(iGlobalTime/s,s*16.0)-mod(p*s+vec2(iGlobalTime/s,s*16.0),vec2(0.05));
		stars=noise(p2*16.0);
		if (stars>0.98) color=vec3(1.0,1.0,1.0);
	}
	
	vec2 visuv2 = p*0.25+vec2(iGlobalTime*0.1,0);
	vec2 visuvm2 = mod(visuv2,vec2(0.05,0.05));
	visuv2-=visuvm2;
	
	float vis2p = hash(visuv2);
	float vis2 = texture2D(iChannel0,vec2(vis2p,0)).y;
	
	vis2 = pow(vis2,20.0)*261.0;
	vis2 *= vis2p;
	
	if (vis2>1.0) vis2=1.0;
	
	if (vis2>0.2&&visuvm2.x<0.09&&visuvm2.y<0.09)
		color+=vec3(vis2,vis2,vis2)*0.5;
	
	return color;
}

vec4 rainbow(vec2 p)
{
	
	p.x-=mod(p.x,0.05);
	float q = max(p.x,-0.1);
	float s = sin(p.x*(8.0)+iGlobalTime*8.0)*0.09;
	s-=mod(s,0.05);
	p.y+=s;
	
	
//p.y += sin(p.x*8.0+iGlobalTime)*0.25;
	
	vec4 c;
	
	if (p.x>0.0) c=vec4(0,0,0,0); else
	if (0.0/6.0<p.y&&p.y<1.0/6.0) c= vec4(255,43,14,255)/255.0; else
	if (1.0/6.0<p.y&&p.y<2.0/6.0) c= vec4(255,168,6,255)/255.0; else
	if (2.0/6.0<p.y&&p.y<3.0/6.0) c= vec4(255,244,0,255)/255.0; else
	if (3.0/6.0<p.y&&p.y<4.0/6.0) c= vec4(51,234,5,255)/255.0; else
	if (4.0/6.0<p.y&&p.y<5.0/6.0) c= vec4(8,163,255,255)/255.0; else
	if (5.0/6.0<p.y&&p.y<6.0/6.0) c= vec4(122,85,255,255)/255.0; else
		c=vec4(0,0,0,0);
	return c;
}

vec4 nyan(vec2 p)
{
	vec2 uv = p*vec2(0.4,1.0);
	float ns=2.0;
	float nt = iGlobalTime*ns; nt-=mod(nt,240.0/256.0/6.0); nt = mod(nt,240.0/256.0);
	float ny = mod(iGlobalTime*ns,1.0); ny-=mod(ny,0.75); ny*=-0.05;
	vec4 color = texture2D(iChannel1,vec2(uv.x/3.0+210.0/256.0-nt+0.05,.5-uv.y-ny));
	if (uv.x<-0.3) color.a = 0.0;
	if (uv.x>0.2) color.a=0.0;
	return color;
}

vec4 scene(vec2 uv)
{
	
	vec4 color = nyan(uv);
	vec3 c2 = background(uv+vec2(0,0));
	vec4 c3 = rainbow(vec2(uv.x+0.4,0.05-uv.y*2.0+.5));
	
	vec4 c4 = mix(vec4(c2,1.0),c3,c3.a);
		
	color = mix(c4,color,color[3]);
	return color;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	float vol = .0;
	for(float x = 0.0; x<1.0; x+=1.0/256.0)
	{
		vol += texture2D(iChannel0,vec2(x,0.0)).y;
	}
	vol*=1.0/256.0;
	vol = pow(vol,16.0)*256.0;
	
	
	vec2 uv = fragCoord.xy / iResolution.xy-0.5;
	uv.x*=iResolution.x/iResolution.y;
	vec2 ouv=uv;
	float t = iGlobalTime-length(uv)*0.5;
	
	uv.x+=sin(t*0.4)*0.05;
	uv.y+=cos(t)*0.05;

	
	float angle = (cos(t*0.461)+cos(t*0.71)+cos(t*0.342)+cos(t*0.512))*0.2;
	angle+=sin(t*16.0)*vol*0.1;
	float zoom = (cos(t*0.364)+cos(t*0.686)+cos(t*0.286)+cos(t*0.496))*0.2;
	uv*=pow(2.0,zoom+1.0-vol*4.0);
	uv=uv*mat2(cos(angle),-sin(angle),sin(angle),cos(angle));
	
	
	vec4 color = vec4(0);
	
	uv.y+=vol;

	for(int x = 0; x<quality; x++)
	for(int y = 0; y<quality; y++)
	{
		float xx = float(x)*2.0/float(quality)/iResolution.y;
		float yy = float(y)*2.0/float(quality)/iResolution.y;
		float n = hash(color.xy+ouv+vec2(float(x),float(y)));
		float s = 1.0-pow(n,10.0)*vol*17.0;
		color += scene((uv+vec2(xx,yy))*s)/float(quality*quality)*(1.0+vol);
	}

	color-=vec4(length(uv)*0.05);
	color.xyz+=vec3(hash(color.xy+ouv))*0.02;
	fragColor = color;
}
