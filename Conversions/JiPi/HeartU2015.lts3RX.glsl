

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

const float dmax = 1000.0;
const int rayiter = 60;

const float wrap = 64.0;

vec3 L = normalize(vec3(0.1, 1.0, 0.5));

const vec3 tgt = vec3(0.0);
const vec3 cpos = vec3(3.0, -1.0, 7.0);

vec2 miss = vec2(1e5, -1.0);


vec2 opU(vec2 a, vec2 b) {
	return a.x < b.x ? a : b;
}


float sdRect(in vec3 pos, in vec2 rmin, in vec2 rmax) {
    vec3 pc = vec3(clamp(pos.xy, rmin, rmax), 0);
    return distance(pos, pc);                   
}

float sdDisc(in vec3 pos, in float r) {
    float l = length(pos.xy);
    vec3 pc = vec3(min(l, r)*pos.xy/l, 0);
    return distance(pos, pc);
}

float sdHeart(in vec3 pos, in float r, in float d) {
    
    pos.x = abs(pos.x);
    pos.xy = sqrt(2.)*0.5*mat2(1.,-1.,1.,1.)*pos.xy;
        
	float ds = sdRect(pos, vec2(-r+d), vec2(r,r-d));
    float dc = sdDisc(pos-vec3(r, 0, 0),r-d);
    
	return min(ds, dc)-d;

}

float sdPlane(in vec3 pos, float t) {
    return pos.x*cos(t) + pos.y*sin(t);
}
    

vec2 map(in vec3 pos) {
	
    const float r = 1.5;
    const float d = .9;
    const float x = .05;
    

    pos.y += 0.4;
    
    float t = 2.0*(iTime - 0.625*sin(2.*iTime));
    
    vec2 rval = vec2(1e6, -1);
    
    for (float i=0.; i<6.; i++) {              
        
        float h1 = sdHeart(pos, r-(2.*i)*x, d-(2.*i)*x);
        float h2 = sdHeart(pos, r-(2.*i+1.)*x, d-(2.*i+1.)*x);
        float p = i<4.?sdPlane(pos, t) : -1e6;
		rval = opU(rval, vec2(max(max(h1, -h2), p), 0.99-0.08*i));
       	t *= -1.25;

    }
    
    return rval;
    
}

vec3 hue(float h) {
	
	vec3 c = mod(h*6.0 + vec3(2, 0, 4), 6.0);
	return h >= 1.0 ? vec3(h-1.0) : clamp(min(c, -c+4.0), 0.0, 1.0);
}

vec2 castRay( in vec3 ro, in vec3 rd, in float maxd )
{
	float precis = 0.001;
    float h=precis*2.0;
    float t = 0.0;
    float m = -1.0;
    for( int i=0; i<rayiter; i++ )
    {
        if( abs(h)<precis||t>maxd ) continue;//break;
        t += h;
	    vec2 res = map( ro+rd*t );
        h = res.x;
	    m = res.y;
    }
	if (t > maxd) { m = -1.0; }
    return vec2( t, m );
}

vec3 calcNormal( in vec3 pos )
{
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 nor = vec3(
	    map(pos+eps.xyy).x - map(pos-eps.xyy).x,
	    map(pos+eps.yxy).x - map(pos-eps.yxy).x,
	    map(pos+eps.yyx).x - map(pos-eps.yyx).x );
	return normalize(nor);
}

vec3 shade( in vec3 ro, in vec3 rd ){
    
    vec3 c = vec3(0.);
    float a = 1.;
    bool hit = true;
    
    for (int i=0; i<2; ++i) {
        
        if (hit) {

            vec2 tm = castRay(ro, rd, dmax);
            vec3 b;
            
            if (tm.y >= 0.0) {
                vec3 n = calcNormal(ro + tm.x * rd);
                vec3 color = hue(tm.y) * 0.55 + 0.45;
                vec3 diffamb = (0.8*dot(n,L)+0.2) * color;
                vec3 R = 2.0*n*dot(n,L)-L;
                float spec = 0.5*pow(clamp(-dot(R, rd), 0.0, 1.0), 20.0);
                b = diffamb + spec;
                ro = ro + tm.x * rd;
                rd = reflect(rd, n);
                ro += 1e-4*rd;
                hit = true;
            } else {
                b = i>0 ? texture(iChannel0, rd).xyz*2.5 : vec3(1.);
                hit = false;
            }

            c = mix(c, b, a);
            a *= .3;
            
        }
        
    }
    
    return c;
    
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	


	const float yscl = 720.0;
	const float f = 900.0;
	
	vec2 uv = (fragCoord.xy - 0.5*iResolution.xy) * yscl / iResolution.y;

	vec3 up = vec3(0.0, 1.0, 0.0);
	
	vec3 rz = normalize(tgt - cpos);
	vec3 rx = normalize(cross(rz,up));
	vec3 ry = cross(rx,rz);
	
	float thetax = 0.0;
	float thetay = 0.0;
	
	if (max(iMouse.x, iMouse.y) > 20.0) { 
		thetax = (iMouse.y - 0.5*iResolution.y) * 3.14/iResolution.y; 
		thetay = (iMouse.x - 0.5*iResolution.x) * -6.28/iResolution.x; 
	}

	float cx = cos(thetax);
	float sx = sin(thetax);
	float cy = cos(thetay);
	float sy = sin(thetay);
	
	mat3 Rx = mat3(1.0, 0.0, 0.0, 
				   0.0, cx, sx,
				   0.0, -sx, cx);
	
	mat3 Ry = mat3(cy, 0.0, -sy,
				   0.0, 1.0, 0.0,
				   sy, 0.0, cy);

	mat3 R = mat3(rx,ry,rz);
	mat3 Rt = mat3(rx.x, ry.x, rz.x,
				   rx.y, ry.y, rz.y,
				   rx.z, ry.z, rz.z);

	vec3 rd = R*Rx*Ry*normalize(vec3(uv, f));
	
	vec3 ro = tgt + R*Rx*Ry*Rt*(cpos-tgt);
    
    L = R*Rx*Ry*Rt*L;

	fragColor.xyz = shade(ro, rd);

	
}
