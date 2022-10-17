

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = 1.-max(sin(3.*D(U)*(1.+.2*vec4(1,2,3,4))),0.);
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
        w = A(U-vec2(1,0));
    Q.xy -= 
        // pressure force
        0.25*vec2(e.z-w.z,n.z-s.z)+
        // magnus force
        0.25*Q.w*vec2(n.w-s.w,e.w-w.w);
    // divergence
    Q.z  = 0.25*(s.y-n.y+w.x-e.x+n.z+e.z+s.z+w.z);
    // curl
    Q.w = 0.25*(s.x-n.x+w.y-e.y);
    
    //Boundary conditions
    vec2 JET = vec2(.5+.25*sin(float(iFrame)/500.),0.1)*R;
    if (iMouse.z>0.) JET = iMouse.xy;
    Q.xy = mix(Q.xy,vec2(0,.7),0.3*smoothstep(1.,-1.,length(U-JET)-6.));
    Q.xy = mix(Q.xy,vec2(0,-.7),0.3*smoothstep(1.,-1.,length(U-(R-JET))-6.));
    if (iFrame<1) Q = vec4(0);
    if (U.x<4.||U.y<4.||R.x-U.x<4.||R.y-U.y<4.)Q.xyw*=0.;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0, (U)/R)
#define C(U) texture(iChannel2, (U)/R)
#define D(U) texture(iChannel3, (U)/R)

#define N 5.
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
void swap (vec2 U, vec2 r, inout vec4 Q) {
	vec4 n = C(U+r);
    if (length(U-n.xy) < length(U-Q.xy)) Q = n;
}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = C(U);
    swap(U, vec2(1,0),Q);
    swap(U, vec2(0,1),Q);
    swap(U,-vec2(1,0),Q);
    swap(U,-vec2(0,1),Q);
    swap(U, vec2(1,1),Q);
    swap(U,vec2(-1,1),Q);
    swap(U,vec2(1,-1),Q);
    swap(U,-vec2(1,1),Q);
    
    Q.xy += A(Q.xy).xy;
    
    if (iFrame < 1) {
    	Q = vec4(floor(0.5+U),0,0);
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = mix(D(U),
            vec4(1)*smoothstep(.1+10.*length(A(U).xy),0.,length(U-C(U).xy))
                              ,.005);
}