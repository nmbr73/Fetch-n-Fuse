

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/**
 * Created by Kamil Kolaczynski (revers) - 2015
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
 *
 * This shader uses code written by: 
 * - iq (raymarching, hash, noise)
 * - otaviogood (runes, https://www.shadertoy.com/view/MsXSRn)
 * Thanks for sharing it guys!
 * 
 * The shader was created and exported from Synthclipse (http://synthclipse.sourceforge.net/)
 */

const float MarchDumping = 1.0;
const float Far = 62.82;
const int MaxSteps = 32;
const float FOV = 0.4;
const vec3 Eye = vec3(0.14, 0.0, 3.4999998);
const vec3 Direction = vec3(0.0, 0.0, -1.0);
const vec3 Up = vec3(0.0, 1.0, 0.0);

// Noise settings:
const float Power = 5.059;
const float MaxLength = 0.9904;
const float Dumping = 10.0;

#define PI 3.141592
#define HALF_PI 1.57079632679

const float DEG_TO_RAD = PI / 180.0;
const float TIME_FACTOR = 0.3;
const float ROTATION_DIST = 16.0;

vec3 hash3(vec3 p) {
	p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
			dot(p, vec3(269.5, 183.3, 246.1)),
			dot(p, vec3(113.5, 271.9, 124.6)));

	return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise(vec3 p) {
	vec3 i = floor(p);
	vec3 f = fract(p);

	vec3 u = f * f * (3.0 - 2.0 * f);

	float n0 = dot(hash3(i + vec3(0.0, 0.0, 0.0)), f - vec3(0.0, 0.0, 0.0));
	float n1 = dot(hash3(i + vec3(1.0, 0.0, 0.0)), f - vec3(1.0, 0.0, 0.0));
	float n2 = dot(hash3(i + vec3(0.0, 1.0, 0.0)), f - vec3(0.0, 1.0, 0.0));
	float n3 = dot(hash3(i + vec3(1.0, 1.0, 0.0)), f - vec3(1.0, 1.0, 0.0));
	float n4 = dot(hash3(i + vec3(0.0, 0.0, 1.0)), f - vec3(0.0, 0.0, 1.0));
	float n5 = dot(hash3(i + vec3(1.0, 0.0, 1.0)), f - vec3(1.0, 0.0, 1.0));
	float n6 = dot(hash3(i + vec3(0.0, 1.0, 1.0)), f - vec3(0.0, 1.0, 1.0));
	float n7 = dot(hash3(i + vec3(1.0, 1.0, 1.0)), f - vec3(1.0, 1.0, 1.0));

	float ix0 = mix(n0, n1, u.x);
	float ix1 = mix(n2, n3, u.x);
	float ix2 = mix(n4, n5, u.x);
	float ix3 = mix(n6, n7, u.x);

	float ret = mix(mix(ix0, ix1, u.y), mix(ix2, ix3, u.y), u.z) * 0.5 + 0.5;
	return ret * 2.0 - 1.0;
}

