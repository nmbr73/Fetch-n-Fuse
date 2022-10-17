

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

#define MAX_STEPS 100
#define MAX_DIST 100.
#define SURF_DIST .001

#define S smoothstep
#define T iTime

mat2 Rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c, -s, s, c);
} 

float sdBox(vec3 p, vec3 s) {
    float a = atan(p.z, p.x);
    float b = atan(p.z, p.y);
    p = 1.1 + .1 * cos(p  + p.x * 8. + iTime)-s;
    //p.xz += .5 + .5 * cos(4. * p.y + iTime);

	return length(max(p, 0.))+min(max(p.x, max(p.y, p.z)), 0.);
}


float myLength(vec2 u) {
    float a = atan(u.x, u.y);
    float b = .5 + .5 * cos(iTime);
    float n = 3. - 18. * a * a;
    u *= vec2(cos(n * a + iTime), sin(n * a + iTime));
    return length(u * cos(a));
}

float GetDist(vec3 p) {
   // float d = sdBox(p, vec3(1));
    //d = mix(length(p) - 0.2, d, .5 );
    float b = .5 + .5 * cos(iTime);
    float a = atan(p.x,p.z);
    float r1 = 0.9;
    float r2 = 0.9;
    float d1 = length(p.xz) - r1;
    float d = p.y * (2. + cos(3. * p.y * b - 18. * a)) * myLength(vec2(d1,p.y)) - r2;   
    
    return 0.05 * d;
}

float RayMarch(vec3 ro, vec3 rd) {
	float dO=0.;
    
    for(int i=0; i<MAX_STEPS; i++) {
    	vec3 p = ro + rd*dO;
        float dS = GetDist(p);
        dO += dS;
        if(dO>MAX_DIST || abs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

vec3 GetNormal(vec3 p) {
	float d = GetDist(p);
    vec2 e = vec2(.01, 0);
    
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

vec3 Bg(vec3 rd) {
    float k = rd.y*.5 + .5;
    
    vec3 col = mix(vec3(.5,0.,0.),vec3(0.),k);
    return col;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
	vec2 m = iMouse.xy/iResolution.xy;

    //float time
    vec3 ro = vec3(6. * cos(0.314159 * iTime), 4., 6. * sin(0.2 * iTime));
    //ro.yz *= Rot(-m.y*3.14+1.);
    //ro.xz *= Rot(-m.x*6.2831);
    
    vec3 rd = GetRayDir(uv, ro, vec3(0,1.,0), 2.2);
    vec3 col = vec3(0);
   
    float d = RayMarch(ro, rd);

    if(d < MAX_DIST) {
        vec3 p = ro + rd * d;
        vec3 n = GetNormal(p);
        vec3 r = reflect(rd, n);
        vec3 rf = refract(rd, n,0.1);
        
        float dif = dot(n, normalize(vec3(1,2,3)))*.5+.5;
        col -= .2 *vec3(dif);
        
        float b = .5 + .5 * cos(iTime);
        
        uv = fragCoord.xy / iResolution.xy;
        col += 1.5 * texture(iChannel0,r / (0.5 + n)).rgb;
         
        float a = atan(rf.z, rf.x);
        float c = atan(rf.z,rf.y);
        float k = 0.25;
        col.r += 0.15 + k * cos(10. * rf.y + 4. * iTime - 3.1415 / 2.);
        col.g += 0.15 + k * cos(10. * rf.y + 4. * iTime);
        col.b += 0.15 + k * cos(10. * rf.y + 4. * iTime + 3.1415 / 2.);
        
        vec3 col2 = col;
        col.r *= col2.g;
        col.g *= col2.b;
        col.b *= col2.r;
        
        col /= 1.-pow(abs(sin(0.2 * length(p.xz) + .5 * p.y - 1. * iTime)),10.);
        col *= (1.- 0.1 * length(p.xz)) * .25 * col;
        
    }
    
    //col.r =0.02;
    col = pow(col, vec3(.4545));	// gamma correction
    
    fragColor = vec4(col,1.0);
}