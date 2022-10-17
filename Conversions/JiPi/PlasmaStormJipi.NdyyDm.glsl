

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Author: paperu
// Title: plasma storm

// 3D noise, from : https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}
float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

mat2 rot(in float a) { return mat2(cos(a),sin(a),-sin(a),cos(a)); }

// by decreasing RES_INV value we can get better precision but at the cost of a worse framerate
#define RES_INV 5.

// some alternatives (ALT1 to ALT4)
#define ALT1

#define MAX_IT_VOL (100*4)/int(RES_INV)
#define STEP_SIZE .01*RES_INV
#define MAX_COL 17./RES_INV*RES_INV*RES_INV
vec3 rmvolum(in vec3 p, in vec3 r, in float t)
{
    vec3 cout = vec3(0.);
    
    for(int i = 0; i < MAX_IT_VOL; i++)
    {
        vec3 q = p;
        float f = smoothstep(0.,1.,q.z*q.z*.2 + cos(t*.1 + q.z*.2));
        q.xy *= rot(-t*.172 - q.z*f);
        q.xz *= rot(t*.48 - q.y*f);
        q.xy *= rot(t*.33 + q.z*f);

        vec3 c1 = vec3(
        	noise(q*1.5 + .15 + t),
            noise(q*1.5 + t),
            noise(q*1.5 - .15 + t)
        );
        float c2 = noise(q*4. + t);
        float c3 = noise(q*8. + t);
        float c4 = noise(q*16. + t);

#ifdef ALT1
        vec3 c = c1*.6 + c2*.2 + c3*.1 + c4*.1;
#endif
#ifdef ALT2
        vec3 c = c1*.7 + c2*.2 + c3*.1;
#endif
#ifdef ALT3
        vec3 c = c1*.9 + c2*.1;
#endif
#ifdef ALT4
        vec3 c = c1;
#endif
        
        cout += c*c*c*c*c*c*c;
        p += r*STEP_SIZE;
    }
    
	return cout/MAX_COL;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 st = (fragCoord.xy - iResolution.xy*.5)/iResolution.y;
	
    fragColor = vec4(rmvolum(vec3(0.,0.,-2.), normalize(vec3(st,.17)), iTime*.75), 1.0);
}