

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main{
    Q = .05*B(U);

}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)


vec2 hash23(vec3 p3)
{  // Dave H
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main{
    Q = vec4(0);
    U = 4.*(U-.5*R)/R.y;
    for (float i = 0.; i < 8.; i++) {
        U = vec2(U.x*U.x-U.y*U.y,2.*U.x*U.y)-vec2(-.6,sin(.05*iTime));
        U /= .5+0.3*dot(U,U);
        Q += .1*length(U)*vec4(.5+sin(2.*U.x+vec3(1,2,3)),1);
    }

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
    Q = .99*B(U);
    for (float i = 0.; i < 30.; i++) {
        vec2 h = 20.*(hash23(vec3(U+R*i,iFrame))*2.-1.);
        vec4 c = A(U+h),
             n = A(U+h+vec2(0,1)),
             e = A(U+h+vec2(1,0)),
             s = A(U+h-vec2(0,1)),
             w = A(U+h-vec2(1,0));

        vec2 g = 2.*R.x*vec2(e.w-w.w,n.w-s.w);

        Q += exp(-length(h-g))*c;
   }
}