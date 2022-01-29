

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Material Point Method" by wyatt. https://shadertoy.com/view/fssyDs
// 2022-01-23 20:24:27

Main
    
    vec4 b = B(U);
    vec4 a = A(b.xy);
    vec4 d = D(b.xy);
    vec4 c = C(U);
    Q = mix(vec4(.8),d,smoothstep(2.,1.,length(U-a.xy)));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R; vec4 M; float T; int I;
#define Main void mainImage(out vec4 Q, vec2 U){UNIS
#define UNIS R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define pi 3.14159265359

float building(vec2 U) {
    
    if (U.y<.9*R.y&&abs(U.x-.3*R.x)<25.)
        return 1.;
    
    if (U.x>.5*R.x&&U.y<.8*R.y
        &&mod(U.x+U.y,6.)<3.)
        return 1.;
    return 0.;

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main
    
    if (building(U)==0.) {Q = vec4(0); return;}

    Q = A(U);
    vec4 d = D(U);
    vec4 c = C(Q.xy);
    vec2 f = vec2(0,-.5/R.y);
    
    for (float x = -3.; x <= 3.; x ++) 
    for (float y = -3.; y <= 3.; y ++)
    if (x!=0.||y!=0.) {
        vec4 c = C(Q.xy+vec2(x,y));
        vec4 b = B(Q.xy+vec2(x,y));
        vec4 a = A(b.xy);
        
        if (U.x<.5*R.x) {
            vec2 u = abs(Q.xy+vec2(x,y)-a.xy);
            if (u.x>.5||u.y>.5)continue;
            vec2 r = a.xy-Q.xy;
            float l = length(r);
            if (l<1.||l>6.) continue;
            float L = length(U-b.xy);
            if ((l-L)<.08*L) 
                f += 3e-1*r*(l-L)/l/L/l;
        }
        else {
            f -= .05*c.w*(c.w-2.)*vec2(x,y)/(x*x+y*y);
        } 
    }

    Q.zw = mix(Q.zw,c.xy,.3);
    Q.zw += f;
    Q.xy += .5*f+Q.zw*inversesqrt(1.+dot(Q.zw,Q.zw));
    
    if (Q.y<1.) Q.y=1., Q.w *= -1.;
    if (Q.x<1.) Q.x=1., Q.z *= -1.;
    if (R.y-Q.y<1.) Q.y=R.y-1., Q.w *= -1.;
    if (R.x-Q.x<1.) Q.x=R.x-1., Q.z *= -1.;

    if (M.z>0.&&U.x>.5*R.x) Q.zw -= 3e-2*(M.xy-Q.xy)/(1.+length((M.xy-Q.xy)));

    if(I<1||(U.x<.5*R.x&&U.y<2.)) {
    
        Q = vec4(U,0,0);
        if (U.x>.5*R.x) Q.w = -.2;
   }
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void XY (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(q.xy).xy)<length(U-A(Q.xy).xy)) Q.xy = q.xy;
}
void ZW (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(q.zw).xy)<length(U-A(Q.zw).xy)) Q.zw = q.zw;
}
Main
    Q = B(U);
    for (int x=-1;x<=1;x++)
    for (int y=-1;y<=1;y++) {
        XY(U,Q,B(U+vec2(x,y)));
    }
    
    if (I%12==0) 
        Q.zw = U;
    else
    {
        float k = exp2(float(11-(I%12)));
        ZW(U,Q,B(U+vec2(0,k)));
        ZW(U,Q,B(U+vec2(k,0)));
        ZW(U,Q,B(U-vec2(0,k)));
        ZW(U,Q,B(U-vec2(k,0)));
    }
    XY(U,Q,Q.zwxy);
    
    if (I<1) Q = vec4(U,U);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main
    Q = vec4(0);
    vec4 w = vec4(0);
    for (float x = -3.; x<=3.; x++)
    for (float y = -3.; y<=3.; y++)
    {
        
        vec4 b = B(U+vec2(x,y));
        vec4 a = A(b.xy);
        vec4 d = D(b.xy);
        vec2 v = a.xy-U;
        vec4 e = 1./(1.+vec4(1)*(x*x+y*y));
        vec2 u = abs(U+vec2(x,y)-a.xy);
        if (u.x>.5||u.y>.5) continue;
        
        w += e;
        Q += vec4(a.zw,d.w,1)*e;
    }
    if (w.x>0.) Q.xy /= w.x;
    if (w.y>0.) Q.z /= w.y;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main
   vec4 a = A(U);
   Q = D(U);
   if (I<1) Q = .5+.5*sin(1.8*3.1*U.x/R.x+vec4(1,2,3,4)),Q.w=0.;
}