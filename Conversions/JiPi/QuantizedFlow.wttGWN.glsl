

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main
{
    Me
    Q = (0.5+0.5*Q.xxxx)*abs(atan(10.*vec4(Q.z,0.5*(Q.z+Q.w),Q.w,1)));
	Q = sin(Q*vec4(1,2,3,4));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main
{
    Me
    
    Them
        
    Q.x += (F).y;
    
        
   	Mouse 
        
   	First
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)

#define Main void mainImage( out vec4 Q, in vec2 U )
#define Me Q = A(U);
#define Them vec4 M = 0.25*(A(U+vec2(0,1))+A(U+vec2(1,0))+A(U-vec2(0,1))+A(U-vec2(1,0)));

#define F .75*(M-Q-.03*Q*(dot(Q.xy,Q.xy)-.5*Q.z))

#define Mouse if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.xy = vec2(sin(U.x+U.y),cos(U.x+U.y));


#define First if (iFrame < 1) {Q = vec4(1,0,0,0); if (length(U-vec2(.2,0.5)*R)<30.) Q.xy = vec2(sin(U.x),cos(U.x));}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main
{
    Me
    
    Them
        
    Q.y -= (F).x;
    
        
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
float angle (vec2 a, vec2 b) {
	return atan(a.x*b.y-a.y*b.x,dot(a.xy,b.xy));
}
Main
{
    Me
    vec4 n = A(U+vec2(0,1)), e = A(U+vec2(1,0)), s = A(U-vec2(0,1)),w = A(U-vec2(1,0));
    
    Q.x = angle(e.xy,w.xy);
    Q.y = angle(n.xy,s.xy);
    
    Q.zw = B(U-.15*Q.xy).zw;
    
    if (iFrame < 1 && length(U-vec2(.2,0.5)*R)<30.) Q.zw = vec2(sin(.1*U.x),cos(.1*U.x));   
 	if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.zw = vec2(sin(iTime),cos(iTime));   
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main
{
    Me
    Them
    vec4 n = B(U+vec2(0,1)), e = B(U+vec2(1,0)),s = B(U-vec2(0,1)), w = B(U-vec2(1,0));
   	Q.z = M.z+0.25*(n.y+e.x-s.y-w.x);
	
}