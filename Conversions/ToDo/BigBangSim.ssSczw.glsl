

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "MPM dambreak" by wyatt. https://shadertoy.com/view/NdSczm
// 2022-01-26 21:42:50

// Fork of "Material Point Method" by wyatt. https://shadertoy.com/view/fssyDs
// 2022-01-26 19:42:56

Main
    vec4 c = C(U);
    Q = 1.-sin(c.w-4.*c.z/R.x+vec4(1,2,3,4));
    Q *= c.w;
    
    Q += .1*D(U).wwww;
    Q = (atan(Q));
    if(U.x<3.||U.y<6.||R.x-U.x<3.||R.y-U.y<3.) Q *= 0.;
    
    //Q.xy = D(U).xy/R.y;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R; vec4 M; float T; int I;
#define Main void mainImage(out vec4 Q, vec2 U){UNIS
#define UNIS R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

float G (float w, float s) {
    return 0.15915494309*exp(-.5*w*w/s/s)/(s*s);
}
float building(vec2 U) {
    
    return 1.;

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
    
    vec2 u = U-.5*R;
    Q = A(U);
    vec4 d = D(U);
    vec4 c = C(Q.xy);
    vec2 f = .0*u/(1.+dot(u,u));
    
    for (float x = -3.; x <= 3.; x ++) 
    for (float y = -3.; y <= 3.; y ++)
    if (x!=0.||y!=0.) {
        vec4 c = C(Q.xy+vec2(x,y));
        vec4 d = D(Q.xy+vec2(x,y));
        f -= .01*(.1*c.w*c.w-4.*d.w)*vec2(x,y)/(x*x+y*y);
    }
    
    //if (length(f)>.1) f = .1*normalize(f);
    Q.zw = mix(Q.zw,c.xy,.001);
    Q.zw += f;
    Q.xy += Q.zw*inversesqrt(1.+dot(Q.zw,Q.zw));
    
    if (Q.y<1.) Q.zw *= 0.;
    if (Q.x<1.) Q.zw *= 0.;
    if (R.y-Q.y<1.) Q.zw *= 0.;
    if (R.x-Q.x<1.) Q.zw *= 0.;

    if (M.z>0.) Q.zw -= 1e-2*(M.xy-Q.xy)/(1.+length((M.xy-Q.xy)));

    if(I<1) {
    
        Q = vec4(U,0,0);
        Q.zw = .5*vec2(-u.y,u.x)/(1.+length(u));
        Q.zw -= 1.*u/(1.+length(u));
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
    vec4 a = A(d.xy);
    float o = _12(d.xy);
    if (cell(U-a.xy)&&i<1) Q[i++]= o;
    
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
        for (int k = 0; k < 4; k++) {
            if (b[k]>0.) {
                vec2 u = _21(b[k]);
                vec4 a = A(u);
                vec2 v = a.xy-U;
                float e = G(length(v),1.);
                w += e;
                Q.xyz += vec3(a.zw,u.x)*e;
                Q.w += G(length(v),1.);
            } else break;
        }
    }
    if (w>0.) Q.xy /= w;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void ZW (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(q.xy).xy)<length(U-A(Q.xy).xy)) Q.xy = q.xy;
}
Main
    vec4 c = C(U);
    Q.w = c.w;
    float m = 0.;
    for (float x = -3.; x <= 3.; x++)
    for (float y = -3.; y <= 3.; y++)
    {
        m += D(U+vec2(x,y)).w/49.;
    }
    
    Q.w = .01*Q.w+.99*m;
    
    if (U.x<3.||R.x-U.x<3.||U.y<3.||R.y-U.y<3.)
        Q.w *= 0.;

    Q.xy = D(U).xy;
    if (I%12==0) 
        Q.xy = U;
    else
    {
        float k = exp2(float(11-(I%12)));
        ZW(U,Q,B(U+vec2(0,k)));
        ZW(U,Q,B(U+vec2(k,0)));
        ZW(U,Q,B(U-vec2(0,k)));
        ZW(U,Q,B(U-vec2(k,0)));
    }
    
}