
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

// "Blurred Net" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

#if 0
#define VAR_ZERO _fminf (iFrame, 0)
#else
#define VAR_ZERO 0
#endif


#define pi 3.1415927f

__DEVICE__ float2 Rot2D (float2 q, float a)
{
  float2 cs;
  float xxxxxxxxxxxxxxxxx;
  cs = sin_f2(a + to_float2 (0.5f * pi, 0.0f));
  return to_float2 (dot (q, to_float2 (cs.x, - cs.y)), dot (swi2(q,y,x), cs));
}

__DEVICE__ mat3 StdVuMat (float el, float az)
{
  float2 ori, ca, sa;
  ori = to_float2 (el, az);
  ca = cos_f2(ori);
  sa = sin_f2(ori);
  return mul_mat3_mat3(to_mat3 (ca.y, 0.0f, - sa.y, 0.0f, 1.0f, 0.0f, sa.y, 0.0f, ca.y) ,
                       to_mat3 (1.0f, 0.0f, 0.0f, 0.0f, ca.x, - sa.x, 0.0f, sa.x, ca.x));
}



__DEVICE__ float4 Hashv4v3 (float3 p)
{
  const float cHashM = 43758.54f;
  float3 cHashVA3 = to_float3 (37.0f, 39.0f, 41.0f);
  return fract_f4(sin_f4(dot (p, cHashVA3) + to_float4(0.0f, cHashVA3.x, cHashVA3.y, cHashVA3.x + cHashVA3.y)) * cHashM);
}

__DEVICE__ float Noisefv3 (float3 p)
{
  float4 t;
  float3 ip, fp;
  ip = _floor (p);
  fp = fract_f3(p);
  fp *= fp * (3.0f - 2.0f * fp);
  t = _mix (Hashv4v3 (ip), Hashv4v3 (ip + to_float3 (0.0f, 0.0f, 1.0f)), fp.z);
  return _mix (_mix (t.x, t.y, fp.x), _mix (t.z, t.w, fp.x), fp.y);
}

__DEVICE__ float Fbm3 (float3 p)
{
  float f, a;
  f = 0.0f;
  a = 1.0f;
  for (int j = 0; j < 5; j ++) {
    f += a * Noisefv3 (p);
    a *= 0.5f;
    p *= 2.0f;
  }
  return f * (1.0f / 1.9375f);
}




__DEVICE__ float3 TrackPath (float t, float2 trkAx, float2 trkAy, float2 trkFx, float2 trkFy )
{
  return to_float3(dot (trkAx, sin_f2 (trkFx * t)), dot (trkAy, sin_f2 (trkFy * t)), t);
}

__DEVICE__ float3 TrackVel (float t, float2 trkAx, float2 trkAy, float2 trkFx, float2 trkFy)
{
  return to_float3(dot (trkAx * trkFx, cos_f2 (trkFx * t)), dot (trkAy * trkFy, cos_f2 (trkFy * t)), 1);
}

__DEVICE__ float ObjDf (float3 p, float2 trkAx, float2 trkAy, float2 trkFx, float2 trkFy)
{
  float4 q;
  q = to_float4_aw(p, 1.0f);
  swi2S(q,x,y, swi2(q,x,y) - swi2(TrackPath (p.z, trkAx, trkAy, trkFx, trkFy),x,y));
  for (float j = 0.0f; j < 8.0f; j ++) {
    swi3S(q,x,y,z, 2.0f * fract (0.5f * swi3(q,x,y,z) + 0.5f) - 1.0f);
    q *= 1.3f / dot (swi3(q,x,y,z), swi3(q,x,y,z));
  }
  return 0.25f * (length (swi3(q,x,y,z)) / q.w - 0.01f);
}

__DEVICE__ float ObjRay (float3 ro, float3 rd, float2 trkAx, float2 trkAy, float2 trkFx, float2 trkFy, float dstFar)
{
  float dHit, d;
  dHit = 0.0f;
  for (int j = VAR_ZERO; j < 220; j ++) {
    d = ObjDf (ro + dHit * rd, trkAx, trkAy, trkFx, trkFy);
    if (d < 0.001f || dHit > dstFar) break;
    dHit += d;
  }
  return dHit;
}

