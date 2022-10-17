

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main
{
    vec4 a = A(U)+C(U), b = B(U)+D(U), c = a*a+b*b;
    Q = vec4(length(c.xy),length(c.zw),length(c),1);
	Q = atan(Q);
    Q = (sin(Q.x-Q.y+Q.z*vec4(1,2,3,4)));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define Main void mainImage( out vec4 Q, in vec2 U )

vec4 F( vec4 Q) {
    float l = length(Q.xy),
          g = length(Q.zw);
    vec4 c = abs(length(Q)-vec4(.55,.55,.5,.5));
	return vec4(vec2(g),vec2(l))+c+3e-3*gl_FragCoord.y;
}

#define dt .8
#define K .5
#define Loss .01

#define Init if (iFrame < 1 || length(U-(0.5+.1*sin(vec2(1,2)*floor(float(iFrame)/4.)*.1))*R)<10.) Q = exp(-.005*length(U-0.5*R))
#define Mouse if ((iMouse.z>0.&&length(U-iMouse.xy)<30. )|| length(U-(0.8+.05*cos(vec2(1,2)*floor(float(iFrame)/4.)*.1))*R)<10.) Q.xyzw += .3
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
    
    Mouse*vec4(0,0,-1,0);
    
    Init*vec4(1,0,0,0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main
{
    vec4 
         M = 0.25*(
         	A(U+vec2(0,1))+A(U+vec2(1,0))+
         	A(U-vec2(0,1))+A(U-vec2(1,0))
         ),
         a = A(U);
    Q = B(U) + dt*(M-a-K*a*F(a));
    
    Mouse*vec4(0,0,0,.3);
    
    Init*vec4(0,-.3,0,0);
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
    
    Mouse*vec4(0,0,-1,0);
    
    Init*vec4(1,0,0,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main
{
    vec4 
         M = 0.25*(
         	A(U+vec2(0,1))+A(U+vec2(1,0))+
         	A(U-vec2(0,1))+A(U-vec2(1,0))
         ),
         a = A(U);
    Q = B(U) + dt*(M-a-K*a*F(a));
    
    Mouse*vec4(0,0,0,.3);
    
    Init*vec4(0,-.3,0,0);
}