
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/presets/webcam.png' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Meta CRT - Single Pass - @P_Malin
// A single pass version of Meta CRT : https://www.shadertoy.com/view/4dlyWX

// This can be used as a webcam fiter in Memix
// See github here : https://github.com/pmalin/pmalin-memix-shaders

//#define FIXED_CAMERA_INDEX 2

#define FLIP_V 0

#define USE_MOUSE 1


#define PI 3.141592654f


///////////////////////////
// Hash Functions
///////////////////////////

// From: Hash without Sine by Dave Hoskins
// https://www.shadertoy.com/view/4djSRW

// *** Use this for integer stepped ranges, ie Value-Noise/Perlin noise functions.
//#define HASHSCALE1 .1031
//#define HASHSCALE3 to_float3(0.1031f, 0.1030f, 0.0973f)
//#define HASHSCALE4 to_float4(1031, 0.1030f, 0.0973f, 0.1099f)

// For smaller input rangers like audio tick or 0-1 UVs use these...
#define HASHSCALE1 443.8975f
#define HASHSCALE3 to_float3(443.897f, 441.423f, 437.195f)
#define HASHSCALE4 to_float3(443.897f, 441.423f, 437.195f, 444.129f)


//----------------------------------------------------------------------------------------
//  1 out, 1 in...
__DEVICE__ float hash11(float p)
{
  float3 p3  = fract_f3(to_float3_s(p) * HASHSCALE1);
  p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
  return fract((p3.x + p3.y) * p3.z);
}

//  2 out, 1 in...
__DEVICE__ float2 hash21(float p)
{
  float3 p3 = fract_f3(to_float3_s(p) * HASHSCALE3);
  p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));

}

