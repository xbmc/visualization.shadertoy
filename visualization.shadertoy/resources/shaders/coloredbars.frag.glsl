// Taken from https://www.shadertoy.com/view/XsBXW1#

#define M_PI          3.1415926535897932384626433832795
#define MAXDISTANCE   32.
#define MAXITERATIONS 32
#define PRECISION     1e-3

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float plane(vec3 p, float z) {
    return p.z - z;
}

float udBox(vec3 p, vec3 b, out int object) {
    float tme = iTime*2.0 + 1024.0;

    p.y += tme / 2.0;
    float rowOffset = rand(vec2(0, floor(p.y/2.0)));
    p.x += tme * (2. * rowOffset + texture(iChannel0, vec2(rowOffset,.25)).x/500.);

    object = int(6. * mod(p.x/(32.*6.), 1.)) + 2;

    vec2 c = vec2(32, 2);
    p.xy = mod(p.xy, c)-.5*c;
    return length(max(abs(p)-b,0.));
}

float getMap(vec3 p, out int object) {
    float distance = MAXDISTANCE;
    float tempDist;
    int tempObject;

    distance = plane(p, 0.0);
    object = 1;

    tempDist = udBox(p, vec3(12.0, 0.5, 0.5), tempObject);
    if (tempDist <= distance) {
	distance = tempDist;
	object = tempObject;
    }
    return distance * 0.5;
}

vec2 castRay(vec3 origin, vec3 direction, out int object) {
    float distance = rand(vec2(direction.y * iTime, direction.x)) * 6.0;
    float delta = 0.0;
    object = 0;

    for (int i = 0; i < MAXITERATIONS; i++) {
	vec3 p = origin + direction * distance;

	delta = getMap(p, object);

	distance += delta;
	if (delta < PRECISION) {
	    return vec2(distance, float(i));
	}
	if (distance > MAXDISTANCE) {
	    object = 0;
	    return vec2(distance, float(i));
	}
    }
    
    object = 0;
    return vec2(distance, MAXITERATIONS);
}

vec3 drawScene(vec3 origin, vec3 direction) {
    vec3 color = vec3(0.0, 0.0, 0.0);
    int object = 0;

    vec2 ray = castRay(origin, direction, object);

    if (object != 0) {
	if (object == 1) {
	    color = vec3(1.0, 1.0, 1.0);
	}
	else if (object == 2) {
	    color = vec3(1.0, 0.3, 0.3);
	}
	else if (object == 3) {
	    color = vec3(0.3, 1.0, 0.3);
	}
	else if (object == 4) {
	    color = vec3(0.3, 0.3, 1.0);
	}
	else if (object == 5) {
	    color = vec3(1.0, 1.0, 0.3);
	}
	else if (object == 6) {
	    color = vec3(0.3, 1.0, 1.0);
	}
	else if (object == 7) {
	    color = vec3(1.0, 0.3, 1.0);
	}
    }
    return mix(color, vec3(0.0, 0.0, 0.0), ray.y/float(MAXITERATIONS));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 p = (fragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    p.x *= aspect;
    
    vec3 color = vec3(0.0, 0.0, 0.0);
    vec3 origin = vec3(0.0, 0.0, 12.0);
    vec3 direction = normalize(vec3(p.x, p.y, -1.0));
    direction.xy = mat2(cos(M_PI/4.0), sin(M_PI/4.0), -sin(M_PI/4.0), cos(M_PI/4.0)) * direction.xy;
    
    color = drawScene(origin, direction);
    
    fragColor = vec4(color, 1.0);
}
