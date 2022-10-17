
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define textureSize  256

__DEVICE__ mat2 rot(in float ang) 
{
   return to_mat2(
                _cosf(ang), -_sinf(ang),
                _sinf(ang),  _cosf(ang));
}

// hash from Dave_Hoskins https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash12(float2 p)
{
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float hash13(float3 p3)
{
  p3  = fract_f3(p3 * 0.1031f);
  p3 += dot(p3, swi3(p3,z,y,x) + 31.32f);
  return fract((p3.x + p3.y) * p3.z);
}

// Box intersection by IQ https://iquilezles.org/articles/boxfunctions
__DEVICE__ float2 boxIntersection( in float3 ro, in float3 rd, in float3 rad, out float3 *oN ) 
{
  float3 m = 1.0f / rd;
  float3 n = m * ro;
  float3 k = abs_f3(m) * rad;
  float3 t1 = -n - k;
  float3 t2 = -n + k;

  float tN = _fmaxf( _fmaxf( t1.x, t1.y ), t1.z );
  float tF = _fminf( _fminf( t2.x, t2.y ), t2.z );

  if( tN > tF || tF < 0.0f) return to_float2_s(-1.0f); // no intersection

  *oN = -1.0f*sign_f3(rd)*step(swi3(t1,y,z,x), swi3(t1,x,y,z)) * step(swi3(t1,z,x,y), swi3(t1,x,y,z));

  return to_float2( tN, tF );
}


// Fog by IQ https://iquilezles.org/articles/fog

__DEVICE__ float3 applyFog( in float3  rgb, float3 fogColor, in float distance)
{
    float fogAmount = _expf( -distance );
    return _mix( fogColor, rgb, fogAmount );
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1

// compute Terrain and update water level 1st pass
__DEVICE__ float boxNoise( in float2 p, in float z )
{
    float2 fl = _floor(p);
    float2 fr = fract_f2(p);
    fr = smoothstep(to_float2_s(0.0f), to_float2_s(1.0f), fr);    
    float res = _mix(_mix( hash13(to_float3_aw(fl, z)),                  hash13(to_float3_aw(fl + to_float2(1,0), z)),fr.x),
                     _mix( hash13(to_float3_aw(fl + to_float2(0,1), z)), hash13(to_float3_aw(fl + to_float2(1,1), z)),fr.x),fr.y);
    return res;
}

__DEVICE__ float Terrain( in float2 p, in float z, in int octaveNum)
{
  float a = 1.0f;
  float f = 0.0f;
  for (int i = 0; i < octaveNum; i++)
  {
    f += a * boxNoise(p, z);
    a *= 0.45f;
    p = 2.0f * mul_mat2_f2(rot(radians(41.0f)) , p);
  }
  return f;
}

__DEVICE__ float2 readHeight(int2 p, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  p = clamp(p, to_int2(0,0), to_int2(textureSize - 1,textureSize - 1));
  return swi2(texture(iChannel0, (make_float2(p)+0.5f)/iResolution),x,y);
} 

__DEVICE__ float4 readOutFlow(int2 p, float2 iResolution, __TEXTURE2D__ iChannel1)
{
  if(p.x < 0 || p.y < 0 || p.x >= textureSize || p.y >= textureSize)
    return to_float4_s(0);
  return texture(iChannel1, (make_float2(p)+0.5f)/iResolution);
} 

__KERNEL__ void MountainsLakesFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    CONNECT_SLIDER4(transitionTime, -1.0f, 10.0f, 5.0f);
    CONNECT_SLIDER5(transitionPercent, -1.0f, 1.0f, 0.3f);
    CONNECT_SLIDER6(initialWaterLevel, -1.0f, 1.0f, 0.05f);
    CONNECT_INTSLIDER0(octaves, 1, 10, 7);
   
    
    fragCoord+=0.5f;   

    // Outside ?
    if( _fmaxf(fragCoord.x, fragCoord.y) > (float)(textureSize) )
    {
        //discard;
        SetFragmentShaderComputedColor(fragColor);        
        return;
    }
           
    // Terrain
    float2 uv = fragCoord / (float)(textureSize);
    float t = iTime / transitionTime;
    float terrainElevation = _mix(Terrain(uv * 4.0f, _floor(t), octaves), Terrain(uv * 4.0f, _floor(t) + 1.0f, octaves), smoothstep(1.0f - transitionPercent, 1.0f, fract(t))) * 0.5f;
    
    // Water
    float waterDept = initialWaterLevel;
    if(iFrame != 0 && Reset == 0)
    {
        int2 p = to_int2_cfloat(fragCoord);
        float2 height = readHeight(p,iResolution,iChannel0);
        //float4 OutFlow = texelFetch(iChannel1, p, 0);
        float4 OutFlow = texture(iChannel1, (make_float2(p)+0.5f)/iResolution);
                
        float totalOutFlow = OutFlow.x + OutFlow.y + OutFlow.z + OutFlow.w;
        float totalInFlow = 0.0f;
        totalInFlow += readOutFlow(p  + to_int2( 1,  0),iResolution,iChannel1).z;
        totalInFlow += readOutFlow(p  + to_int2( 0,  1),iResolution,iChannel1).w;
        totalInFlow += readOutFlow(p  + to_int2(-1,  0),iResolution,iChannel1).x;
        totalInFlow += readOutFlow(p  + to_int2( 0, -1),iResolution,iChannel1).y;
        waterDept = height.y - totalOutFlow + totalInFlow;
    }
    fragColor = to_float4(terrainElevation, waterDept, 0, 1);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__DEVICE__ float computeOutFlowDir(float2 centerHeight, int2 pos, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  float2 dirHeight = readHeight(pos,iResolution,iChannel0);
  return _fmaxf(0.0f, (centerHeight.x + centerHeight.y) - (dirHeight.x + dirHeight.y));
}

__KERNEL__ void MountainsLakesFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0);

  CONNECT_SLIDER0(attenuation, 0.0f, 1.0f, 0.995f);
  CONNECT_SLIDER1(strenght, -1.0f, 1.0f, 0.25f);
  CONNECT_SLIDER2(minTotalFlow, -1.0f, 1.0f, 0.0001f);

  fragCoord+=0.5f;

  int2 p = to_int2_cfloat(fragCoord);
  // Init to zero at frame 0
  if(iFrame == 0 || Reset)
  {
    fragColor = to_float4_s(0);
    SetFragmentShaderComputedColor(fragColor);
    return;
  }    
  
  // Outside ?
  if( _fmaxf(p.x, p.y) > textureSize )
  {
    SetFragmentShaderComputedColor(fragColor);
    return;
  }
        
    
  float4 oOutFlow = texture(iChannel1, (make_float2(p)+0.5f)/iResolution);
  float2 height = readHeight(p,iResolution, iChannel0);
  float4 nOutFlow;        
  nOutFlow.x = computeOutFlowDir(height, p + to_int2( 1,  0),iResolution,iChannel0);
  nOutFlow.y = computeOutFlowDir(height, p + to_int2( 0,  1),iResolution,iChannel0);
  nOutFlow.z = computeOutFlowDir(height, p + to_int2(-1,  0),iResolution,iChannel0);
  nOutFlow.w = computeOutFlowDir(height, p + to_int2( 0, -1),iResolution,iChannel0);
  nOutFlow = attenuation * oOutFlow + strenght * nOutFlow;
  float totalFlow = nOutFlow.x + nOutFlow.y + nOutFlow.z + nOutFlow.w;
  if(totalFlow > minTotalFlow)
  {
    if(height.y < totalFlow)
    {
      nOutFlow = nOutFlow * (height.y / totalFlow);
    }
  }
  else
  {
    nOutFlow = to_float4_s(0);
  }

  fragColor = nOutFlow;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1

// water level 2nd pass
__KERNEL__ void MountainsLakesFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  
    fragCoord+=0.5f;

    // Outside ?
    if( _fmaxf(fragCoord.x, fragCoord.y) > (float)(textureSize) )
    {
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
           
    // Water
    int2 p = to_int2_cfloat(fragCoord);
    float2 height = readHeight(p,iResolution,iChannel0);
    float4 OutFlow = texture(iChannel1, (make_float2(p)+0.5f)/iResolution);
    float totalOutFlow = OutFlow.x + OutFlow.y + OutFlow.z + OutFlow.w;
    float totalInFlow = 0.0f;
    totalInFlow += readOutFlow(p  + to_int2( 1,  0),iResolution,iChannel1).z;
    totalInFlow += readOutFlow(p  + to_int2( 0,  1),iResolution,iChannel1).w;
    totalInFlow += readOutFlow(p  + to_int2(-1,  0),iResolution,iChannel1).x;
    totalInFlow += readOutFlow(p  + to_int2( 0, -1),iResolution,iChannel1).y;
    float waterDept = height.y - totalOutFlow + totalInFlow;

    fragColor = to_float4(height.x, waterDept, 0, 1);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0

__KERNEL__ void MountainsLakesFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_SLIDER0(attenuation, 0.0f, 1.0f, 0.995f);
  CONNECT_SLIDER1(strenght, -1.0f, 1.0f, 0.25f);
  CONNECT_SLIDER2(minTotalFlow, -1.0f, 1.0f, 0.0001f);

  fragCoord+=0.5f;  

  int2 p = to_int2_cfloat(fragCoord);
  
  // Outside ?
  if( _fmaxf(p.x, p.y) > textureSize )
  {
    SetFragmentShaderComputedColor(fragColor);
    return;
  }
      
  float4 oOutFlow = texture(iChannel1,  (make_float2(p)+0.5f)/iResolution);
  float2 height = readHeight(p,iResolution,iChannel0);
  float4 nOutFlow;        
  nOutFlow.x = computeOutFlowDir(height, p + to_int2( 1,  0),iResolution,iChannel0);
  nOutFlow.y = computeOutFlowDir(height, p + to_int2( 0,  1),iResolution,iChannel0);
  nOutFlow.z = computeOutFlowDir(height, p + to_int2(-1,  0),iResolution,iChannel0);
  nOutFlow.w = computeOutFlowDir(height, p + to_int2( 0, -1),iResolution,iChannel0);
  nOutFlow = attenuation * oOutFlow + strenght * nOutFlow;
  float totalFlow = nOutFlow.x + nOutFlow.y + nOutFlow.z + nOutFlow.w;
  if(totalFlow > minTotalFlow)
  {
    if(height.y < totalFlow)
    {
      nOutFlow = nOutFlow * (height.y / totalFlow);
    }
  }
  else
  {
    nOutFlow = to_float4_s(0);
  }
  fragColor = nOutFlow;

  SetFragmentShaderComputedColor(fragColor);
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 3' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel0

// Created by David Gallardo - xjorma/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0


#define AA
#define GAMMA 1

__DEVICE__ float2 getHeight(in float3 p, float2 iResolution, __TEXTURE2D__ iChannel0, float boxHeight)
{
  p = (p + 1.0f) * 0.5f;
  float2 p2 = swi2(p,x,z) * to_float2_s((float)(textureSize)) / iResolution;
  p2 = _fminf(p2, to_float2_s((float)(textureSize) - 0.5f) / iResolution);

  float2 h = swi2(_tex2DVecN(iChannel0,p2.x,p2.y,15),x,y);
  h.y += h.x;
  return h - boxHeight;
} 

union A2F
 {
   float2  F;  
   float  A[2];
 };

__DEVICE__ float3 getNormal(in float3 p, int comp, float2 iResolution, __TEXTURE2D__ iChannel0, float boxHeight)
{
  float d = 2.0f / (float)(textureSize);
 
  A2F hMidF;
  hMidF.F = getHeight(p,iResolution,iChannel0,boxHeight); 
  float hMid = hMidF.A[comp];
  
  A2F hRightF;
  hRightF.F = getHeight(p + to_float3(d, 0, 0),iResolution,iChannel0,boxHeight); 
  float hRight = hRightF.A[comp];
  
  A2F hTopF;
  hTopF.F = getHeight(p + to_float3(0, 0, d),iResolution,iChannel0,boxHeight); 
  float hTop = hTopF.A[comp];
    
  return normalize(cross(to_float3(0, hTop - hMid, d), to_float3(d, hRight - hMid, 0)));
}

__DEVICE__ float3 terrainColor(in float3 p, in float3 n, out float *spec, float2 iResolution, __TEXTURE2D__ iChannel1, float3 color[2])
{
  *spec = 0.1f;
  float3 c = color[0];
  float cliff = smoothstep(0.8f, 0.3f, n.y);
  c = _mix(c, to_float3_s(0.25f), cliff);
  *spec = _mix(*spec, 0.3f, cliff);
  float snow = smoothstep(0.05f, 0.25f, p.y) * smoothstep(0.5f, 0.7f, n.y);
  
  c = _mix(c, color[1], snow);
  *spec = _mix(*spec, 0.4f, snow);
  float3 t = swi3(texture(iChannel1, swi2(p,x,z) * 5.0f),x,y,z);
  return _mix(c, c * t, 0.75f);
}

__DEVICE__ float3 undergroundColor(float d, float3 color[4])
{
  d *= 6.0f;
  d = _fminf(d, 3.0f - 0.001f);
  float fr = fract(d);
  float fl = _floor(d);
  return _mix(color[(int)(fl)], color[(int)(fl) + 1], fr);
}


__DEVICE__ float3 Render(in float3 ro, in float3 rd, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1,
                            float boxHeight, float3 light, float3 backgroundColor, float *alpha, float3 Color1[2], float3 Color2[4])
{
    // Render
    bool bkgAlpha = false;
  
    float3 n=to_float3_s(0.0f);
    float2 ret = boxIntersection(ro, rd, to_float3(1, boxHeight, 1), &n);
    if(ret.x > 0.0f)
    {
        float3 pi = ro + rd * ret.x;
        // Find Terrain
        float3 tc=to_float3_s(0.0f);
        float3 tn=to_float3_s(0.0f);
        float tt = ret.x;
        float2 h = getHeight(pi,iResolution,iChannel0, boxHeight);
        float spec = 0.0f; //Initialisierungfehler !!
        if(pi.y < h.x)
        {
            tn = n;
            tc = undergroundColor(h.x - pi.y, Color2);
            
            //if(isnan(tc.x)) tc = to_float3_s(1.0f); 
        }
        else
        {
            for (int i = 0; i < 80; i++)
            {
                float3 p = ro + rd * tt;
                float h = p.y - getHeight(p,iResolution,iChannel0,boxHeight).x;
                if (h < 0.0002f || tt > ret.y)
                    break;
                tt += h * 0.4f;
            }
            tn = getNormal(ro + rd * tt, 0,iResolution,iChannel0,boxHeight);
            tc = terrainColor(ro + rd * tt, tn, &spec,iResolution,iChannel1, Color1);
        }
        
        {
            float3 lightDir = normalize(light - (ro + rd * tt));
            tc = tc * (_fmaxf( 0.0f, dot(lightDir, tn)) + 0.3f);
            spec *= _powf(_fmaxf(0.0f, dot(lightDir, reflect(rd, tn))), 10.0f);
            tc += spec;            
        }
        
        if(tt > ret.y)
        {
            tc = backgroundColor;
            bkgAlpha = true;
        }
        
        // Find Water
        float wt = ret.x;
        h = getHeight(pi,iResolution,iChannel0,boxHeight);
        float3 waterNormal;
        if(pi.y < h.y)
        {
            waterNormal = n;
        }
        else
        {
            for (int i = 0; i < 80; i++)
            {
                float3 p = ro + rd * wt;
                float h = p.y - getHeight(p,iResolution,iChannel0,boxHeight).y;
                if (h < 0.0002f || wt > _fminf(tt, ret.y))
                    break;
                wt += h * 0.4f;
            }
            waterNormal = getNormal(ro + rd * wt, 1,iResolution,iChannel0,boxHeight);
        }

        if(wt < ret.y)
        {
            float dist = (_fminf(tt, ret.y) - wt);
            float3 p = waterNormal;
            float3 lightDir = normalize(light - (ro + rd * wt));
                        
            tc = applyFog( tc, to_float3(0,0,0.4f), dist * 15.0f);
            
            float spec = _powf(_fmaxf(0.0f, dot(lightDir, reflect(rd, waterNormal))), 20.0f);
            tc += 0.5f * spec * smoothstep(0.0f, 0.1f, dist);
        }
        
        if(bkgAlpha == false) *alpha = 1.0f;


        return tc;
    }
    return backgroundColor;
}


__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta )
{
  float3 cw = normalize(ta-ro);
  float3 up = to_float3(0, 1, 0);
  float3 cu = normalize( cross(cw,up) );
  float3 cv = normalize( cross(cu,cw) );
  return to_mat3_f3( cu, cv, cw );
}


__DEVICE__ float3 vignette(float3 color, float2 q, float v)
{
    color *= 0.3f + 0.8f * _powf(16.0f * q.x * q.y * (1.0f - q.x) * (1.0f - q.y), v);
    return color;
}


__KERNEL__ void MountainsLakesFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

  CONNECT_COLOR0(backgroundColor, 0.2f, 0.2f, 0.2f, 1.0f);
  
  CONNECT_COLOR1(CliffColor, 0.21f, 0.50f, 0.07f, 1.0f);
  CONNECT_COLOR2(SnowColor, 0.95f, 0.95f, 0.85f, 1.0f);
  
  CONNECT_COLOR3(UndergroundColor1, 0.5f, 0.45f, 0.5f, 1.0f);
  CONNECT_COLOR4(UndergroundColor2, 0.40f, 0.35f, 0.25f, 1.0f);
  CONNECT_COLOR5(UndergroundColor3, 0.55f, 0.50f, 0.4f, 1.0f);
  CONNECT_COLOR6(UndergroundColor4, 0.45f, 0.30f, 0.20f, 1.0f);    
  
  float3 Color1[2] = {swi3(CliffColor,x,y,z), swi3(SnowColor,x,y,z)};
  float3 Color2[4] = {swi3(UndergroundColor1,x,y,z), swi3(UndergroundColor2,x,y,z), swi3(UndergroundColor3,x,y,z), swi3(UndergroundColor4,x,y,z)};
  
  CONNECT_COLOR7(light, 0.0f, 4.0f, 2.0f, 1.0f);
  
  CONNECT_SLIDER3(boxHeight, -1.0f, 1.0f, 0.45f);
  
  CONNECT_INTSLIDER1(NrAA, 1, 4, 4);


  float3 tot = to_float3_s(0.0f);
  float alpha = backgroundColor.w;    
      
  float2 mouse = swi2(iMouse,x,y);
  if(length(swi2(mouse,x,y)) < 10.0f)
      mouse = iResolution * 0.5f;
        
#ifdef AA
  float2 rook[4];
    rook[0] = to_float2( 1.0f/8.0f, 3.0f/8.0f);
    rook[1] = to_float2( 3.0f/8.0f,-1.0f/8.0f);
    rook[2] = to_float2(-1.0f/8.0f,-3.0f/8.0f);
    rook[3] = to_float2(-3.0f/8.0f, 1.0f/8.0f);
    for( int n=0; n<NrAA; ++n )
    {
        // pixel coordinates
        float2 o = rook[n];
        float2 p = (-iResolution + 2.0f*(fragCoord+o))/iResolution.y;
#else //AA
        float2 p = (-iResolution + 2.0f*fragCoord)/iResolution.y;
#endif //AA
 
        // camera
        float theta  = radians(360.0f)*(mouse.x/iResolution.x-0.5f) + iTime*0.2f;
        float phi  = radians(90.0f)*(mouse.y/iResolution.y-0.5f)-1.0f;
        float3 ro = 2.0f * to_float3( _sinf(phi)*_cosf(theta),_cosf(phi),_sinf(phi)*_sinf(theta));
        //vec3 ro = to_float3(0.0f,0.2f,4.0f);
        float3 ta = to_float3_s( 0 );
        // camera-to-world transformation
        mat3 ca = setCamera( ro, ta );
        //vec3 cd = ca[2];    
        
        float3 rd =  mul_mat3_f3(ca , normalize(to_float3_aw(p,1.5f)));
        
        float3 col = Render(ro, rd,iResolution,iChannel0,iChannel1, boxHeight, swi3(light,x,y,z), swi3(backgroundColor,x,y,z), &alpha, Color1,Color2);
        
        tot += col;
            
#ifdef AA
    }
    tot /= (float)NrAA;
#endif
    
    tot = vignette(tot, fragCoord / iResolution, 0.6f);
    #if GAMMA
      tot = pow_f3(tot, to_float3_s(1.0f / 2.2f));
    #endif

  fragColor = to_float4_aw( tot, alpha );

  SetFragmentShaderComputedColor(fragColor);
}