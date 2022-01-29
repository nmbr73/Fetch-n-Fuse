

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    U -= .5*R;
    U *= .6/(.8+.35*U.y/R.y);
    U += .5*R;
    U.y += .15*R.y;
    vec4 a = A(U),
         n = A(U+vec2(0,1)),
         e = A(U+vec2(1,0)),
         s = A(U-vec2(0,1)),
         w = A(U-vec2(1,0));
    vec2 g = vec2(e.w-w.w,n.w-s.w);
    float f = 0.;
    vec2 u = U;
    for (float i = -10.; i < 10.; i++) {
        float w = pow(A(u).w,1.+.01*i);
        f += w/10.;
        u = vec2(.5,0)*R+(u-vec2(.5,0)*R)*(1.-.005);
    }
    Q = .4+.4*sin(1.-3.*U.y/R.y+5.*H/R.y+vec4(1,2,3,4));
    Q += 2.*H/R.y;
    vec2 d = normalize(vec2(.5,0)*R-U);
    vec2 p = U;
    float W = 0.;
    for (float i = 0.; i < 200.; i++) {
        p += 6.*d;
        W += .01*A(p).w;
    }
    Q -= .4*W;
    vec4 C = .8+.3*sin(W+1./(1.+a.w)+2.-3.*U.y/R.y+3.*H/R.y+vec4(1,2,3,4));
    Q = mix(Q,C,f);
    //Q = vec4(0);
    { // stars
    for (float i = 1.;i < 5.; i++)
    for (int k = 0; k < 9; k++) {
        vec2 u = round(U)+vec2(k%3,k/3)-1.;
        vec3 h = hash(u)*2.-1.;
        vec2 r = u-U+h.xy;
        float l = 5./i*length(r);
        Q += 8./i/sqrt(i)*exp(-1e1*f)*10.*(1.+.5*sin(iTime+6.2*h.z))*exp(-5e1*l)*max(2.*U.y/R.y-1.,0.);
    }}

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec4 T (vec2 U) {
    return A(U-A(U).xy);
}
Main {
    Q = T(U);
    vec4 n = T(U+vec2(0,1)),
         e = T(U+vec2(1,0)),
         s = T(U-vec2(0,1)),
         w = T(U-vec2(1,0));
    Q.xy= Q.xy-.25*vec2(e.z-w.z,n.z-s.z);
    Q.xy= Q.xy-.05*vec2(s.w-n.w,e.w-w.w);
    Q.z = .25*(n.z+e.z+s.z+w.z-n.y-e.x+s.y+w.x);
    Q.y += 1e-4*Q.w*(1.-2.*U.y/R.y);
    float h = H;
    Q.w = mix(Q.w,1.,smoothstep(2.*h,h,U.y));
    if(U.x<1.||R.x-U.x<1.) Q.xyw *= 0.;
    if(U.y<1.||R.y-U.y<1.) Q.xyw *= 0.;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)
#define H 50.*exp(-10.*(U.x-.5*R.x)*(U.x-.5*R.x)/R.x/R.x)
#define ei(a) mat2(cos(a),-sin(a),sin(a),cos(a))
vec3 hash (vec2 p)
{
vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
p3 += dot(p3, p3.yxz+33.33);
return fract((p3.xxy+p3.yzz)*p3.zyx);
}
vec3 noise(vec2 p){
    vec4 w = vec4(floor(p),ceil (p));
    vec3 _00 = hash(w.xy),
         _01 = hash(w.xw),
         _10 = hash(w.zy),
         _11 = hash(w.zw),
         _0 = mix(_00,_01,fract(p.y)),
         _1 = mix(_10,_11,fract(p.y));
     return mix(_0,_1,fract(p.x));
}
vec3 fbm (vec2 p) {
    vec3 w = vec3(0);
    float N = 5.;
    for (float i = 1.; i < N; i++) {
        p *= 1.7*ei(.5);
        w += noise(p)/N/i;
    }
    return w;
}


// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec4 T (vec2 U) {
    return A(U-A(U).xy);
}
Main {
    Q = T(U);
    vec4 n = T(U+vec2(0,1)),
         e = T(U+vec2(1,0)),
         s = T(U-vec2(0,1)),
         w = T(U-vec2(1,0));
    Q.xy= Q.xy-.25*vec2(e.z-w.z,n.z-s.z);
    Q.xy= Q.xy-.05*vec2(s.w-n.w,e.w-w.w);
    Q.z = .25*(n.z+e.z+s.z+w.z-n.y-e.x+s.y+w.x);
    Q.y += 1e-4*Q.w*(1.-2.*U.y/R.y);
    float h = H;
    Q.w = mix(Q.w,1.,smoothstep(2.*h,h,U.y));
    if(U.x<1.||R.x-U.x<1.) Q.xyw *= 0.;
    if(U.y<1.||R.y-U.y<1.) Q.xyw *= 0.;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec4 T (vec2 U) {
    return A(U-A(U).xy);
}
Main {
    Q = T(U);
    vec4 n = T(U+vec2(0,1)),
         e = T(U+vec2(1,0)),
         s = T(U-vec2(0,1)),
         w = T(U-vec2(1,0));
    Q.xy= Q.xy-.25*vec2(e.z-w.z,n.z-s.z);
    Q.xy= Q.xy-.05*vec2(s.w-n.w,e.w-w.w);
    Q.z = .25*(n.z+e.z+s.z+w.z-n.y-e.x+s.y+w.x);
    Q.y += 1e-4*Q.w*(1.-2.*U.y/R.y);
    float h = H;
    Q.w = mix(Q.w,1.,smoothstep(2.*h,h,U.y));
    if(U.x<1.||R.x-U.x<1.) Q.xyw *= 0.;
    if(U.y<1.||R.y-U.y<1.) Q.xyw *= 0.;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
vec4 T (vec2 U) {
    return A(U-A(U).xy);
}
Main {
    Q = T(U);
    vec4 n = T(U+vec2(0,1)),
         e = T(U+vec2(1,0)),
         s = T(U-vec2(0,1)),
         w = T(U-vec2(1,0));
    Q.xy= Q.xy-.25*vec2(e.z-w.z,n.z-s.z);
    Q.xy= Q.xy-.05*vec2(s.w-n.w,e.w-w.w);
    Q.z = .25*(n.z+e.z+s.z+w.z-n.y-e.x+s.y+w.x);
    Q.y += 1e-4*Q.w*(1.-2.*U.y/R.y);
    float h = H;
    Q.w = mix(Q.w,1.,smoothstep(2.*h,h,U.y));
    if(U.x<1.||R.x-U.x<1.) Q.xyw *= 0.;
    if(U.y<1.||R.y-U.y<1.) Q.xyw *= 0.;
}