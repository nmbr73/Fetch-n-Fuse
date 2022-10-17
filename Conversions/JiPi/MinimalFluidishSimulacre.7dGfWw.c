
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Dave Hoskins https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash13(float3 p3)
{
  p3  = fract_f3(p3 * 0.1031f);
  p3 += dot(p3, swi3(p3,z,y,x) + 31.32f);
  return fract((p3.x + p3.y) * p3.z);
}

#define T(uv) _tex2DVecN(iChannel0,(uv).x,(uv).y,15).w

__KERNEL__ void MinimalFluidishSimulacreFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
        //Blending
    CONNECT_SLIDER2(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(TexOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(TexMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);

    fragCoord+=0.5f;

    // the salt of life
    float noise = hash13(to_float3_aw(fragCoord, iFrame));
    
    // coordinates
    float2 uv = fragCoord/iResolution;

    // random spawn
    float height = clamp(0.001f/noise,0.0f,1.0f);
    
    // mouse interaction
    if (iMouse.z > 0.0f)
        height += clamp(0.02f/length(uv-swi2(iMouse,x,y)/iResolution), 0.0f, 1.0f);
    
    // move uv toward slope direction
    float2 e = to_float2(0.2f*noise,0);
    float2 normal = normalize(to_float2(T(uv+swi2(e,x,y))-T(uv-swi2(e,x,y)),T(uv+swi2(e,y,x))-T(uv-swi2(e,y,x))));
    uv += 5.0f * normal * noise / iResolution;

    // accumulate and fade away
    height = _fmaxf(height, _tex2DVecN(iChannel0,uv.x,uv.y,15).w - 0.005f*noise);
    
    // lighting
    e = to_float2(2.0f/iResolution.y, 0);
    normal = normalize(to_float2(T(uv+swi2(e,x,y))-T(uv-swi2(e,x,y)),T(uv+swi2(e,y,x))-T(uv-swi2(e,y,x))));
    float light = dot(normal, to_float2(0,-1))*0.5f+0.5f;
    
    if (isnan(light)) light = 0.0f;
    
    
    
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel1,fragCoord/iResolution);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          light = _mix(light,(tex.x+TexOff)*TexMul,Blend);

        if ((int)Modus&4)
          height = _mix(height,(tex.x+TexOff)*TexMul,Blend);
        
        if ((int)Modus&8)
        {         
          height = _mix(height,(tex.x+TexOff)*TexMul,Blend);
          light  = _mix(light,tex.x,Blend);
        }
        
      }
    }
    
    
    
    fragColor = to_float4_aw(to_float3_s(light*height), height);
  
    
    if ( iFrame < 1 || Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0



// Minimal Fluidish Simulacre

// found by accident that shifting pixels
// on a height map with the slope direction
// calculated by sampling neighbors at random long range
// produces somehow turbulent fluid movement

// i'm amazed that it produces such organic patterns
// when there is no perlin noise, no gyroid, no force fields
// just white grainy noise and slope movement

// this accident is dedicated to Cornus Ammonis
// which works inspired me in so many ways
// and because this shader looks like a drunken version of his work

__KERNEL__ void MinimalFluidishSimulacreFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);

    float2 uv = fragCoord/iResolution;
    fragColor = to_float4_aw(swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z) + swi3(Color,x,y,z) - 0.5f, Color.w);

    //fragColor = to_float4_aw(swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z), Color.w);
    
  SetFragmentShaderComputedColor(fragColor);
}