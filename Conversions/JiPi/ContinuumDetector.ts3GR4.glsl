

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
void mainImage( out vec4 Q, vec2 U )
{
    
    
    Q = A(U);
    vec4 d = vec4(0);
    float w = 0.;
    vec2 g = vec2(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++) {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec4 d = a - Q;
        float l = length(d);
        d += a - Q;
        w += l;
        g += l * u;
    }
    vec3 n = normalize(vec3(g,.001));
    Q *= 0.7+0.5*dot(n,normalize(vec3(1,1,1)));

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
void mainImage( out vec4 Q, vec2 U )
{
    
    
    Q = A(U);
    vec4 s = vec4(0);
    float n = 1e-5, w = 0.;
    vec2 g = vec2(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++) {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec4 d = a - Q;
        float l = length(d);
        w += l;
        g += l * u;
        float weight = max(exp(-200.*l*l)-.2,-1e-3);
       	n+=weight;
        s += a*weight;
    }
    Q += (1.-w/8.)*(s/n-Q);
    Q = clamp(Q,0.,1.);
    if (iFrame < 20) Q = B(U);
    if (iMouse.z>0.) Q = mix(Q,Q,exp(-50.*length(U-iMouse.xy)/R.y));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
void mainImage( out vec4 Q, vec2 U )
{
    
    
    Q = A(U);
    vec4 s = vec4(0);
    float n = 1e-5, w = 0.;
    vec2 g = vec2(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++) {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec4 d = a - Q;
        float l = length(d);
        w += l;
        g += l * u;
        float weight = max(exp(-200.*l*l)-.2,-1e-3);
       	n+=weight;
        s += a*weight;
    }
    Q += (1.-w/8.)*(s/n-Q);
    Q = clamp(Q,0.,1.);
    if (iFrame < 20) Q = B(U);
    if (iMouse.z>0.) Q = mix(Q,Q,exp(-50.*length(U-iMouse.xy)/R.y));
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
void mainImage( out vec4 Q, vec2 U )
{
    
    
    Q = A(U);
    vec4 s = vec4(0);
    float n = 1e-5, w = 0.;
    vec2 g = vec2(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++) {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec4 d = a - Q;
        float l = length(d);
        w += l;
        g += l * u;
        float weight = max(exp(-200.*l*l)-.2,-1e-3);
       	n+=weight;
        s += a*weight;
    }
    Q += (1.-w/8.)*(s/n-Q);
    Q = clamp(Q,0.,1.);
    if (iFrame < 20) Q = B(U);
    if (iMouse.z>0.) Q = mix(Q,Q,exp(-50.*length(U-iMouse.xy)/R.y));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
void mainImage( out vec4 Q, vec2 U )
{
    
    
    Q = A(U);
    vec4 s = vec4(0);
    float n = 1e-5, w = 0.;
    vec2 g = vec2(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++) {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec4 d = a - Q;
        float l = length(d);
        w += l;
        g += l * u;
        float weight = max(exp(-200.*l*l)-.2,-1e-3);
       	n+=weight;
        s += a*weight;
    }
    Q += (1.-w/8.)*(s/n-Q);
    Q = clamp(Q,0.,1.);
    if (iFrame < 20) Q = B(U);
    if (iMouse.z>0.) Q = mix(Q,Q,exp(-50.*length(U-iMouse.xy)/R.y));
}