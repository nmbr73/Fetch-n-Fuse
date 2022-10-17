

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAX_STEPS 400
#define MAX_DIST 50.
#define SURF_DIST .001

//nabbed from blacklemori
vec3 erot(vec3 p, vec3 ax, float rot) {
  return mix(dot(ax, p)*ax, p, cos(rot)) + cross(ax,p)*sin(rot);
}

mat2 Rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c, -s, s, c);
}

float sdBox(vec3 p, vec3 s) {
    p = abs(p)-s;
	return length(max(p, 0.))+min(max(p.x, max(p.y, p.z)), 0.);
}

vec3 distort(vec3 p) {
    float time = 0.5 * iTime;
    
    vec3 q = p;
    float m = 3.5 + 3. * cos( 0.4 * length(p) -  0.5 * iTime);
    float th = 0.2 * iTime;
    for (float i = 0.; i < 9.; i++) {
        th += -0.1 * iTime;
        q.xy *= Rot(th);
        q.zy *= Rot(th);
        q = abs(q) - m; //sabs cool too
        m *= 0.28 + 0.1 * i;
    }
    
    float spd = 0.01;
    //float time = iTime;
    float cx = cos(time);
    float cy = cos(time + 2. * pi / 3.);
    float cz = cos(time - 2. * pi / 3.);
    q = erot(q, normalize(vec3(cx,cy,cz)), iTime);
    return cross(p, q);
}

float GetDist(vec3 p) {
   
    
    float sd = length(p - vec3(0, 3., -3.5)) - 2.2;
    
    //p = mix(sabs(p) - 0., sabs(p) - 1., 0.5 + 0.5 * thc(4., iTime));
    
    p = distort(p);
    //p.xz *= Rot(4. * p.y + iTime);
   // p = sabs(p) - 0.25;
    float d = length(p) - 1.; // was 0.8
    d *= 0.05; //smaller than I'd ike it to be
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
	vec2 m = iMouse.xy/iResolution.xy;

    vec3 ro = vec3(0, 3, -3.5);
   // ro.yz *= Rot(-m.y*3.14+1.);
    //ro.xz *= Rot(-m.x*6.2831);
    
    vec3 rd = GetRayDir(uv, ro, vec3(0,0.,0), 0.5);
    vec3 col = vec3(0);
   
    float d = RayMarch(ro, rd, 1.);

    float IOR = 1.5;
    if(d<MAX_DIST) {
        vec3 p = ro + rd * d;
        vec3 n = GetNormal(p);
        //vec3 r = reflect(rd, n);
        /*
        vec3 rdIn = refract(rd, n, 1./IOR);
        vec3 pIn = p - 30. * SURF_DIST * n;
        float dIn = RayMarch(pIn, rdIn, -1.);

        vec3 pExit = pIn + dIn * rdIn;
        vec3 nExit = GetNormal(pExit);
        */
        float dif = dot(n, normalize(vec3(1,2,3)))*.5+.5;
       // col = vec3(dif);
        
        //float fresnel = 1.-pow(1.+dot(rd, n), 1.);
       // col = 1. * vec3(fresnel);
        
        col = mix(vec3(dif), 0.5 + 0.5 * n, exp(-0.2 * length(p)));
        //p = distort(p);
       
        col *= 1. + 0.6 * thc(3., 4. * n.y - 0. * iTime);
        col = clamp(col, 0., 1.);
       // col *= 1.-exp(-0.5 - 0.5 * p.y);
        
        vec3 e = vec3(1.);
        col *= pal(length(p) * 0.1 + -0.05, e, e, e, 0.4 * vec3(0,1,2)/3.);
        col = clamp(col, 0., 1.);
        col *= 2. * exp(-0.2 * length(p));
        col *= 0.8 + 0.2 * n.y;
       
        //col += dif;
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