

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 a = A(U);
    Q = abs(sin(.3+a.w+a));
    float d = map(U,R);
    Q = mix(Q,vec4(0),smoothstep(6.,5.,d));

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// divergence step

void mainImage( out vec4 Q, in vec2 U )
{
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    Q = A(U);
    Q.z -= 0.25*(n.y-s.y+e.x-w.x);
    Q.w -= 0.125*(n.y*n.w-s.y*s.w+e.x*e.w-w.x*w.w);
    
    
    
    
    //Boundary Conditions
    float M = 0.5;
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.)Q = vec4(M,0,Q.z,0);
    if (iFrame < 1) Q = vec4(M,0,0,0);
    if (iMouse.z>0.)Q = mix(Q,vec4(.4*cos(.1*iTime),.4*sin(.1*iTime),Q.z,-1),smoothstep(6.,4.,length(U-iMouse.xy)));
	float d = map(U,R);
    Q = mix(Q,vec4(0,0,Q.z,1),smoothstep(6.,5.,d));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texelFetch(iChannel0,ivec2(U),0)
#define B(U) texelFetch(iChannel1,ivec2(U),0)
float map (vec2 u,vec2 r) {
    u = 4.*(u-vec2(0.3,0.5)*r)/r.y;
    float a = 2.5;
    u *= mat2(cos(a),-sin(a),sin(a),cos(a));
    vec2 c = vec2(1.,.4);
    for (int i = 0; i < 10; i++) 
    	u = vec2(u.x*u.x-u.y*u.y,2.*u.x*u.y)-c;
	return length(u);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// gradient step
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    Q = A(U);
    Q.xy -= 0.25*vec2(e.z-w.z,n.z-s.z);
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// blur pass
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    Q = m;
    
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// advection
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    Q = A(U);
    Q = 0.25*(
    	mix(Q,n,max(0.,-m.y))+
    	mix(Q,e,max(0.,-m.x))+
    	mix(Q,s,max(0.,+m.y))+
    	mix(Q,w,max(0.,+m.x))
    );
}
//interpolation way :
/*U -= 0.25*B(U).xy;
    U -= 0.25*B(U).xy;
    U -= 0.25*B(U).xy;
    U -= 0.25*B(U).xy;
    Q = A(U);*/
// pure automata way :
/*vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    Q = A(U);
    Q = 0.25*(
    	mix(Q,n,max(0.,-m.y))+
    	mix(Q,e,max(0.,-m.x))+
    	mix(Q,s,max(0.,+m.y))+
    	mix(Q,w,max(0.,+m.x))
    );*/
