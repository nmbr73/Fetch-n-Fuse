

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
float f (vec4 a) {return sqrt(dot(a,a));}
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 a = A(U);
    float
        n = f(A(U+vec2(0,1))),
        e = f(A(U+vec2(1,0))),
        s = f(A(U-vec2(0,1))),
        w = f(A(U-vec2(1,0)));
    vec3 no = normalize(vec3(e-w,n-s,10));
    Q = vec4(.8,.2,1,1)*abs(atan(.1*f(a)))*(0.5+0.5*texture(iChannel1,-no));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 m = M(U);
    
    Q.xy += G;
    Q.zw += .5*Q.xy;
    
    
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.zw = vec2(1,-1);
    if (iFrame < 1) Q = vec4(0);
    if (length(U-0.5*R)<30.&&iFrame<10) Q.zw = vec2(1,-1);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define M(U) (0.25*(A((U)+vec2(0,1))+A((U)-vec2(0,1))+A((U)+vec2(1,0))+A((U)-vec2(1,0))) )
#define G (m.zw-Q.zw)-.6*Q.zw*(1e-4*U.y+1.+exp(-.5*dot(Q.zw,Q.zw)))+.003*(m.xy-Q.xy)
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 m = M(U);
    
    Q.xy += G;
    Q.zw += .5*Q.xy;
    
    
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.zw = vec2(1,-1);
    if (iFrame < 1) Q = vec4(0);
    if (length(U-0.5*R)<30.&&iFrame<10) Q.zw = vec2(1,-1);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 m = M(U);
    
    Q.xy += G;
    Q.zw += .5*Q.xy;
    
    
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.zw = vec2(1,-1);
    if (iFrame < 1) Q = vec4(0);
    if (length(U-0.5*R)<30.&&iFrame<10) Q.zw = vec2(1,-1);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 m = M(U);
    
    Q.xy += G;
    Q.zw += .5*Q.xy;
    
    
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.zw = vec2(1,-1);
    if (iFrame < 1) Q = vec4(0);
    if (length(U-0.5*R)<30.&&iFrame<10) Q.zw = vec2(1,-1);
}