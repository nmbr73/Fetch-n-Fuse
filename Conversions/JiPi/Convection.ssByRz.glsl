

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Material Point Method" by wyatt. https://shadertoy.com/view/fssyDs
// 2022-01-24 00:02:17

Main
    
    vec4 b = B(U);
    vec4 a = A(b.xy);
    vec4 d = D(b.xy);
    vec4 c = C(U);
    Q = 2.*d*exp(-.5*length(U-a.xy));
    
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
    
    if (length(U-vec2(.5,.8)*R)<.4*R.x) return 1.;
    
    if (length(U-vec2(.5,.2)*R)<.4*R.x) return 1.;
    
    return 0.;

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main
    
    if (building(U)==0.) {Q = vec4(0); return;}

    Q = A(U);
    vec4 d = D(U);
    vec4 c = C(Q.xy);
    vec2 f = vec2(0,-2./R.y*sign(U.y-.5*R.y));
    
    for (float x = -2.; x <= 2.; x ++) 
    for (float y = -2.; y <= 2.; y ++)
    if (x!=0.||y!=0.) {
        vec4 c = C(Q.xy+vec2(x,y));
        vec4 b = B(Q.xy+vec2(x,y));
        vec4 dd = D(b.xy+vec2(x,y));
        f -= .03*c.w*(c.w-.6)*vec2(x,y)/(x*x+y*y);
        f -= 0.01*c.w*abs(dd.x-d.x)*vec2(x,y)/sqrt(x*x+y*y);
    }
    
    if (length(f)>1.) f = normalize(f);
    Q.zw = c.xy;
    Q.zw += f;
    Q.xy += .5*f+Q.zw*inversesqrt(1.+dot(Q.zw,Q.zw));
    
    if (Q.y<1.) Q.y=1., Q.w *= -1.;
    if (Q.x<1.) Q.x=1., Q.z *= -1.;
    if (R.y-Q.y<1.) Q.y=R.y-1., Q.w *= -1.;
    if (R.x-Q.x<1.) Q.x=R.x-1., Q.z *= -1.;

    if (M.z>0.) Q.zw -= 3e-2*(M.xy-Q.xy)/(1.+length((M.xy-Q.xy)));

    if(I<1) {
    
        Q = vec4(U,0,0);
        // if (length(U-vec2(.9)*R)<.02*R.x) Q.zw = vec2(-2.5,-1.5);
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
        vec4 e = 1./(1.+vec4(8,8,0,4)*(x*x+y*y));
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
   if (I<1) Q = 1.+sin(1.75+.5*3.1*U.y/R.y+vec4(1,2,3,4)),Q.w=0.;
}