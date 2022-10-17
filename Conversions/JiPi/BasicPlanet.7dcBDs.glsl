

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float hash(vec3 v3) {
	return fract(sin(dot(v3, vec3(12.3, 45.6, 78.9))) * 987654.321);
}

float noise(vec3 v3) {
	vec3 i = floor(v3);
	vec3 f = fract(v3);
	vec3 b = smoothstep(0.0, 1.0, f);
	vec2 bin = vec2(0.0, 1.0);
	return 2.0 * mix(
		mix(
			mix(hash(i + bin.xxx), hash(i + bin.yxx), b.x),
			mix(hash(i + bin.xyx), hash(i + bin.yyx), b.x),
			b.y
		),
		mix(
			mix(hash(i + bin.xxy), hash(i + bin.yxy), b.x),
			mix(hash(i + bin.xyy), hash(i + bin.yyy), b.x),
			b.y
		),
		b.z
	) - 1.0;
}

vec3 rotate_y(vec3 p, float t) {
    vec3 a = vec3(0.0, 1.0, 0.0);
	return mix(dot(a, p) * a, p, cos(t)) + cross(a, p) * sin(t);
}


const float RS = 5.0;
const float C = 0.1; // height of cloud
const float RC = RS + C;
const float H = 1.0;

float fbm_core(vec3 p, float amp, float freq, float mul_amp, float mul_freq) {
	float h = 0.0;
	for (int i = 0; i < 6; ++i) {
		h += amp * noise(p * freq);
		amp *= mul_amp;
		freq *= mul_freq;
	}
	return h;
}

float height(vec3 p) {
	if (length(p) > RC) return 0.0;
	p = normalize(p) * RS;
	p = rotate_y(p, iTime * 0.1);
	return fbm_core(p, 0.8, 0.2, 0.4, 2.7) * H;
}

float d_sea(vec3 p) {
	return length(p) - RS;
}

float d_ground(vec3 p) {
	return length(p) - (max(height(p), 0.0) + RS);
}

float d_sphere(vec3 p, float radius) {
	return length(p) - radius;
}

vec4 rt_sphere(vec3 p, vec3 rd, float radius) {
	float hit = 0.0;
	for(int i = 0; i < 100; ++i) {
		float d = d_sphere(p, radius);
		p += d * rd;
		if (d < 0.01) {
			hit = 1.0;
			break;
		}
	}
	return vec4(p, hit);
}

vec3 normal_ground(vec3 p) {
	mat3 k = mat3(p, p, p) - mat3(0.001);
	return normalize(d_ground(p) - vec3(d_ground(k[0]), d_ground(k[1]), d_ground(k[2])));
}

vec3 light_dir() {
    return normalize(vec3(0.6 * sin(0.1 * iTime), 0.5, 1.0));
}

vec3 shade_star(vec3 p, vec3 rd) {
	float h = height(p);
	vec3 snow = vec3(1.0);
	vec3 sand = vec3(0.7, 0.66, 0.53);
	vec3 grass = vec3(0.1, 0.7, 0.3);
	float snow_r = exp(-abs(h - 0.5 * H) * 20.0);
	float sand_r = exp(-abs(h - 0.25 * H) * 20.0);
	float grass_r = exp(-abs(h) * 20.0);
	float sum_r = snow_r + sand_r + grass_r;
	vec3 dif_mat = (snow * snow_r + sand * sand_r + grass * grass_r) / sum_r;
	vec3 ng = normal_ground(p);
    vec3 L = light_dir();
	float dif_pow = max(0.0, dot(L, ng));
	vec3 ground = vec3(0.1) + dif_mat * dif_pow;

	vec3 ns = normalize(p);
	vec3 rs = reflect(rd, ns);

	vec3 sea = vec3(0.1, 0.4, 0.9) * (0.1 * dif_pow - 2.0 * h / H + 0.9 * exp(max(dot(L, rs), 0.0))) + 0.1 * sin(h/H);
	return h < 0.0 ? sea : ground;
}

float cloud(vec3 p) {
	p = rotate_y(p, iTime * -0.1);
	p += 0.1 * iTime;
	return pow(clamp(abs(fbm_core(p, 1.1, 0.3, 0.5, 2.2)), 0.0, 1.0), 2.0);
}

float cloud_shadow(vec3 p) {
	p -= light_dir() * H;
	return 1.0 - cloud(p) * 0.1;
}

vec3 bg(vec2 uv) {
	float t = iTime * 0.0001;
	vec3 from = vec3(t * 2.0, t, -1.0);
	float s = 0.1, fade = 1.0;
	vec3 v = vec3(0.0);
	for (int r = 0; r < 20; ++r) {
		vec2 rot_uv = uv * mat2(cos(s), sin(s), -sin(s), cos(s));
		vec3 p = vec3(rot_uv, -s) * 0.25 + from;
		float repeat = 2.0;
		p = mod(p + 0.5 * repeat, repeat) - 0.5 * repeat;
		p *= 10.0;
		float pa = 0.0, a = 0.0;
		for (int i = 0; i < 17; ++i) {
			p = abs(p) / dot(p, p) - 0.53;
			a += abs(length(p) - pa);
			pa = length(p);
		}
		a *= a * a;
		v += vec3(s, s*s, s*s*s*s) * a * 0.0015 * fade;
		fade *= 0.75;
		s += 0.1;
	}
	return v * 0.005;
}

vec3 render_base(vec2 uv, vec3 p, vec3 rd) {
	vec4 rt = rt_sphere(p, rd, RS);
	if (rt.w > 0.0) {
		return shade_star(rt.xyz, rd) * cloud_shadow(rt.xyz);
	} else {
		vec3 sky_base_color = vec3(0.1, 0.4, 0.8);
		float sky_mix = smoothstep(RS, RS + 10.0 * C, length(rd.xy * 10.0));
		return mix(sky_base_color, bg(uv), sky_mix);
	}
}

vec3 render(vec2 uv, vec3 p, vec3 rd) {
	vec4 rt = rt_sphere(p, rd, RC);
	float ccol = rt.w > 0.0 ? cloud(rt.xyz) : 0.0;
	return mix(render_base(uv, p, rd), vec3(1.0), ccol);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {

	vec2 uv = (2.0 * fragCoord - iResolution.xy) / min(iResolution.x, iResolution.y);
	vec3 p = vec3(0.0, 0.0, 10.0);
	vec3 rd = normalize(vec3(uv, -1.));


	fragColor = vec4( render(uv, p, rd), 1.0 );

}