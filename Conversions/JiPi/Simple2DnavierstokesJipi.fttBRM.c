
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3
// Connect Buffer A 'Texture: Barrier' to iChannel4


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Advection & force



#define VelocityTextureA iChannel3

__KERNEL__ void Simple2DnavierstokesJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, sampler2D iChannel3)
{

    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(TexBarrier, 0);

    fragCoord+=0.5f;


    CONNECT_POINT0(ForceXY, 0.0f, 0.0f );
    CONNECT_POINT1(ForceAreaMinXY, 0.0f, 0.0f );
    CONNECT_POINT2(ForceAreaMaxXY, 0.0f, 0.0f );

    // Magic force within a rectangle.
    const float2 Force = to_float2(100.0f, 0.0f) + ForceXY;
    const float2 ForceAreaMin = to_float2(0.0f, 0.2f) + ForceAreaMinXY; 
    const float2 ForceAreaMax = to_float2(0.06f, 0.8f) + ForceAreaMaxXY;

    CONNECT_POINT3(BarrierPositionXY, 0.0f, 0.0f );
    CONNECT_SLIDER0(BarrierRadiusSq, -10.0f, 1.0f, 0.01f);

    // Circular barrier.
    const float2 BarrierPosition = to_float2(0.2f, 0.5f)+BarrierPositionXY;
    //const float BarrierRadiusSq = 0.01f;


    float2 inverseResolution = to_float2_s(1.0f) / iResolution;
    float2 uv = fragCoord * inverseResolution;

    // Simple advection by backstep.
    // Todo: Try better methods like MacCormack (http://http.developer.nvidia.com/GPUGems3/gpugems3_ch30.html)
    float2 oldVelocity = swi2(_tex2DVecN(VelocityTextureA,uv.x,uv.y,15),x,y);
    float2 samplePos = uv - oldVelocity * iTimeDelta * inverseResolution;
    float2 outputVelocity = swi2(_tex2DVecN(VelocityTextureA,samplePos.x,samplePos.y,15),x,y);
    
    // Add force.
    if(uv.x > ForceAreaMin.x && uv.x < ForceAreaMax.x &&
       uv.y > ForceAreaMin.y && uv.y < ForceAreaMax.y)
    {
      outputVelocity += Force * iTimeDelta;
    }
    
    // Clamp velocity at borders to zero.
    if(uv.x > 1.0f - inverseResolution.x ||
        uv.y > 1.0f - inverseResolution.y ||
        uv.x < inverseResolution.x ||
        uv.y < inverseResolution.y)
    {
        outputVelocity = to_float2(0.0f, 0.0f);
    }
    
    
    // Circle barrier.
    if ( TexBarrier )
    {
      float tex = texture(iChannel4,fragCoord/iResolution).w;
      if (tex > 0.0f)
      {
          fragColor = to_float4(0.0f, 0.0f, 999.0f, 0.0f);
      }
      else
      {
          fragColor = to_float4(outputVelocity.x, outputVelocity.y, 0.0f, 0.0f);
      }   
    
    }
    else
    {
      float2 toBarrier = BarrierPosition - uv;
      toBarrier.x *= inverseResolution.y / inverseResolution.x;
      if(dot(toBarrier, toBarrier) < BarrierRadiusSq)
      {
          fragColor = to_float4(0.0f, 0.0f, 999.0f, 0.0f);
      }
      else
      {
          fragColor = to_float4(outputVelocity.x, outputVelocity.y, 0.0f, 0.0f);
      } 
    }
    
    if( Reset ) 
       fragColor = to_float4(0.0f, 0.0f, 0.0f, 0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


// Compute divergence.

#define VelocityTexture iChannel0

__KERNEL__ void Simple2DnavierstokesJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    float2 inverseResolution = to_float2_s(1.0f) / iResolution;
    float2 uv = fragCoord * inverseResolution;
    
    // Obstacle?
    if(_tex2DVecN(VelocityTexture,uv.x,uv.y,15).z > 0.0f)
    {
        fragColor = to_float4_s(0.0f);
        
        SetFragmentShaderComputedColor(fragColor);      
        return;
    }

    float x0 = texture(VelocityTexture, uv - to_float2(inverseResolution.x, 0)).x;
    float x1 = texture(VelocityTexture, uv + to_float2(inverseResolution.x, 0)).x;
    float y0 = texture(VelocityTexture, uv - to_float2(0, inverseResolution.y)).y;
    float y1 = texture(VelocityTexture, uv + to_float2(0, inverseResolution.y)).y;
    float divergence = ((x1-x0) + (y1-y0)) * 0.5f;
    fragColor = to_float4_s(divergence);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


// Jacobi iteration
// For a more accurate result, this should be executed multiple times.

#define DivergenceTexture iChannel1
#define PressureTexture iChannel2
//#define VelocityTexture iChannel0



__DEVICE__ float samplePressure(float2 pos, float2 border, __TEXTURE2D__ VelocityTexture, __TEXTURE2D__ PressureTexture)
{
    // Obstacle?
    if(_tex2DVecN(VelocityTexture,pos.x,pos.y,15).z > 0.0f)
    {
      return 0.0f;
    }
    
    // Boundary condition: Vanish for at walls.
    if(pos.x > 1.0f - border.x || pos.y > 1.0f - border.y ||
        pos.x < border.x || pos.y < border.y)
    {
      return 0.0f;
    }
     else
    {
      return _tex2DVecN(PressureTexture,pos.x,pos.y,15).x;
    }
}

__KERNEL__ void Simple2DnavierstokesJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    fragCoord+=0.5f;
    
    float2 inverseResolution;
    float2 border;
    float2 uv;

    inverseResolution = to_float2_s(1.0f) / iResolution;
    border = inverseResolution * 2.0f;
    uv = fragCoord * inverseResolution;
    
    float div = _tex2DVecN(DivergenceTexture,uv.x,uv.y,15).x;
    float x0 = samplePressure(uv - to_float2(inverseResolution.x, 0), border, VelocityTexture, PressureTexture);
    float x1 = samplePressure(uv + to_float2(inverseResolution.x, 0), border, VelocityTexture, PressureTexture);
    float y0 = samplePressure(uv - to_float2(0, inverseResolution.y), border, VelocityTexture, PressureTexture);
    float y1 = samplePressure(uv + to_float2(0, inverseResolution.y), border, VelocityTexture, PressureTexture);
    
    fragColor = to_float4_s((x0 + x1 + y0 + y1 - div) * 0.25f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// Subtract pressure gradient to ensure zero divergence.

//#define PressureTexture iChannel2
//#define VelocityTexture iChannel0

__KERNEL__ void Simple2DnavierstokesJipiFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{
  
    fragCoord+=0.5f;  

    float2 inverseResolution = to_float2_s(1.0f) / iResolution;
    float2 uv = fragCoord * inverseResolution;
    
    float x0 = texture(PressureTexture, uv - to_float2(inverseResolution.x, 0)).x;
    float x1 = texture(PressureTexture, uv + to_float2(inverseResolution.x, 0)).x;
    float y0 = texture(PressureTexture, uv - to_float2(0, inverseResolution.y)).x;
    float y1 = texture(PressureTexture, uv + to_float2(0, inverseResolution.y)).x;
    float2 pressureGradient = (to_float2(x1, y1) - to_float2(x0, y0)) * 0.5f;
    float2 oldV = swi2(_tex2DVecN(VelocityTexture,uv.x,uv.y,15),x,y);
    
    fragColor = to_float4(oldV.x - pressureGradient.x, oldV.y - pressureGradient.y, 0.0f, 0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


#define VELOCITY 0 
#define PRESSURE 1
#define DIVERGENCE 2

#define SHOW VELOCITY


__DEVICE__ float4 showPressure(float2 uv, __TEXTURE2D__ iChannel2)
{
    return abs_f4(_tex2DVecN(iChannel2,uv.x,uv.y,15)) * 0.05f;
}

__DEVICE__ float4 showVelocity(float2 uv, __TEXTURE2D__ iChannel0)
{
    float4 color = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    if(color.z > 0.0f) // obstacle
    {
        return to_float4_s(0.5f);
    }
    else
    {
        return abs_f4(color) * 0.008f;
    }
}

__DEVICE__ float4 showDivergence(float2 uv, __TEXTURE2D__ iChannel1)
{
    // Divergence should be as close to 0 as possible.. sadly it isn't.
    return abs_f4(_tex2DVecN(iChannel1,uv.x,uv.y,15));
}

__KERNEL__ void Simple2DnavierstokesJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  
  CONNECT_BUTTON0(Modus, 0, Velocity, Pressure, Divergence, BTN4, BTN5);
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_SLIDER1(Brightness, -1.0f, 10.0f, 1.0f);
  

    float2 uv = fragCoord / iResolution;

    if (Modus == 1)
      fragColor = showVelocity(uv, iChannel0);
    else
      if (Modus == 2)  
        fragColor = showPressure(uv, iChannel2);
      else
      if (Modus == 3)
        fragColor = showDivergence(uv, iChannel1);
    
    float pseudoAlpha = (float) (fragColor.x <= 0.0f && fragColor.y <= 0.0f ? 0.0f : 1.0f);
    
    fragColor += (Color-0.5f) * pseudoAlpha * Brightness;
    fragColor.w = Color.w;

  SetFragmentShaderComputedColor(fragColor);
}