

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float truc(vec3 p) {
	vec3 t;
	t.x = cos(p.x) + sin(p.y);
	t.y = cos(p.y) - sin(p.z);
	t.z = cos(p.z) + sin(p.x);
	return length(t) - 0.8;
}

float displace(vec3 p) {
	p = texture(iChannel0, (p.xz) / 20.).rgb / 20.;
	return p.x + p.y + p.z;
}

float map(vec3 p) {
	float d = 100.0;
	d = truc(p);
	d += displace(p);
	return d;
}

float intersect(vec3 ro, vec3 rd) {
	float t = 0.0;
	for (int i = 0; i < 50; i++) {
	float d = map(ro + rd * t);
	if (d <= 0.01) return t;
	t += d;
	}
	return 0.0;
}

vec3 normal(vec3 p) {
	float eps = 0.1;
	return normalize(vec3(
		map(p + vec3(eps, 0, 0)) - map(p - vec3(eps, 0, 0)),
		map(p + vec3(0, eps, 0)) - map(p - vec3(0, eps, 0)),
		map(p + vec3(0, 0, eps)) - map(p - vec3(0, 0, eps))
	));
}

float occ(vec3 p) {
	float ao = 0.0;
	float eps = 0.5;
	ao += map(p + vec3(eps, eps, eps));
	ao += map(p + vec3(eps, eps, -eps));
	ao += map(p + vec3(eps,-eps, eps));
	ao += map(p + vec3(eps, -eps, -eps));
	ao += map(p + vec3(-eps, eps, eps));
	ao += map(p + vec3(-eps, eps, -eps));
	ao += map(p + vec3(-eps, -eps, eps));
	ao += map(p + vec3(-eps, -eps, -eps));
	return ao / (8.0 * eps);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 uv = fragCoord.xy / iResolution.xy - 0.5;
	uv.x = uv.x * iResolution.x / iResolution.y;
	
	float t = 10.0 + iTime * 0.2;
	float cr = 32.0;
	
	vec3 ro = vec3(cos(t) * cr, 0.0, sin(t) * cr);
	vec3 ta = vec3(cos(t - 0.1) * cr, 0.0, sin(t - 0.1) * cr);
	vec3 ww = normalize(vec3(ta - ro));
	vec3 vv = vec3(0,1,0);
	vec3 uu = normalize(cross(ww, vv));
	vec3 l1 = normalize(vec3(1, 1, 1));
	vec3 l2 = normalize(vec3(-1, -1, 1));
	
	vec3 fcolor = vec3(0.0);
	for (int i = 0; i < 4; i++) {
	
		// Reinder dof ! thx
		
		const float fov = 1.0;
		vec3 er = normalize(vec3(uv.xy, fov));
		vec3 rd = er.x * uu + er.y * vv + er.z * ww;
		
		float rnd = fract(sin(iTime * 20322.1232)) / 13211.123;
		vec3 go = 0.01 * vec3((rnd - vec2(0.3)) * 2., 0.0);
		vec3 gd = normalize( er*4.0 - go );
		
		ro += go.x * uu + go.y * vv;
		rd += gd.x * uu + gd.y * vv;
		rd = normalize(rd);
		
		float d = intersect(ro, rd);
		vec3 color = vec3(0);
		
		if (d > 0.0) {
			vec3 pi = ro + rd * d;
			vec3 ni = normal(pi);
			float dif = (dot(ni, l1) + dot(ni, l2)) / 2.0;
			float spec1 = pow(max(0.5, dot(reflect(l1, ni), rd)), 20.0);
			float spec2 = pow(max(0.5, dot(reflect(l2, ni), rd)), 20.0);
			vec3 peps = ro + rd * (d - .3);
			vec3 rn = reflect(rd, ni);
			vec3 rf = texture(iChannel1, rn).xyz;
			float ao = occ(pi);
			color = vec3(1, 0, 0) * dif * ao + spec1 + spec2 + rf * 0.8;
		}
		color *= min(1.0, 9./d);
		fcolor += color - pow(length(uv.xy), 6.0) * 2.0;
	}
	fragColor = vec4(fcolor / 4.,1.0);
}