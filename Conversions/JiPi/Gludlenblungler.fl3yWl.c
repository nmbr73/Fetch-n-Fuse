
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define M (+8U*4U*8U*4U* (1U<<0U)\
           +8U*4U*8U*    3U\
           +8U*4U*       5U\
           +8U*          3U\
           +             7U)
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0



#define A(u) _tex2DVecN(iChannel0,(u).x/iResolution.x, (u).y/iResolution.y, 15)

__KERNEL__ void GludlenblunglerFuse__Buffer_A(float4 fragColor, float2 u, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    u+=0.5f;

    float2 v = 1.0f*(u        *2.0f-iResolution)/iResolution.y;
    float2 m = 1.0f*(swi2(iMouse,x,y)*2.0f-iResolution)/iResolution.y;

    float4 a =  A(u+to_float2( 1, 0))
               +A(u+to_float2( 0, 1))
               +A(u+to_float2(-1, 0))
               +A(u+to_float2( 0,-1))
               +A(u+to_float2( 1, 1))
               +A(u+to_float2(-1, 1))
               +A(u+to_float2( 1,-1))
               +A(u+to_float2(-1,-1))
               +A(u+to_float2( 0, 0))
               
               +A(u+to_float2( 2,-2))
               +A(u+to_float2( 2,-1))
               +A(u+to_float2( 2, 0))
               +A(u+to_float2( 2, 1))
               +A(u+to_float2( 2, 2))
               +A(u+to_float2( 1, 2))
               +A(u+to_float2( 0, 2))
               +A(u+to_float2(-1, 2))
               +A(u+to_float2(-2, 2))
               +A(u+to_float2(-2, 1))
               +A(u+to_float2(-2, 0))
               +A(u+to_float2(-2,-1))
               +A(u+to_float2(-2,-2))
               +A(u+to_float2(-1,-2))
               +A(u+to_float2( 0,-2))
               +A(u+to_float2( 1,-2))
               
               +A(u+to_float2( 3, 0))
               +A(u+to_float2( 0, 3))
               +A(u+to_float2(-3, 0))
               +A(u+to_float2( 0,-3));
    uint s = (uint)(dot(a,to_float4(1,1,1,0))+0.1f);
    float4 o = swi4(A(u+to_float2(0,0)),x,x,y,z);
         o.x = (float)((M>>s)&1U);

    if(iFrame==0 || iMouse.z>0.5f)
    {
        o = _floor(fract_f4(_cosf(dot(u,to_float2(1.76543f,iTime+1.5363f)))*to_float4(2467.5678f,
                                                                                      3467.5678f,
                                                                                      4467.5678f,
                                                                                      5467.5678f))+0.5f);
        o*= step(dot(v,v),dot(m,m));
    }
    fragColor = o;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0



__KERNEL__ void GludlenblunglerFuse__Buffer_B(float4 fragColor, float2 u, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    u+=0.5f;
  
    float2 v = 1.0f*(u        *2.0f-iResolution)/iResolution.y;
    float2 m = 1.0f*(swi2(iMouse,x,y)*2.0f-iResolution)/iResolution.y;

    float4 a =  A(u+to_float2( 1, 0))
               +A(u+to_float2( 0, 1))
               +A(u+to_float2(-1, 0))
               +A(u+to_float2( 0,-1))
               +A(u+to_float2( 1, 1))
               +A(u+to_float2(-1, 1))
               +A(u+to_float2( 1,-1))
               +A(u+to_float2(-1,-1))
               +A(u+to_float2( 0, 0))
               
               +A(u+to_float2( 2,-2))
               +A(u+to_float2( 2,-1))
               +A(u+to_float2( 2, 0))
               +A(u+to_float2( 2, 1))
               +A(u+to_float2( 2, 2))
               +A(u+to_float2( 1, 2))
               +A(u+to_float2( 0, 2))
               +A(u+to_float2(-1, 2))
               +A(u+to_float2(-2, 2))
               +A(u+to_float2(-2, 1))
               +A(u+to_float2(-2, 0))
               +A(u+to_float2(-2,-1))
               +A(u+to_float2(-2,-2))
               +A(u+to_float2(-1,-2))
               +A(u+to_float2( 0,-2))
               +A(u+to_float2( 1,-2))
               
               +A(u+to_float2( 3, 0))
               +A(u+to_float2( 0, 3))
               +A(u+to_float2(-3, 0))
               +A(u+to_float2( 0,-3));
    uint s = (uint)(dot(a,to_float4(1,1,1,0))+0.1f);
    float4 o = swi4(A(u+to_float2(0,0)),x,x,y,z);
         o.x = (float)((M>>s)&1U);

    if(iFrame==0||iMouse.z>0.5f)
    {
        o = _floor(fract_f4(_cosf(dot(u,to_float2(1.76543f,iTime+1.5363f)))*to_float4(2467.5678f,
                                                                                      3467.5678f,
                                                                                      4467.5678f,
                                                                                      5467.5678f))+0.5f);
        o*= step(dot(v,v),dot(m,m));
    }
    fragColor = o;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void GludlenblunglerFuse__Buffer_C(float4 fragColor, float2 u, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    u+=0.5f;

    float2 v = 1.0f*(u        *2.0f-iResolution)/iResolution.y;
    float2 m = 1.0f*(swi2(iMouse,x,y)*2.0f-iResolution)/iResolution.y;

    float4 a =  A(u+to_float2( 1, 0))
               +A(u+to_float2( 0, 1))
               +A(u+to_float2(-1, 0))
               +A(u+to_float2( 0,-1))
               +A(u+to_float2( 1, 1))
               +A(u+to_float2(-1, 1))
               +A(u+to_float2( 1,-1))
               +A(u+to_float2(-1,-1))
               +A(u+to_float2( 0, 0))
               
               +A(u+to_float2( 2,-2))
               +A(u+to_float2( 2,-1))
               +A(u+to_float2( 2, 0))
               +A(u+to_float2( 2, 1))
               +A(u+to_float2( 2, 2))
               +A(u+to_float2( 1, 2))
               +A(u+to_float2( 0, 2))
               +A(u+to_float2(-1, 2))
               +A(u+to_float2(-2, 2))
               +A(u+to_float2(-2, 1))
               +A(u+to_float2(-2, 0))
               +A(u+to_float2(-2,-1))
               +A(u+to_float2(-2,-2))
               +A(u+to_float2(-1,-2))
               +A(u+to_float2( 0,-2))
               +A(u+to_float2( 1,-2))
               
               +A(u+to_float2( 3, 0))
               +A(u+to_float2( 0, 3))
               +A(u+to_float2(-3, 0))
               +A(u+to_float2( 0,-3));
    uint s = (uint)(dot(a,to_float4(1,1,1,0))+0.1f);
    float4 o = swi4(A(u+to_float2(0,0)),x,x,y,z);
         o.x = (float)((M>>s)&1U);

    if(iFrame==0||iMouse.z>0.5f)
    {
        o = _floor(fract_f4(_cosf(dot(u,to_float2(1.76543f,iTime+1.5363f)))*to_float4(2467.5678f,
                                                                                      3467.5678f,
                                                                                      4467.5678f,
                                                                                      5467.5678f))+0.5f);
        o*= step(dot(v,v),dot(m,m));
    }
    fragColor = o;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void GludlenblunglerFuse__Buffer_D(float4 fragColor, float2 u, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    u+=0.5f;

    float2 v = 1.0f*(u        *2.0f-iResolution)/iResolution.y;
    float2 m = 1.0f*(swi2(iMouse,x,y)*2.0f-iResolution)/iResolution.y;

    float4 a =  A(u+to_float2( 1, 0))
               +A(u+to_float2( 0, 1))
               +A(u+to_float2(-1, 0))
               +A(u+to_float2( 0,-1))
               +A(u+to_float2( 1, 1))
               +A(u+to_float2(-1, 1))
               +A(u+to_float2( 1,-1))
               +A(u+to_float2(-1,-1))
               +A(u+to_float2( 0, 0))
               
               +A(u+to_float2( 2,-2))
               +A(u+to_float2( 2,-1))
               +A(u+to_float2( 2, 0))
               +A(u+to_float2( 2, 1))
               +A(u+to_float2( 2, 2))
               +A(u+to_float2( 1, 2))
               +A(u+to_float2( 0, 2))
               +A(u+to_float2(-1, 2))
               +A(u+to_float2(-2, 2))
               +A(u+to_float2(-2, 1))
               +A(u+to_float2(-2, 0))
               +A(u+to_float2(-2,-1))
               +A(u+to_float2(-2,-2))
               +A(u+to_float2(-1,-2))
               +A(u+to_float2( 0,-2))
               +A(u+to_float2( 1,-2))
               
               +A(u+to_float2( 3, 0))
               +A(u+to_float2( 0, 3))
               +A(u+to_float2(-3, 0))
               +A(u+to_float2( 0,-3));
    uint s = (uint)(dot(a,to_float4(1,1,1,0))+0.1f);
    float4 o = swi4(A(u+to_float2(0,0)),x,x,y,z);
         o.x = (float)((M>>s)&1U);

    if(iFrame==0||iMouse.z>0.5f)
    {
        o = _floor(fract_f4(_cosf(dot(u,to_float2(1.76543f,iTime+1.5363f)))*to_float4(2467.5678f,
                                                                                      3467.5678f,
                                                                                      4467.5678f,
                                                                                      5467.5678f))+0.5f);
        o*= step(dot(v,v),dot(m,m));
    }
    fragColor = o;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0

__KERNEL__ void GludlenblunglerFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    //u+=0.5f;
    float4 a = A(fragCoord);
    fragColor = cos_f4(dot(a,to_float4(1,2,4,8)*56.0f)+to_float4(0,1,2,3));

  SetFragmentShaderComputedColor(fragColor);
}