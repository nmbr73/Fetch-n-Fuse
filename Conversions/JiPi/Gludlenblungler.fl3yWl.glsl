

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 a = A(fragCoord);
    fragColor = cos(dot(a,vec4(1,2,4,8)*56.)+vec4(0,1,2,3));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    vec2 v = 1.*(u        *2.-iResolution.xy)/iResolution.y;
    vec2 m = 1.*(iMouse.xy*2.-iResolution.xy)/iResolution.y;

    vec4 a = +A(u+vec2( 1, 0))
             +A(u+vec2( 0, 1))
             +A(u+vec2(-1, 0))
             +A(u+vec2( 0,-1))
             +A(u+vec2( 1, 1))
             +A(u+vec2(-1, 1))
             +A(u+vec2( 1,-1))
             +A(u+vec2(-1,-1))
             +A(u+vec2( 0, 0))
             
             +A(u+vec2( 2,-2))
             +A(u+vec2( 2,-1))
             +A(u+vec2( 2, 0))
             +A(u+vec2( 2, 1))
             +A(u+vec2( 2, 2))
             +A(u+vec2( 1, 2))
             +A(u+vec2( 0, 2))
             +A(u+vec2(-1, 2))
             +A(u+vec2(-2, 2))
             +A(u+vec2(-2, 1))
             +A(u+vec2(-2, 0))
             +A(u+vec2(-2,-1))
             +A(u+vec2(-2,-2))
             +A(u+vec2(-1,-2))
             +A(u+vec2( 0,-2))
             +A(u+vec2( 1,-2))
             
             +A(u+vec2( 3, 0))
             +A(u+vec2( 0, 3))
             +A(u+vec2(-3, 0))
             +A(u+vec2( 0,-3));
    uint s = uint(dot(a,vec4(1,1,1,0))+.1);
    vec4 o = A(u+vec2(0,0)).xxyz;
         o.x=float((M>>s)&1U);

    if(iFrame==0||iMouse.z>.5)
    {
        o = floor(fract(cos(dot(u,vec2(1.76543,iTime+1.5363)))*vec4(2467.5678,
                                                                    3467.5678,
                                                                    4467.5678,
                                                                    5467.5678))+.5);
        o*= step(dot(v,v),dot(m,m));
    }
    fragColor = o;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    vec2 v = 1.*(u        *2.-iResolution.xy)/iResolution.y;
    vec2 m = 1.*(iMouse.xy*2.-iResolution.xy)/iResolution.y;

    vec4 a = +A(u+vec2( 1, 0))
             +A(u+vec2( 0, 1))
             +A(u+vec2(-1, 0))
             +A(u+vec2( 0,-1))
             +A(u+vec2( 1, 1))
             +A(u+vec2(-1, 1))
             +A(u+vec2( 1,-1))
             +A(u+vec2(-1,-1))
             +A(u+vec2( 0, 0))
             
             +A(u+vec2( 2,-2))
             +A(u+vec2( 2,-1))
             +A(u+vec2( 2, 0))
             +A(u+vec2( 2, 1))
             +A(u+vec2( 2, 2))
             +A(u+vec2( 1, 2))
             +A(u+vec2( 0, 2))
             +A(u+vec2(-1, 2))
             +A(u+vec2(-2, 2))
             +A(u+vec2(-2, 1))
             +A(u+vec2(-2, 0))
             +A(u+vec2(-2,-1))
             +A(u+vec2(-2,-2))
             +A(u+vec2(-1,-2))
             +A(u+vec2( 0,-2))
             +A(u+vec2( 1,-2))
             
             +A(u+vec2( 3, 0))
             +A(u+vec2( 0, 3))
             +A(u+vec2(-3, 0))
             +A(u+vec2( 0,-3));
    uint s = uint(dot(a,vec4(1,1,1,0))+.1);
    vec4 o = A(u+vec2(0,0)).xxyz;
         o.x=float((M>>s)&1U);

    if(iFrame==0||iMouse.z>.5)
    {
        o = floor(fract(cos(dot(u,vec2(1.76543,iTime+1.5363)))*vec4(2467.5678,
                                                                    3467.5678,
                                                                    4467.5678,
                                                                    5467.5678))+.5);
        o*= step(dot(v,v),dot(m,m));
    }
    fragColor = o;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    vec2 v = 1.*(u        *2.-iResolution.xy)/iResolution.y;
    vec2 m = 1.*(iMouse.xy*2.-iResolution.xy)/iResolution.y;

    vec4 a = +A(u+vec2( 1, 0))
             +A(u+vec2( 0, 1))
             +A(u+vec2(-1, 0))
             +A(u+vec2( 0,-1))
             +A(u+vec2( 1, 1))
             +A(u+vec2(-1, 1))
             +A(u+vec2( 1,-1))
             +A(u+vec2(-1,-1))
             +A(u+vec2( 0, 0))
             
             +A(u+vec2( 2,-2))
             +A(u+vec2( 2,-1))
             +A(u+vec2( 2, 0))
             +A(u+vec2( 2, 1))
             +A(u+vec2( 2, 2))
             +A(u+vec2( 1, 2))
             +A(u+vec2( 0, 2))
             +A(u+vec2(-1, 2))
             +A(u+vec2(-2, 2))
             +A(u+vec2(-2, 1))
             +A(u+vec2(-2, 0))
             +A(u+vec2(-2,-1))
             +A(u+vec2(-2,-2))
             +A(u+vec2(-1,-2))
             +A(u+vec2( 0,-2))
             +A(u+vec2( 1,-2))
             
             +A(u+vec2( 3, 0))
             +A(u+vec2( 0, 3))
             +A(u+vec2(-3, 0))
             +A(u+vec2( 0,-3));
    uint s = uint(dot(a,vec4(1,1,1,0))+.1);
    vec4 o = A(u+vec2(0,0)).xxyz;
         o.x=float((M>>s)&1U);

    if(iFrame==0||iMouse.z>.5)
    {
        o = floor(fract(cos(dot(u,vec2(1.76543,iTime+1.5363)))*vec4(2467.5678,
                                                                    3467.5678,
                                                                    4467.5678,
                                                                    5467.5678))+.5);
        o*= step(dot(v,v),dot(m,m));
    }
    fragColor = o;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    vec2 v = 1.*(u        *2.-iResolution.xy)/iResolution.y;
    vec2 m = 1.*(iMouse.xy*2.-iResolution.xy)/iResolution.y;

    vec4 a = +A(u+vec2( 1, 0))
             +A(u+vec2( 0, 1))
             +A(u+vec2(-1, 0))
             +A(u+vec2( 0,-1))
             +A(u+vec2( 1, 1))
             +A(u+vec2(-1, 1))
             +A(u+vec2( 1,-1))
             +A(u+vec2(-1,-1))
             +A(u+vec2( 0, 0))
             
             +A(u+vec2( 2,-2))
             +A(u+vec2( 2,-1))
             +A(u+vec2( 2, 0))
             +A(u+vec2( 2, 1))
             +A(u+vec2( 2, 2))
             +A(u+vec2( 1, 2))
             +A(u+vec2( 0, 2))
             +A(u+vec2(-1, 2))
             +A(u+vec2(-2, 2))
             +A(u+vec2(-2, 1))
             +A(u+vec2(-2, 0))
             +A(u+vec2(-2,-1))
             +A(u+vec2(-2,-2))
             +A(u+vec2(-1,-2))
             +A(u+vec2( 0,-2))
             +A(u+vec2( 1,-2))
             
             +A(u+vec2( 3, 0))
             +A(u+vec2( 0, 3))
             +A(u+vec2(-3, 0))
             +A(u+vec2( 0,-3));
    uint s = uint(dot(a,vec4(1,1,1,0))+.1);
    vec4 o = A(u+vec2(0,0)).xxyz;
         o.x=float((M>>s)&1U);

    if(iFrame==0||iMouse.z>.5)
    {
        o = floor(fract(cos(dot(u,vec2(1.76543,iTime+1.5363)))*vec4(2467.5678,
                                                                    3467.5678,
                                                                    4467.5678,
                                                                    5467.5678))+.5);
        o*= step(dot(v,v),dot(m,m));
    }
    fragColor = o;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define M (+8U*4U*8U*4U* (1U<<0U)\
           +8U*4U*8U*    3U\
           +8U*4U*       5U\
           +8U*          3U\
           +             7U)