

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Me
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
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


#define Me Q = A(U);
#define Them vec4 M = 0.25*(A(U+vec2(0,1))+A(U+vec2(1,0))+A(U-vec2(0,1))+A(U-vec2(1,0)));

#define F .5*(M-Q-Q*(dot(Q.xy,Q.xy)-length(Q.xy))-.01*Q.z)

#define Mouse if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.xy = vec2(sin(U.x+U.y),cos(U.x+U.y));


#define First if (iFrame < 1) {Q = vec4(1,0,0,0); if (length(U-0.5*R)<30.) Q.xy = vec2(sin(U.x),cos(U.x));}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Me
    
    Them
        
    Q.y -= (F).x;
    
        
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
float angle (vec2 a, vec2 b) {
	return atan(a.x*b.y-a.y*b.x,dot(a.xy,b.xy));
}
void mainImage( out vec4 Q, in vec2 U)
{
    Me
    vec4
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    
    Q.x = angle(e.xy,w.xy);
    Q.y = angle(n.xy,s.xy);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U)
{
    Me
    Them
    vec4
        n = B(U+vec2(0,1)),
        e = B(U+vec2(1,0)),
        s = B(U-vec2(0,1)),
        w = B(U-vec2(1,0));
   Q.z = mix(Q.z,M.z,.5)+0.25*(n.y+e.x-s.y-w.x);
	
}