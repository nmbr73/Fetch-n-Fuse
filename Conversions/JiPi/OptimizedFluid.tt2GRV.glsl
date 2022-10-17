

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
	Improvements:
		account for curl when advecting
		separate advection step removes extra sampling steps
		account for divergence of color
		acount for the magnus force

*/

float ln (vec3 p, vec3 a, vec3 b) {return length(p-a-(b-a)*min(dot(p-a,b-a),0.)/dot(b-a,b-a));}
void mainImage( out vec4 Q, in vec2 U )
{
    float 
        n = length(C(U+vec2(0,1))),
        e = length(C(U+vec2(1,0))),
        s = length(C(U-vec2(0,1))),
        w = length(C(U-vec2(1,0)));
    vec4 a = A(U);
    Q = C(U);
    Q += .1*(0.25*(n+e+s+w)-Q);
    vec3 no = normalize(vec3(e-w,n-s,.3+.2*Q.w));
    float light = ln(vec3(2,2,2),vec3(U/R.y,0),vec3(U/R.y,0)+reflect(normalize(vec3((U-0.5*R)/R.y,1)),no));
    Q *= .8*exp(-.5*vec4(1,2,3,4)*light)+.8*exp(-.06*vec4(1,2,3,4)*light);
	Q = atan(3.*Q)*.666;
    
    U = 0.5*R-abs(U-0.5*R);
    Q += (0.5-0.5*cos(6.*a.z*(1.+.5*vec4(1,2,3,4))))*smoothstep(1.,-1.,min(U.x,U.y)-6.);
    

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
    Q.w = mix(Q.w,0.25*(s.x-n.x+w.y-e.y),0.5);
    
    //Boundary conditions
    vec2 JET = vec2(.5+.25*sin(float(iFrame)/500.),0.1)*R;
    if (iMouse.z>0.) JET = iMouse.xy;
    Q.xy = mix(Q.xy,vec2(0,.7),0.3*smoothstep(1.,-1.,length(U-JET)-6.));
    if (iFrame<1) Q = vec4(0);
    if (U.x<4.||U.y<4.||R.x-U.x<4.||R.y-U.y<4.)Q.xyw*=0.;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0, (U)/R)
#define C(U) texture(iChannel2, (U)/R)
#define D(U) texture(iChannel3, (U)/R)

#define N 3.
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
// draw color and acount for divergence
void mainImage( out vec4 Q, in vec2 U )
{
    Q = C(U);
    // neighborhood
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
    	nc = C(U+vec2(0,1)),
        ec = C(U+vec2(1,0)),
        sc = C(U-vec2(0,1)),
        wc = C(U-vec2(1,0));
    Q = max(Q,0.);
    // divergence 
    Q += 0.25*(s.y*sc-n.y*nc+w.x*wc-e.x*ec);
    // boundary conditions
    vec2 JET = vec2(.5+.25*sin(float(iFrame)/500.),0.1)*R;
    if (iMouse.z>0.) JET = iMouse.xy;
    Q = mix(Q,0.5+0.5*cos(float(iFrame)/900.*(1.5+.5*vec4(1,2,3,4))),smoothstep(1.,-1.,length(U-JET)-5.));
    if (iFrame<1)Q = vec4(0);
    if (U.x<4.||U.y<4.||R.x-U.x<4.||R.y-U.y<4.)Q*=0.;
}