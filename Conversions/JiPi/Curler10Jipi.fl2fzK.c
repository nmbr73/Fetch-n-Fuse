
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



__DEVICE__ float RandomFloat(float2 testCoord)
{
  // From: https://www.shadertoy.com/view/Xd23Dh
  // (just removed some dimensions)
  float testCoordInGeneratorSpace = 
    dot(testCoord, to_float2(127.1f, 311.7f));

  return fract(_sinf(testCoordInGeneratorSpace) * 43758.5453f);
}

__DEVICE__ float2 RandomVec2(float2 testCoord)
{
  // From: https://www.shadertoy.com/view/Xd23Dh
  // (just removed some dimensions)
  float2 testCoordInGeneratorSpace = to_float2(
    dot(testCoord, to_float2(127.1f, 311.7f)),
    dot(testCoord, to_float2(269.5f, 183.3f)));

  return fract_f2(sin_f2(testCoordInGeneratorSpace) * 43758.5453f);
}

__DEVICE__ float3 RandomVec3(float2 testCoord)
{
    // From: https://www.shadertoy.com/view/Xd23Dh
    // (just removed some dimensions)
    float3 testCoordInGeneratorSpace = to_float3(
        dot(testCoord, to_float2(127.1f, 311.7f)),
        dot(testCoord, to_float2(269.5f, 183.3f)),
        dot(testCoord, to_float2(419.2f, 371.9f)));

  return fract_f3(sin_f3(testCoordInGeneratorSpace) * 43758.5453f);
}

__DEVICE__ float2 TransformFromCanvasTextureToFramedTexture(
  float2 canvasTextureCoord,
  float2 canvasTextureSize,
  float2 framedTextureSize)
{  
  float2 result = (canvasTextureCoord / canvasTextureSize);

  float canvasAspectRatio = (canvasTextureSize.x / canvasTextureSize.y);
  float framedAspectRatio = (framedTextureSize.x / framedTextureSize.y);

  if (framedAspectRatio < canvasAspectRatio)
  {
    float relativeAspectRatio = (canvasAspectRatio / framedAspectRatio);

    result.x *= relativeAspectRatio;
    result.x -= (0.5f * (relativeAspectRatio - 1.0f));
  }
  else
  {
    float relativeAspectRatio = (framedAspectRatio / canvasAspectRatio);

    result.y *= relativeAspectRatio;
    result.y -= (0.5f * (relativeAspectRatio - 1.0f));
  }

  return result;
}

__DEVICE__ bool TextureCoordIsInBounds(float2 texCoord)
{
  float2 inBounds = (
        step(to_float2_s(0.0f), texCoord) * 
        step(texCoord, to_float2_s(1.0f)));
    
    return ((inBounds.x > 0.5f) && (inBounds.y > 0.5f));
}

__DEVICE__ float4 SampleWebcam(
  __TEXTURE2D__  webcamTexture,
  float2 webcamSize,
    bool horizontallyMirrorWebcam,
    float2 canvasCoord,
  float2 canvasSize)
{
  float2 webcamCoord = 
        TransformFromCanvasTextureToFramedTexture(
            canvasCoord, 
            canvasSize, 
            webcamSize);
float zzzzzzzzzzzzzzzzzz;
    if (horizontallyMirrorWebcam)
    {
      webcamCoord.x = _mix(1.0f, 0.0f, webcamCoord.x);
    }
    
    float4 result = TextureCoordIsInBounds(webcamCoord) ? 
        pow_f4(_tex2DVecN(webcamTexture,webcamCoord.x,webcamCoord.y,15), to_float4_s(2.0f)) : 
        to_float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    return result;
}

__DEVICE__ float GetBrightness(float4 color)
{    
    return (0.333f * (color.x + color.y + color.z));
}

