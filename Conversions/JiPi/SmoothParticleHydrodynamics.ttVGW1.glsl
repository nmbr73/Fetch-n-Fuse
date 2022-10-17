

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float c (vec2 U) {
   vec4 a = A(U);
   return .01*a.x+smoothstep(.1,1.,10.*a.x + a.y) ;
}
Main
{
    vec4 a = A(U), b=C(U);
   float 
       q = c(U),
       n = c(U+vec2(0,1)),
       e = c(U+vec2(1,0)),
       s = c(U-vec2(0,1)),
       w = c(U-vec2(1,0));
    vec3 no = normalize(vec3(e-w,n-s,-.001));
    no = reflect(no,vec3(0,0,-1));
    Q = abs(sin((2.*a.x+.1)*vec4(1,2,3,4)+length(a.zw)));
    Q *= q*(1.+texture(iChannel1,no))*.5;
    Q *= 1.-.1*smoothstep(2.,0.,length(U-b.xy));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)

#define Main void mainImage( out vec4 Q, in vec2 U )
#define Neighborhood vec4 n = A(U+vec2(0,1)), e = A(U+vec2(1,0)), s = A(U-vec2(0,1)), w = A(U-vec2(1,0)), m = 0.25*(n+e+s+w); 
#define rot(a) mat2(cos(a),-sin(a),sin(a),cos(a))
#define div 0.25*(n.y-s.y+e.x-w.x)

#define N 6.
#define For for (float i = -(N); i<=(N); i++)
#define S vec4(3.5,1,4,4)
#define Gaussian(i) 0.3989422804/S*exp(-.5*(i)*(i)/S/S)
#define Init if (iFrame < 1) 
#define Border if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.)
#define Mouse if (iMouse.z>0.&&length(U-iMouse.xy)<30.) 
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void X (inout vec4 Q, vec2 U, vec2 r) {
	vec4 n = A(U+r);
    if (length(U-n.xy)<length(U-Q.xy)) Q = n;
}
Main
{
    Q = A(U);
    X(Q,U,vec2(1,0));
    X(Q,U,vec2(0,1));
    X(Q,U,-vec2(1,0));
    X(Q,U,-vec2(0,1));
    X(Q,U,vec2(3,0));
    X(Q,U,vec2(0,3));
    X(Q,U,-vec2(3,0));
    X(Q,U,-vec2(0,3));
    
    Q.zw = mix(Q.zw,C(Q.xy).zw,.01);
    Q.w -= 5e-4;
    Q.zw += B(Q.xy).xy;
    Q.xy += Q.zw;
    
    if (Q.x<1.) {Q.x = 1.; Q.z *= -1.;}
    if (Q.y<1.) {Q.y = 1.; Q.w *= -1.;}
    if (R.x-Q.x<1.) {Q.x = R.x-1.; Q.z *= -1.;}
    if (R.y-Q.y<1.) {Q.y = R.y-1.; Q.w *= -1.;}
    
    if (iMouse.z>0.) {
    	if (length(U-iMouse.xy)<length(U-Q.xy))
            Q.xy = iMouse.xy;
    }
    if (iFrame < 1 && length (U-0.5*R) < 30.) Q.xy = U;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main
{
    Q = vec4(0);
    For {
        vec4 a = A(U+vec2(i,i));
        float p = exp(-length(U+vec2(i,i)-a.xy));
        Q += Gaussian(i) * vec4(vec2(p),a.zw);
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main
{
    Q = vec4(0);
    For Q += Gaussian(i) * A(U+vec2(-i,i));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main
{
    Neighborhood
    Q = A(U);
    Q.xy = (1.5*(Q.x+.01)*vec2(e.x-w.x,n.x-s.x)-0.7*vec2(e.y-w.y,n.y-s.y));
}