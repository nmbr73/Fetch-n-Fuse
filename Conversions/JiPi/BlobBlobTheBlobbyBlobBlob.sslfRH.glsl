

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "RayMarching starting point" 
// by Martijn Steinrucken aka The Art of Code/BigWings - 2020
// The MIT License
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// Email: countfrolic@gmail.com
// Twitter: @The_ArtOfCode
// YouTube: youtube.com/TheArtOfCodeIsCool
// Facebook: https://www.facebook.com/groups/theartofcode/
//
// You can use this shader as a template for ray marching shaders

#define MAX_STEPS 400
#define MAX_DIST 10.
#define SURF_DIST .001

#define S smoothstep
#define T iTime

mat2 Rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c, -s, s, c);
}

float sdBox(vec3 p, vec3 s) {
    p = abs(p)-s;
	return length(max(p, 0.))+min(max(p.x, max(p.y, p.z)), 0.);
}


float sdBox(vec4 p, vec4 s) {
    p = abs(p)-s;
	return length(max(p, 0.))+min( max(max(p.x, p.y), max(p.z, p.w)), 0. );
}

#define pi 3.14159

float shape(vec4 q) {
    float as = 2.; 
    float ls = 0.5;
    float t = 0.5;
    //q.xw *= Rot(as * atan(q.x, q.w) + ls * length(q.xw) - 0. * iTime);// - 2. * pi / 3.);
    //q.yw *= Rot(as * atan(q.y, q.w) + ls * length(q.yw) - 0. * iTime);
    //q.zw *= Rot(as * atan(q.z, q.w) + ls * length(q.zw) - 0. * iTime);// + 2. * pi / 3.);
    float m = 1.;
    // fractal q.w (no idea what this does)
    for (int i = 0; i<5; i++) {
        q.w = sabs(q.w) - m;
        m *= 0.5;
    }
    q.xy *= Rot(as * smin(q.w, q.z) + t * iTime);
    q.yz *= Rot(as * smin(q.x, q.w) + t * iTime);
    q.zw *= Rot(as * smin(q.y, q.x) + t * iTime);
    q.wx *= Rot(as * smin(q.z, q.y) + t * iTime);
    
    // torus looks trippy as fuck but is buggy
    /*
    float r1 = 0.5;
    float r2 = 0.3;
    float d1 = length(q.xz) - r1;
    float d2 = length(vec2(q.w, d1)) - r2;
    */
    // sharper box makes curves more distinguished
    float b = 0.5 - 0.5 * thc(4., 0.5 * q.w + 0.25 * iTime);
    float m1 = mix(0.2, 0.4, b);
    float m2 = mix(0.7, 0.1, b);
    float d = sdBox(q, vec4(m1)) - m2; 
    return d;
}

float GetDist(vec3 p) {
    float w = length(p) * 2. + 0.25 * iTime;

    // tried using blacklemori's technique with q.w
    // couldnt get it working outside of the sphere
    float center = floor(w) + 0.5;
   // float neighbour = center + ((w < center) ? -1.0 : 1.0);

    vec4 q = vec4(p, w);
    //float a = atan(p.x, p.z);
    
    float me = shape(q - vec4(0,0,0,center));
    //float next = shape(q - vec4(0,0,0,neighbour)); // incorrect but looks okay
    float d = me;//smin(me, next);
    
    float d2 = length(p) - 1.;
   // d = -smin(d, -d2);
    //d = max(d, d2);
    return 0.7 * d;
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

    vec3 ro = vec3(0, 3, -3);
    ro.yz *= Rot(-m.y*3.14+1.);
    ro.xz *= Rot(-m.x*6.2831);
    
    vec3 rd = GetRayDir(uv, ro, vec3(0,0.,0), 1.8);
    vec3 col = vec3(0);
   
    float d = RayMarch(ro, rd, 1.);

    float IOR = 1.05;
    if(d<MAX_DIST) {
        vec3 p = ro + rd * d;
        vec3 n = GetNormal(p);
        vec3 r = reflect(rd, n);

        float dif = dot(n, normalize(vec3(1,2,3)))*.5+.5;
        col = vec3(dif);
        
        vec3 rdIn = refract(rd, n, 1./IOR);
        
        vec3 pEnter = p - n*SURF_DIST*30.;
        float dIn = RayMarch(pEnter, rdIn, -1.); // inside the object
        
        vec3 pExit = pEnter + rdIn * dIn; // 3d position of exit
        vec3 nExit = -GetNormal(pExit);
        
        float fresnel = pow(1.+dot(rd, n), 3.);
        col = 2.5 * vec3(fresnel);
        col *= 0.55 + 0.45 * cross(nExit, n);
        //col *= 1. - exp(-0.1 * dIn);
        col = clamp(col, 0., 1.);
       // col = 1.-col;
       vec3 e = vec3(1.);
        col *= (p.y + 0.95) * pal(1., e, e, e, vec3(0.,0.33,0.66)); //cba to lookup color
        col *= 0.6 + 0.4 * n.y;
    }
    
    col = pow(col, vec3(.4545));	// gamma correction
    col += 0.04;
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