
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Pebbles' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// 09.11.2021.
// Made by Darko Supe (omegasbk)

#define MAX_STEPS 100

#define MAX_DIST 100.0f
#define MIN_DIST 0.0002f

__DEVICE__ float sdfSphere(float3 c, float r, float3 p, float iTime, bool bumpMan, float bump, __TEXTURE2D__ iChannel0)
{
    if (bumpMan)
      return distance_f3(p, c) - r + texture(iChannel0, swi2(p,x,y)).x / bump;  
    else
      return distance_f3(p, c) - r + texture(iChannel0, swi2(p,x,y)).x / ((_sinf(iTime) + 1.0f) * 80.0f);
}

__DEVICE__ float getDist(float3 p, float iTime, bool bumpMan, float bump, float bumpR, __TEXTURE2D__ iChannel0)
{
    // Setup scene
    return sdfSphere(to_float3_s(0.0f), bumpR, p, iTime, bumpMan, bump, iChannel0);
}

__DEVICE__ float rayMarch(float3 ro, float3 rd, float iTime, bool bumpMan, float bump, float bumpR, __TEXTURE2D__ iChannel0)
{
    float dist = 0.0f;
    
    for (int i = 0; i < MAX_STEPS; i++)
    {
      float3 itPos = ro + rd * dist;
      float itDist = getDist(itPos,iTime,bumpMan,bump,bumpR,iChannel0);
      
      dist += itDist;
      
      if (dist > MAX_DIST || dist < MIN_DIST)  
          break;
    }    
    
    return dist;
}

__DEVICE__ float3 getNormal(float3 p, float iTime, bool bumpMan, float bump, float bumpR, __TEXTURE2D__ iChannel0)
{
    float2 e = to_float2(0.01f, 0.0f);    
    return normalize(to_float3(getDist(p + swi3(e,x,y,y),iTime,bumpMan,bump,bumpR,iChannel0), getDist(p + swi3(e,y,x,y),iTime,bumpMan,bump,bumpR,iChannel0), getDist(p + swi3(e,y,y,x),iTime,bumpMan,bump,bumpR,iChannel0)));    
}

__DEVICE__ float getLight(float3 p, float iTime, float3 lightPos, bool bumpMan, float bump, float bumpR, __TEXTURE2D__ iChannel0)
{
    if (iTime > 0.0f) lightPos = to_float3(_sinf(iTime * 3.0f), 3.0f, -2.2f);
    float3 lightDir = normalize(p - lightPos);
    
    return -dot(getNormal(p,iTime,bumpMan,bump,bumpR,iChannel0), lightDir);    
}

__KERNEL__ void CookbookSdfSphereJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    
    CONNECT_COLOR0(Color, 0.5f, 0.6f, 0.6f, 1.0f);
    CONNECT_CHECKBOX0(LightMan, 0);
    CONNECT_POINT0(LightXY, 3.0f, 3.0f);
    CONNECT_SLIDER0(LightZ, -100.0f, 100.0f, -2.2f);
    
    CONNECT_CHECKBOX1(bumpMan, 0);
    CONNECT_SLIDER1(Bump, -100.0f, 100.0f, 1.0f);
    CONNECT_SLIDER2(BumpR, -2.0f, 2.0f, 0.8f);
    
    float2 uv = fragCoord/iResolution - 0.5f;
    uv.x *= iResolution.x / iResolution.y;
    
    float focalDist = 0.6f;
    float3 ro = to_float3(0.0f, 0.0f, -1.6f);
    float3 rd = to_float3(uv.x, uv.y, focalDist);   
    
    float3 col = to_float3_s(0.0f);
    
    float dist = rayMarch(ro, rd, iTime,bumpMan,Bump,BumpR, iChannel0);
    if (dist < MAX_DIST)
    {
      float3 pHit = ro + rd * dist;
      col = swi3(Color,x,y,z);//to_float3(0.5f, 0.6f, 0.6f);
      
      if (LightMan) iTime = 0.0f;
      col *= to_float3_s(getLight(pHit,iTime, to_float3_aw(LightXY, LightZ),bumpMan,Bump,BumpR, iChannel0)) + to_float3_s(0.1f);
    }    

    // Output to screen
    fragColor = to_float4_aw(col,Color.w);

  SetFragmentShaderComputedColor(fragColor);
}