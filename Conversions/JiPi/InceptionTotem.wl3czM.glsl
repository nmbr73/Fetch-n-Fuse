

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Processed by 'GLSL Shader Shrinker' (Shrunk by 125 characters)
// (https://github.com/deanthecoder/GLSLShaderShrinker)

// 'Inception Totem' dean_the_coder (Twitter: @deanthecoder)
// https://www.shadertoy.com/view/wl3czM
//
// Another quick and small demo, playing around with creating
// a better wood texture than I've made in the past.
// Still not happy with it - I think it needs some specular in
// the lighter areas, and probably worth of a new shader experiment.
//
// If the totem falls over, let me know...
//
// Thanks to Evvvvil, Flopine, Nusan, BigWings, Iq, Shane
// and a bunch of others for sharing their knowledge!

// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

float time;

struct Hit {
	float d;
	int id;
	vec3 uv;
};

// Thanks Shane - https://www.shadertoy.com/view/lstGRB
float n31(vec3 p) {
	const vec3 s = vec3(7, 157, 113);
	vec3 ip = floor(p);
	p = fract(p);
	p = p * p * (3. - 2. * p);
	vec4 h = vec4(0, s.yz, s.y + s.z) + dot(ip, s);
	h = mix(fract(sin(h) * 43.5453), fract(sin(h + s.x) * 43.5453), p.x);
	h.xy = mix(h.xz, h.yw, p.y);
	return mix(h.x, h.y, p.z);
}

float n21(vec2 p) {
	const vec3 s = vec3(7, 157, 0);
	vec2 h,
	     ip = floor(p);
	p = fract(p);
	p = p * p * (3. - 2. * p);
	h = s.zy + dot(ip, s.xy);
	h = mix(fract(sin(h) * 43.5453), fract(sin(h + s.x) * 43.5453), p.x);
	return mix(h.x, h.y, p.y);
}

float n11(float p) {
	float ip = floor(p);
	p = fract(p);
	vec2 h = fract(sin(vec2(ip, ip + 1.) * 12.3456) * 43.5453);
	return mix(h.x, h.y, p * p * (3. - 2. * p));
}

float smin(float a, float b, float k) {
	float h = clamp(.5 + .5 * (b - a) / k, 0., 1.);
	return mix(b, a, h) - k * h * (1. - h);
}

Hit minH(Hit a, Hit b) {
	if (a.d < b.d) return a;
	return b;
}

mat2 rot(float a) {
	float c = cos(a),
	      s = sin(a);
	return mat2(c, s, -s, c);
}

float sdCyl(vec3 p, vec2 hr) {
	vec2 d = abs(vec2(length(p.xz), p.y)) - hr;
	return min(max(d.x, d.y), 0.) + length(max(d, 0.));
}

float sdCapsule(vec3 p, float h, float r) {
	p.y -= clamp(p.y, 0., h);
	return length(p) - r;
}

vec3 getRayDir(vec3 ro, vec2 uv) {
	vec3 f = normalize(-ro),
	     r = normalize(cross(vec3(0, 1, 0), f));
	return normalize(f + r * uv.x + cross(f, r) * uv.y);
}

float wood(vec2 p) {
	p.x *= 71.;
	p.y *= 1.9;
	return n11(n21(p) * 30.);
}

Hit map(vec3 p) {
	float t,
	      f = p.y;
	p.x += .2 + cos(time * 10.) * .05;
	p.z += 3.5 + sin(time * 10.) * .05;
	p.xz *= rot(time * 150.);
	p.xy *= rot(mix(.02, .04, sin(time * .001) * .5 - .5));
	p.y -= .4;
	t = 1. - abs(p.y / .4 + .07);
	return minH(Hit(f, 1, p), Hit(smin(sdCyl(p, vec2(smoothstep(0., 1., t * t * t) * .35, .4)), sdCapsule(p + vec3(0, .35, 0), .8, .01), mix(.03, .3, t * .7)), 2, p));
}

vec3 calcN(vec3 p, float t) {
	float h = .004 * t;
	vec3 n = vec3(0);
	for (int i = min(iFrame, 0); i < 4; i++) {
		vec3 e = .5773 * (2. * vec3(((i + 3) >> 1) & 1, (i >> 1) & 1, i & 1) - 1.);
		n += e * map(p + e * h).d;
	}

	return normalize(n);
}

