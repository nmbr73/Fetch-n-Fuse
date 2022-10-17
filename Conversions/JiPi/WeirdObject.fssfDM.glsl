

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAX_STEPS 400
#define MAX_DIST 10.
#define SURF_DIST .001 

//nabbed from blacklemori
vec3 erot(vec3 p, vec3 ax, float rot) {
  return mix(dot(ax, p)*ax, p, cos(rot)) + cross(ax,p)*sin(rot);
}

mat2 Rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c, -s, s, c);
}

vec3 distort(vec3 p) {
    float time = 0.25 * iTime;
    
    float sc = 1.5;//exp(-0.5 * length(p));// + 0.5 * cos(0.5 * length(p) - iTime);
    
    float tp = 2.*pi/3.;
    //float val = length(p);//.;//cos(time)*p.x + cos(time + tp)*p.y + cos(time-tp)*p.z;
    
    float c  = cos(time + sc * smin(p.y, p.z) + pi * cos(0.1 * iTime));
    float c2 = cos(time + sc * smin(p.z, p.x) + pi * cos(0.1 * iTime + tp));
    float c3 = cos(time + sc * smin(p.x, p.y) + pi * cos(0.1 * iTime - tp));

    vec3 q = erot(normalize(p), normalize(vec3(c,c2,c3)), pi + 0.3 * cos(2. * length(p) - iTime));
    //q = cross(q, vec3(c3,c,c2));
    return cross(p, q);
}

float GetDist(vec3 p) {
   
    float sd = length(p - vec3(0, 2, -4)) - 1.2;
    
    //p = mix(sabs(p) - 0., sabs(p) - 1., 0.5 + 0.5 * thc(4., iTime));
       
    p.xz *= Rot(0.2 * iTime);
    p = distort(p);
    
    //p.xz *= Rot(4. * p.y + iTime);
    //p = sabs(p) - 0.25;
    
    float d = length(p) - 0.5;
    d *= 0.05; // MUCH smaller than I'd like it to be
       
    // looks okayish with torus
    /*
    float r1 = 2.;
    float r2 = 0.2;
    float d1 = length(p.xz) - r1;
    
    float a = atan(p.x ,p.z);
    vec2 u = vec2(d1, p.y);  
    u *= Rot(1.5 * a);
    u.x = abs(u.x) - r2;
    float d2 = length(u) - r2;
    d2 *= 0.05;
    */
    
    d = -smin(-d, sd); 
    
    return d;
}

float RayMarch(vec3 ro, vec3 rd, float z) {
	float dO=0.;
    
    for(int i=0; i<MAX_STEPS; i++) {
    	vec3 p = ro + rd*dO;
        float dS = z * GetDist(p);
        dO += dS;
        if(dO>MAX_DIST || abs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

vec3 GetNormal(vec3 p) {
	float d = GetDist(p);
    vec2 e = vec2(.001, 0);
    
    vec3 n = d - vec3(
        GetDist(p-e.xyy),
        GetDist(p-e.yxy),
        GetDist(p-e.yyx));
    
    return normalize(n);
}

vec3 GetRayDir(vec2 uv, vec3 p, vec3 l, float z) {
    vec3 f = normalize(l-p),
        r = normalize(cross(vec3(0,1,0), f)),
        u = cross(f,r),
        c = f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i);
    return d;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
	//vec2 m = iMouse.xy/iResolution.xy;

    vec3 ro = vec3(0, 2, -4);
    // ro.yz *= Rot(-m.y*3.14+1.);
    // ro.xz *= Rot(-m.x*6.2831);
    
    vec3 rd = GetRayDir(uv, ro, vec3(0,0.,0), 1.);
    vec3 col = vec3(0);
   
    float d = RayMarch(ro, rd, 1.);

    float IOR = 1.5;
    if(d<MAX_DIST) {
        vec3 p = ro + rd * d;
        vec3 n = GetNormal(p);

        // float dif = dot(n, normalize(vec3(1,2,3)))*.5+.5;
        // col = vec3(dif);
        
        float fresnel = pow(1.+dot(rd, n), 2.);
        col = vec3(fresnel);
        
        // p = distort(p);
        col *= 2.2 + 1.8 * thc(14., 24. * length(p) - 1. * iTime);
        col = clamp(col, 0., 1.);
        col *= 1.-exp(-1.1 - 0.5 * p.y);
        col *= 0.9 + 0.5 * n.y;
        vec3 e = vec3(1.);
        col *= pal(length(p) * 0.2 - 0.08 * iTime, e, e, e, 0.5 * vec3(0,1,2)/3.);
        col = clamp(col, 0., 1.);
        col *= 2.8 * exp(-0.8 * length(p));
    }
    
    col = pow(col, vec3(.4545));	// gamma correction
    
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define pi 3.14159

#define thc(a,b) tanh(a*cos(b))/tanh(a)
#define ths(a,b) tanh(a*sin(b))/tanh(a)
#define sabs(x) sqrt(x*x+1e-2)

vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

float h21 (vec2 a) {
    return fract(sin(dot(a.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float mlength(vec2 uv) {
    return max(abs(uv.x), abs(uv.y));
}

float mlength(vec3 uv) {
    return max(max(abs(uv.x), abs(uv.y)), abs(uv.z));
}

// (SdSmoothMin) stolen from here: https://www.shadertoy.com/view/MsfBzB
float smin(float a, float b)
{
    float k = 0.12;
    float h = clamp(0.5 + 0.5 * (b-a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}