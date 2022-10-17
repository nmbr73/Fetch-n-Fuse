
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define SQUARES 5.0f
//#define WIGGLE 2.0f

__DEVICE__ float2 hash22(float2 p)
{
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));

}

__KERNEL__ void SquaresAllSlidingAboutFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_SLIDER0(SQUARES, -100.0f, 100.0f, 5.0f);
  CONNECT_SLIDER1(WIGGLE, -100.0f, 100.0f, 2.0f);
  
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    
    float2 a = hash22(_floor(uv*SQUARES) + _floor(WIGGLE*iTime))-0.5f;
    a *= 1.4f;
    
    if(iTime < 2.0f || Reset)
        fragColor = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    else
    {
        fragColor = texture(iChannel0, uv+a/iResolution);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SquaresAllSlidingAboutFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    // Normalized pixel coordinates (from 0 to 1)
    fragColor = texture(iChannel0, fragCoord/iResolution);

  SetFragmentShaderComputedColor(fragColor);
}