__DEVICE__ float3 ObjNf (float3 p, float2 trkAx, float2 trkAy, float2 trkFx, float2 trkFy)
{
  float zzzzzzzzzzzzzzzzzzzzzzzzzz;
  float _v[4];
  float2 e;
  e = to_float2 (0.001f, -0.001f);
  for (int j = VAR_ZERO; j < 4; j ++) {
    _v[j] = ObjDf (p + ((j < 2) ? ((j == 0) ? swi3(e,x,x,x) : swi3(e,x,y,y)) : ((j == 2) ? swi3(e,y,x,y) : swi3(e,y,y,x))), trkAx, trkAy, trkFx, trkFy);
  }
  float4 v = to_float4(_v[0],_v[1],_v[2],_v[3]);
  v.x = - v.x;
  return normalize (2.0f * swi3(v,y,z,w) - dot (v, to_float4_s (1.0f)));
}

__DEVICE__ float3 BgCol (float3 rd, float tCur)
{
  float t, gd, b;
  t = tCur * 4.0f;
  b = dot (to_float2 (_atan2f (rd.x, rd.y), 0.5f * pi - _acosf (rd.z)), to_float2 (2.0f, _sinf (rd.x)));
  gd = clamp (_sinf (5.0f * b + t), 0.0f, 1.0f) * clamp (_sinf (3.5f * b - t), 0.0f, 1.0f) +
       clamp (_sinf (21.0f * b - t), 0.0f, 1.0f) * clamp (_sinf (17.0f * b + t), 0.0f, 1.0f);
  return _mix (to_float3 (0.6f, 0.5f, 0.0f), to_float3 (0.9f, 0.4f, 0.2f), 0.5f + 0.5f * rd.z) *
              (0.24f + 0.44f * (rd.z + 1.0f) * (rd.z + 1.0f)) * (1.0f + 0.2f * gd);
}

__DEVICE__ float4 ShowScene (float3 ro, float3 rd, float2 trkAx, float2 trkAy, float2 trkFx, float2 trkFy, float dstFar, float3 ltDir, float tCur)
{
  float4 col4;
  float3 col, bgCol, vn;
  float dstObj;
  dstObj = ObjRay (ro, rd, trkAx, trkAy, trkFx, trkFy, dstFar);
  bgCol = BgCol (rd, tCur);
  if (dstObj < dstFar) {
    ro += dstObj * rd;
    vn = ObjNf (ro, trkAx, trkAy, trkFx, trkFy);
    col4 = _mix (to_float4 (0.6f, 0.8f, 0.6f, 0.1f), to_float4 (0.8f, 0.8f, 0.9f, 0.5f), smoothstep (0.5f, 0.55f,
                Fbm3 (16.0f * ro)));
    col = swi3(col4,x,y,z) * (0.2f + 0.8f * _fmaxf (dot (ltDir, vn), 0.0f)) +
               col4.w * to_float3 (1.0f, 1.0f, 0.0f) * _powf (_fmaxf (dot (ltDir, vn), 0.0f), 32.0f);
    col = _mix (col, bgCol, smoothstep (0.2f, 1.0f, dstObj / dstFar));
  } else col = bgCol;
  return to_float4_aw(clamp (col, 0.0f, 1.0f), dstObj);
}

#define txSize iResolution

#define AA  0

