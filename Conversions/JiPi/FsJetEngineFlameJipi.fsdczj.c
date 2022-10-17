
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Gray Noise Small' to iChannel0


//
// Based on "Flame in the Wind" by kuvkar (https://www.shadertoy.com/view/4tXXRn)
//

__DEVICE__ float GetNoise(float2 uv, __TEXTURE2D__ iChannel0) // -> (-0.375f, 0.375f)
{
    float n = (_tex2DVecN(iChannel0,uv.x,uv.y,15).x - 0.5f) * 0.5f; // -0.25f, 0.25
    n += (_tex2DVecN(iChannel0, uv.x * 2.0f,uv.y * 2.0f, 15).x - 0.5f) * 0.5f * 0.5f; // -0.375f, 0.375
    
    return n;
}

__DEVICE__ mat2 GetRotationMatrix(float angle)
{
    mat2 m;
    //m[0][0] = _cosf(angle); m[0][1] = -_sinf(angle);
    //m[1][0] = _sinf(angle); m[1][1] = _cosf(angle);

    m = to_mat2(_cosf(angle),-_sinf(angle),_sinf(angle),_cosf(angle));

    return m;
}

#define flamePersonalitySeed 0.5f

// -----------------------------------------------
__KERNEL__ void FsJetEngineFlameJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Quad, 1);
    CONNECT_CHECKBOX1(AlphaColor1, 0);
    
    CONNECT_COLOR0(Color1, 1.0f, 1.0f, 0.6f, 1.0f);
    CONNECT_COLOR1(Color2, 1.0f, 1.0f, 1.0f, 1.0f);
    CONNECT_COLOR2(Color3, 0.89f, 0.27f, 0.043f, 1.0f);
    CONNECT_COLOR3(ColorBKG, 1.0f, 1.0f, 1.0f, 1.0f);
    
    
    CONNECT_SLIDER0(FlameSpeed, 0.0f, 3.0f, 0.8f);
    CONNECT_SLIDER1(NoiseResolution, 0.0f, 3.0f, 0.7f);
    CONNECT_SLIDER2(FlameWidthOff, 0.0f, 3.0f, 0.1f);
    CONNECT_SLIDER3(FlameWidthMul, 0.0f, 3.0f, 0.4f);
    CONNECT_SLIDER4(VariationH, 0.0f, 3.0f, 1.4f);
    
    float2 flameSpacePosition = fragCoord / iResolution - to_float2(0.5f, 0.0f); // (x=[-0.5f, 0.5], y=[0.0, 1.0])
    flameSpacePosition.x *= (iResolution.x / iResolution.y); // obey aspect ratio
        
        
    // Simulate quad
    if(Quad)
    {  
      flameSpacePosition.x *= 1.4f;
      if (_fabs(flameSpacePosition.x) > 1.0f)
      {
          fragColor = to_float4_s(0.0f);
          SetFragmentShaderComputedColor(fragColor);
          return;
      }
    }
    
    float paramFlameProgress = iTime;
    
    //#define FlameSpeed 0.8f
    float2 noiseOffset = to_float2(flamePersonalitySeed, flamePersonalitySeed - paramFlameProgress * FlameSpeed);
    
    ////////////////////////////////////////
    
    float2 uv = flameSpacePosition;        
    
    //
    // Get noise for this fragment and time
    //
    
    //#define NoiseResolution 0.7f
    // (-0.375f, 0.375f)
    float fragmentNoise = GetNoise(uv * NoiseResolution + noiseOffset, iChannel0);
    
    //
    // Rotate fragment based on noise
    //
    
    float angle = fragmentNoise;

    // Tune amount of chaos
    //angle *= 0.45f;

    // Rotate (and add)
    uv += mul_mat2_f2(GetRotationMatrix(angle) , uv);

    //
    // Calculate flameness
    //
    
    //float flameWidth = 0.1f + 0.4f * _fminf(1.0f, _sqrtf(flameSpacePosition.y / 0.4f)); // Taper down 
    float flameWidth = FlameWidthOff + FlameWidthMul * _fminf(1.0f, _sqrtf(flameSpacePosition.y / 0.4f)); // Taper down 
    float flameness = 1.0f - _fabs(uv.x) / flameWidth;
    
    // Taper flame up depending on randomized height
    float variationH = (fragmentNoise + 0.5f) * VariationH;//1.4f;
    flameness *= smoothstep(1.1f, variationH * 0.5f, flameSpacePosition.y);    
    
    //
    // Emit
    //
    
    //float3 col1 = _mix(to_float3(1.0f, 1.0f, 0.6f), to_float3(1.0f, 1.0f, 1.0f), flameness);
    float3 col1 = _mix(swi3(Color1,x,y,z), swi3(Color2,x,y,z), flameness);
    //col1 = _mix(to_float3(227.0f/255.0f, 69.0f/255.0f, 11.0f/255.0f), col1, smoothstep(0.3f, 0.8f, flameness));    
    col1 = _mix(swi3(Color3,x,y,z), col1, smoothstep(0.3f, 0.8f, flameness));    
    float alpha = smoothstep(0.0f, 0.5f, flameness);
    
    //---------------------------------------------
    // Blend with black background    
    
    //fragColor = _mix(to_float4_s(1.0f), to_float4_aw(col1, 1.0f), alpha);
    fragColor = _mix(ColorBKG, to_float4_aw(col1, 1.0f), alpha);

    if (AlphaColor1) fragColor.w = Color1.w;


  SetFragmentShaderComputedColor(fragColor);
}