float sdBox(vec3 p, vec3 b) {
	vec3 d = abs(p) - b;
	return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

vec3 rotateY(vec3 p, float a) {
	float sa = sin(a);
	float ca = cos(a);
	return vec3(ca * p.x + sa * p.z, p.y, ca * p.z - sa * p.x);
}

float getAngle(float x) {
	return ((1.0 - x) * 100.0 - 15.0) * DEG_TO_RAD;
}

float tween(float time) {
	float t = fract(time * TIME_FACTOR);

	float stop = 0.25;
	float range = 1.0 - stop;
	float k = sin((sin(sin(HALF_PI) * HALF_PI)) * HALF_PI) * 0.9;

	float ret = sin((sin(sin(t / range * HALF_PI) * HALF_PI)) * HALF_PI) * 0.9;
	float stp = step(range, t);

	return ret * (1.0 - stp) + stp * mix(k, 1.0, (t - range) / (1.0 - range));
}

vec3 transformCube(vec3 p) {
	p.x -= ROTATION_DIST;

	p = rotateY(p, getAngle(tween(iTime)));
	p.x += ROTATION_DIST;
	return p;
}

float map(vec3 p) {
	vec3 q = transformCube(p);
	return sdBox(q, vec3(1.0, 1.0, 0.0001));
}

vec2 castRay(vec3 ro, vec3 rd) {
	float tmin = 0.0;
	float tmax = Far;

	float precis = 0.002;
	float t = tmin;
	float m = -1.0;

	for (int i = 0; i < MaxSteps; i++) {
		float res = map(ro + rd * t);
		if (res < precis || t > tmax) {
			break;
		}
		t += res * MarchDumping;
		m = 1.0;
	}

	if (t > tmax) {
		m = -1.0;
	}
	return vec2(t, m);
}

float udSegment(vec2 p, vec2 start, vec2 end) {
	vec2 dir = start - end;
	float len = length(dir);
	dir /= len;

	vec2 proj = clamp(dot(p - end, dir), 0.0, len) * dir + end;
	return distance(p, proj);
}

/**
 * Rune function by Otavio Good.
 * https://www.shadertoy.com/view/MsXSRn
 */
float rune(vec2 uv, vec2 seed) {
	float ret = 100.0;

	for (int i = 0; i < 4; i++) {
		// generate seeded random line endPoints - just about any texture_ should work.
		// Hopefully this randomness will work the same on all GPUs (had some trouble with that)
		vec2 posA = texture(iChannel0, floor(seed + 0.5) / iChannelResolution[0].xy).xy;
		vec2 posB = texture(iChannel0, floor(seed + 1.5) / iChannelResolution[0].xy).xy;

		seed += 2.0;
		// expand the range and mod it to get a nicely distributed random number - hopefully. :)
		posA = fract(posA * 128.0);
		posB = fract(posB * 128.0);

		// each rune touches the edge of its box on all 4 sides
		if (i == 0) {
			posA.y = 0.0;
		}
		if (i == 1) {
			posA.x = 0.999;
		}
		if (i == 2) {
			posA.x = 0.0;
		}
		if (i == 3) {
			posA.y = 0.999;
		}

		// snap the random line endpoints to a grid 2x3
		vec2 snaps = vec2(2.0, 3.0);
		posA = (floor(posA * snaps) + 0.5) / snaps;	// to center it in a grid cell
		posB = (floor(posB * snaps) + 0.5) / snaps;

		if (distance(posA, posB) < 0.0001) {
			continue; // eliminate dots.
		}

		// Dots (degenerate lines) are not cross-GPU safe without adding 0.001 - divide by 0 error.
		float d = udSegment(uv, posA, posB + 0.001);
		ret = min(ret, d);
	}
	return ret;
}

float distToObject(vec2 p) {
	p *= 0.2;

	vec2 newSeed = vec2(iTime * TIME_FACTOR + 1.0);
    newSeed.y *= 0.2;
    newSeed = floor(newSeed);
    newSeed *= 4.0;
        
	return rune(p, newSeed - 0.41);
}

float normalizeScalar(float value, float max) {
	return clamp(value, 0.0, max) / max;
}

vec3 color(vec2 p) {
	vec3 coord = vec3(p, iTime * 0.25);
	float n = abs(noise(coord));
	n += 0.5 * abs(noise(coord * 2.0));
	n += 0.25 * abs(noise(coord * 4.0));
	n += 0.125 * abs(noise(coord * 8.0));

	n *= (100.001 - Power);
	float dist = distToObject(p);
	float k = normalizeScalar(dist, MaxLength);
	n *= dist / pow(1.001 - k, Dumping);

	vec3 col = vec3(1.0, 0.25, 0.08) / n;
	return pow(col, vec3(2.0));
}

vec3 render(vec3 ro, vec3 rd) {
	vec3 col = vec3(0.0);
	vec2 res = castRay(ro, rd);

	float t = res.x;
	float m = res.y;

	if (m > 0.0) {
		vec3 pos = ro + t * rd;

		vec3 q = transformCube(pos);
		vec2 uv = q.xy * 3.0;

		col = color(uv + 2.5);
	}

	return vec3(clamp(col, 0.0, 1.0));
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 q = fragCoord.xy / iResolution.xy;
	vec2 coord = 2.0 * q - 1.0;
	coord.x *= iResolution.x / iResolution.y;
	coord *= FOV;

	vec3 dir = normalize(Direction);
	vec3 up = Up;
	vec3 upOrtho = normalize(up - dot(dir, up) * dir);
	vec3 right = normalize(cross(dir, upOrtho));

	vec3 ro = Eye;
	vec3 rd = normalize(dir + coord.x * right + coord.y * upOrtho);

	vec3 col = render(ro, rd);
	col = pow(col, vec3(0.4545));

	fragColor = vec4(col, 1.0);
}
