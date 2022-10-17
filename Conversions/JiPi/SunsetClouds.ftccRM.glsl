

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    Q = .9*A(U);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define Main void mainImage(out vec4 Q, in vec2 U)
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
float hash(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}
#define ei(a) mat2(cos(a),sin(a),-sin(a),cos(a))

float noise ( vec3 h ) {
    vec3 f = floor(h);
    vec3 c = ceil(h);
    vec3 r = fract(h);
    float _000 = hash(f),
          _001 = hash(vec3(f.xy,c.z)),
          _010 = hash(vec3(f.x,c.y,f.z)),
          _011 = hash(vec3(f.x,c.yz)),
          _100 = hash(vec3(c.x,f.yz)),
          _101 = hash(vec3(c.x,f.y,c.z)),
          _110 = hash(vec3(c.xy,f.z)),
          _111 = hash(c),
          _00 = mix(_000,_001,r.z),
          _01 = mix(_010,_011,r.z),
          _10 = mix(_100,_101,r.z),
          _11 = mix(_110,_111,r.z),
          _0 = mix(_00,_01,r.y),
          _1 = mix(_10,_11,r.y);
          return mix(_0,_1,r.x);
}
float fbm (vec3 p)
{
    float w = 0.;
    float N = 10.;
    for (float i = 1.; i < N; i++)
    {
        p.xy *= 2.*ei(2.);
        p.yz *= ei(1.);
        w += 4.*noise(p)/N*pow(2.,-i);
    }
    return w;
}
Main {
    U = 3.*(U-.5*R)/R.y;
    U.xy +=1.5;
    vec3 v = 2.*vec3(U,3.-.2*float(iFrame)/60.);
    float w = fbm(v)-.02;
    w *= 1.5*exp(.05*v.z);
    w *= .8*smoothstep(-5.,1.,exp(-v.x)-v.y+.5*v.z-2.);
    w = smoothstep(.12,0.21,w);
    Q = vec4(w*w);
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
    Q = vec4(1);
    for (float x = 0.; x < 40.; x++){
        float a = A(U+4.*vec2(x,.3*x)).x;
        Q -= .01*Q*a;
    }
    Q = vec4(1,2,3,4)*.2+Q*sin(.5+Q-(1.1-sqrt(float(iFrame)/60.)*.2)+vec4(1,2,3,4));
    Q.w = A(U).x;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
    Q = A(U);
    Q = mix(C(U),Q*Q,Q.w);
    
    if (iFrame < 1) {
        Q = 
        .5+.5*sin(5.+U.y/R.y+vec4(1,2,3,4));
    }
}