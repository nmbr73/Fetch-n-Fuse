
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Hue to RGB function from Fabrice's shadertoyunofficial blog:
#define hue2rgb(hue) 0.6f + 0.6f * cos_f3(6.3f * hue + to_float3(0.0f, 23.0f, 21.0f))




__DEVICE__ float3 irri(float hue) {
  return 0.5f+ 0.5f *cos_f3(( 9.0f*hue)+ to_float3(0,23.0f,21.0f));
}



__DEVICE__ float metaballs(float2 uv, float time, float numOfBalls, float distanceTraveled, float radius) {                  
  float size = 0.9f;          
  const float startIndex = numOfBalls;
  const float endIndex = numOfBalls * 2.0f;
    
    for(float i = startIndex; i < endIndex; i+=1.0f) {          // create x number of balls                      // get rads for control point
      float radius = distanceTraveled * _sinf(time + i * 2.0f);    // calculate radius
      float2 ball = radius * to_float2(_sinf(i), _cosf(i));          // ball position
      size += 1.0f / _powf(i, distance_f2(uv, ball));          // metaball calculation
    }
    return size;
}

__DEVICE__ float dFdx(float value, float2 fragCoord, float2 iResolution)
{
   return ( value*fragCoord.x / iResolution.x );
}
__DEVICE__ float dFdy(float value, float2 fragCoord, float2 iResolution)
{
   return ( value*fragCoord.y / iResolution.y );
}

__DEVICE__ float aastep(float threshold, float value, float2 fragCoord, float2 iResolution) {

    float afwidth = length(to_float2(dFdx(value, fragCoord, iResolution), dFdy(value, fragCoord, iResolution))) / _fmaxf(iResolution.x,iResolution.y);
    return smoothstep(threshold-afwidth, threshold+afwidth, value);
 }

#define R iResolution

__KERNEL__ void FeedbackPracticeJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;

	const float radius = 0.15f;
    const float2 center = to_float2_s(0.0f);

	const float numOfBalls = 10.0f;
	const float distanceTraveled = 1.5f;
	const float speed = 0.8f;
	const float rotationSpeed = 0.5f;

	
    float2 screenCenter = 0.5f * iResolution;
    float2 uv = (fragCoord - screenCenter) / iResolution.y;
    
    
     float zoom = 0.99f;
     float2 direction = to_float2(-0.0025f,0.0025f)*0.0f;
     float4 previousColor =  texture(iChannel0, ((fragCoord - screenCenter) * zoom / iResolution + 0.5f)+direction);
     float edgeSmoothing = 1.0f/_fmaxf(iResolution.x,iResolution.y);
      //previousColor *= iFeedbackFadeRate;
      float2 off = to_float2(_sinf(iTime)*0.5f,0.0f);
      float blob = metaballs((uv+off)*8.0f, iTime, numOfBalls, distanceTraveled, radius);
      float shape = smoothstep(
        1.0f,
        1.0f+ edgeSmoothing,
        blob
      );
      shape = aastep(1.0f-_sinf(iTime)*0.5f+0.5f, blob, fragCoord, iResolution);
      //shape = smoothstep(dFdx(uv.y),1.0f-dFdx(uv.y),shape);
      float3 col = irri(shape+ iTime *0.25f);
      float4 newColor = to_float4_aw(shape*col,1.0f);
      float4 color = _fmaxf(previousColor,newColor);
      // swi3(color,x,y,z) += shape*irri(iTime*0.1f)*1.1f;
      color = clamp(color, 0.0f, 1.0f);
      //fragColor += (1.0f-fragColor.w) * to_float4( swi3(color,x,y,z), 1 ) *color.w;
      
      fragColor = color*0.95f;
      
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void FeedbackPracticeJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  float2 uv = fragCoord / iResolution;
  fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);


  SetFragmentShaderComputedColor(fragColor);
}