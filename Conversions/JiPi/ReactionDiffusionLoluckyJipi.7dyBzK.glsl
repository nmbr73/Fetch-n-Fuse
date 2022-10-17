

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec3 viridis(float t) {

    const vec3 c0 = vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
    const vec3 c1 = vec3(0.1050930431085774, 1.404613529898575, 1.384590162594685);
    const vec3 c2 = vec3(-0.3308618287255563, 0.214847559468213, 0.09509516302823659);
    const vec3 c3 = vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
    const vec3 c4 = vec3(6.228269936347081, 14.17993336680509, 56.69055260068105);
    const vec3 c5 = vec3(4.776384997670288, -13.74514537774601, -65.35303263337234);
    const vec3 c6 = vec3(-5.435455855934631, 4.645852612178535, 26.3124352495832);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));

}

void mainImage( out vec4 fragColor, in vec2 uv )
{
    vec2 ab = texture(iChannel0, uv/iResolution.xy*res/iResolution.xy).xy;
    float v = clamp(1.+ab.y-ab.x,0.,1.);
    fragColor = vec4(viridis(v),0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 uv ) {
    if(iFrame==0) {
        uv = (2. * uv - res) / res.y;
        fragColor = vec4(1,length(uv)<.1,0,0);
        return;
    }
    
    if(any(greaterThan(uv, res))) return;
    fragColor = render(iResolution.xy, iChannel0, uv);
    if(distance(uv,iMouse.xy/iResolution.xy*res)<.01*iResolution.y && iMouse.z>0.)
        fragColor.y = 0.;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//settings
vec2 dif = vec2(1,.5);
float f = 0.055; // 0.040 // 0.030
float k = 0.062;
vec2 res = 200.*vec2(16./9.,1);

#define T(uv) texture(ch, (uv)/res).xy

vec4 render( vec2 res, sampler2D ch, vec2 uv )
{
    vec2 v = T(uv);

    vec3 e = vec3(0, -1, 1);
    return vec4(
        v + dif * (- v
            + 0.2 * T(uv+e.xy)
            + 0.2 * T(uv+e.xz)
            + 0.2 * T(uv+e.yx)
            + 0.2 * T(uv+e.zx)
            + 0.05 * T(uv+e.zy)
            + 0.05 * T(uv+e.yz)
            + 0.05 * T(uv+e.yy)
            + 0.05 * T(uv+e.zz)
        ) + v.x * v.y * v.y * vec2(-1,1)
          + vec2(f, f + k) * (e.zx - v),
    0,0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 uv ) {
    if(any(greaterThan(uv, res))) return;
    fragColor = render(iResolution.xy, iChannel0, uv);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 uv ) {
    if(any(greaterThan(uv, res))) return;
    fragColor = render(iResolution.xy, iChannel0, uv);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 uv ) {
    if(any(greaterThan(uv, res))) return;
    fragColor = render(iResolution.xy, iChannel0, uv);
}