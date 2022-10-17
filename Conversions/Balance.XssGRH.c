
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution


__DEVICE__ float moise2(float2 c, float t)
{
  return fract(_sinf(dot(to_float2(18.69781f,79.98463f),fract(c + t))) * 4958.1694f);
}

__DEVICE__ void waterUpd( out float4 *fragColor, in float2 fragCoord, float2 resolution, float time, __TEXTURE2D__ sampler )
{
    float2 uv = fragCoord / resolution;
    float val = moise2( uv, _floor( time * 4.0f ) * 0.1234f);
    float rain = 1.0f - step( val, 0.9998f );
    rain *= _fmaxf( 1.0f - fract( time * 4.0f) - 0.7f, 0.0f );

    if( time < 2.0f )
    {
        *fragColor = to_float4_aw( to_float3_s( 0.5f ), 1.0f );
        return;
    }
    
    float2 uv1 = fract( ( fragCoord + to_float2( 1.0f, 0.0f )) / resolution );
    float2 uv2 = fract( ( fragCoord + to_float2( -1.0f, 0.0f )) / resolution );
    float2 uv3 = fract( ( fragCoord + to_float2( 0.0f, 1.0f )) / resolution );
    float2 uv4 = fract( ( fragCoord + to_float2( 0.0f, -1.0f )) / resolution );
    
    float4 inCenter = _tex2DVecN( sampler,uv.x,uv.y,15);
    float neighbourAvg = 
        _tex2DVecN( sampler,uv1.x,uv1.y,15).x + 
        _tex2DVecN( sampler,uv2.x,uv2.y,15).x +
        _tex2DVecN( sampler,uv3.x,uv3.y,15).x +
        _tex2DVecN( sampler,uv4.x,uv4.y,15).x;
    float curSpd = inCenter.y - 0.5f;
    float curAlt = inCenter.x - 0.5f; 
    float spring = neighbourAvg * 0.25f - 0.5f - curAlt;
    spring -= curAlt * 0.2f;
    curSpd *= 0.996f;
    curSpd += spring * 0.32f;
    curSpd += rain * 100.0f;
    curAlt += curSpd * 0.15f;
    curAlt *= 0.996f;
    curSpd = clamp( curSpd, -0.5f, 0.5f ) + 0.5f;
    curAlt = clamp( curAlt, -0.5f, 0.5f ) + 0.5f;
    *fragColor = to_float4( curAlt, curSpd, 0.0f, 1.0f );
    
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void BalanceFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    waterUpd( &fragColor, fragCoord, iResolution, iTime, iChannel0 );


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void BalanceFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
 
    waterUpd( &fragColor, fragCoord, iResolution, iTime, iChannel0 );

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float3 yinYang( in float2 fragCoord, float2 R, float iTime,  __TEXTURE2D__ iChannel1, float2 TexturPos, float TexturSize, float TexturBright )
{
  // some old unpublished shader utilized
  float2 uv = fragCoord / iResolution;
  uv -= to_float2( 0.5f,0.5f );
  float aspect = iResolution.x / iResolution.y;
  uv.x *= aspect;
  float3 col = to_float3( 1.0f, 1.0f, 1.0f );
  col = swi3(texture(iChannel1, ((fragCoord/R)+TexturPos)*TexturSize),x,y,z)*TexturBright;
  
  
  float2 offset = to_float2( uv.y, -uv.x );
  float2 uv2 = uv + offset * _sinf( length( uv ) * 20.0f + iTime) * 1.55f;
  float lightness = 1.0f * _sinf( 10.0f *uv2.x ) * _cosf( 10.0f * uv2.y );
  lightness += 1.0f * _cosf( 10.0f *uv.x ) * _sinf( -10.0f * uv.y );
  lightness = _powf( clamp(lightness, 0.0f, 1.0f ), 0.45f );
  lightness *= 2.0f - 2.9f * dot( uv, uv );
  return col;// * _fmaxf( lightness, 0.0f );
}

__DEVICE__ float3 sky( float3 ray, float2 R, float iTime,  __TEXTURE2D__ iChannel1, float2 TexturPos, float TexturSize, float TexturBright )
{
    float horizonLight = 0.2f - ray.y;
    ray.y = _fmaxf( 0.0f, ray.y );
    float dayTime = _cosf( iTime * 0.05f);
    
    float3 moonDir = normalize( to_float3( 0.1f, -0.2f * dayTime + 0.1f, 0.9f ) );
    float moonDot = dot( moonDir, ray );
    float moon = smoothstep( 0.996f, 0.997f, moonDot );
    float3 ret = to_float3( 0.0f, 0.07f, 0.12f);
    ret += moon * to_float3( 1.5f, 0.6f, 0.4f );
    ret += smoothstep( 0.9f, 1.0f, moonDot ) * to_float3( 0.1f, 0.0f, 0.0f );
    float dayTimeN = 0.5f + 0.5f * dayTime;
    float3 horizonColor =  (1.0f -  dayTimeN) * to_float3( 0.1f, 0.95f, 0.4f ) + dayTimeN * to_float3( 1.0f, 0.3f, -0.1f );
    ret += horizonLight * horizonColor ;
    ret *= 3.0f;
    ret *= clamp( 0.15f * iTime - 0.6f, 0.0f, 1.0f );
    
    float yinYangVal = clamp( iTime, 0.0f, 1.0f );
    
    yinYangVal *= _fmaxf( _cosf( iTime * 0.1f ) - 0.3f, 0.0f );
    yinYangVal *= yinYangVal * yinYangVal;
    
    ret += yinYangVal * yinYang( (swi2(ray,x,y) + to_float2(0.25f, 0.0f)) * iResolution.x * 1.6f, R, iTime, iChannel1, TexturPos,TexturSize, TexturBright );
    
    return ret;
}

__KERNEL__ void BalanceFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_POINT0(TexturPos, 0.0f, 0.0f);
    CONNECT_SLIDER0(TexturSize, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER1(TexturBright, 0.0f, 10.0f, 1.0f);
  
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    uv -= to_float2( 0.5f,0.7f );
    uv *= 0.5f;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    
    float3 ray = to_float3( uv.x, uv.y, 1.0f );
    ray = normalize( ray );
    
    if( ray.y >= 0.0f )
    {
        fragColor = to_float4_aw( sky(ray,R,iTime,iChannel1, TexturPos,TexturSize, TexturBright), 1.0f );
        
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    float2 targetUv = to_float2( 0.5f + ray.x * 0.07f / -ray.y, ray.z * 0.07f / -ray.y );
    targetUv *= iResolution.y / 300.0f;
    targetUv.y += _sinf( iTime * 0.03f );
    targetUv.x += _cosf( iTime * 0.03f );
    
    float tex = texture( iChannel0, fract( targetUv ) ).x - 0.5f;
    targetUv *= to_float2_s( 1.0f - 0.502f * tex );
    targetUv *= 200.0f / iResolution.y;
    
    float2 scale = 1.0f / iResolution;
    float2 offs = to_float2(1.0f, 1.0f) * scale;
    
    tex = texture( iChannel0, fract( targetUv ) ).x;
    
    float texL = texture( iChannel0, fract( targetUv - to_float2(offs.x, 0.0f) ) ).x;
    float texD = texture( iChannel0, fract( targetUv - to_float2(0.0f, offs.y) ) ).x;
    float3 normal = to_float3( tex - texL, 0.4f, tex - texD );
    normal /= length( normal );
    
    float3 reflection = normalize( ray - 2.0f * dot( ray, normal ) * normal );
    
    float fresnel = ( 1.0f - 3.0f * dot( normal, reflection ) );
    fragColor = to_float4_aw( sky(reflection,R,iTime,iChannel1, TexturPos,TexturSize, TexturBright) * fresnel, 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}