__KERNEL__ void BlurredNetFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, float3 iChannelResolution[])
{
  
  fragCoord+=0.5f;
  
  float3 ltDir;
  float2 trkAx, trkAy, trkFx, trkFy;
  float tCur, dstFar;

  mat3 vuMat;
  float4 mPtr, col4;
  float3 ro, rd, vd;
  float2 canvas, uv, uvv;
  float el, az, zmFac, sr, asp, vFly;
  canvas = iResolution;
  uv = 2.0f * fragCoord / canvas - 1.0f;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  mPtr = iMouse;
  swi2S(mPtr,x,y, swi2(mPtr,x,y) / canvas - 0.5f);
  asp = canvas.x / canvas.y;
  zmFac = 3.0f;
  az = 0.0f;
  el = 0.0f;
  if (mPtr.z > 0.0f) {
    az += 2.0f * pi * mPtr.x;
    el += pi * mPtr.y;
  }
  trkAx = 0.25f * to_float2 (2.0f, 0.9f);
  trkAy = 0.25f * to_float2 (1.3f, 0.66f);
  trkFx = 2.0f * to_float2 (0.2f, 0.23f);
  trkFy = 2.0f * to_float2 (0.17f, 0.24f);
  vFly = 0.5f;
  ro = TrackPath (vFly * tCur, trkAx, trkAy, trkFx, trkFy) + 1.0f;
  vd = normalize (TrackVel (vFly * tCur, trkAx, trkAy, trkFx, trkFy));
  az += _atan2f (vd.x, vd.z);
  vuMat = StdVuMat (el, az);
  ltDir = mul_mat3_f3(vuMat , normalize (to_float3 (-1.0f, 1.0f, -1.0f)));
  dstFar = 50.0f;
#if ! AA
  const float naa = 1.0f;
#else
  const float naa = 3.0f;
#endif  
  col4 = to_float4_s (0.0f);
  sr = 2.0f * mod_f (dot (mod_f (_floor (0.5f * (uv + 1.0f) * canvas), 2.0f), to_float2_s (1.0f)), 2.0f) - 1.0f;
  for (float a = float (VAR_ZERO); a < naa; a +=1.0f) {
    uvv = (uv + step (1.5f, naa) * Rot2D (to_float2 (0.5f / canvas.y, 0.0f), sr * (0.667f * a + 0.5f) *
           pi)) / zmFac;

    rd = mul_mat3_f3(vuMat,  normalize (to_float3 (2.0f * _tanf (0.5f * _atan2f (uvv.x / asp, 1.0f)) * asp, uvv.y, 1.0f)));
    col4 += (1.0f / naa) * ShowScene (ro, rd, trkAx, trkAy, trkFx, trkFy, dstFar,ltDir, tCur);
  }
  fragColor = col4;
  
  SetFragmentShaderComputedColor(fragColor);  
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// "Blurred Net" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License


#define txBuf iChannel0



__KERNEL__ void BlurredNetFuse(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
  
  fragCoord+=0.5f;
  
  float4 txVal, col4;
  float2 canvas, uv, duv;
  float dstFoc, dPix, r;
  float ga = pi * (3.0f - _sqrtf(5.0f));
  const float NP = 32.0f;
  canvas = iResolution;
  uv = fragCoord / canvas;
  dstFoc = 5.0f; // (from "Losing Focus 2")
  dPix = _fabs (dstFoc - _tex2DVecN (txBuf,uv.x,uv.y,15).w);
  duv = 0.0008f * dPix * to_float2 (canvas.y / canvas.x, 1.0f);
  col4 = to_float4_s (0.0f);
  for (float n = (float)(VAR_ZERO); n < NP; n+=1.0f) {
    r = _sqrtf (n / NP);
    txVal = texture (txBuf, uv + duv * r * sin_f2 (n * ga + to_float2 (0.5f * pi, 0.0f)));
    col4 += to_float4_aw(swi3(txVal,x,y,z), 1.0f) * _expf (-1.0f * r * r) *
                       clamp (1.0f + (1.0f + 0.1f * txVal.w * txVal.w) * (_fabs (dstFoc - txVal.w) - dPix), 0.0f, 1.0f);
  }
  //swi3(col4,x,y,z) /= col4.w;
  col4.x /= col4.w;
  col4.y /= col4.w;
  col4.z /= col4.w;
  
  fragColor = to_float4_aw(swi3(col4,x,y,z), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}