///  2 out, 3 in...
__DEVICE__ float2 hash23(float3 p3)
{
  p3 = fract_f3(p3 * HASHSCALE3);
  p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

//  1 out, 3 in...
__DEVICE__ float hash13(float3 p3)
{
  p3  = fract_f3(p3 * HASHSCALE1);
  p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
  return fract((p3.x + p3.y) * p3.z);
}


#define kMaxTraceDist 1000.0f
#define kFarDist      1100.0f

#define MAT_FG_BEGIN  10

///////////////////////////
// Scene
///////////////////////////

struct SceneResult
{
  float fDist;
  int iObjectId;
  float3 vUVW;
};
    
__DEVICE__ void Scene_Union( inout SceneResult *a, in SceneResult b )
{
    if ( b.fDist < (*a).fDist )
    {
        *a = b;
    }
}

    
__DEVICE__ void Scene_Subtract( inout SceneResult *a, in SceneResult b )
{
    if ( (*a).fDist < -b.fDist )
    {
        (*a).fDist = -b.fDist;
        (*a).iObjectId = b.iObjectId;
        (*a).vUVW = b.vUVW;
    }
}

__DEVICE__ SceneResult Scene_GetDistance( float3 vPos );    

__DEVICE__ float3 Scene_GetNormal(const in float3 vPos)
{
  
    const float fDelta = 0.0001f;
    float2 e = to_float2( -1, 1 );
    
    float3 vNormal = 
                      Scene_GetDistance( swi3(e,y,x,x) * fDelta + vPos ).fDist * swi3(e,y,x,x) + 
                      Scene_GetDistance( swi3(e,x,x,y) * fDelta + vPos ).fDist * swi3(e,x,x,y) + 
                      Scene_GetDistance( swi3(e,x,y,x) * fDelta + vPos ).fDist * swi3(e,x,y,x) + 
                      Scene_GetDistance( swi3(e,y,y,y) * fDelta + vPos ).fDist * swi3(e,y,y,y);
    
    return normalize( vNormal );
}    
    
__DEVICE__ SceneResult Scene_Trace( const in float3 vRayOrigin, const in float3 vRayDir, float minDist, float maxDist )
{  
  SceneResult result;
  result.fDist = 0.0f;
  result.vUVW = to_float3_s(0.0f);
  result.iObjectId = -1;
    
  float t = minDist;
  const int kRaymarchMaxIter = 128;
  for(int i=0; i<kRaymarchMaxIter; i++)
  {    
    float epsilon = 0.0001f * t;
    result = Scene_GetDistance( vRayOrigin + vRayDir * t );
    if ( _fabs(result.fDist) < epsilon )
    {
      break;
    }
                        
        if ( t > maxDist )
        {
            result.iObjectId = -1;
          t = maxDist;
            break;
        }       
        
        if ( result.fDist > 1.0f )
        {
            result.iObjectId = -1;            
        }    
        
        t += result.fDist;        
  }
    
    result.fDist = t;

    return result;
}    

__DEVICE__ float Scene_TraceShadow( const in float3 vRayOrigin, const in float3 vRayDir, const in float fMinDist, const in float fLightDist )
{
    //return 1.0f;
    //return ( Scene_Trace( vRayOrigin, vRayDir, 0.1f, fLightDist ).fDist < fLightDist ? 0.0f : 1.0f;
    
    float res = 1.0f;
    float t = fMinDist;
    for( int i=0; i<16; i++ )
    {
      float h = Scene_GetDistance( vRayOrigin + vRayDir * t ).fDist;
      res = _fminf( res, 8.0f*h/t );
      t += clamp( h, 0.02f, 0.10f );
      if( h<0.0001f || t>fLightDist ) break;
    }
    return clamp( res, 0.0f, 1.0f );    
}

__DEVICE__ float Scene_GetAmbientOcclusion( const in float3 vPos, const in float3 vDir )
{
    float fOcclusion = 0.0f;
    float fScale = 1.0f;
    for( int i=0; i<5; i++ )
    {
        float fOffsetDist = 0.001f + 0.1f*float(i)/4.0f;
        float3 vAOPos = vDir * fOffsetDist + vPos;
        float fDist = Scene_GetDistance( vAOPos ).fDist;
        fOcclusion += (fOffsetDist - fDist) * fScale;
        fScale *= 0.4f;
    }
    
    return clamp( 1.0f - 30.0f*fOcclusion, 0.0f, 1.0f );
}

///////////////////////////
// Lighting
///////////////////////////
    
struct SurfaceInfo
{
    float3 vPos;
    float3 vNormal;
    float3 vBumpNormal;    
    float3 vAlbedo;
    float3 vR0;
    float fSmoothness;
    float3 vEmissive;
};
    
__DEVICE__ SurfaceInfo Scene_GetSurfaceInfo( const in float3 vRayOrigin,  const in float3 vRayDir, SceneResult traceResult, __TEXTURE2D__ iChannel0, float iTime );

struct SurfaceLighting
{
    float3 vDiffuse;
    float3 vSpecular;
};
    
__DEVICE__ SurfaceLighting Scene_GetSurfaceLighting( const in float3 vRayDir, in SurfaceInfo surfaceInfo );

__DEVICE__ float Light_GIV( float dotNV, float k)
{
  return 1.0f / ((dotNV + 0.0001f) * (1.0f - k)+k);
}

__DEVICE__ void Light_Add(inout SurfaceLighting *lighting, SurfaceInfo surface, const in float3 vViewDir, const in float3 vLightDir, const in float3 vLightColour)
{
  float fNDotL = clamp(dot(vLightDir, surface.vBumpNormal), 0.0f, 1.0f);
  
  (*lighting).vDiffuse += vLightColour * fNDotL;
    
  float3 vH = normalize( -1.0f*vViewDir + vLightDir );
  float fNdotV = clamp(dot(-1.0f*vViewDir, surface.vBumpNormal), 0.0f, 1.0f);
  float fNdotH = clamp(dot(surface.vBumpNormal, vH), 0.0f, 1.0f);
    
  float alpha = 1.0f - surface.fSmoothness;
  // D

  float alphaSqr = alpha * alpha;
  float denom = fNdotH * fNdotH * (alphaSqr - 1.0f) + 1.0f;
  float d = alphaSqr / (PI * denom * denom);

  float k = alpha / 2.0f;
  float vis = Light_GIV(fNDotL, k) * Light_GIV(fNdotV, k);

  float fSpecularIntensity = d * vis * fNDotL;    
  (*lighting).vSpecular += vLightColour * fSpecularIntensity;    
}

__DEVICE__ void Light_AddPoint(inout SurfaceLighting *lighting, SurfaceInfo surface, const in float3 vViewDir, const in float3 vLightPos, const in float3 vLightColour)
{    
  float3 vPos = surface.vPos;
  float3 vToLight = vLightPos - vPos;  
    
  float3 vLightDir = normalize(vToLight);
  float fDistance2 = dot(vToLight, vToLight);
  float fAttenuation = 100.0f / (fDistance2);
  
  float fShadowFactor = Scene_TraceShadow( surface.vPos, vLightDir, 0.1f, length(vToLight) );
  
  Light_Add( lighting, surface, vViewDir, vLightDir, vLightColour * fShadowFactor * fAttenuation);
}

__DEVICE__ void Light_AddDirectional(inout SurfaceLighting *lighting, SurfaceInfo surface, const in float3 vViewDir, const in float3 vLightDir, const in float3 vLightColour)
{  
  float fAttenuation = 1.0f;
  float fShadowFactor = Scene_TraceShadow( surface.vPos, vLightDir, 0.1f, 10.0f );
  
  Light_Add( lighting, surface, vViewDir, vLightDir, vLightColour * fShadowFactor * fAttenuation);
}

__DEVICE__ float3 Light_GetFresnel( float3 vView, float3 vNormal, float3 vR0, float fGloss )
{
    float NdotV = _fmaxf( 0.0f, dot( vView, vNormal ) );

    return vR0 + (to_float3_s(1.0f) - vR0) * _powf( 1.0f - NdotV, 5.0f ) * _powf( fGloss, 20.0f );
}

__DEVICE__ void Env_AddPointLightFlare(inout float3 vEmissiveGlow, const in float3 vRayOrigin, const in float3 vRayDir, const in float fIntersectDistance, const in float3 vLightPos, const in float3 vLightColour)
{
  float3 vToLight = vLightPos - vRayOrigin;
  float fPointDot = dot(vToLight, vRayDir);
  fPointDot = clamp(fPointDot, 0.0f, fIntersectDistance);

  float3 vClosestPoint = vRayOrigin + vRayDir * fPointDot;
  float fDist = length(vClosestPoint - vLightPos);
  vEmissiveGlow += sqrt_f3(vLightColour * 0.05f / (fDist * fDist));
}

__DEVICE__ void Env_AddDirectionalLightFlareToFog(inout float3 *vFogColour, const in float3 vRayDir, const in float3 vLightDir, const in float3 vLightColour)
{
  float fDirDot = clamp(dot(vLightDir, vRayDir) * 0.5f + 0.5f, 0.0f, 1.0f);
  float kSpreadPower = 2.0f;
  *vFogColour += vLightColour * _powf(fDirDot, kSpreadPower) * 0.25f;
}


///////////////////////////
// Rendering
///////////////////////////

__DEVICE__ float4 Env_GetSkyColor( const float3 vViewPos, const float3 vViewDir, __TEXTURE2D__ iChannel1 );
__DEVICE__ float3 Env_ApplyAtmosphere( const in float3 vColor, const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist );
__DEVICE__ float3 FX_Apply( in float3 vColor, const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist);

__DEVICE__ float4 Scene_GetColorAndDepth( float3 vRayOrigin, float3 vRayDir, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, float iTime )
{
  float3 vResultColor = to_float3_s(0.0f);
            
  SceneResult firstTraceResult;
    
  float fStartDist = 0.0f;
  float fMaxDist = 10.0f;
  
  float3 vRemaining = to_float3_s(1.0f);
    
  for( int iPassIndex=0; iPassIndex < 3; iPassIndex++ )
  {
    SceneResult traceResult = Scene_Trace( vRayOrigin, vRayDir, fStartDist, fMaxDist );

    if ( iPassIndex == 0 )
    {
        firstTraceResult = traceResult;
    }
    
    float3 vColor = to_float3_s(0);
    float3 vReflectAmount = to_float3_s(0);
      
    if( traceResult.iObjectId < 0 )
    {
      vColor = swi3(Env_GetSkyColor( vRayOrigin, vRayDir, iChannel1 ),x,y,z);//.rgb;
    }
    else
    {
            
      SurfaceInfo surfaceInfo = Scene_GetSurfaceInfo( vRayOrigin, vRayDir, traceResult, iChannel0, iTime );
      SurfaceLighting surfaceLighting = Scene_GetSurfaceLighting( vRayDir, surfaceInfo );
          
      // calculate reflectance (Fresnel)
      vReflectAmount = Light_GetFresnel( -vRayDir, surfaceInfo.vBumpNormal, surfaceInfo.vR0, surfaceInfo.fSmoothness );
      
      vColor = (surfaceInfo.vAlbedo * surfaceLighting.vDiffuse + surfaceInfo.vEmissive) * (to_float3_s(1.0f) - vReflectAmount); 
            
      float3 vReflectRayOrigin = surfaceInfo.vPos;
      float3 vReflectRayDir = normalize( reflect( vRayDir, surfaceInfo.vBumpNormal ) );
      fStartDist = 0.001f / _fmaxf(0.0000001f,_fabs(dot( vReflectRayDir, surfaceInfo.vNormal ))); 

      vColor += surfaceLighting.vSpecular * vReflectAmount;            

      vColor = Env_ApplyAtmosphere( vColor, vRayOrigin, vRayDir, traceResult.fDist );
      vColor = FX_Apply( vColor, vRayOrigin, vRayDir, traceResult.fDist );
            
      vRayOrigin = vReflectRayOrigin;
      vRayDir = vReflectRayDir;
    }
        
    vResultColor += vColor * vRemaining;
    vRemaining *= vReflectAmount;        
  }
 
  return to_float4_aw( vResultColor, firstTraceResult.fDist );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////
// Scene Description
/////////////////////////

// Materials

#define MAT_SKY       -1
#define MAT_DEFAULT    0
#define MAT_SCREEN     1
#define MAT_TV_CASING    2
#define MAT_TV_TRIM      3
#define MAT_CHROME       4


__DEVICE__ float3 PulseIntegral( float3 x, float s1, float s2 )
{
  // Integral of function where result is 1.0f between s1 and s2 and 0 otherwise        

  // V1
  //if ( x > s2 ) return s2 - s1;
  //else if ( x > s1 ) return x - s1;
  //return 0.0f; 
    
  // V2
  //return clamp( (x - s1), 0.0f, s2 - s1);
  //return t;
    
  return clamp( (x - s1), to_float3_s(0.0f), to_float3_s(s2 - s1));
}

__DEVICE__ float PulseIntegral( float x, float s1, float s2 )
{
  // Integral of function where result is 1.0f between s1 and s2 and 0 otherwise        

  // V1
  //if ( x > s2 ) return s2 - s1;
  //else if ( x > s1 ) return x - s1;
  //return 0.0f; 
    
  // V2
  //return clamp( (x - s1), 0.0f, s2 - s1);
  //return t;
    
  return clamp( (x - s1), (0.0f), (s2 - s1));
}

__DEVICE__ float3 Bayer( float2 vUV, float2 vBlur )
{
    float3 x = to_float3_s(vUV.x);
    float3 y = to_float3_s(vUV.y);           

    x += to_float3(0.66f, 0.33f, 0.0f);
    y += 0.5f * step( fract_f3( x * 0.5f ), to_float3_s(0.5f) );
        
    //x -= 0.5f;
    //y -= 0.5f;
    
    x = fract_f3( x );
    y = fract_f3( y );
    
    // cell centered at 0.5
    
    float2 vSize = to_float2(0.16f, 0.75f);
    
    float2 vMin = 0.5f - vSize * 0.5f;
    float2 vMax = 0.5f + vSize * 0.5f;
    
    float3 vResult= to_float3_s(0.0f);
    
    float3 vResultX = (PulseIntegral( x + vBlur.x, vMin.x, vMax.x) - PulseIntegral( x - vBlur.x, vMin.x, vMax.x)) / _fminf( vBlur.x, 1.0f);
    float3 vResultY = (PulseIntegral(y + vBlur.y, vMin.y, vMax.y) - PulseIntegral(y - vBlur.y, vMin.y, vMax.y))  / _fminf( vBlur.y, 1.0f);
    
    vResult = _fminf(vResultX,vResultY)  * 5.0f;
        
    //vResult = to_float3_s(1.0f);

    return vResult;
}

__DEVICE__ float3 GetPixelMatrix( float2 vUV )
{
#if 0
    float2 dx = dFdx( vUV );
    float2 dy = dFdy( vUV );
    float dU = length( to_float2( dx.x, dy.x ) );
    float dV = length( to_float2( dx.y, dy.y ) );
    if (dU <= 0.0f || dV <= 0.0f ) return to_float3_s(1.0f);
    return Bayer( vUV, to_float2(dU, dV) * 1.0f);
#else
    return to_float3_s(1.0f);
#endif
}

__DEVICE__ float Scanline( float y, float fBlur )
{   
    float fResult = _sinf( y * 10.0f ) * 0.45f + 0.55f;
    return _mix( fResult, 1.0f, _fminf( 1.0f, fBlur ) );
}


__DEVICE__ float GetScanline( float2 vUV )
{
#if 0
    vUV.y *= 0.25f;
    float2 dx = dFdx( vUV );
    float2 dy = dFdy( vUV );
    float dV = length( to_float2( dx.y, dy.y ) );
    if (dV <= 0.0f ) return 1.0f;
    return Scanline( vUV.y, dV * 1.3f );
#else
    return 1.0f;
#endif
}


__DEVICE__ float2 kScreenRsolution = {480.0f, 576.0f};

struct Interference
{
    float noise;
    float scanLineRandom;
};

__DEVICE__ float InterferenceHash(float p)
{
    float hashScale = 0.1031f;

    float3 p3  = fract_f3(to_float3(p, p, p) * hashScale);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}


__DEVICE__ float InterferenceSmoothNoise1D( float x )
{
    float f0 = _floor(x);
    float fr = fract(x);

    float h0 = InterferenceHash( f0 );
    float h1 = InterferenceHash( f0 + 1.0f );

    return h1 * fr + h0 * (1.0f - fr);
}


__DEVICE__ float InterferenceNoise( float2 uv, float iTime )
{
  float displayVerticalLines = 483.0f;
  float scanLine = _floor(uv.y * displayVerticalLines); 
  float scanPos = scanLine + uv.x;
  float timeSeed = fract( iTime * 123.78f );
    
  return InterferenceSmoothNoise1D( scanPos * 234.5f + timeSeed * 12345.6f );
}
    
__DEVICE__ Interference GetInterference( float2 vUV, float iTime )
{
    Interference interference;
        
    interference.noise = InterferenceNoise( vUV, iTime );
    interference.scanLineRandom = InterferenceHash(vUV.y * 100.0f + fract(iTime * 1234.0f) * 12345.0f);
    
    return interference;
}
    
__DEVICE__ float3 SampleScreen( float3 vUVW, __TEXTURE2D__ iChannel0, float iTime )
{   
    float3 vAmbientEmissive = to_float3_s(0.1f);
    float3 vBlackEmissive = to_float3_s(0.02f);
    float fBrightness = 1.75f;
    float2 vResolution = to_float2(480.0f, 576.0f);
    float2 vPixelCoord = swi2(vUVW,x,y) * vResolution;
    
    float3 vPixelMatrix = GetPixelMatrix( vPixelCoord );
    float fScanline = GetScanline( vPixelCoord );
      
    float2 vTextureUV = swi2(vUVW,x,y);
    //vec2 vTextureUV = vPixelCoord;
    vTextureUV = _floor(vTextureUV * vResolution * 2.0f) / (vResolution * 2.0f);
 
     
    Interference interference = GetInterference( vTextureUV, iTime );

    float noiseIntensity = 0.1f;
    
    //vTextureUV.x += (interference.scanLineRandom * 2.0f - 1.0f) * 0.025f * noiseIntensity;
           
    float2 vSampleUV = vTextureUV;
    #if FLIP_V
    vSampleUV.y = 1.0f - vSampleUV.y;
    #endif
    
    float3 vPixelEmissive = swi3(texture( iChannel0, swi2(vSampleUV,x,y)),x,y,z);//.rgb;
        
    vPixelEmissive = vPixelEmissive * vPixelEmissive;
        
    vPixelEmissive = clamp( vPixelEmissive + (interference.noise - 0.5f) * 2.0f * noiseIntensity, 0.0f, 1.0f );
    
    float3 vResult = (vPixelEmissive * fBrightness + vBlackEmissive) * vPixelMatrix * fScanline + vAmbientEmissive;
    
    // TODO: feather edge?
    //if( any( greaterThanEqual( swi2(vUVW,x,y), to_float2_s(1.0f) ) ) || any ( lessThan( swi2(vUVW,x,y), to_float2_s(0.0f) ) ) || ( vUVW.z > 0.0f ) )
    
    if((vUVW.x >= 1.0f || vUVW.y >= 1.0f)  ||  (vUVW.x < 0.0f || vUVW.y < 0.0f) || ( vUVW.z > 0.0f ) )
    {
      return to_float3_s(0.0f);
    }
    
    return vResult;
    
}

__DEVICE__ float Checker(float2 vUV)
{
  return step(fract((_floor(vUV.x) + _floor(vUV.y)) * 0.5f), 0.25f);
}

__DEVICE__ SurfaceInfo Scene_GetSurfaceInfo( const in float3 vRayOrigin,  const in float3 vRayDir, SceneResult traceResult, __TEXTURE2D__ iChannel0, float iTime )
{
    SurfaceInfo surfaceInfo;
    
    surfaceInfo.vPos = vRayOrigin + vRayDir * (traceResult.fDist);
    
    surfaceInfo.vNormal = Scene_GetNormal( surfaceInfo.vPos ); 
    surfaceInfo.vBumpNormal = surfaceInfo.vNormal;
    surfaceInfo.vAlbedo = to_float3_s(1.0f);
    surfaceInfo.vR0 = to_float3_s( 0.02f );
    surfaceInfo.fSmoothness = 1.0f;
    surfaceInfo.vEmissive = to_float3_s( 0.0f );
    //return surfaceInfo;
        
    if ( traceResult.iObjectId == MAT_DEFAULT )
    {
      surfaceInfo.vR0 = to_float3_s( 0.02f );
      float checker = Checker(swi2(traceResult.vUVW,x,z) * 4.0f);
      surfaceInfo.vAlbedo = _mix( to_float3(0.9f,0.2f,0.2f),to_float3(0.2f,0.2f,0.9f), checker );                        
      surfaceInfo.fSmoothness = clamp( 1.0f - surfaceInfo.vAlbedo.x * surfaceInfo.vAlbedo.x * 2.0f, 0.0f, 1.0f);
        
    }
    
    if ( traceResult.iObjectId == MAT_SCREEN )
    {
      surfaceInfo.vAlbedo = to_float3_s(0.02f); 
      surfaceInfo.vEmissive = SampleScreen( traceResult.vUVW, iChannel0, iTime );        
    }

    if ( traceResult.iObjectId == MAT_TV_CASING )
    {
      surfaceInfo.vAlbedo = to_float3(0.5f, 0.4f, 0.3f); 
      surfaceInfo.fSmoothness = 0.4f;        
    }
    
    if ( traceResult.iObjectId == MAT_TV_TRIM )
    {
      surfaceInfo.vAlbedo = to_float3(0.03f, 0.03f, 0.05f); 
      surfaceInfo.fSmoothness = 0.5f;
    }    

    if ( traceResult.iObjectId == MAT_CHROME )
    {
      surfaceInfo.vAlbedo = to_float3(0.01f, 0.01f, 0.01f); 
      surfaceInfo.fSmoothness = 0.9f;
      surfaceInfo.vR0 = to_float3_s( 0.8f );
    }    
 
    return surfaceInfo;
}

// Scene Description

__DEVICE__ float SmoothMin( float a, float b, float k )
{
  //return _fminf(a,b);
  
  
  //float k = 0.06f;
  float h = clamp( 0.5f + 0.5f*(b-a)/k, 0.0f, 1.0f );
  return _mix( b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ float UdRoundBox( float3 p, float3 b, float r )
{
    //vec3 vToFace = _fabs(p) - b;
    //vec3 vConstrained = _fmaxf( vToFace, 0.0f );
    //return length( vConstrained ) - r;
    return length(_fmaxf(abs_f3(p)-b, to_float3_s(0.0f)))-r;
}

__DEVICE__ SceneResult Scene_GetCRT( float3 vScreenDomain, float2 vScreenWH, float fScreenCurveRadius, float fBevel, float fDepth )
{
    SceneResult resultScreen;
#if 1
    float3 vScreenClosest;
    swi2S(vScreenClosest,x,y, _fmaxf(abs_f2(swi2(vScreenDomain,x,y))-vScreenWH, to_float2_s(0.0f)));
    float2 vCurveScreenDomain = swi2(vScreenDomain,x,y);
    vCurveScreenDomain = clamp( vCurveScreenDomain, -vScreenWH, vScreenWH );
    float fCurveScreenProjection2 = fScreenCurveRadius * fScreenCurveRadius - vCurveScreenDomain.x * vCurveScreenDomain.x - vCurveScreenDomain.y * vCurveScreenDomain.y;
    float fCurveScreenProjection = _sqrtf( fCurveScreenProjection2 ) - fScreenCurveRadius;
    vScreenClosest.z = vScreenDomain.z - clamp( vScreenDomain.z, -fCurveScreenProjection, fDepth );
    resultScreen.vUVW.z = vScreenDomain.z + fCurveScreenProjection;        
    resultScreen.fDist = (length( vScreenClosest ) - fBevel) * 0.95f;
    //resultScreen.fDist = (length( vScreenDomain - to_float3(0,0,fScreenCurveRadius)) - fScreenCurveRadius - fBevel);    
#endif    
    
#if 0
    float3 vScreenClosest;
    swi3S(vScreenClosest,x,y,z, _fmaxf(_fabs(swi3(vScreenDomain,x,y,z))-to_float3_aw(vScreenWH, fDepth),0.0f));
    float fRoundDist = length( swi3(vScreenClosest,x,y,z) ) - fBevel;
    float fSphereDist = length( vScreenDomain - to_float3(0,0,fScreenCurveRadius) ) - (fScreenCurveRadius + fBevel);    
    resultScreen.fDist = _fmaxf(fRoundDist, fSphereDist);
#endif    
    
    swi2S(resultScreen.vUVW,x,y, (swi2(vScreenDomain,x,y) / vScreenWH) * 0.5f + 0.5f);
    resultScreen.iObjectId = MAT_SCREEN;
    return resultScreen;
}

__DEVICE__ SceneResult Scene_GetComputer( float3 vPos )
{
    SceneResult resultComputer;
    resultComputer.vUVW = swi3(vPos,x,z,y);
  
    float fXSectionStart = -0.2f;
    float fXSectionLength = 0.15f;
    float fXSectionT = clamp( (vPos.z - fXSectionStart) / fXSectionLength, 0.0f, 1.0f);
    float fXSectionR1 = 0.03f;
    float fXSectionR2 = 0.05f;
    float fXSectionR = _mix( fXSectionR1, fXSectionR2, fXSectionT );
    float fXSectionZ = fXSectionStart + fXSectionT * fXSectionLength;
    
    float2 vXSectionCentre = to_float2(fXSectionR, fXSectionZ );
    float2 vToPos = swi2(vPos,y,z) - vXSectionCentre;
    float l = length( vToPos );
    if ( l > fXSectionR ) l = fXSectionR;
    float2 vXSectionClosest = vXSectionCentre + normalize(vToPos) * l;
    //float fXSectionDist = length( vXSectionClosest ) - fXSectionR;
    
    float x = _fmaxf( _fabs( vPos.x ) - 0.2f, 0.0f );

    //resultComputer.fDist = length( to_float3(x, vXSectionClosest - swi2(vPos,y,z)) )-0.01f;
    resultComputer.fDist = length( to_float3(x, vXSectionClosest.x-vPos.y, vXSectionClosest.x-vPos.z) )-0.01f;
    //resultComputer.fDist = x;

    resultComputer.iObjectId = MAT_TV_CASING;
//
    float3 vKeyPos = swi3(vPos,x,y,z) - to_float3(0,0.125f,0);
    vKeyPos.y -= vKeyPos.z * (fXSectionR2 - fXSectionR1) * 2.0f / fXSectionLength;
    float fDomainRepeatScale = 0.02f;
    if ( fract(vKeyPos.z * 0.5f / fDomainRepeatScale + 0.25f) > 0.5f) vKeyPos.x += fDomainRepeatScale * 0.5f;
    float2 vKeyIndex = round(swi2(vKeyPos,x,z) / fDomainRepeatScale);
    vKeyIndex.x = clamp( vKeyIndex.x, -8.0f, 8.0f );
    vKeyIndex.y = clamp( vKeyIndex.y, -10.0f, -5.0f );
    //swi2(vKeyPos,x,z) = (fract( swi2(vKeyPos,x,z) / fDomainRepeatScale ) - 0.5f) * fDomainRepeatScale;
    swi2S(vKeyPos,x,z, (swi2(vKeyPos,x,z) - (vKeyIndex) * fDomainRepeatScale));
    swi2S(vKeyPos,x,z, swi2(vKeyPos,x,z) / 0.7f + vKeyPos.y);
    SceneResult resultKey;    
    resultKey.vUVW = swi3(vPos,x,z,y);
    resultKey.fDist = UdRoundBox( vKeyPos, to_float3_s(0.01f), 0.001f );
    resultKey.iObjectId = MAT_TV_TRIM;
    Scene_Union( &resultComputer, resultKey );
//    
    return resultComputer;
}

__DEVICE__ SceneResult Scene_GetDistance( float3 vPos )
{
    SceneResult result;
    
    //result.fDist = vPos.y;
    float fBenchBevel = 0.01f;
    result.fDist = UdRoundBox( vPos - to_float3(0,-0.02f-fBenchBevel,0.0f), to_float3(2.0f, 0.02f, 1.0f), fBenchBevel );
    result.vUVW = vPos;
    result.iObjectId = MAT_DEFAULT;        
    
    float3 vSetPos = to_float3(0.0f, 0.0f, 0.0f);
    float3 vScreenPos = vSetPos + to_float3(0.0f, 0.25f, 0.00f);
    
    //vPos.x = fract( vPos.x - 0.5f) - 0.5f;
    
    float2 vScreenWH = to_float2(4.0f, 3.0f) / 25.0f;

    SceneResult resultSet;
    resultSet.vUVW = swi3(vPos,x,z,y);
    resultSet.fDist = UdRoundBox( vPos - vScreenPos - to_float3(0.0f,-0.01f,0.2f), to_float3(0.21f, 0.175f, 0.18f), 0.01f );
    resultSet.iObjectId = MAT_TV_CASING;
    Scene_Union( &result, resultSet );

    SceneResult resultSetRecess;
    resultSetRecess.vUVW = swi3(vPos,x,z,y);
    resultSetRecess.fDist = UdRoundBox( vPos - vScreenPos - to_float3(0.0f,-0.0f, -0.05f), to_float3_aw(vScreenWH + 0.01f, 0.05f) + 0.005f, 0.015f );
    resultSetRecess.iObjectId = MAT_TV_TRIM;
    Scene_Subtract( &result, resultSetRecess );
    
    SceneResult resultSetBase;
    resultSetBase.vUVW = swi3(vPos,x,z,y);
    float fBaseBevel = 0.03f;
    resultSetBase.fDist = UdRoundBox( vPos - vSetPos - to_float3(0.0f,0.04f,0.22f), to_float3(0.2f, 0.04f, 0.17f) - fBaseBevel, fBaseBevel );
    resultSetBase.iObjectId = MAT_TV_CASING;
    Scene_Union( &result, resultSetBase );

    SceneResult resultScreen = Scene_GetCRT( vPos - vScreenPos, vScreenWH, 0.75f, 0.02f, 0.1f );
    Scene_Union( &result, resultScreen );    
    
      //SceneResult resultComputer = Scene_GetComputer( vPos - to_float3(0.0f, 0.0f, -0.1f) );
      //Scene_Union( &result, resultComputer );

    SceneResult resultSphere;
    resultSet.vUVW = swi3(vPos,x,z,y);
    resultSet.fDist = length(vPos - to_float3(0.35f,0.075f,-0.1f)) - 0.075f;
    resultSet.iObjectId = MAT_CHROME;
    Scene_Union( &result, resultSet );    
    
    return result;
}



// Scene Lighting

//__DEVICE__ float3 g_vSunDir = normalize(to_float3(0.3f, 0.4f, -0.5f));
__DEVICE__ float3 g_vSunColor = {1.0f*3.0f, 0.95f *3.0f, 0.8f * 3.0f};//to_float3(1, 0.95f, 0.8f) * 3.0f;
__DEVICE__ float3 g_vAmbientColor = {0.8f, 0.8f, 0.8f}; // * 1.0f;

__DEVICE__ SurfaceLighting Scene_GetSurfaceLighting( const in float3 vViewDir, in SurfaceInfo surfaceInfo )
{
    float3 g_vSunDir = normalize(to_float3(0.3f, 0.4f, -0.5f));
  
    SurfaceLighting surfaceLighting;
    
    surfaceLighting.vDiffuse = to_float3_s(0.0f);
    surfaceLighting.vSpecular = to_float3_s(0.0f);    
    
    Light_AddDirectional( &surfaceLighting, surfaceInfo, vViewDir, g_vSunDir, g_vSunColor );
    
    Light_AddPoint( &surfaceLighting, surfaceInfo, vViewDir, to_float3(1.4f, 2.0f, 0.8f), to_float3(1,1,1) * 0.2f );
    
    float fAO = Scene_GetAmbientOcclusion( surfaceInfo.vPos, surfaceInfo.vNormal );
    // AO
    surfaceLighting.vDiffuse += fAO * (surfaceInfo.vBumpNormal.y * 0.5f + 0.5f) * g_vAmbientColor;
    
    return surfaceLighting;
}

// Environment

__DEVICE__ float4 Env_GetSkyColor( const float3 vViewPos, const float3 vViewDir, __TEXTURE2D__ iChannel1 )
{
  float4 vResult = to_float4( 0.0f, 0.0f, 0.0f, kFarDist );

#if 1
    float3 vEnvMap = swi3(decube_f3( iChannel1, swi3(vViewDir,z,y,x)),x,y,z);//.rgb;
    swi3S(vResult,x,y,z, vEnvMap);
#endif    
    
#if 0
    float3 vEnvMap = swi3(decube_f3( iChannel1, swi3(vViewDir,z,y,x)),x,y,z);//.rgb;
    vEnvMap = vEnvMap * vEnvMap;
    float kEnvmapExposure = 0.999f;
    swi3S(vResult,x,y,z, -1.0f * log2_f3(to_float3_s(1.0f) - vEnvMap * kEnvmapExposure));

#endif
    
//    swi3S(vResult,x,y,z, _mix( to_float3(0.3f,0.8f,0.9f),to_float3(0.3f,0.4f,0.9f), vViewDir.y ));
    
    // Sun
    float3 g_vSunDir = normalize(to_float3(0.3f, 0.4f, -0.5f));
    float NdotV = dot( g_vSunDir, vViewDir );
    swi3S(vResult,x,y,z, swi3(vResult,x,y,z) + smoothstep( _cosf(radians(0.7f)), _cosf(radians(0.5f)), NdotV ) * g_vSunColor * 100.0f);

    return vResult;  
}

__DEVICE__ float Env_GetFogFactor(const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist )
{    
  float kFogDensity = 0.00001f;
  return _expf(fDist * -kFogDensity);  
}

__DEVICE__ float3 Env_GetFogColor(const in float3 vDir)
{    
  return to_float3(0.2f, 0.5f, 0.6f) * 2.0f;    
}

__DEVICE__ float3 Env_ApplyAtmosphere( const in float3 vColor, const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist )
{
  //return vColor;
  float3 vResult = vColor;
    
    
  float fFogFactor = Env_GetFogFactor( vRayOrigin, vRayDir, fDist );
  float3 vFogColor = Env_GetFogColor( vRayDir ); 
  
  //float3 g_vSunDir = normalize(to_float3(0.3f, 0.4f, -0.5f));  
  //Env_AddDirectionalLightFlareToFog( &vFogColor, vRayDir, g_vSunDir, g_vSunColor * 3.0f);    
  
  vResult = _mix( vFogColor, vResult, fFogFactor );

  return vResult;      
}


__DEVICE__ float3 FX_Apply( in float3 vColor, const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist)
{    
  return vColor;
}


__DEVICE__ float4 MainCommon( float3 vRayOrigin, float3 vRayDir, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, float iTime )
{
  float4 vColorLinAndDepth = Scene_GetColorAndDepth( vRayOrigin, vRayDir, iChannel0, iChannel1, iTime );    
  swi3S(vColorLinAndDepth,x,y,z, _fmaxf( swi3(vColorLinAndDepth,x,y,z), to_float3_s(0.0f) ));
    
  float4 vFragColor = vColorLinAndDepth;
  
  float fExposure = 2.0f;
  
  swi3S(vFragColor,x,y,z, swi3(vFragColor,x,y,z) * fExposure);
  
  vFragColor.w = vColorLinAndDepth.w;
  
  return vFragColor;
}

///////////////////////////
// Camera
///////////////////////////

struct CameraState
{
    float3 vPos;
    float3 vTarget;
    float fFov;
    float2 vJitter;
    float fPlaneInFocus;
};

__DEVICE__ mat3 Cam_GetWorldToCameraRotMatrix( const CameraState cameraState )
{
        
  float3 vForward = normalize( cameraState.vTarget - cameraState.vPos );
  float3 vRight = normalize( cross(to_float3(0, 1, 0), vForward) );
  float3 vUp = normalize( cross(vForward, vRight) );
    
  return to_mat3_f3( vRight, vUp, vForward );
}

__DEVICE__ float2 Cam_GetViewCoordFromUV( float2 vUV, float2 res )
{
  float2 vWindow = vUV * 2.0f - 1.0f;
  vWindow.x *= res.x / res.y;

  return vWindow;  
}

__DEVICE__ void Cam_GetCameraRay( float2 vUV, float2 res, CameraState cam, out float3 *vRayOrigin, out float3 *vRayDir )
{
    float2 vView = Cam_GetViewCoordFromUV( vUV, res );
    *vRayOrigin = cam.vPos;
    float fPerspDist = 1.0f / _tanf( radians( cam.fFov ) );
    *vRayDir = normalize( mul_mat3_f3(Cam_GetWorldToCameraRotMatrix( cam ) , to_float3_aw( vView, fPerspDist ) ));
}

__DEVICE__ float2 Cam_GetUVFromWindowCoord( float2 vWindow, float2 res )
{
    float2 vScaledWindow = vWindow;
    vScaledWindow.x *= res.y / res.x;

    return (vScaledWindow * 0.5f + 0.5f);
}

__DEVICE__ float2 Cam_WorldToWindowCoord(const in float3 vWorldPos, const in CameraState cameraState )
{
    float3 vOffset = vWorldPos - cameraState.vPos;
    float3 vCameraLocal;

    vCameraLocal = mul_f3_mat3(vOffset , Cam_GetWorldToCameraRotMatrix( cameraState ));
  
    float2 vWindowPos = swi2(vCameraLocal,x,y) / (vCameraLocal.z * _tanf( radians( cameraState.fFov ) ));
    
    return vWindowPos;
}


__DEVICE__ CameraState GetCameraPosition( int index, int FIXED_CAMERA_INDEX, float3 vFocus )
{
    CameraState cam = { to_float3_s(0.0f), to_float3_s(0.0f), 0.0f, to_float2_s(0.0f), 0.0f };

    //float3 vFocus = to_float3(0,0.25f,-0.012f);   
    
    if ( index > 9 )
    {
      index = (int)(hash11((float)(index) / 10.234f) * 100.0f);
      index = index % 10;
    }

    //#ifdef FIXED_CAMERA_INDEX
    if (FIXED_CAMERA_INDEX)
      index = FIXED_CAMERA_INDEX; // Force a camera position
    //#endif
    
    if ( index == 0 )
    {
        cam.vPos = to_float3(-0.1f,0.24f,-0.08f);
        cam.vTarget = to_float3(0.15f,0.25f,0.1f);
        cam.fFov = 10.0f;
    }
    if ( index == 1 )
    {
        cam.vPos = to_float3(0.01f,0.25f,-0.8f);
        cam.vTarget = to_float3(0,0.25f,0.1f);
        cam.fFov = 10.0f;
    }
    if ( index == 2 )
    {
        cam.vPos = to_float3(-0.4f,0.3f,-1.0f);
        cam.vTarget = to_float3(0.25f,0.18f,0.5f);
        cam.fFov = 10.0f;
    }
    if ( index == 3 )
    {
        cam.vPos = to_float3(-0.8f,0.5f,-1.5f);
        cam.vTarget = to_float3(0.2f,0.1f,0.5f);
        cam.fFov = 8.0f;
    }
    if ( index == 4 )
    {
        cam.vPos = to_float3(0.5f,0.3f,-0.5f);
        cam.vTarget = to_float3(-0.4f,0.1f,0.5f);
        cam.fFov = 16.0f;
    }
    if ( index == 5 )
    {
        cam.vPos = to_float3(-0.244f,0.334f,-0.0928f);
        cam.vTarget = to_float3(0,0.25f,0.1f);
        cam.fFov = 20.0f;
    }
    if ( index == 6 )
    {
        cam.vPos = to_float3(0.0f,0.1f,-0.5f);
        cam.vTarget = to_float3(0.08f,0.2f,-0.1f);
        vFocus = cam.vTarget; 
        cam.fFov = 20.0f;
    }
    if ( index == 7 )
    {
        cam.vPos = to_float3(-0.01f,0.01f,-0.25f);
        cam.vTarget = to_float3(0.01f,0.27f,0.1f);
        vFocus = cam.vTarget; 
        cam.fFov = 23.0f;
    }
    if ( index == 8 )
    {
        cam.vPos = to_float3(-0.23f,0.3f,-0.05f);
        cam.vTarget = to_float3(0.1f,0.2f,0.1f);
        cam.fFov = 15.0f;
    }
    if ( index == 9 )
    {
        cam.vPos = to_float3(0.4f,0.2f,-0.2f);
        cam.vTarget = to_float3(-0.1f,0.25f,0.1f);
        cam.fFov = 12.0f;
    }
    
    cam.fPlaneInFocus = length( vFocus - cam.vPos);
    cam.vJitter = to_float2_s(0.0f);        
    
    return cam;
}


__DEVICE__ float3 Tonemap( float3 x, bool BT709 )
{
  if ( BT709 )
  {
    float3 luminanceCoeffsBT709 = to_float3( 0.2126f, 0.7152f, 0.0722f );
    float f = dot( x, luminanceCoeffsBT709 );
    x /= f;        
    f = 1.0f - _expf(-f);    
    x *= f;    
    x = _mix( x, to_float3_s(f), f*f );
    
    return x;
  }
  else
  {
    float a = 0.010f;
    float b = 0.132f;
    float c = 0.010f;
    float d = 0.163f;
    float e = 0.101f;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );    
  }
}


__DEVICE__ float GetVignetting( const in float2 vUV, float fScale, float fPower, float fStrength )
{
  float2 vOffset = (vUV - 0.5f) * _sqrtf(2.0f) * fScale;
  float fDist = _fmaxf( 0.0f, 1.0f - length( vOffset ) );
  float fShade = 1.0f - _powf( fDist, fPower );
  fShade = 1.0f - fShade * fStrength;

  return fShade;
}


__KERNEL__ void MetaCrtSinglePassFuse(float4 vFragColor, float2 vFragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);

    CONNECT_CHECKBOX0(BT709, 0);
    CONNECT_CHECKBOX1(MouseRotation, 0);

    CONNECT_SLIDER0(ZoomMouse, -20.0f, 20.0f, 0.0f);
    
    CONNECT_INTSLIDER0(FIXED_CAMERA_INDEX, 0, 9, 0);
    
    CONNECT_POINT0(UV, 0.0f, 0.0f );
    CONNECT_SLIDER1(FocusZ, -10.0f, 10.0f, 1.0f);

    CONNECT_SLIDER2(fSequenceSegLength, 0.0f, 30.0f, 5.0f);
    
    CONNECT_SLIDER3(VignetteScale, 0.0f, 5.0f, 0.7f);
    CONNECT_SLIDER4(VignettePower, 0.0f, 10.0f, 2.0f);
    CONNECT_SLIDER5(VignetteStrength, -1.0f, 5.0f, 1.0f);
    
    //CONNECT_SLIDER6(UVX, -5.0f, 5.0f, 0.0f);
    //CONNECT_SLIDER7(UVY, -5.0f, 5.0f, 0.0f);


    float3 vFocus = to_float3(0,0.25f,-0.012f);// + to_float3_aw(FocusXY, FocusZ);   

    float2 vUV = vFragCoord / iResolution;///2.0f; 

    //vUV.x *= iResolution.x/iResolution.y;//1.777f;

    vUV -= UV;//to_float2(UVX, UVY);     // durch Verwendung von UV wird Bild verzerrt, warum auch immer
    
    vUV -= to_float2_s(0.5f);
    vUV *= FocusZ; 
    vUV += to_float2_s(0.5f);

    CameraState cam = { to_float3_s(0.0f), to_float3_s(0.0f), 0.0f, to_float2_s(0.0f), 0.0f };
    
    
      CameraState camA = { to_float3_s(0.0f), to_float3_s(0.0f), 0.0f, to_float2_s(0.0f), 0.0f };;
      CameraState camB = { to_float3_s(0.0f), to_float3_s(0.0f), 0.0f, to_float2_s(0.0f), 0.0f };;
    
      float fSeqTime = iTime;
      //float fSequenceSegLength = 5.0f;
      float fSeqIndex = _floor(fSeqTime / fSequenceSegLength);
      float fSeqPos = fract(fSeqTime / fSequenceSegLength);
      int iIndex = (int)(fSeqIndex);
      int iIndexNext = (int)(fSeqIndex) + 1;
      camA = GetCameraPosition(iIndex, FIXED_CAMERA_INDEX, vFocus);
      camB = GetCameraPosition(iIndexNext, FIXED_CAMERA_INDEX, vFocus);
      
      float t = smoothstep(0.3f, 1.0f, fSeqPos);
      cam.vPos = _mix(camA.vPos, camB.vPos, t );
      cam.vTarget = _mix(camA.vTarget, camB.vTarget, t );
      cam.fFov = _mix(camA.fFov, camB.fFov, t );
      cam.fPlaneInFocus = _mix(camA.fPlaneInFocus, camB.fPlaneInFocus, t );
    
    
#if USE_MOUSE    
    if ( iMouse.z > 0.0f )
    {
      //float fDist = 0.01f + 3.0f * (iMouse.y / iResolution.y);
      float fDist = 0.01f + 3.0f * ZoomMouse;

      float fAngle = (iMouse.x / iResolution.x) * radians(360.0f);
      float fElevation = (iMouse.y / iResolution.y) * radians(90.0f);
      //float fElevation = 0.15f * radians(90.0f);    

      cam.vPos = to_float3(_sinf(fAngle) * fDist * _cosf(fElevation), _sinf(fElevation) * fDist, _cosf(fAngle) * fDist * _cosf(fElevation));
      cam.vTarget = to_float3(0,0.25f,0.1f);
      cam.vPos += cam.vTarget;
      cam.fFov = 20.0f / (1.0f + fDist * 0.5f);
      //float3 vFocus = to_float3(0,0.25f,-0.012f);      
      cam.fPlaneInFocus = length( vFocus - cam.vPos );
    }
    else
    {
      if(MouseRotation)
      {       
        float fAngle = (iMouse.x / iResolution.x) * radians(360.0f);
        float fElevation = (iMouse.y / iResolution.y) * radians(90.0f);
        cam.vPos += to_float3(_sinf(fAngle) * _cosf(fElevation), _sinf(fElevation), _cosf(fAngle) * _cosf(fElevation));
      }
    }
    
#endif    
    
#if 0
    {
      float fDist = 0.5f;

      float fAngle = 0.6f * PI * 2.0f;
      float fElevation = 0.2f;
      
      cam.vPos = to_float3(_sinf(fAngle) * fDist * _cosf(fElevation),_sinf(fElevation) * fDist,_cosf(fAngle) * fDist * _cosf(fElevation));
      cam.vTarget = to_float3(0.05f,0.25f,0.1f);
      cam.vPos +=cam.vTarget;
      cam.fFov = 22.0f;
      //float3 vFocus = to_float3(0,0.25f,-0.012f);      
      cam.fPlaneInFocus = length( vFocus - cam.vPos );
    }
#endif

//#define ENABLE_TAA_JITTER    
#ifdef ENABLE_TAA_JITTER
    cam.vJitter = hash21( fract( iTime ) ) - 0.5f;
#endif
    
            
    float3 vRayOrigin, vRayDir;
    float2 vJitterUV = vUV + cam.vJitter / iResolution;
    Cam_GetCameraRay( vJitterUV, iResolution, cam, &vRayOrigin, &vRayDir );
 
    float fHitDist = 0.0f;
    vFragColor = MainCommon( vRayOrigin, vRayDir, iChannel0, iChannel1, iTime );
    
    //float fShade = GetVignetting( vUV, 0.7f, 2.0f, 1.0f );
    float fShade = GetVignetting( vUV, VignetteScale, VignettePower, VignetteStrength );
    
    swi3S(vFragColor,x,y,z, swi3(vFragColor,x,y,z) * fShade);
    
    swi3S(vFragColor,x,y,z, Tonemap( swi3(vFragColor,x,y,z), BT709 ));

    vFragColor += Color-0.5f;
    vFragColor.w = Color.w;

  SetFragmentShaderComputedColor(vFragColor);
}