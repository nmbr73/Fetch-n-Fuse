

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main
{
    Them;
    vec3 no = normalize(vec3(
    	length(e)-length(w),
    	length(n)-length(s), 5
    )), re = reflect(no,vec3(0,0,1));
	Q = (0.5+0.5*texture(iChannel2,re))*
        (sin(atan(length(A(U)))*vec4(1,2,3,4)));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main
{
	Q = A(U);
    Them;
    
    Q.xz += dt*(m-Q+Q*F(Q)).yw;
    
    Mouse;
    Init;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main
{
	Q = A(U);
    Them;
    
    Q.yw -= dt*(m-Q+Q*F(Q)).xz;
    Q += Loss*(m-Q);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define Main void mainImage( out vec4 Q, in vec2 U )
#define Them vec4 n = A(U+vec2(0,1)), e = A(U+vec2(1,0)), s = A(U-vec2(0,1)), w = A(U-vec2(1,0)), m = 0.25*(n+e+s+w);
#define dt 1.
#define F(Q) .5*( 1./(1.+length(Q)+dot(Q,Q)) + 1e-3*(U.y)*(iMouse.z>0.?0.:1.))
#define Loss .00
#define Init if (length(U-0.5*R) < 10.) Q = vec4(1,1,1,-1);
#define Mouse if (iMouse.z>0.&&length(U-iMouse.xy)<20.) Q += .03*vec4(1,0,0,-1);
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main
{
	Q = A(U);
    Them;
    
    Q.xz += dt*(m-Q+Q*F(Q)).yw;
    
    Mouse;
    Init;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main
{
	Q = A(U);
    Them;
    
    Q.yw -= dt*(m-Q+Q*F(Q)).xz;
    Q += Loss*(m-Q);
}