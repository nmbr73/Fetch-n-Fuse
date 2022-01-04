
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Texture: Abstract 1' to iChannel0


    /*| refract_f3    |*/__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta) {
    /*| refract_f3    |*/    float dotNI = dot(N, I);
    /*| refract_f3    |*/    float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    /*| refract_f3    |*/    if (k < 0.0f) {
    /*| refract_f3    |*/      return to_float3_s(0.0f);
    /*| refract_f3    |*/    }
    /*| refract_f3    |*/    return eta * I - (eta * dotNI * _sqrtf(k)) * (N*-1.5f); //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
    /*| refract_f3    |*/ }
    
    
  __DEVICE__ float3 _refract_f3_2(float3 I, float3 N, float eta) {

    float cosI  = 1.0f-dot(N*1.0f,I*1.0f);
    float sinT2 = eta * eta * (1.0f - cosI * cosI);
    if ( sinT2 > 1.0f) return to_float3_s(0.0f);
    float cosT = _sqrtf(1.0f-sinT2);
    return eta * I + (eta * cosI - cosT) * (-N*2.0f);  // !!!! -N erzielt bessere Werte
}


__DEVICE__ float3 linearLight( float3 s, float3 d )
{
  return 2.0f * s + d * 2.0f - 1.0f;
}

__DEVICE__ float2 uvToCuv( float2 uv, float2 maxuv )
{
      return 2.0f * uv - maxuv;
}
__DEVICE__ float2 cuvToUv( float2 cuv, float2 maxuv )
{
    return (cuv + maxuv) * 0.5f;
}
#define texture(ch,uv) _tex2DVecN(iChannel0, uv.x, uv.y, 15)


__KERNEL__ void Lighten2DFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    float eta = 1.0f / 1.501f;
    float FP = 1.24f;

    float fract_time = fract_f(iTime);
    float cos_time = (_cosf(iTime));

    float2 uv = fragCoord/iResolution.y;
    float2 maxuv = to_float2(iResolution.x / iResolution.y, 1.0f);  
    
    
    float ratio = iResolution.x/iResolution.y;
    uv.x /= ratio;
    maxuv.x /= ratio;
    
    //centrilize uv coordinates   
    float2 cuv = uvToCuv(uv, maxuv);
    
    cuv.x *= ratio;
        
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    float2 ballCoord = to_float2(cos_time, 0.0f);
    
    if (iMouse.z>0.0f)
    {
        ballCoord = swi2(iMouse,x,y)/iResolution - 0.5f;
        ballCoord.x *= 3.5f;
        ballCoord.y *= 2.0f;
        
        cos_time = ballCoord.x;
    }
    
    float ballRadius = 0.5f;
    
    float dist = distance_f2(ballCoord, cuv);
    
    if (dist < ballRadius * 0.995f)
    {  
        float2 balluv = (cuv - ballCoord) / ballRadius;
        float3 norm = to_float3(balluv.x, balluv.y, -_sqrtf(1.0f - dist / ballRadius));
        
        float3 spec = reflect(to_float3(0, 0, 1), norm);
        float3 refr = _refract_f3_2(to_float3(0, 0, 1), norm, eta);
        //refr = to_float3_s(0.0f);    
            
            
        float4 ambient = to_float4(0.09f, 0.08f, 0.07f, 0.0f);
        float4 spec_light = texture(iChannel0, cuvToUv(swi2(spec,x,y) + ballCoord, maxuv));
        float4 refr_light = texture(iChannel0, cuvToUv(swi2(refr,x,y) + ballCoord, maxuv));

        float F = (1.0f - eta) * (1.0f - eta) / ((1.0f + eta) * (1.0f + eta));
        float ratio = F + (1.0f - F) * _powf(dist / ballRadius, FP);
        
        fragColor = _mix(refr_light, spec_light, ratio) + ambient;

        //glare drawing
        float2 lightPos = to_float2(cos_time, 0.5f);
        float distanceToLight = _fabs(cos_time) * 0.5f + 0.5f;
        float4 lightColor = to_float4(0.8f, 1.0f, 0.85f ,1);
        
        float glareSize = 0.5f;
        glareSize *= distanceToLight;
        
        float glareHardness = 0.0f;
        float glareMax = 1.0f;
        
        float2 glareOffset = -lightPos * 0.5f;   
        
        float refrGlareDistance = distance_f2(glareOffset, swi2(refr,x,y));
        float specGlareDistance = distance_f2(glareOffset, swi2(spec,x,y));
        
        if (refrGlareDistance < glareSize)
        {
            float glarePower = 1.0f - smoothstep(glareSize * glareHardness, glareSize, refrGlareDistance);
            fragColor = _mix(fragColor, to_float4_aw(linearLight(swi3(fragColor,x,y,z), swi3(lightColor,x,y,z)), 1.0f ), glarePower * glareMax);
        }
        
        if (specGlareDistance < glareSize)
        {
            float glarePower = 1.0f - smoothstep(glareSize * glareHardness, glareSize, specGlareDistance);
            fragColor = _mix(fragColor, to_float4_aw(linearLight(swi3(fragColor,x,y,z), swi3(lightColor,x,y,z)), 1.0f), glarePower * glareMax);
        }
    }


  SetFragmentShaderComputedColor(fragColor);
}