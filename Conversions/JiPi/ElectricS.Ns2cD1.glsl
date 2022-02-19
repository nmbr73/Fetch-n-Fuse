

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// This is a quick and messy hack. Make of it what you want.

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 st = fragCoord/min(iResolution.x, iResolution.y);
    float time = iTime;
    
    vec2 x, p, gwool, gtemp;
	float nwool, swool, wwool;
	p = vec2(0.0);
	x = vec2(st);
	swool = 12.0;
	wwool = 0.5;
    nwool = 0.0;

    for(int i=0; i<4; i++) {
	  nwool += wwool*psrdnoise(swool*x, p, sqrt(swool)*0.2*sin(time-nwool), gtemp);
	  gwool += wwool*gtemp;
	  wwool *= 0.55;
	  swool *= 2.2;
	}

    float mwool = 0.5+0.1*nwool+0.15*length(gwool);
	const vec3 gray = vec3(0.5);
	const vec3 white = vec3(1.0, 1.0, 1.0);
	const vec3 black = vec3(0.0);
	const vec3 blue = vec3(0.2,0.5,1.0);

    // Slightly over-saturated on purpose - clips to flat white
    vec3 woolcolor = mix(gray, white, 1.1*mwool);
    
    float r1 = length(x-vec2(0.5+0.02*sin(time*0.91), 0.45+0.02*sin(time*0.7)));
    float sheep1 = 1.0-aastep(0.3, r1+0.02*nwool+0.01*length(gwool));
    float r2 = length(x-vec2(1.3+0.02*sin(time*0.83), 0.6+0.02*sin(time*0.67)));
    float sheep2 = 1.0-aastep(0.25, r2+0.02*nwool+0.01*length(gwool));
    float sheep = max(sheep1, sheep2);
 
    float freqspark = 8.0;
    float ampspark = 1.0;
    float nspark = 0.0;
    vec2 g, gspark = vec2(0.0);

    // Start with two terms of similar frequency to stomp out the
    // regular "beat" of psrdnoise when it's animated rapidly,
    // and then tuck on a fractal sum
    nspark = ampspark*psrdnoise(0.5*freqspark*x*0.931,
            vec2(0.0), 1.81*freqspark*time, g);
    gspark += g*0.5*ampspark;
    nspark += ampspark*psrdnoise(0.5*freqspark*x*1.137,
            vec2(0.0), -2.27*freqspark*time, g);
    gspark += g*0.5*ampspark;
    float nflare = nspark; // Save low-frequency part for "lighting effect"
    for (int i=0; i<3; i++) {
        nspark += ampspark*psrdnoise(freqspark*x-0.2*gspark,
            vec2(0.0), 2.0*freqspark*time, g);
        gspark += g*ampspark;
        freqspark *=1.82;
        ampspark *= 0.68;
    }

    float sparkmask = 1.0 -
        smoothstep(0.2, 0.5, r1+0.02*nflare) * smoothstep(0.15, 0.4, r2+0.02*nflare);

    // Strongly over-saturated on purpose - clips to cyan and white
    vec3 bgcolor = mix(vec3(0.0,0.0,0.0), blue, 5.0*nspark*sparkmask);

    woolcolor = woolcolor + (1.0-smoothstep(0.5, 1.2, sparkmask))*max(0.0, nflare)*blue;
    vec3 mixcolor = mix(bgcolor, woolcolor*(1.0-min(3.0*r1*r1,3.0*r2*r2)), sheep);
    fragColor = vec4(mixcolor, 1.0);
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// A convenient anti-aliased step() using auto derivatives
float aastep(float threshold, float value) {
    float afwidth = 0.7 * length(vec2(dFdx(value), dFdy(value)));
    return smoothstep(threshold-afwidth, threshold+afwidth, value);
}

// psrdnoise (c) Stefan Gustavson and Ian McEwan,
// ver. 2021-12-02, published under the MIT license:
// https://github.com/stegu/psrdnoise/
float psrdnoise(vec2 x, vec2 period, float alpha, out vec2 gradient)
{
	vec2 uv = vec2(x.x+x.y*0.5, x.y);
	vec2 i0 = floor(uv), f0 = fract(uv);
	float cmp = step(f0.y, f0.x);
	vec2 o1 = vec2(cmp, 1.0-cmp);
	vec2 i1 = i0 + o1, i2 = i0 + 1.0;
	vec2 v0 = vec2(i0.x - i0.y*0.5, i0.y);
	vec2 v1 = vec2(v0.x + o1.x - o1.y*0.5, v0.y + o1.y);
	vec2 v2 = vec2(v0.x + 0.5, v0.y + 1.0);
	vec2 x0 = x - v0, x1 = x - v1, x2 = x - v2;
	vec3 iu, iv, xw, yw;
	if(any(greaterThan(period, vec2(0.0)))) {
		xw = vec3(v0.x, v1.x, v2.x);
		yw = vec3(v0.y, v1.y, v2.y);
		if(period.x > 0.0)
			xw = mod(vec3(v0.x, v1.x, v2.x), period.x);
		if(period.y > 0.0)
			yw = mod(vec3(v0.y, v1.y, v2.y), period.y);
		iu = floor(xw + 0.5*yw + 0.5); iv = floor(yw + 0.5);
	} else {
		iu = vec3(i0.x, i1.x, i2.x); iv = vec3(i0.y, i1.y, i2.y);
	}
	vec3 hash = mod(iu, 289.0);
	hash = mod((hash*51.0 + 2.0)*hash + iv, 289.0);
	hash = mod((hash*34.0 + 10.0)*hash, 289.0);
	vec3 psi = hash*0.07482 + alpha;
	vec3 gx = cos(psi); vec3 gy = sin(psi);
	vec2 g0 = vec2(gx.x, gy.x);
	vec2 g1 = vec2(gx.y, gy.y);
	vec2 g2 = vec2(gx.z, gy.z);
	vec3 w = 0.8 - vec3(dot(x0, x0), dot(x1, x1), dot(x2, x2));
	w = max(w, 0.0); vec3 w2 = w*w; vec3 w4 = w2*w2;
	vec3 gdotx = vec3(dot(g0, x0), dot(g1, x1), dot(g2, x2));
	float n = dot(w4, gdotx);
	vec3 w3 = w2*w; vec3 dw = -8.0*w3*gdotx;
	vec2 dn0 = w4.x*g0 + dw.x*x0;
	vec2 dn1 = w4.y*g1 + dw.y*x1;
	vec2 dn2 = w4.z*g2 + dw.z*x2;
	gradient = 10.9*(dn0 + dn1 + dn2);
	return 10.9*n;
}