float calcShadow(vec3 p, vec3 ld) {
	// Thanks iq.
	float s = 1.,
	      t = .1;
	for (float i = 0.; i < 20.; i++) {
		float h = map(p + ld * t).d;
		s = min(s, 15. * h / t);
		t += h;
		if (s < .001 || t > 6.) break;
	}

	return clamp(s, 0., 1.);
}

// Quick ambient occlusion.
float ao(vec3 p, vec3 n, float h) { return map(p + h * n).d / h; }

vec3 vignette(vec3 c, vec2 fc) {
	vec2 q = fc.xy / iResolution.xy;
	c *= .5 + .5 * pow(16. * q.x * q.y * (1. - q.x) * (1. - q.y), .4);
	return c;
}

vec3 lights(vec3 p, vec3 rd, float d, Hit h) {
	vec3 mat,
	     ld = normalize(vec3(6, 3, -10) - p),
	     ld2 = ld * vec3(-1, 1, 1),
	     n = calcN(p, d);
	if (h.id == 1) {
		// Table.
		mat = mix(mix(vec3(.17, .1, .05), vec3(.08, .05, .03), wood(p.xz)), vec3(.2, .16, .08), .3 * wood(p.xz * .2));
		n.x -= smoothstep(.98, 1., pow(abs(sin(p.x * 2.4)), 90.)) * .3;
		n = normalize(n);
	}
	else // Totem.
        mat = .03 * mix(vec3(.4, .3, .2), mix(vec3(.6, .3, .2), 2. * vec3(.7, .6, .5), n31(h.uv * 1e2)), n31(h.uv * 36.5));

	float ao = dot(vec3(ao(p, n, .2), ao(p, n, .5), ao(p, n, 2.)), vec3(.3, .4, .3)),
	      l1 = max(0., .1 + .9 * dot(ld, n)),
	      spe = smoothstep(0., 1., pow(max(0., dot(rd, reflect(ld, n))), 20.)) * 10. + smoothstep(0., 1., pow(max(0., dot(rd, reflect(ld2, n))), 20.)) * 2.,
	      fre = smoothstep(.7, 1., 1. + dot(rd, n));

	l1 *= mix(.4, 1., mix(calcShadow(p, ld), calcShadow(p, ld2), .3));
	return mix(mat * (l1 * ao + spe) * vec3(2, 1.6, 1.4), vec3(.005), fre);
}

vec3 march(vec3 ro, vec3 rd) {
	// Raymarch.
	vec3 p, c;
	float d = .01;
	Hit h;
	for (float i = 0.; i < 90.; i++) {
		p = ro + rd * d;
		h = map(p);
		if (abs(h.d) < .0015) break;
		if (d > 48.) return vec3(0); // Distance limit reached - Stop.
		d += h.d;
	}

	c = lights(p, rd, d, h) * exp(-d * .14);
    float f = smoothstep(-2.2, -3., p.z) * (h.id == 1 ? .4 : 1.);
	if (f > 0.0) {
		// Show reflection on the totem.
		ro = p;
		rd = reflect(rd, calcN(p, d));
		d = .1;
		for (float i = 0.; i < 90.; i++) {
			p = ro + rd * d;
			h = map(p);
			if (abs(h.d) < .002 || d > 1.) break;
			d += h.d;
		}

		c = mix(c, d > 1. ? vec3(0) : lights(p, rd, d, h), .2 * f);
	}

	return c;
}

void mainImage(out vec4 fragColor, vec2 fc) {
	time = mod(iTime * .2, 30.);
    
	vec3 ro = vec3(0, 0, -5),
	     col = vec3(0);
	ro.yz *= rot(-.13 - sin(time * .3) * .02);
	ro.xz *= rot(.07 + cos(time) * .02);
    
	for (float dx = 0.; dx <= 1.; dx++) {
		for (float dy = 0.; dy <= 1.; dy++) {
			vec2 uv = (fc + vec2(dx, dy) * .5 - .5 * iResolution.xy) / iResolution.y;
			col += march(ro, getRayDir(ro, uv));
		}
	}

	col /= 4.;

	fragColor = vec4(vignette(pow(col * 3., vec3(.45)), fc), 0);
}