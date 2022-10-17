

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// history:
//  - v.01, mac browser crash fixed
//  - v.00, f1rs7 p05t

#define TRACE_STEPS 128
#define TRACE_EPSILON .001
#define REFLECT_EPSILON .1
#define TRACE_DISTANCE 30.
#define NORMAL_EPSILON .01
#define REFLECT_DEPTH 4
#define NUM_BALLS 7
#define CUBEMAP_SIZE 128



vec3 balls[NUM_BALLS];

float touching_balls(in vec3 at) {
	float sum = 0.;
	for (int i = 0; i < NUM_BALLS; ++i) {
		float r = length(balls[i] - at);
		sum += 1. / (r * r);
	}
	return 1. - sum;
}

void update_balls(float t) {

    for (int i = 0; i < NUM_BALLS; ++i) {
		balls[i] = 3. * vec3(
			sin(.3+float(i+1)*t),
			cos(1.7+float(i-5)*t),
			1.1*sin(2.3+float(i+7)*t));
	}
}

float world(in vec3 at) {
	return touching_balls(at);
}

vec3 normal(in vec3 at) {
	vec2 e = vec2(0., NORMAL_EPSILON);
	return normalize(vec3(world(at+e.yxx)-world(at), 
						  world(at+e.xyx)-world(at),
						  world(at+e.xxy)-world(at)));
}

vec4 raymarch(in vec3 pos, in vec3 dir, in float maxL) {
	float l = 0.;
	for (int i = 0; i < TRACE_STEPS; ++i) {
		float d = world(pos + dir * l);
		if (d < TRACE_EPSILON*l) break; // if we return here, browser will crash on mac os x, lols
		l += d;
		if (l > maxL) break;
	}
	return vec4(pos + dir * l, l);
}

vec3 lookAtDir(in vec3 dir, in vec3 pos, in vec3 at) {
	vec3 f = normalize(at - pos);
	vec3 r = cross(f, vec3(0.,1.,0.));
	vec3 u = cross(r, f);
	return normalize(dir.x * r + dir.y * u + dir.z * f);
}

// http://the-witness.net/news/2012/02/seamless-cube-map-filtering/
vec3 cube(in vec3 v) {
   float M = max(max(abs(v.x), abs(v.y)), abs(v.z));
   float scale = (float(CUBEMAP_SIZE) - 1.) / float(CUBEMAP_SIZE);
   if (abs(v.x) != M) v.x *= scale;
   if (abs(v.y) != M) v.y *= scale;
   if (abs(v.z) != M) v.z *= scale;
   return texture(iChannel0, v).xyz;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    float t = iTime * .11;
	update_balls(t);
	float aspect = iResolution.x / iResolution.y;
	vec2 uv = (fragCoord.xy / iResolution.xy * 2. - 1.) * vec2(aspect, 1.);
	
	vec3 pos = vec3(cos(2.+4.*cos(t))*10., 2.+8.*cos(t*.8), 10.*sin(2.+3.*cos(t)));
	vec3 dir = lookAtDir(normalize(vec3(uv, 2.)), pos.xyz, vec3(balls[0]));
	
	vec3 color = vec3(0.);
	float k = 1.;
	for (int reflections = 0; reflections < REFLECT_DEPTH; ++reflections) {
		vec4 tpos = raymarch(pos, dir, TRACE_DISTANCE);
		if (tpos.w >= TRACE_DISTANCE) {
			color += cube(dir);
			break;
		}
		color += vec3(.1) * k;
		k *= .6;
		dir = normalize(reflect(dir, normal(tpos.xyz)));
		pos = tpos.xyz + dir * REFLECT_EPSILON;
	}
	fragColor = vec4(color, 0.);
}