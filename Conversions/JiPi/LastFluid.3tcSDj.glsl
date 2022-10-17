

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    vec4 b = B(U);
    Q = abs(sin(b+b.w));
        
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main {
	Q = B(U-B(U).xy);
    Neighborhood;
    Q.xy -= grd;   
    																			if ((iMouse.z>0.&&length(U-iMouse.xy)<10.)||(iFrame<1&&length(U-0.5*R)<6.))Q.xyw = vec3(0.4*sin(0.2*iTime),0.4*cos(0.2*iTime),1);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
	Q = B(U-A(U).xy);
    Neighborhood;
    Q.w  -= div;
    																		Neighborhood2; Q.xyz -= 0.25*(N*n.y-S*s.y+E*e.x-W*w.x).xyz;
    																		if ((iMouse.z>0.&&length(U-iMouse.xy)<10.)||(iFrame<1&&length(U-0.5*R)<6.))Q.xyz = abs(vec3(sin(0.2121*iTime),cos(0.43*iTime),sin(.333*iTime)));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Neighborhood vec4 n = A(U+vec2(0,1)), e = A(U+vec2(1,0)), s = A(U-vec2(0,1)), w = A(U-vec2(1,0)), m = 0.25*(n+e+s+w);
#define Neighborhood2 vec4 N = B(U+vec2(0,1)), E = B(U+vec2(1,0)), S = B(U-vec2(0,1)), W = B(U-vec2(1,0)), M = 0.25*(n+e+s+w);
#define grd 0.25*vec2(e.w-w.w,n.w-s.w);
#define div 0.25*(e.x-w.x+n.y-s.y);
#define Main void mainImage(out vec4 Q, vec2 U)
