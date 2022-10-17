

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Fluid Reaction" by wyatt. https://shadertoy.com/view/3tfBWr
// 2020-08-03 18:33:18

// Fork of "4-Substance" by wyatt. https://shadertoy.com/view/3lffzM
// 2020-08-03 02:14:45

// Fork of "Multi-Substance" by wyatt. https://shadertoy.com/view/WtffRM
// 2020-08-01 02:57:11

Main {
    Q = 1.-2.5*sin(A(U).wwww*(1.3+.2*vec4(1,2,3,4)));

}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)

            #define r 1.15
#define N 15.
#define S vec4(4,7,1,1)
#define Gaussian(i) 0.3989422804/S*exp(-.5*(i)*(i)/S/S)

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main {
    if (iFrame%2<1) {
        Q = vec4(0);
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            vec2 u = vec2(x,y);
            vec4 a = A(U+u);
            vec2 w1 = clamp(U+u+a.xy-0.5*r,U - 0.5,U + 0.5),
                 w2 = clamp(U+u+a.xy+0.5*r,U - 0.5,U + 0.5);
            float m = (w2.x-w1.x)*(w2.y-w1.y)/(r*r);
            Q.xyz += m*a.w*a.xyz;
            Q.w += m*a.w;
        }
        if (Q.w>0.)
            Q.xyz/=Q.w;
        if (iFrame < 1) 
        {
            Q = vec4(0,0,1,0);
            if (length(U-vec2(0.5)*R)<.3*R.y)Q.w = .3;
        }
        if (iMouse.z>0.&&length(U-iMouse.xy)<20.) Q.xw = vec2(.25,.3);
        if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;
    } else {
    	Q = A(U);vec4 q = Q, dd = D(U);
    for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    if (x!=0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u), b = B(U+u), d = D(U+u);
        u = (u)/dot(u,u);
        Q.xy -= q.w*0.125*(-d.w*a.w+a.w*(a.w*a.z-1.-3.*a.w))*u;
    	Q.z  -= q.w*0.125*a.w*dot(u,a.xy-q.xy);
    }
    Q.xy = mix(Q.xy,D(U).xy,Q.w);
    if (Q.w < 1e-3) Q.z *= 0.;
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
    vec4 a = A(U);
   	Q = mix(D(U),a,a.w);
    
    vec4 m = 0.25*(D(U+vec2(0,1))+D(U+vec2(1,0))+D(U-vec2(0,1))+D(U-vec2(1,0)));
    Q = mix(Q,m,vec4(0,0,1,.1));
    
    if (length(Q.xy)>0.) 
        Q.xy = .2*normalize(Q.xy)*Q.w;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
    if (iFrame%2<1) {
        Q = vec4(0);
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            vec2 u = vec2(x,y);
            vec4 a = A(U+u);
            vec2 w1 = clamp(U+u+a.xy-0.5*r,U - 0.5,U + 0.5),
                 w2 = clamp(U+u+a.xy+0.5*r,U - 0.5,U + 0.5);
            float m = (w2.x-w1.x)*(w2.y-w1.y)/(r*r);
            Q.xyz += m*a.w*a.xyz;
            Q.w += m*a.w;
        }
        if (Q.w>0.)
            Q.xyz/=Q.w;
        if (iFrame < 1) 
        {
            Q = vec4(0,0,1,0);
            if (length(U-vec2(0.5)*R)<.3*R.y)Q.w = .3;
        }
        if (iMouse.z>0.&&length(U-iMouse.xy)<20.) Q.xw = vec2(.25,.3);
        if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;
    } else {
    	Q = A(U);vec4 q = Q, dd = D(U);
    for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    if (x!=0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u), b = B(U+u), d = D(U+u);
        u = (u)/dot(u,u);
        Q.xy -= q.w*0.125*(-d.w*a.w+a.w*(a.w*a.z-1.-3.*a.w))*u;
    	Q.z  -= q.w*0.125*a.w*dot(u,a.xy-q.xy);
    }
    Q.xy = mix(Q.xy,D(U).xy,Q.w);
    if (Q.w < 1e-3) Q.z *= 0.;
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main {
    vec4 a = A(U);
   	Q = mix(D(U),a,a.w);
    
    vec4 m = 0.25*(D(U+vec2(0,1))+D(U+vec2(1,0))+D(U-vec2(0,1))+D(U-vec2(1,0)));
    Q = mix(Q,m,vec4(0,0,1,.1));
    
    if (length(Q.xy)>0.) 
        Q.xy = .2*normalize(Q.xy)*Q.w;
}