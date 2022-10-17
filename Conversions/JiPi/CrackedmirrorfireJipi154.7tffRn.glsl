

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

vec2 hash( vec2 p ) {
	p = vec2( dot(p,vec2(354.3,542.8)), dot(p,vec2(185.4,196.3)) );

	return -1.0 + 2.*fract(sin(p) * 68556.4357786);
}

float noise( in vec2 p ) {
    float f1 = 0.366;
    float f2 = 0.211324865; 
	vec2 k = floor( p + (p.x+p.y) *f1);
    vec2 a = p - k + (k.x+k.y) * f2;
    vec2 s = step(a.yx,a.xy);
    vec2 b = a - s + f2;
	vec2 c = a - 1.0 + 2.0*f2;

    vec3 h = max( 0.5-vec3(dot(a,b), dot(b,b), dot(c,c) ), 0.0 );

	vec3 n = h*h*h*h*vec3( dot(a,hash(k+0.0)), dot(b,hash(k+s)), dot(c,hash(k+1.0)));

    return dot( n, vec3(70.0) );
}


float fbm ( in vec2 p ) {
    float f = 0.0;
    mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
    f  = 0.5000*noise(p); p = m*p;
    f += 0.2500*noise(p); p = m*p;
    f += 0.1250*noise(p); p = m*p;
    f += 0.0625*noise(p); p = m*p;
    f = 0.5 + 0.5 * f;
    return f;
}

vec3 map(vec2 uv) {
    vec2 s = vec2(1./630., 1./354.);
    float p =  fbm(uv);
    float h = fbm(uv + s * vec2(1., 0));
    float v = fbm(uv + s * vec2(0, 1.));
   	vec2 xy = (p - vec2(h, v))*40.;
    return vec3(xy + .2, 9.);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){

    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 fmove= vec2(-0.02, 0.0);
    vec3 m = map(uv * vec2(1., 0.3) + fmove*iTime);
    vec2 disp = clamp((m.xy - .5) * 0.15, -1., 1.);
    uv += disp;
    vec2 fmove1= vec2(-0.02, -0.3);
    vec2 uv1 = (uv * vec2(1.0, 0.5)) + fmove1 * iTime;
    
    float n = fbm(3.2 * uv1);
    float col = pow(1.0 - uv.y, 4.) * 4.;
    float colN = n * col; 

    vec3 color = colN * vec3(3.*n, 3.*n*n*n, n*n*n*n);
    vec3 color2 = colN * vec3(2.*n, 3.2*n*n*n,2.*n*n*n*n);
    color = mix(color, color2.rgb, pow(1.0 - uv.y, 1.5));
    
    fragColor = vec4(color , 1.0);
   ;
}