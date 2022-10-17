

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main
{
    vec4 a = A(U);
    vec4 b = B(U);
    vec4 c = C(U);
    vec4 d = D(U);
    float e = dot(a,a)+dot(b,b);
	Q = sin(a*a+e*vec4(1,2,3,4));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define Main void mainImage( out vec4 Q, in vec2 U )

#define F(x) (abs((x)-.5)+U.y*1e-3)

#define dt .75
#define K .75
#define Loss .01

#define Border if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q *= 0.;
#define Init if (iFrame < 1) Q = exp(-.04*length(U-0.5*R))
#define Mouse if (iMouse.z>0.) Q.xyzw += .5*exp(-.001*dot(U-iMouse.xy,U-iMouse.xy))
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main
{
    vec4 
         N = A(U+vec2(0,1)), 
         E = A(U+vec2(1,0)), 
         S = A(U-vec2(0,1)), 
         W = A(U-vec2(1,0)),
         M = 0.25*(N+E+S+W),
         a = A(U), b = B(U);
    Q = a + dt*b + Loss*(M-a);
    
    Mouse*vec4(1,0,-1,0);
    
    Init*vec4(1,0,-1,0);
    Border
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main
{
    vec4 
         M = 0.125*(
         	A(U+vec2(0,1))+A(U+vec2(1,0))+
         	A(U-vec2(0,1))+A(U-vec2(1,0))+
            A(U+vec2(1,1))+A(U+vec2(1,-1))+
         	A(U-vec2(1,1))+A(U-vec2(1,-1))
         ),
         a = A(U);
    float P = F(length(a));
    Q = B(U) + dt*(M-a-K*a*P);
    
    Mouse*vec4(0,K,0,-K);
    
    Init*vec4(0,K,0,-K);
    Border
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main
{
    vec4 
         N = A(U+vec2(0,1)), 
         E = A(U+vec2(1,0)), 
         S = A(U-vec2(0,1)), 
         W = A(U-vec2(1,0)),
         M = 0.25*(N+E+S+W),
         a = A(U), b = B(U);
    Q = a + dt*b + Loss*(M-a);
    
    Mouse*vec4(1,0,-1,0);
    
    Init*vec4(1,0,-1,0);
    Border
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main
{
    vec4 
         M = 0.125*(
         	A(U+vec2(0,1))+A(U+vec2(1,0))+
         	A(U-vec2(0,1))+A(U-vec2(1,0))+
            A(U+vec2(1,1))+A(U+vec2(1,-1))+
         	A(U-vec2(1,1))+A(U-vec2(1,-1))
         ),
         a = A(U);
    float P = F(length(a));
    Q = B(U) + dt*(M-a-K*a*P);
    
    Mouse*vec4(0,K,0,-K);
    
    Init*vec4(0,K,0,-K);
    Border
}