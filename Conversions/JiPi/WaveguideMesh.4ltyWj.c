
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1

#define R iResolution

__DEVICE__ float4 bufB( in float2 coord, float2 R, __TEXTURE2D__ iChannel1 )
{
    float2 uv = coord/iResolution;
    return _tex2DVecN(iChannel1,uv.x,uv.y,15)*2.0f-1.0f;
}
__KERNEL__ void WaveguideMeshFuse__Buffer_A(float4 fragColor, float2 c, float2 iResolution, float4 iMouse, sampler2D iChannel1)
{
    c+=0.5f;

    float2 uv = c/iResolution;
    float2 e = to_float2(1, 0);
    float v = 0.5f*(bufB(c-swi2(e,x,y),R,iChannel1).x + bufB(c-swi2(e,y,x),R,iChannel1).y + bufB(c+swi2(e,x,y),R,iChannel1).z + bufB(c+swi2(e,y,x),R,iChannel1).w);
    fragColor = to_float4_s(v);
    if (iMouse.z > 0.0f)
      fragColor += to_float4_s(_sinf(_fmaxf(0.0f,25.0f-0.5f*distance_f2(c, swi2(iMouse,x,y)))));
    fragColor = fragColor*0.5f+0.5f;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__DEVICE__ float bufA( in float2 coord, float2 R, __TEXTURE2D__ iChannel0 )
{
    float2 uv = coord/iResolution;
    return _tex2DVecN(iChannel0,uv.x,uv.y,15).w*2.0f-1.0f;
}
#ifdef XXX
__DEVICE__ float4 bufB( in float2 coord, float2 R, __TEXTURE2D__ iChannel1 )
{
    float2 uv = coord/iResolution;
    return _tex2DVecN(iChannel1,uv.x,uv.y,15)*2.0f-1.0f;
}
#endif
__KERNEL__ void WaveguideMeshFuse__Buffer_B(float4 fragColor, float2 c, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    c+=0.5f;

    const float DECAY = 0.999f;

    float2 uv = c/iResolution;
    float2 e = to_float2(1, 0);
    float p = bufA(c,R,iChannel0);
    fragColor = to_float4(
        p-bufB(c+swi2(e,x,y),R,iChannel1).z,
        p-bufB(c+swi2(e,y,x),R,iChannel1).w,
        p-bufB(c-swi2(e,x,y),R,iChannel1).x,
        p-bufB(c-swi2(e,y,x),R,iChannel1).y
    )*DECAY*0.5f+0.5f;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void WaveguideMeshFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    float v = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
    fragColor = to_float4_s(v);

  SetFragmentShaderComputedColor(fragColor);
}