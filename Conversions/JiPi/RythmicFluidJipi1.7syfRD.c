
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Blending' to iChannel3
// Connect Buffer A 'Texture: Abstract 2' to iChannel2
// Connect Buffer A 'Texture: Abstract 1' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define FFT(f) texture(iChannel1, to_float2(f, 0.0f)).x
//#define _PIXEL(x, y) texture(iChannel0, (uv + to_float2(x, y) / iResolution)).x //sehr merkwürdig

#define PIXEL(_x, _y) texture(iChannel0, (uv + to_float2(_x, _y) / iResolution)).x

__KERNEL__ void RythmicFluidJipi1Fuse__Buffer_A(float4 out_color, float2 coordinates, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    
    //Blending
    CONNECT_SLIDER2(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    coordinates+=0.5f;
    
    float2 uv = coordinates / iResolution;
        
    float v = PIXEL(0.0f, 0.0f);
    v = PIXEL(
        _sinf(PIXEL(v, 0.0f)  - PIXEL(-v, 0.0f) + 3.1415f) * v * 0.4f, 
        _cosf(PIXEL(0.0f, -v) - PIXEL(0.0f , v) - 1.57f) * v * 0.4f
    );
    v += _powf(FFT(_powf(v*0.1f, 1.5f) * 0.25f) * 1.5f, 3.0f);
    v -= _powf(length(_tex2DVecN(iChannel2,uv.x,uv.y,15)) + 0.05f, 3.0f) * 0.08f;
    v *= 0.925f + FFT(v)*0.1f;
    
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel3,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          v = _mix(v,(tex.x+BlendOff)*BlendMul,Blend);
      }
      else
        if ((int)Modus&32) //Special
         v = _mix(v,(tex.x+BlendOff)*BlendMul,Blend);
      
    }
    
    
    out_color.x = v;

  SetFragmentShaderComputedColor(out_color);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void RythmicFluidJipi1Fuse(float4 out_color, float2 coordinates, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = coordinates/iResolution;
    float v = _tex2DVecN(iChannel0,uv.x,uv.y,15).x * 1.5f;
        
    float3 color = pow_f3(to_float3(_cosf(v), _tanf(v), _sinf(v)) * 0.5f + 0.5f, to_float3_s(0.5f));
    float3 e = to_float3_aw(to_float2_s(1.0f) / iResolution, 0.0f);
    float3 grad = normalize(to_float3(
                                      texture(iChannel0, uv + swi2(e,x,z)).x - texture(iChannel0, uv - swi2(e,x,z)).x, 
                                      texture(iChannel0, uv + swi2(e,z,y)).x - texture(iChannel0, uv - swi2(e,z,y)).x, 1.0f));
    float3 light = to_float3(0.26f, -0.32f, 0.91f);
    float diffuse = dot(grad, light);
    float spec = _powf(_fmaxf(0.0f, -reflect(light, grad).z), 32.0f);
    
    out_color = to_float4_aw((color * diffuse) + spec, 1.0f);
    
  SetFragmentShaderComputedColor(out_color);
}