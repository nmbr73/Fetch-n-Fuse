

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 a = A(U),
        h = A(U+2.),
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    vec3 no = normalize(vec3(length(e.xy)-length(w.xy),length(n.xy)-length(s.xy),1));
    vec4 tx = texture(iChannel1,-reflect(-no,vec3(0,0,1)));
    tx*=0.5+0.5*tx;
    Q = (.8+0.2*dot(no,normalize(vec3(1))))*(0.7+.1*tx)*atan(.3*length(a.xy)*vec4(1,2,3,4)/3.);

    Q = mix((.1+.5*texture(iChannel2,(U-no.xy)/R))-.05*length(h.xy),Q,.1+.9*Q);

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 m = 0.25*(A(U+vec2(0,1))+A(U-vec2(0,1))+A(U+vec2(1,0))+A(U-vec2(1,0)));

    Q.y -= .3*(G).x+0.2*(m.y-Q.y)+0.2*(m.x-Q.x);
    
	if (iFrame < 1) 
        Q.xy=normalize(exp(-.01*dot(U-0.5*R,U-0.5*R))*vec2(sin(10.*iTime+.1*U.x),cos(10.*iTime+.1*U.x))+vec2(1,0));
        
    if (iMouse.z>0.&&length(U-iMouse.xy)<30.) 
        Q.xy=normalize(exp(-.01*dot(U-iMouse.xy,U-iMouse.xy))*vec2(sin(10.*iTime+.1*U.x),cos(10.*iTime+.1*U.x))+vec2(1,0));
         
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 m = 0.25*(A(U+vec2(0,1))+A(U-vec2(0,1))+A(U+vec2(1,0))+A(U-vec2(1,0)));

    Q.x += .3*(G).y+0.2*(m.y-Q.y)+0.2*(m.x-Q.x);
    
    
  
	                                    
	if (iFrame < 1) 
        Q.xy=normalize(exp(-.01*dot(U-0.5*R,U-0.5*R))*vec2(sin(10.*iTime+.1*U.x),cos(10.*iTime+.1*U.x))+vec2(1,0));
        
    if (iMouse.z>0.&&length(U-iMouse.xy)<30.) 
        Q.xy=normalize(exp(-.01*dot(U-iMouse.xy,U-iMouse.xy))*vec2(sin(10.*iTime+.1*U.x),cos(10.*iTime+.1*U.x))+vec2(1,0));
         
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)


#define G m-Q*(2.+exp(-.5*dot(Q.xy,Q.xy)))