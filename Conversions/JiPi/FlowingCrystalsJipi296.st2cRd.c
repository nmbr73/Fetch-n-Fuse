
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ mat2 Rotate2D(float angle)
{
    return to_mat2(_cosf(angle), -_sinf(angle),
                _sinf(angle), _cosf(angle));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------


__DEVICE__ float2 N22(float2 p)
{
    float3 a = fract_f3(swi3(p,x,y,x) * to_float3(1278.67f, 3134.61f, 298.647f));
    a += dot(a, a + 318.978f);
    return fract(to_float2(a.x * a.y, a.y * a.z)) * 0.516846f;
}

__DEVICE__ float N21(float2 p)
{
    return fract(_sinf(dot(p, to_float2(12.9898f, 78.233f))) * 43758.5453f);
}

__DEVICE__ void VoronoiUV(float2 uvIn, float flatteningIn, float timeIn, out float2 *uvOut, out float *nuclearDistOut, out float *edgeDistOut, out float2 *cellID)
{
    *cellID = to_float2_s(0.0f);

    float2 gv = fract_f2(uvIn) - 0.5f;
    float2 id = _floor(uvIn);

    *nuclearDistOut = 100000.0f;
    *edgeDistOut = 100000.0f;

    for (float y = -1.0f; y <= 1.0f; y+=1.0f)
    {
        for(float x = -1.0f; x <= 1.0f; x+=1.0f)
        {
            float2 offset = to_float2(x, y);

            float2 cellindex = id + offset;
            float2 n = N22(cellindex);
            float2 p = offset + sin_f2(n * timeIn) * 0.5f;

            float2 diff = gv - p;
            float d = length(diff);

            if (d < *nuclearDistOut)
            {
                *nuclearDistOut = d;
                *cellID = cellindex;
                *uvOut = diff;
            }
        }
    }

    for (float y = -1.0f; y <= 1.0f; y+=1.0f)
    {
        for (float x = -1.0f; x <= 1.0f; x+=1.0f)
        {
            float2 offset = to_float2(x, y);

            float2 cellindex = id + offset;
            float2 n = N22(cellindex);
            float2 p = offset + sin_f2(n * timeIn) * 0.5f;

            float2 diff = gv - p;

            float2 toCenter = (*uvOut + diff) * 0.5f;
            float2 cellDifference = normalize(diff - *uvOut);
            float edgeDistance = dot(toCenter, cellDifference);
            *edgeDistOut = _fminf(*edgeDistOut, edgeDistance);
        }
    }
    
    *edgeDistOut = *edgeDistOut * flatteningIn;
    *edgeDistOut *= 2.0f;
    *edgeDistOut = _fminf(1.0f, *edgeDistOut);
}


__KERNEL__ void FlowingCrystalsJipi296Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    CONNECT_SLIDER0(density1, 0.0f, 20.0f, 3.0f);
    CONNECT_SLIDER1(density2, 0.0f, 20.0f, 3.0f);
    CONNECT_SLIDER2(density3, 0.0f, 20.0f, 3.0f);

    CONNECT_SLIDER3(flattening1, 0.0f, 30.0f, 15.0f);
    CONNECT_SLIDER4(flattening2, 0.0f, 30.0f, 8.0f);
    CONNECT_SLIDER5(flattening3, 0.0f, 30.0f, 3.0f);

    //float density1 = 3.0f;
    //float density2 = 3.0f;
    //float density3 = 3.0f;

    // 1.0f is typical Voronoi edge distance, increase to flatten faces
    //float flattening1 = 15.0f;
    //float flattening2 = 8.0f;
    //float flattening3 = 3.0f;




    float2 uv = fragCoord/iResolution;
    uv.x *= iResolution.x / iResolution.y;

    float2 uv1;
    float nuclearDist1;
    float edgeDist1;
    float2 cellID1;
    VoronoiUV(uv * density1, flattening1, iTime + 5.4864f, &uv1, &nuclearDist1, &edgeDist1, &cellID1);
    
    float rotSpeed1 = N21(cellID1) * 2.0f - 1.0f;
    uv1 = mul_mat2_f2(Rotate2D(rotSpeed1 * iTime * 0.5f) , uv1);
    
    float2 uv2;
    float nuclearDist2;
    float edgeDist2;
    float2 cellID2;
    VoronoiUV(uv1 * density2, flattening2, iTime + 12.4864f, &uv2, &nuclearDist2, &edgeDist2, &cellID2);
    
    float rotSpeed2 = N21(cellID2) * 2.0f - 1.0f;
    uv2 = mul_mat2_f2(Rotate2D(rotSpeed2 * iTime * 0.5f) , uv2);
    
    float2 uv3;
    float nuclearDist3;
    float edgeDist3;
    float2 cellID3;
    VoronoiUV(uv2 * density3, flattening3, iTime + 37.0846f, &uv3, &nuclearDist3, &edgeDist3, &cellID3);
    
    float3 color = to_float3_s(edgeDist1 * edgeDist2 * edgeDist3);
    
    fragColor = to_float4_aw(color, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float getHeight(float2 uv, __TEXTURE2D__ iChannel0) {
  return _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
}

// From dmmn's 'Height map to normal map' - https://www.shadertoy.com/view/MsScRt
__DEVICE__ float4 bumpFromDepth(float2 uv, float2 resolution, float scale, __TEXTURE2D__ iChannel0) {
  float2 step = 1.0f / resolution;
    
  float height = getHeight(uv,iChannel0);
    
  float2 dxy = height - to_float2(
      getHeight(uv + to_float2(step.x, 0.0f),iChannel0), 
      getHeight(uv + to_float2(0.0f, step.y),iChannel0)
  );
    
  return to_float4_aw(normalize(to_float3_aw(dxy * scale / step, 1.0f)), height);
}

__KERNEL__ void FlowingCrystalsJipi296Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_COLOR0(Light, 0.7f, 0.8f, 1.0f, 1.0f);
  CONNECT_SLIDER6(pseudoRefraction, -1.0f, 1.0f, 0.1f);
  
  //float pseudoRefraction = 0.1f; // not real refraction ;)
  float3 lightColor = swi3(Light,x,y,z);//to_float3(0.7f, 0.8f, 1.0f);

  float2 uv = fragCoord / iResolution;

  float3 heightColor = to_float3_s(getHeight(uv,iChannel0));
  
  float3 normal = swi3(bumpFromDepth(uv, iResolution, 1.0f,iChannel0),x,y,z);
  heightColor = to_float3_s(0.5f) + 0.5f * pow_f3(heightColor, to_float3_s(2.0f));
  
  float3 camDir = normalize(to_float3_aw(uv, 2.0f));
  
  float2 lightPosXY = mul_mat2_f2(Rotate2D(iTime) , to_float2(-10.0f, -10.0f));
  
  float3 lightPos = normalize(to_float3_aw(lightPosXY, 2.0f)) + to_float3(0.5f, 0.5f, 0.0f);
  
  float3 lightDir = to_float3_aw(uv, 0.0f) - lightPos;
  lightDir = reflect(lightDir, normal);
  
  float scalar = dot(camDir, lightDir);
  scalar = clamp(scalar, 0.0f, 1.0f);
  
  float3 shine = to_float3_s(scalar);
  
  //uv.x *= (iResolution.x/iResolution.y);
  //uv *= iResolution.x / 1024.0f;
  
  //normal.x /= R.x/R.y;
  
  
  float3 color = swi3(texture(iChannel1, uv - swi2(normal,x,z) * pseudoRefraction),x,y,z);
  color *= heightColor;
  color += shine * lightColor;
  
  fragColor = to_float4_aw(color, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}