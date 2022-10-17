

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
    Neighborhood
    vec3 no = normalize(vec3(grad,1)),
         re = reflect(no,vec3(.03*(U-0.5*R)/R.y,1));
    Q = a.zzzz*0.5+0.5+.04*a.xyyw;
    Q *= 0.5+0.5*texture(iChannel1,re);
    Q *= 1.+0.5*no.xyzz*no.z;
    Q *= 3.*Q;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)

#define Main void mainImage( out vec4 Q, in vec2 U )
#define Neighborhood vec4 n = A(U+vec2(0,1)), e = A(U+vec2(1,0)), s = A(U-vec2(0,1)), w = A(U-vec2(1,0)), m = 0.25*(n+e+s+w); 
#define rot(a) mat2(cos(a),-sin(a),sin(a),cos(a))
#define grad 0.25*vec2(e.x-w.x,n.x-s.x)
#define div 0.25*(n.y-s.y+e.x-w.x)

#define N 21.
#define For for (float i = -(N-5.); i<=(N); i++)
#define S 4.
#define Gaussian(i) 0.3989422804/S*exp(-.5*(i)*(i)/S/S)
#define Init if (iFrame < 1) 
#define Border if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.)
#define Mouse if (iMouse.z>0.&&length(U-iMouse.xy)<30.) 
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main
{
    
    U -= 0.5*R;
    U *= (1.-0.03+.015*exp(-.5*length(U)/R.y))*rot(.005*exp(-length(U)/R.y));
    U += 0.5*R;
    U += .2*A(U).yx*vec2(1,-1);
    U += .2*A(U).xy;
    Neighborhood   
    Q = A(U);
    vec4 c = C(U), b = B(U);
    
    Q.z  = c.z*.4-S*div;
    Q.xy = Q.xy*.99+c.xy*.5;
    if (length(b.xy)>0.) Q.xy = mix(Q.xy,.5*(2.-Q.z)*normalize(b.xy),.1);
    
    
    Mouse Q*=0.;
    Init Q = sin(.01*U.xyxx);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main
{
    Q = vec4(0);
    For Q += Gaussian(i) * A(U+vec2(i,i)).z;
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
    Q.xy = grad;
}