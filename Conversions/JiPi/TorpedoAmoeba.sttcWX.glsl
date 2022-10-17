

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
    if((iFrame&3)==0){a = a.xyzw;}
    if((iFrame&3)==1){a = a.yzwx;}
    if((iFrame&3)==2){a = a.zwxy;}
    if((iFrame&3)==3){a = a.wxyz;}
    fragColor = cos(dot(a,vec4(1,2,4,8)*3.)+vec4(0,1,2,3));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define A(u) texture(iChannel0,(u)/iResolution.xy)
void mainImage( out vec4 fragColor, in vec2 u )
{
    vec4 a = +A(u+vec2( 1, 0))
             +A(u+vec2( 0, 1))
             +A(u+vec2(-1, 0))
             +A(u+vec2( 0,-1))
             +A(u+vec2( 1, 1))
             +A(u+vec2(-1, 1))
             +A(u+vec2( 1,-1))
             +A(u+vec2(-1,-1))
             +A(u+vec2( 0, 0));
    uint s = uint(dot(a,vec4(1,1,1,0)));
    vec4 o = A(u+vec2(0,0)).xxyz;
         o.x=float(((+16U*8U*16U*8U* (1U<<11U) 
                     +16U*8U*16U*    5U
                     +16U*8U*        0U
                     +16U*           1U
                     +               7U)>>s)&1U);

    if(iFrame==0||iMouse.z>.5)
    {
        vec2 v = 1.*(u        *2.-iResolution.xy)/iResolution.y;
        vec2 m = 1.*(iMouse.xy*2.-iResolution.xy)/iResolution.y;
        o = floor(fract(cos(dot(u,vec2(1.76543,iTime+22.5363)))*vec4(2467.5678,
                                                                     3467.5678,
                                                                     4467.5678,
                                                                     5467.5678))+.5);
        o*= step(dot(v,v),dot(m,m)*.1);
    }
    fragColor = o;
}