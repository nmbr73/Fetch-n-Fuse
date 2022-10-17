

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Controls in Common

void mainImage( out vec4 Q, in vec2 U )
{
    vec4 
        n = (C(U+vec2(0,1))),
        e = (C(U+vec2(1,0))),
        s = (C(U-vec2(0,1))),
        w = (C(U-vec2(1,0)));
    vec4 c = C(U);
    vec3 no = normalize(vec3(e.x-w.x+e.y-w.y,n.x-s.x+n.y-s.y,.5));
    Q = abs(sin(1.+3.*c.z+sqrt(c.y)*(1.+.5*vec4(1,2,3,4))));
    float a = .5;
    no.zy *= mat2(cos(a),-sin(a),sin(a),cos(a));
    no.zx *= mat2(cos(a),-sin(a),sin(a),cos(a));
    Q*=max(0.,.2+.8*no.z);
	
    
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Calculate forces and pressure
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        c = C(U),
        nc = C(U+vec2(0,1)),
        ec = C(U+vec2(1,0)),
        sc = C(U-vec2(0,1)),
        wc = C(U-vec2(1,0));
    Q.xy -= 1./(1.+2.*sqrt(c.y))*(
        // slope force
        0.25*(.5*vec2(ec.x-wc.x,nc.x-sc.x)+vec2(ec.y-wc.y,nc.y-sc.y))+
        // pressure force
        0.25*vec2(e.z-w.z,n.z-s.z)+
        // magnus force
        0.25*Q.w*vec2(n.w-s.w,e.w-w.w));
    Q.xy *= min(1.,c.y);
    // divergence
    Q.z  = 0.25*(s.y-n.y+w.x-e.x+n.z+e.z+s.z+w.z);
    // curl
    Q.w = 0.25*(s.x-n.x+w.y-e.y);
    if (length(Q.xy) > .8) Q.xy = .8*normalize(Q.xy);
    
    //Boundary conditions
    if (iFrame<1) Q = vec4(0);
    
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0, (U)/R)
#define B(U) texture(iChannel1, (U)/R)
#define C(U) texture(iChannel2, (U)/R)
#define D(U) texture(iChannel3, (U)/R)

#define N 3.

#define PRECIPITATION 1.
#define EVAPORATION .0001
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Advect along velocity and curl feild
void mainImage( out vec4 Q, in vec2 U )
{
    for (float i = 0.; i< N;i++) {
        Q = A(U);
        float co = cos(Q.w/N), si = sin(Q.w/N);
        U -= Q.xy*mat2(co,-si,si,co)/N;
    }
    Q = A(U);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Advect along velocity and curl feild
void mainImage( out vec4 Q, in vec2 U )
{
    for (float i = 0.; i< N;i++) {
        Q = A(U);
        float co = cos(Q.w/N), si = sin(Q.w/N);
        U -= Q.xy*mat2(co,-si,si,co)/N;
    }
    Q = C(U);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = C(U);
    // neighborhood
    vec4 
        a = A(U),
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
    	nc = C(U+vec2(0,1)),
        ec = C(U+vec2(1,0)),
        sc = C(U-vec2(0,1)),
        wc = C(U-vec2(1,0));
    Q = mix(Q,0.25*(nc+ec+sc+wc),vec4(0.,.1,0,0));
    // divergence 
    Q += 0.25*(s.y*sc-n.y*nc+w.x*wc-e.x*ec);
    
    // x : height y : water, z : sediment 
    float m = 0.25*(D(U+vec2(0,1)).x+D(U+vec2(1,0)).x+D(U-vec2(1,0)).x+D(U-vec2(0,1)).x);
    float me = D(U).x;
    float l = m-me;
   	Q.x = me + 0.01*l*l*l;
    if (iMouse.z>0.) Q.x += .5/(1.+.01*dot(U-iMouse.xy,U-iMouse.xy));
    float x = .05*(Q.y*length(a.xy)*(1.-Q.z)-.1*Q.z);
    Q.z += x;
    Q.x -= x;
    Q.y = Q.y*(1.-EVAPORATION) + PRECIPITATION/R.x;
    Q = max(Q,0.);
    // boundary conditions
    if (iFrame<5)Q = vec4(10.+1.-U.y/R.y+.1*B(U).x+2.*exp(-length(U-0.5*R)/R.y),.3,0,0);
    if (U.x<2.||U.y<2.||R.y-U.y<2.||R.x-U.x<2.) Q*=0.;

}