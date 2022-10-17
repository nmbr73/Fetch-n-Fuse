

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAX_STEPS 400
#define MAX_DIST 100.
#define SURF_DIST .001

#define FK(k) floatBitsToInt(k*k/7.)^floatBitsToInt(k)
float hash(float a, float b) {
    int x = FK(a), y = FK(b);
    return float((x*x+y)*(y*y-x)-x)/2.14e9;
}

vec3 erot(vec3 p, vec3 ax, float ro) {
  return mix(dot(ax, p)*ax, p, cos(ro)) + cross(ax,p)*sin(ro);
}

vec3 face(vec3 p) {
     vec3 a = abs(p);
     return step(a.yzx, a.xyz)*step(a.zxy, a.xyz)*sign(p);
}

float sdBox(vec3 p, vec3 s) {
    p = abs(p)-s;
	return length(max(p, 0.))+min(max(p.x, max(p.y, p.z)), 0.);
}

vec3 getRo() {
    vec2 m = iMouse.xy/iResolution.xy;
    float r = 3.;
    float t = 0.2 * iTime;
    vec3 ro = vec3(r * cos(t), 2, r * cos(t) + sin(t));
    //ro.yz *= Rot(-m.y*3.14+1.);
    //ro.xz *= Rot(-m.x*6.2831);
    return ro;
}

float GetDist(vec3 p) {
    float sc = 0.125;
    
    //p.xz *= Rot(.5 * length(p.xz) - 0.1 * iTime);
    float c1 = test(sc * p.xy, iResolution.y);
    float c2 = test(sc * p.yz, iResolution.y);
    float c3 = test(sc * p.zx, iResolution.y);

    p.y += 0.05 * cos(atan(p.x, p.z) + iTime);
  
    float r1 = 1.;
    float r2 = 1.;
    float d1 = length(p.xz) - r1;
    float d2 = length(vec2(d1,p.y)) - r2;
    //d2 = length(p) - 1.5;
     d2 += (0.5 + 0.5 * thc(4., iTime/3.)) * 0.5 * max(max(c1,c2), c3);
    return 0.15 * d2;
}

float RayMarch(vec3 ro, vec3 rd, float z) {
	
    float dO=0.;
    float s = sign(z);
    for(int i=0; i<MAX_STEPS; i++) {
    	vec3 p = ro + rd*dO;
        float dS = GetDist(p);
        if (s != sign(dS)) { z *= 0.5; s = sign(dS); }
        if(abs(dS)<SURF_DIST || dO>MAX_DIST) break;
        dO += dS*z; 
    }
    
    return min(dO, MAX_DIST);
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
	
    vec3 ro = getRo();
    
    vec3 rd = GetRayDir(uv, ro, vec3(0), 1.5);
    vec3 col = vec3(0);
   
    float d = RayMarch(ro, rd, 1.);

    float IOR = 1.05;
    vec3 p = ro + rd * d;
    if(d<MAX_DIST) {
        
        vec3 n = GetNormal(p);
        vec3 r = reflect(rd, n);

        vec3 pIn = p - 4. * SURF_DIST * n;
        vec3 rdIn = refract(rd, n, 1./IOR);
        float dIn = RayMarch(pIn, rdIn, -1.);
        
        vec3 pExit = pIn + dIn * rdIn;
        vec3 nExit = -GetNormal(pExit); // *-1.; ?

        float dif = dot(n, normalize(vec3(1,2,3)))*.5+.5;
        col = vec3(dif);
        
        float sc = 0.5;
        float c1 = test(sc * p.xy, iResolution.y);
        float c2 = test(sc * p.yz, iResolution.y);
        float c3 = test(sc * p.zx, iResolution.y);

        vec3 c4 = texture(iChannel1, sc + sc * p.xy).rgb;
        vec3 c5 = texture(iChannel1, sc + sc * p.yz).rgb;
        vec3 c6 = texture(iChannel1, sc + sc * p.zx).rgb;

        float fres = pow(clamp(1. + dot(rd, n),0.,1.), 1.);
        //col += fres;
        
        vec3 an = abs(n);
        vec3 c = vec3(c1 * n.z + c2 * n.x + c3 * n.y);
        vec3 cc = c4 * n.z + c5 * n.x + c6 * n.y;
        
        //col = c;
        col = mix(col * cc, c, cc * fres);
        //col *= n.y;
        
        vec3 e = vec3(0.5);
        col += exp(0.5-8. * abs(cos(12. * log(length(p.xz)) - iTime)))
            * pal(max(max(c1,c2),c3) - c1 * c2 * c3, e, e, e, 0.55 * vec3(0,1,2)/3.);
        
        //col = c;
    }
    col = mix(col, vec3(1), exp(-10. * length(p.xz)));
    col = pow(col, vec3(.4545));	// gamma correction
    
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define pi 3.14159

#define thc(a,b) tanh(a*cos(b))/tanh(a)
#define ths(a,b) tanh(a*sin(b))/tanh(a)
#define sabs(x) sqrt(x*x+1e-2)
//#define sabs(x, k) sqrt(x*x+k)-0.1

#define Rot(a) mat2(cos(a), -sin(a), sin(a), cos(a))

float test(in vec2 uv, float res)
{
    uv -= floor(uv) + 0.5;

    float k = 10. / res;
    float m = 0.25;
    
    float d = length(uv);
    float s = smoothstep(-k, k, -length(uv) + m);
       
    for (int i = 0; i < 6; i++) {
        uv = abs(uv) - m;
        m *= 0.5;
        s = max(s, smoothstep(-k, k, -length(uv) + m));
    }
    
    return s;
}

float sfloor(float a, float b) {
    return floor(b) + 0.5 + 0.5 * tanh(a * (fract(b) - 0.5)) / tanh(0.5 * a);
}

float cc(float a, float b) {
    float f = thc(a, b);
    return sign(f) * pow(abs(f), 0.25);
}

float cs(float a, float b) {
    float f = ths(a, b);
    return sign(f) * pow(abs(f), 0.25);
}

vec3 pal(in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d) {
    return a + b*cos( 6.28318*(c*t+d) );
}

float h21(vec2 a) {
    return fract(sin(dot(a.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float mlength(vec2 uv) {
    return max(abs(uv.x), abs(uv.y));
}

float mlength(vec3 uv) {
    return max(max(abs(uv.x), abs(uv.y)), abs(uv.z));
}

float smin(float a, float b) {
    float k = 0.12;
    float h = clamp(0.5 + 0.5 * (b-a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}