__DEVICE__ float4 ComputePseudoGaussianBlur(
  __TEXTURE2D__  source,
    float2 texelSize,
    float2 fragCoord,
    float2 blurDirection) // Unit-length. Typically (1, 0) or (0, 1).
{
    const int k_blurRadius = 10;
  
    float4 accumulator = to_float4_s(0.0f);
    float totalWeight = 0.0f;
    
    float oneOverBlurRadius = (1.0f / 10.0f);

    float2 sampleUvIncrement = (blurDirection * texelSize);
    float2 sampleUv = (
        (fragCoord * texelSize) - 
        ((float)(k_blurRadius) * sampleUvIncrement));
    
    float uncurvedSampleWeight = -1.0f;
    float uncurvedSampleWeightIncrement = (1.0f / (float)(k_blurRadius));
    
    for (int index = (-1 * k_blurRadius); index <= k_blurRadius; index++)
    {
        float4 sampleColor = _tex2DVecN(source,sampleUv.x,sampleUv.y,15);

        // NOTE: Small fudge to make the outermost texels non-zero.
        float sampleWeight = smoothstep(
            (1.0f + oneOverBlurRadius), 
            0.0f, 
            _fabs(uncurvedSampleWeight));
        sampleWeight = (sampleWeight * sampleWeight * sampleWeight); 

        accumulator += (sampleColor * sampleWeight);
        totalWeight += sampleWeight;
        
        sampleUv += sampleUvIncrement;
        uncurvedSampleWeight += uncurvedSampleWeightIncrement;
    }

    return (accumulator / totalWeight);
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel0


// Pass: Webcam perspective correction.

__KERNEL__ void Curler10JipiFuse__Buffer_A(float4 outFragColor, float2 fragCoord, float2 iResolution, float3 iChannelResolution[], sampler2D iChannel0)
{
    fragCoord+=0.5f; 
    outFragColor = SampleWebcam(
    iChannel0,
    //iChannelResolution[0].xy,
        iResolution,
      true, // horizontallyMirrorWebcam
      fragCoord,
    iResolution);
    
    outFragColor.w = GetBrightness(outFragColor);


  SetFragmentShaderComputedColor(outFragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Pass: Vertical blur.

__KERNEL__ void Curler10JipiFuse__Buffer_B(float4 outFragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f; 
    outFragColor = ComputePseudoGaussianBlur(
        iChannel0,
        (1.0f / iResolution),
        fragCoord,
        to_float2(0.0f, 1.0f));

  SetFragmentShaderComputedColor(outFragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// Pass: Horizontal blur.

__KERNEL__ void Curler10JipiFuse__Buffer_C(float4 outFragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f; 
    outFragColor = ComputePseudoGaussianBlur(
        iChannel0,
        (1.0f / iResolution),
        fragCoord,
        to_float2(1.0f, 0.0f));

  SetFragmentShaderComputedColor(outFragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel2
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


// Pass: Curl and simulation.



__DEVICE__ float2 ComputeGradient(float2 fragUv, float2 s_texelSize, __TEXTURE2D__ iChannel0)
{
    // HAAAAACK!
    
    float4 center = _tex2DVecN(iChannel0,fragUv.x,fragUv.y,15);
    float4 right = texture(iChannel0, (fragUv + to_float2(s_texelSize.x, 0.0f)));
    float4 up = texture(iChannel0, (fragUv + to_float2(0.0f, s_texelSize.y)));

    return to_float2((center.w - right.w), (center.w - up.w));
} 

__KERNEL__ void Curler10JipiFuse__Buffer_D(float4 outFragColor, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    fragCoord+=0.5f; 

    float2 s_texelSize = (1.0f / iResolution);
    
    float2 fragUv = (fragCoord * s_texelSize);
    
    float2 gradient = ComputeGradient(fragUv, s_texelSize, iChannel0);
    float2 curl = to_float2((-1.0f * gradient.y), gradient.x);
    
    float4 selfState = _tex2DVecN(iChannel1,fragUv.x,fragUv.y,15);
    
    float2 frameOffset = to_float2(mod_f((float)(iFrame), 97.0f), mod_f((float)(iFrame), 101.0f));
    float2 frameRandom = RandomVec2(fragUv + frameOffset);
    
    if (frameRandom.x > selfState.w)
    {
        float4 camera = _tex2DVecN(iChannel2,fragUv.x,fragUv.y,15);
        
        swi3S(outFragColor,x,y,z, _mix(swi3(selfState,x,y,z), swi3(camera,x,y,z), 0.8f));
        //swi3(outFragColor,x,y,z) = swi3(camera,x,y,z);
        
        //outFragColor.w = _mix(outFragColor.w, 1.0f, frameRandom.y);
        outFragColor.w = 1.0f;
    }
    else
    {
        float2 normalizedCurl = (curl / (length(curl) + 0.00001f));
            
        float4 curlSourceState = texture(iChannel1, (fragUv + (normalizedCurl * s_texelSize)));
        
        outFragColor = curlSourceState;
        //swi2(outFragColor,x,y) = smoothstep(to_float2(-1.0f), to_float2_s(1.0f), (25.0f * curl));
        
        outFragColor.w -= (0.005f * iTimeDelta);
    }

  SetFragmentShaderComputedColor(outFragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void Curler10JipiFuse(float4 outFragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f; 
    float4 selfState = texture(iChannel0, (fragCoord / iResolution));
    
    outFragColor = selfState;
    //outFragColor = (selfState * _powf(selfState.w, 20.0f));
    outFragColor.w = 1.0f;

  SetFragmentShaderComputedColor(outFragColor);
}