

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "4-Substance" by wyatt. https://shadertoy.com/view/3lffzM
// 2020-08-03 18:26:39

// Fork of "Multi-Substance" by wyatt. https://shadertoy.com/view/WtffRM
// 2020-08-01 02:57:11

Main {
    Q = 1.2*D(U);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)

            #define r 1.3
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
            Q = vec4(0,0,.1,0);
            if (length(U-vec2(0.5)*R)<.3*R.y)Q.w = .8;
        }
        if (iMouse.z>0.&&length(U-iMouse.xy)<20.) Q.w = .3;
        if (U.x<2.||U.y<2.||R.x-U.x<2.||R.y-U.y<2.) Q *= 0.;
    } else {
    	Q = A(U);vec4 q = Q;
    for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    if (x!=0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u), b = B(U+u), c = C(U+u), d = D(U+u);
        u = (u)/dot(u,u);
        Q.xy -= q.w*0.125*(
            a.w*(a.w*a.z-.8-.1*b.w-.1*c.w)+
            -d.x
           )*u;
    	Q.z -= q.w*0.125*a.w*(dot(u,a.xy-q.xy));
    }
    if (Q.w < 1e-3) Q.z *= 0.;
    Q.xy *= 0.999;
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
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
            Q = vec4(0,0,.1,0);
            if (length(U-vec2(0.5)*R)<.3*R.y)Q.w = .8;
        }
        if (iMouse.z>0.&&length(U-iMouse.xy)<20.) Q.w = .3;
        if (U.x<2.||U.y<2.||R.x-U.x<2.||R.y-U.y<2.) Q *= 0.;
    } else {
    	Q = A(U);vec4 q = Q;
    for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    if (x!=0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u), b = B(U+u), c = C(U+u), d = D(U+u);
        u = (u)/dot(u,u);
        Q.xy -= q.w*0.125*(
            a.w*(a.w*a.z-.8-.1*b.w-.1*c.w)+
            -d.y
           )*u;
    	Q.z -= q.w*0.125*a.w*(dot(u,a.xy-q.xy));
    }
    if (Q.w < 1e-3) Q.z *= 0.;
    Q.xy *= 0.999;
    }
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
            Q = vec4(0,0,.1,0);
            if (length(U-vec2(0.5)*R)<.3*R.y)Q.w = .8;
        }
        if (iMouse.z>0.&&length(U-iMouse.xy)<20.) Q.w = .3;
        if (U.x<2.||U.y<2.||R.x-U.x<2.||R.y-U.y<2.) Q *= 0.;
    } else {
    	Q = A(U);vec4 q = Q;
    for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    if (x!=0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u), b = B(U+u), c = C(U+u), d = D(U+u);
        u = (u)/dot(u,u);
        Q.xy -= q.w*0.125*(
            a.w*(a.w*a.z-.8-.1*b.w-.1*c.w)+
            -d.z
           )*u;
    	Q.z -= q.w*0.125*a.w*(dot(u,a.xy-q.xy));
    }
    if (Q.w < 1e-3) Q.z *= 0.;
    Q.xy *= 0.999;
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Fork of "Multi-Substance" by wyatt. https://shadertoy.com/view/WtffRM
// 2020-08-01 02:57:11

Main {
    vec4
        n = A(U+vec2(0,1))+B(U+vec2(0,1))+C(U+vec2(0,1)),
        e = A(U+vec2(1,0))+B(U+vec2(1,0))+C(U+vec2(1,0)),
        s = A(U-vec2(0,1))+B(U-vec2(0,1))+C(U-vec2(0,1)),
        w = A(U-vec2(1,0))+B(U-vec2(1,0))+C(U-vec2(1,0));
    vec3 norm = 
        normalize(vec3(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w,10)),
        ref = reflect(vec3(0,0,-1),norm);
   
	vec4 a = A(U), b = B(U), c = C(U);
    Q = vec4(a.w,b.w,c.w,1);
    Q.w = length(texture(iChannel3,ref).xyz);
}