

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAX_STEPS 100
#define MAX_DIST 10.
#define SURF_DIST .001

#define S smoothstep
#define T iTime

mat2 Rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c, -s, s, c);
}

vec2 GetDist(vec3 p) {

    float m = 0.8;
    
    vec3 id = vec3(1);
    float time = 0.15 * iTime;
    float n = 3.;
    for (float i = 0.; i < n; i++) {
       // id 
        id *= 0.5;
        id += vec3(step(p.x,0.), step(p.y,0.), step(p.z,0.));
        time += 2. * pi / n;
        p = abs(p) - m;
        p.xy *= Rot(time - 2. * pi / 3.);
        p.yz *= Rot(time);
        p.zx *= Rot(time + 2. * pi / 3.);
        //float s = 0.5 + 0.5 * cos(time + 4. * 0.);//step(0., cos(time + 2. * p.x));
        //m *= 0.6 * (1.-s) + s * 0.4;
               
        m *= 0.5;
       
        //m *= mix(0.25, 0.5, s);
        //m *= 0.6 + 0.4 * thc(8., 0.2 * iTime + time);
    }
    
    float i = h21(id.xy);
    float j = h21(id.yz);
    float k = h21(vec2(i, j)); // need better 3d noise
    float k2 = 0.375 + 0.125 * thc(8., 2. * pi * k + 0. * mlength(p) - 0.5 * iTime);
    
    float d = mlength(p) - 2. * m * k2;

    //d = length(p) - 2. * m;
    return vec2(0.8 * d, k);
}

float RayMarch(vec3 ro, vec3 rd) {
	float dO=0.;
    
    for(int i=0; i<MAX_STEPS; i++) {
    	vec3 p = ro + rd*dO;
        float dS = GetDist(p).x;
        dO += dS;
        if(dO>MAX_DIST || abs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

vec3 GetNormal(vec3 p) {
	float d = GetDist(p).x;
    vec2 e = vec2(.001, 0);
    
    vec3 n = d - vec3(
        GetDist(p-e.xyy).x,
        GetDist(p-e.yxy).x,
        GetDist(p-e.yyx).x);
    
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

float GetLight(vec3 p, vec3 lightPos) {
   // vec3 lightPos = vec3(0, 5, 6);
    //lightPos.xz += vec2(sin(iTime), cos(iTime))*2.;
    vec3 l = normalize(lightPos-p);
    vec3 n = GetNormal(p);
    
    float dif = clamp(dot(n, l), 0., 1.);
    float d = RayMarch(p+n*SURF_DIST*2., l);
    dif = 0.15 + 0.85 * dif * step(length(lightPos-p), d);
   // if(d<length(lightPos-p)) dif = 0.;
    
    return dif;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
	vec2 m = iMouse.xy/iResolution.xy;

    vec3 ro = vec3(0, 3, -3);
    ro.yz *= Rot(-m.y*3.14+1.);
    ro.xz *= Rot(-m.x*6.2831);
    
    vec3 rd = GetRayDir(uv, ro, vec3(0,0.,0), 1.4);
    vec3 col = vec3(0);
   
    float d = RayMarch(ro, rd);

    if(d<MAX_DIST) {
        vec3 p = ro + rd * d;
        vec3 n = GetNormal(p);
        vec3 r = reflect(rd, n);

        float dif2 = GetLight(p,vec3(4., 2., 4.));
        col = vec3(dif2);

        float dif = dot(n, normalize(vec3(1,2,3)))*.5+.5;
        col *= vec3(dif);
        
        // VERY sloppy way of doing it
        float k = GetDist(p).y;
        //col *= 0.5 + 0.5 * thc(4., 2. * pi * k + iTime);
        vec3 e = vec3(1.);
        col *= pal(k + 0.5 * pi * thc(8., 2. * pi * k + 0.1 * iTime), 
                   e, e, e, 0.5 * vec3(0.,0.33,0.66));
        //col *= 0.52 + 0.48 * cos(4. * length(p) - iTime);
    }
    
    col = pow(col, vec3(.4545));	// gamma correction
    
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define pi 3.14159

float thc(float a, float b) {
    return tanh(a * cos(b)) / tanh(a);
}

float ths(float a, float b) {
    return tanh(a * sin(b)) / tanh(a);
}

vec2 thc(float a, vec2 b) {
    return tanh(a * cos(b)) / tanh(a);
}

vec2 ths(float a, vec2 b) {
    return tanh(a * sin(b)) / tanh(a);
}

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

float mlength(vec3 p) {
    return max(max(abs(p.x), abs(p.y)), abs(p.z));
}

