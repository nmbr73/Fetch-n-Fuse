
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Blending' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Buffer format:
// R - Moving Particle Count (can be temporarily greater than 1 if particles collide)
// G - Frozen Particle Count (should only ever be 0 or 1)



//const int kKeySpace = 32;

// float2 sTexelSize;

__DEVICE__ float randomFloat(float2 testCoord)
{
  // From: https://www.shadertoy.com/view/Xd23Dh
  // (just removed some dimensions)
  float testCoordInGeneratorSpace = dot(testCoord, to_float2(127.1f, 311.7f));

  return fract(_sinf(testCoordInGeneratorSpace) * 43758.5453f);
}

__DEVICE__ bool coordIsContainedInFragment(
    float2 testCoord,
    float2 fragCoord)
{
    float2 delta = (testCoord - fragCoord);
    return (
        (-0.5f < delta.x) && (delta.x <= 0.5f) && 
        (-0.5f < delta.y) && (delta.y <= 0.5f));
}

__DEVICE__ int wrapDirectionIndex(int unboundedDirectionIndex)
{
    return (
        unboundedDirectionIndex + 
      ((unboundedDirectionIndex < 0) ? 8 : 0) +
      ((unboundedDirectionIndex > 7) ? -8 : 0));
}

__DEVICE__ float getFragRandom(float2 fragCoord, float iTime, float2 sTexelSize)
{
    return randomFloat((fragCoord * sTexelSize) + mod_f(iTime, 15.0f));;
}

__DEVICE__ int directionFractionToIndex(float directionFraction)
{
    return int(directionFraction * 7.999999f);
}


__KERNEL__ void IceCrystalsJipiFuse__Buffer_A(float4 outFragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER0(sd1, -1.0f, 20.0f, 8.0f);
    CONNECT_SLIDER1(sd2, -1.0f, 10.0f, 0.5f);
    CONNECT_SLIDER2(brush_radius, -1.0f, 50.0f, 10.0f);
    CONNECT_SLIDER3(Blend, 0.0f, 1.0f, 0.0f);
        
    fragCoord+=0.5f;
    
    float2 kDirectionDeltas[8] = {
                                  to_float2(1.0f, 0.0f),
                                  to_float2(1.0f, 1.0f),
                                  to_float2(0.0f, 1.0f),
                                  to_float2(-1.0f, 1.0f),
                                  to_float2(-1.0f, 0.0f),
                                  to_float2(-1.0f, -1.0f),
                                  to_float2(0.0f, -1.0f),
                                  to_float2(1.0f, -1.0f)};
    

    float2 sTexelSize = (1.0f / iResolution);
        
    float4 selfState = texture(iChannel0, (fragCoord * sTexelSize));
    
    outFragColor = selfState;
    
    float selfRandom = getFragRandom(fragCoord, iTime, sTexelSize);
        
        
    if (iMouse.z > 0.5f) 
    {
       //if (iMouse.x == fragCoord.x && iMouse.y == fragCoord.y) outFragColor = to_float4(0.0f, 1.0f, 0.0f, 0.0f);
       if(distance_f2(fragCoord, swi2(iMouse,x,y)) < brush_radius) outFragColor = to_float4(0.0f, 1.0f, 0.0f, 0.0f);
    }      

    if (Blend > 0.0f)
    {
       float4 tex = texture(iChannel1, fragCoord/iResolution);
       if(tex.w>0.0f)
         outFragColor.y = _mix(outFragColor.y,tex.x,Blend);
    }
        
        
    if (iFrame == 0 || Reset)
    {
        // If we're the seed-crystal, else we're possibly a moving particle.
        if (coordIsContainedInFragment((iResolution / 2.0f), fragCoord))
        {
            outFragColor = to_float4(0.0f, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            outFragColor = to_float4(step(0.8f, selfRandom), 0.0f, 0.0f, 0.0f);
        }
    }
    else
    {        
        // If we're frozen.
        if (selfState.y > 0.5f)
        {
            // TODO: Handshake with neighbors to let any remaining moving particles escape to empty cells.
            outFragColor.x = 0.0f;
        }
        else
        {
            //float selfDirection = _floor((selfRandom * 8.0f) + 0.5f);
            float selfDirection = _floor((selfRandom * sd1) + sd2);
            
            // Accept moving particles from neighbors.
            {
                float incomingMovingParticleCount = 0.0f;
                for (int directionIndex = 0; directionIndex < 8; directionIndex++)
                {
                    float2 neighborFragCoord = (fragCoord + kDirectionDeltas[directionIndex]);
                    float neighborRandom = getFragRandom(neighborFragCoord,iTime,sTexelSize);

                    // If this neighbor is trying to move into our cell.
                    if (directionFractionToIndex(neighborRandom) == wrapDirectionIndex(directionIndex + 4))
                    {
                        float4 neighborState = texture(iChannel0, (neighborFragCoord * sTexelSize));

                        if (neighborState.x > 0.5f)
                        {
                            incomingMovingParticleCount += 1.0f;
                        }
                    }
                }

                outFragColor.x += incomingMovingParticleCount;
            }
            
            // If we contained a moving particle.
            if (selfState.x > 0.5f)
            {
                float2 destinationFragCoord = (fragCoord + kDirectionDeltas[directionFractionToIndex(selfRandom)]);
                float4 destinationState = texture(iChannel0, (destinationFragCoord * sTexelSize));
                
                // If we just froze.
                if (destinationState.y > 0.5f)
                {
                    outFragColor.y = 1.0f; // Become frozen.

                    // TODO: Handshake with neighbors to let any remaining moving particles escape to empty cells.
                    outFragColor.x = 0.0f;
                }
                else
                {
                    // We're losing one particle to movement towards whichever neighbor is accepting movement from us.
                    outFragColor.x = _fmaxf(0.0f, (outFragColor.x - 1.0f));
                }
            }
        }
    }

  SetFragmentShaderComputedColor(outFragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void IceCrystalsJipiFuse(float4 outFragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(OnlyIce, 0);
    CONNECT_COLOR0(Color, 0.85f, 0.9f, 1.0f, 1.0f);
    CONNECT_COLOR1(BKGColor, 0.25f, 0.25f, 0.7f, 1.0f);
  
    fragCoord+=0.5f;
    float2 sTexelSize = (1.0f / iResolution);

    float Alpha = 1.0f;
    
    float4 selfState = texture(iChannel0, (fragCoord * sTexelSize));
    
    outFragColor = BKGColor;//to_float4(0.25f, 0.25f, 0.7f, 1.0f);
    
    if (selfState.y > 0.5f)
    {
        outFragColor = Color;//to_float4(0.85f, 0.9f, 1.0f, 1.0f);
    }
    else if (selfState.x > 0.5f)
    {
        if (OnlyIce)
          outFragColor = BKGColor;
        else
          outFragColor = _mix(outFragColor, to_float4(0.6f, 0.6f, 1.0f, 1.0f), (1.0f - (1.0f / (1.0f + selfState.x))));
        
      Alpha = Color.w;  
    }

  SetFragmentShaderComputedColor(outFragColor);
}