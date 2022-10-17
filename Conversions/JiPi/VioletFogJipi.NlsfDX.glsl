

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<


float rand(vec2 n) {
	return fract(cos(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(vec2 n) {
	const vec2 d = vec2(0.0, 1.0);
	vec2 b = floor(n), f = smoothstep(vec2(0.0), vec2(1.0), fract(n));
	return mix(mix(rand(b), rand(b + d.yx), f.x), mix(rand(b + d.xy), rand(b + d.yy), f.x), f.y);
}

float fbm(vec2 n) {
	float total = 0.0, amplitude = 1.0;
	for (int i = 0; i < 4; i++) {
		total += noise(n) * amplitude;
		n += n;
		amplitude *= 0.5;
	}
	return total;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    const vec3 c1 = vec3(124.0/255.0, 0.0/255.0, 97.0/255.0);
	const vec3 c2 = vec3(173.0/255.0, 0.0/255.0, 161.4/255.0);
	const vec3 c3 = vec3(0.2, 0.0, 0.0);
	const vec3 c4 = vec3(164.0/255.0, 1.0/255.0, 214.4/255.0);
	const vec3 c5 = vec3(0.1);
	const vec3 c6 = vec3(0.9);

	vec2 speed = vec2(0.1, 0.4);
	float shift = 1.6;
	vec2 p = fragCoord.xy * 8.0 / iResolution.xx;
	float q = fbm(p - iTime * 0.1);
	vec2 r = vec2(fbm(p + q + iTime * speed.x - p.x - p.y), fbm(p + q - iTime * speed.y));
	vec3 c = mix(c1, c2, fbm(p + r)) + mix(c3, c4, r.x) - mix(c5, c6, r.y);
	float grad = fragCoord.y / iResolution.y;
	fragColor = vec4(c * cos(shift * fragCoord.y / iResolution.y), 1.0);
	fragColor.xyz *= 1.0-grad;
}