

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    vec4
        n = A(U+vec2(0,1))+B(U+vec2(0,1)),
        e = A(U+vec2(1,0))+B(U+vec2(1,0)),
        s = A(U-vec2(0,1))+B(U-vec2(0,1)),
        w = A(U-vec2(1,0))+B(U-vec2(1,0));
    vec3 norm = 
        normalize(vec3(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w,3)),
        ref = reflect(vec3(0,0,-1),norm);
   
	vec4 a = A(U), b = B(U);
    Q = 1.2*(a.w+b.w)*sin(-2.1+3.*a.w*a.z+(b.w*b.z+.4)*vec4(1,2,3,4));
    Q += 0.6*(Q+.4)*C(U+40.*(a.w+b.w)*norm.xy);
	Q *= 0.8+texture(iChannel3,ref);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main {
    Q = vec4(0);
	for (int x = -1; x <= 1; x++)
	for (int y = -1; y <= 1; y++)
    {
        vec2 u = vec2(x,y);
    	vec4 a = A(U+u);
        #define q 1.125
		vec2 w1 = clamp(U+u+a.xy-0.5*q,U - 0.5,U + 0.5),
             w2 = clamp(U+u+a.xy+0.5*q,U - 0.5,U + 0.5);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        Q.xyz += m*a.w*a.xyz;
        Q.w += m*a.w;
    }
    if (Q.w>0.)
    	Q.xyz/=Q.w;
    if (iFrame < 1) 
    {
        Q = vec4(0,0,.1,0);
        if (length(U/R-0.3)<0.2)Q.w = 1.;
    }
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.xw = vec2(.5,.5);
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
    Q = vec4(0);
	for (int x = -1; x <= 1; x++)
	for (int y = -1; y <= 1; y++)
    {
        vec2 u = vec2(x,y);
    	vec4 a = A(U+u);
        #define q 1.125
		vec2 w1 = clamp(U+u+a.xy-0.5*q,U - 0.5,U + 0.5),
             w2 = clamp(U+u+a.xy+0.5*q,U - 0.5,U + 0.5);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        Q.xyz += m*a.w*a.xyz;
        Q.w += m*a.w;
    }
    if (Q.w>0.)
    	Q.xyz/=Q.w;
    if (iFrame < 1) 
    {
        Q = vec4(0,0,.1,0);
        if (length(U/R-0.7)<0.2)Q.w = 1.;
    }
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.xw = vec2(.5,.5);
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
	Q = A(U);vec4 q = Q;
    for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    if (x != 0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u), b = B(U+u);
        u = (u)/dot(u,u);
        Q.xy -= q.w*0.125*a.w*(a.w*a.z-1.)*u;
        Q.xy -= q.w*0.125*b.w*(b.w*b.z+1.)*u;
    	Q.z -= q.w*0.125*a.w*(dot(u,a.xy-q.xy));
    }
    if (Q.w < 1e-3) Q.z *= 0.;
    Q.y -= 5e-4*Q.w;
    Q.xy *= 0.999;
   
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main {
	Q = A(U);vec4 q = Q;
    for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    if (x != 0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u), b = B(U+u);
        u = (u)/dot(u,u);
        Q.xy -= q.w*0.125*a.w*(a.w*a.z-1.)*u;
        Q.xy -= q.w*0.125*b.w*(b.w*b.z+1.)*u;
    	Q.z -= q.w*0.125*a.w*(dot(u,a.xy-q.xy));
    }
    if (Q.w < 1e-3) Q.z *= 0.;
    Q.y -= 5e-4*Q.w;
    Q.xy *= 0.999;
   
}