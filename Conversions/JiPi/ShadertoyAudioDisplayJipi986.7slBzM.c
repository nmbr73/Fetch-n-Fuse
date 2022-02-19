
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/894a09f482fb9b2822c093630fc37f0ce6cfec02b652e4e341323e4b6e4a4543.mp3' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Not mine!!!
__DEVICE__ float3 hsv2rgb( in float3 c ) {
  float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f);
  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing
  return c.z * _mix(to_float3_s(1.0f), rgb, c.y);
}

__KERNEL__ void ShadertoyAudioDisplayJipi986Fuse(float4 O, float2 u, float2 iResolution, sampler2D iChannel0)
{
  
    CONNECT_CHECKBOX0(Spektrum, 1);
    CONNECT_INTSLIDER0(BlockCnt,1,5,1);  
  
    u+=0.5f;
    float2 U = u/iResolution;
  
    float N = 32.0f * BlockCnt,
         _x = fract(U.x * N),
         _y = texture(iChannel0, to_float2((_floor(U.x * N)+0.5f) / N, 1-Spektrum)).x;
    
    if(_fabs(_x*2.0f-1.0f) < 0.75f)
      swi3S(O,x,y,z, clamp(1.0f - 90.0f*(U.y - _y) ,0.0f,1.0f)
                     * hsv2rgb(to_float3((1.0f - _y)*0.6f, 0.5f, 0.9f)) );

  O.w=1.0f;


  SetFragmentShaderComputedColor(O);
}