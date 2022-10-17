

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAX_STEPS 100
#define MAX_DIST 50.
#define SURF_DIST 0.0001

#define pi 3.14159

vec3 pal(in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d) {
    return a + b*cos( 6.28318*(c*t+d) );
}

mat2 Rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c, -s, s, c);
}

vec3 distort(vec3 p) {
    float o = 2.* pi / 3.;
    float t = 3. * length(p) - 0.5 * iTime;
   // p = abs(p) - 0.5;
    p.xy *= Rot(t - o);
    p.yz *= Rot(t);
    p.zx *= Rot(t + o);
    return fract(0.8 * p) - 0.5;
}

float GetDist(vec3 p) {
   
    p = distort(p); 
    float d = length(p.xz) - 0.5;
    
    // lower k => more "fog"
    float k = 0.25;
    //return length(p) -0.3 + SURF_DIST;
    return k * length(vec2(d, p.y)) + 1. * SURF_DIST;
}

float RayMarch(vec3 ro, vec3 rd, float z) {
	
    float dO=0.;
    
    for(int i=0; i<MAX_STEPS; i++) {
    	vec3 p = ro + rd*dO;
        float dS = GetDist(p);
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

    float t = 0.125 * iTime, o = 2. * pi / 3.;
    vec3 ro = 3. * vec3(cos(t - o), cos(t), cos(t + o));

    vec3 rd = GetRayDir(uv, ro, vec3(0), 0.95);
    vec3 col = vec3(0);
   
    float d = RayMarch(ro, rd, 1.);

    if(d<MAX_DIST) {
        vec3 p = ro + rd * d;
        vec3 n = GetNormal(p);

        vec3 dp = distort(p);

        float dif = dot(n, normalize(vec3(1,2,3)))*.5+.5;
       // col = vec3(step(0., dif));
        
        // darken with distance from origin
        float v = exp(-0.31 * length(p));
        
        // idk what this does
        v = smoothstep(0., 1., v);
        v *= v;
      
        // color + lighten
        vec3 e = vec3(1);
        col = v * pal(0.77 + 0.15 * length(p), e, e, e, 0.8 * vec3(0,1,2)/3.);    
        //col -= 0.1;
    }
    
    fragColor = vec4(col,1.0);
}