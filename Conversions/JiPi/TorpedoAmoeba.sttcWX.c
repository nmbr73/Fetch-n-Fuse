
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


#define A(u) _tex2DVecN(iChannel0,(u).x/iResolution.x, (u).y/iResolution.y, 15)

__KERNEL__ void TorpedoAmoebaFuse__Buffer_A(float4 fragColor, float2 u, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    u+=0.5f;

    float4 a =  A(u+to_float2( 1, 0))
               +A(u+to_float2( 0, 1))
               +A(u+to_float2(-1, 0))
               +A(u+to_float2( 0,-1))
               +A(u+to_float2( 1, 1))
               +A(u+to_float2(-1, 1))
               +A(u+to_float2( 1,-1))
               +A(u+to_float2(-1,-1))
               +A(u+to_float2( 0, 0));
    uint s = (uint)(dot(a,to_float4(1,1,1,0)));
    float4 o = swi4(A(u+to_float2(0,0)),x,x,y,z);
         o.x = (float)(((+16U*8U*16U*8U* (1U<<11U) 
                         +16U*8U*16U*    5U
                         +16U*8U*        0U
                         +16U*           1U
                         +               7U)>>s)&1U);

    if(iFrame==0||iMouse.z>0.5f)
    {
        float2 v = 1.0f*(u        *2.0f-iResolution)/iResolution.y;
        float2 m = 1.0f*(swi2(iMouse,x,y)*2.0f-iResolution)/iResolution.y;
        o = _floor(fract_f4(_cosf(dot(u,to_float2(1.76543f,iTime+22.5363f)))*to_float4(2467.5678f,
                                                                                       3467.5678f,
                                                                                       4467.5678f,
                                                                                       5467.5678f))+0.5f);
        o*= step(dot(v,v),dot(m,m)*0.1f);
    }
    fragColor = o;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0

__KERNEL__ void TorpedoAmoebaFuse(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{

    float4 a = A(fragCoord);
    if((iFrame&3)==0){a = swi4(a,x,y,z,w);}
    if((iFrame&3)==1){a = swi4(a,y,z,w,x);}
    if((iFrame&3)==2){a = swi4(a,z,w,x,y);}
    if((iFrame&3)==3){a = swi4(a,w,x,y,z);}
    fragColor = cos_f4(dot(a,to_float4(1,2,4,8)*3.0f)+to_float4(0,1,2,3));

  SetFragmentShaderComputedColor(fragColor);
}