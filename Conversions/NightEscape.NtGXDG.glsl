

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main 

 U = 2.*(U-.5*R)/R.y;
 float d = 1.+1.9*(U.y);
 vec2 u = U*3./d;
 if (d<0.) {
    u=-u;
    u.x += 5.*(fbm(.1*u)*2.-1.);
    u.y += 5.*(fbm(.01*u)*2.-1.);
    U.x += .2*(fbm(.03*u+.01*T)*2.-1.);
    U.y += .5*(fbm(.01*u)*2.-1.);
 }
 d = abs(d);
 u.x += sin(u.y);
 float cloud = clamp(1.-.01*(pow(5.*fbmt(10.+u+.01*T),3.)),0.,1.);
 float night = .8*clamp(.5-.05*u.x,0.,1.);
 float stars = .5+.5*clamp(2e6*pow(fbm(29.*u),15.),0.,1.);
 vec4 sunset = .2+.5*sin(-.9-.1*u.y+2e-2*u.x+vec4(1,2,3,4));
 sunset = mix(sunset,2.-2.*vec4(stars),night);
 Q = mix(vec4(exp(-.01*u.x*u.x)),sunset,cloud);
 float mountain = 17.*pow(fbm(vec2(10.+.05*U.x,0)),7.);
 Q *= vec4(1.-step(d-mountain,0.));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R; float T; int I;
#define Main void mainImage(out vec4 Q, in vec2 U) {R = iResolution.xy; T = iTime; I = iFrame;
#define ei(a) mat2(cos(a),-sin(a),sin(a),cos(a))
float hash(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
float noise(vec2 p)
{
    vec4 w = vec4(
        floor(p),
        ceil (p)  );
    float 
        _00 = hash(w.xy),
        _01 = hash(w.xw),
        _10 = hash(w.zy),
        _11 = hash(w.zw),
    _0 = mix(_00,_01,fract(p.y)),
    _1 = mix(_10,_11,fract(p.y));
    return mix(_0,_1,fract(p.x));
}
float fbmt (vec2 p) {
    float w = 0.;
    #define N 11.
    for (float i = 0.; i < N; i++)
    {
        p *= 1.7*ei(1e-3*T);
        w += noise(p)/N;
    }
    return w;
}
float fbm (vec2 p) {
    float w = 0.;
    #define N 11.
    for (float i = 0.; i < N; i++)
    {
        p *= 1.7*ei(.5);
        w += noise(p)/N;
    }
    return w;
}
vec4 pw (vec4 p, float a) {
    return vec4(pow(p.x,a),pow(p.y,a),pow(p.z,a),pow(p.w,a));
}