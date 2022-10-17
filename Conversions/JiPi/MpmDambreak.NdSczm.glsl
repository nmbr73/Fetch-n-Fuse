

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Material Point Method" by wyatt. https://shadertoy.com/view/fssyDs
// 2022-01-26 19:42:56

Main
    vec4 c = C(U);
    Q = 1.-sin(c.w-4.*c.z/R.x+vec4(1,2,3,4));
    Q *= c.w;
    
    if(U.x<3.||U.y<6.||R.x-U.x<3.||R.y-U.y<3.) Q *= 0.;
    
    //Q = D(U)/R.x/R.x;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R; vec4 M; float T; int I;
#define Main void mainImage(out vec4 Q, vec2 U){UNIS
#define UNIS R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
float erf(in float x) {
    //x *= std;
    //return sign(x) * sqrt(1.0 - exp(-1.239192 * x * x));
    return sign(x) * sqrt(1.0 - exp2(-1.787776 * x * x)); // likely faster version by @spalmer
}
float erfstep (float a, float b, float x) {
    return .5*(erf(b-x)-erf(a-x));
}
float G (float w, float s) {
    return 0.15915494309*exp(-.5*w*w/s/s)/(s*s);
}
float building(vec2 U) {
    
    if (U.x<.5*R.x) return 1.;
    return 0.;

}
bool cell (vec2 u) {
    return u.x>=0.&&u.y>=0.&&u.x<1.&&u.y<1.;
}
float _12(vec2 U) {
    U = floor(U);
    return U.x+U.y*R.x;
}
vec2 _21(float i) {
    return .5+vec2(mod(i,R.x),floor(i/R.x));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main
    
    if (building(U)==0.) {Q = vec4(0); return;}

    Q = A(U);
    vec4 d = D(U);
    vec4 c = C(Q.xy);
    vec2 f = vec2(0,-.2/R.y);
    
    for (float x = -2.; x <= 2.; x ++) 
    for (float y = -2.; y <= 2.; y ++)
    if (x!=0.||y!=0.) {
        vec4 cc = C(Q.xy+vec2(x,y));
        f -= .05*c.w*cc.w*(cc.w-1.)*vec2(x,y)/(x*x+y*y);
    }
    
    if (length(f)>.1) f = .1*normalize(f);
    Q.zw = mix(Q.zw,c.xy,.5);
    Q.zw += f;
    Q.xy += .5*f+Q.zw*inversesqrt(1.+dot(Q.zw,Q.zw));
    
    if (Q.y<1.) Q.zw *= 0.;
    if (Q.x<1.) Q.zw *= 0.;
    if (R.y-Q.y<1.) Q.zw *= 0.;
    if (R.x-Q.x<1.) Q.zw *= 0.;

    if (M.z>0.) Q.zw -= 1e-2*(M.xy-Q.xy)/(1.+length((M.xy-Q.xy)));

    if(I<1) {
    
        Q = vec4(U,0,0);
        // if (length(U-vec2(.9)*R)<.02*R.x) Q.zw = vec2(-2.5,-1.5);
    }
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<

Main
    Q = vec4(-1);
    int i = 0;
    for (int x=-2;x<=2;x++)
    for (int y=-2;y<=2;y++) {
       vec4 b = B(U+vec2(x,y));
       for (int k = 0; k < 4; k++) if (b[k]>0.) {
           vec2 u = _21(b[k]);
           vec4 a = A(u);
           if (cell(a.xy-U))
               Q[i++] = b[k];
           if (i>3) break;
       }
       if (i>3) break;
    }
    vec4 d = D(U);
    vec4 a = A(_21(d.x));
    if (cell(a.xy-U)&&i<4
        &&d.x!=Q.x&&d.x!=Q.y&&d.x!=Q.z&&d.x!=Q.w
    ) Q[i]= d.x;
    
    if (I<1) Q = vec4(_12(U),0,0,0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main
    Q = vec4(0);
    float w = 0.;
    for (float x = -3.; x<=3.; x++)
    for (float y = -3.; y<=3.; y++)
    {
        
        vec4 b = B(U+vec2(x,y));
        float s = 1.;
        float n = dot(vec4(b.x>0.,b.y>0.,b.z>0.,b.w>0.),vec4(1));
        for (int k = 0; k < 4; k++) {
            if (b[k]>0.) {
                vec2 u = _21(b[k]);
                vec4 a = A(u);
                vec2 v = a.xy-U;
                float e = G(length(v),s);
                w += e;
                Q.xyz += vec3(a.zw,u.x)*e;
                Q.w += e;
            } else break;
        }
    }
    if (w>0.) Q.xy /= w;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void XY (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(_21(q.x)).xy)<length(U-A(_21(Q.x)).xy)) Q.x = q.x;
}
void ZW (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(_21(q.y)).xy)<length(U-A(_21(Q.y)).xy)) Q.y = q.y;
}
Main
    Q = B(U);
    for (int x=-1;x<=1;x++)
    for (int y=-1;y<=1;y++) {
        XY(U,Q,B(U+vec2(x,y)));
    }
    
    if (I%12==0) 
        Q.y = _12(U);
    else
    {
        float k = exp2(float(11-(I%12)));
        ZW(U,Q,B(U+vec2(0,k)));
        ZW(U,Q,B(U+vec2(k,0)));
        ZW(U,Q,B(U-vec2(0,k)));
        ZW(U,Q,B(U-vec2(k,0)));
    }
    XY(U,Q,Q.yxzw);
    
    if (I<1) Q = vec4(_12(U));
}