
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// "Jackson Meets Gauss" by dr2 - 2018
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License


#define pi 3.14159f

#define txBuf iChannel0
//#define txSize iResolution

__DEVICE__ float2 Hashv2v2 (float2 p)
{

  const float cHashM = 43758.54f;
  const float2 cHashVA2 = to_float2 (37.1f, 61.7f);
  const float2 e = to_float2 (1.0f, 0.0f);
  return fract_f2 (sin_f2 (to_float2 (dot (p + swi2(e,y,y), cHashVA2), dot (p + swi2(e,x,y), cHashVA2))) * cHashM);
}


__DEVICE__ float2 GaussRand (float2 seed)   // Box-Muller
{
  float2 r;
  r = 0.001f + 0.999f * Hashv2v2 (seed + 0.001f);
  return _sqrtf (-2.0f * _logf (r.x)) * sin_f2 (2.0f * pi * (r.y + to_float2 (0.0f, 0.25f)));
}

__DEVICE__ float SegDist (float2 p, float2 v1, float2 v2)
{
  
  float2 a, b;
  float s;
  a = p - v1;
  b = v2 - v1;
  s = length (b);
  b = (s > 0.0f) ? b / s : to_float2_s (0.0f);
  return length (a - clamp (dot (a, b), 0.0f, s) * b);
}

__DEVICE__ float LineDist (float2 p, float2 v1, float2 v2)
{

  float2 a, b;
  float s;
  a = p - v1;
  b = v2 - v1;
  s = length (b);
  b = (s > 0.0f) ? b / s : to_float2_s (0.0f);
  return length (a - dot (a, b) * b);
}

__DEVICE__ float3 HsvToRgb (float3 c)
{
  float3 p;
  p = abs_f3 (fract_f3 (swi3(c,x,x,x) + to_float3 (1.0f, 2.0f/3.0f, 1.0f/3.0f)) * 6.0f - 3.0f);
  return c.z * _mix (to_float3_s (1.0f), clamp (p - 1.0f, 0.0f, 1.0f), c.y);
}


__DEVICE__ float4 LoadTx (float2 uv, __TEXTURE2D__ txBuf)
{
  return _tex2DVecN (txBuf,uv.x,uv.y,15);
}

__DEVICE__ float4 Loadv4 (int2 idVar, float2 txSize, __TEXTURE2D__ txBuf)
{
  return _tex2DVecN (txBuf, (idVar.x + 0.5f) / txSize.x,(idVar.y + 0.5f) / txSize.y,15);
}

__DEVICE__ void Savev4 (int2 idVar, float4 val, inout float4 *fCol, float2 fCoord)
{
  float2 d;
  d = abs_f2 (fCoord - make_float2 (idVar) - 0.5f);
  if (_fmaxf (d.x, d.y) < 0.5f) *fCol = val;
  float dddddddddddddddddddddddddddddddddddddddd;
}




__DEVICE__ float4 ShowScene (float2 uv, float4 col, float2 pPen, float2 ppPen, float nStep)
{
  float2 p;
  p = 2.0f * (uv - 0.5f);
  return (SegDist (p, ppPen, pPen) < 0.007f) ? to_float4_aw (HsvToRgb (to_float3 (mod_f (0.001f * nStep, 1.0f),
          0.8f + 0.2f * _sinf (2.0f * pi * 0.01f * nStep), 1.0f - 0.5f * smoothstep (0.006f, 0.007f,
          LineDist (p, ppPen, pPen)))), 1.0f) : _mix (col, to_float4_s (0.1f), 0.0005f);
}



__KERNEL__ void JacksonMeetsGaussFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, int iFrame, float3 iChannelResolution[], sampler2D iChannel0)
{

  fragCoord+=0.5f;

  float2 pPen, ppPen;
  float tCur, nStep;

  float2 txSize = iResolution;

  float4 stDat;
  float2 txSizeP, uvtx, dp;
  int2 iFrag;
  bool doInit;
  tCur = iTime;
  uvtx = fragCoord / txSize;
  iFrag = to_int2_cfloat (fragCoord);
  doInit = false;
  if (iFrame <= 5) {
    doInit = true;
    txSizeP = txSize;
  } else {
    stDat = Loadv4 (to_int2 (0, 0), iResolution,txBuf);
    pPen = swi2(stDat,x,y);
    ppPen = swi2(stDat,z,w);
    stDat = Loadv4 (to_int2 (1, 0),iResolution,txBuf);
    txSizeP.x = stDat.x;
    nStep = stDat.y;
  }

  if (txSize.x != txSizeP.x) doInit = true;
  fragColor = doInit ? to_float4_s (0.1f) : ShowScene (uvtx, LoadTx (uvtx,txBuf),pPen,ppPen,nStep);
  if (iFrag.y == 0 && iFrag.x <= 1) {
    if (doInit) {
      ppPen = to_float2_s (0.0f);
      pPen = ppPen;
      nStep = 0.0f;
    } else {
      ppPen = pPen;
      dp = 0.07f * GaussRand (pPen + tCur);
      if (_fabs (pPen.x + dp.x) > 0.98f) dp.x *= -1.0f;
      if (_fabs (pPen.y + dp.y) > 0.98f) dp.y *= -1.0f;
      pPen += dp;
      ++ nStep;
    }
    if (iFrag.x == 0)      stDat = to_float4_f2f2 (pPen, ppPen);
    else if (iFrag.x == 1) stDat = to_float4 (txSize.x, nStep, 0.0f, 0.0f);
    Savev4 (iFrag, stDat, &fragColor, fragCoord);
  }
  

  SetFragmentShaderComputedColor(fragColor);  
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// "Jackson Meets Gauss" by dr2 - 2018
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

//float4 LoadTx (float2 uv);


//const float pi = 3.14159f;

__DEVICE__ float BlkHit (float3 ro, float3 rd, float bSize, float3 *qnBlk, float2 *qBlk, float dstFar )
{
  float3 v, tm, tp, u;
  float dMin, dn, df;
  if (rd.x == 0.0f) rd.x = 0.001f;
  if (rd.y == 0.0f) rd.y = 0.001f;
  if (rd.z == 0.0f) rd.z = 0.001f;
  v = ro / rd;
  tp = bSize / abs_f3 (rd) - v;
  tm = - tp - 2.0f * v;
  dn = _fmaxf (max (tm.x, tm.y), tm.z);
  df = _fminf (min (tp.x, tp.y), tp.z);
  dMin = dstFar;
  if (df > 0.0f && dn < df) {
    dMin = dn;
    *qnBlk = -1.0f * sign_f3 (rd) * step (swi3(tm,z,x,y), tm) * step (swi3(tm,y,z,x), tm);
    u = (v + dn) * rd;
    *qBlk = to_float2 (dot (swi3(u,z,x,y), *qnBlk), dot (swi3(u,y,z,x), *qnBlk)) / bSize;
  }

  return dMin;
}

__DEVICE__ float3 ShowScene (float3 ro, float3 rd, float dstFar,float3 ltDir, __TEXTURE2D__ txBuf)
{
  float3 col;
  float3 qnBlk;
  float2 qBlk;
  
  if (BlkHit (ro, rd, 1.0f,&qnBlk,&qBlk,dstFar) < dstFar)  col = swi3(LoadTx (0.5f * qBlk + 0.5f, txBuf),x,y,z) *
                                                              (0.2f + 0.8f * _fmaxf (dot (qnBlk, ltDir), 0.0f)) +
                                                               0.1f * _powf (_fmaxf (dot (normalize (ltDir - rd), qnBlk), 0.0f), 32.0f);
  else col = to_float3 (0.6f, 0.6f, 1.0f) * (0.3f + 0.15f * (rd.y + 1.0f) * (rd.y + 1.0f));
  return clamp (col, 0.0f, 1.0f);
}

__KERNEL__ void JacksonMeetsGaussFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  float3 ltDir;
  
  float dstFar, tCur;

  mat3 vuMat;
  float4 mPtr;
  float3 ro, rd, col;
  float2 canvas, uv, ori, ca, sa;
  float el, az;
  canvas = iResolution;
  uv = 2.0f * fragCoord / canvas - 1.0f;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  mPtr = iMouse;
  //swi2(mPtr,x,y) = swi2(mPtr,x,y) / canvas - 0.5f;
  mPtr.x = mPtr.x / canvas.x - 0.5f;
  mPtr.y = mPtr.y / canvas.y - 0.5f;
  az = 0.0f;
  el = 0.0f;
  if (mPtr.z > 0.0f) {
    az += 2.0f * pi * mPtr.x;
    el += pi * mPtr.y;
  } else {
    az += 0.1f *  pi * tCur;
    el += 0.073f *  pi * tCur;
  }
  ori = to_float2 (el, az);
  ca = cos_f2(ori);
  sa = sin_f2(ori);
  vuMat = mul_mat3_mat3(to_mat3 (ca.y, 0.0f, - sa.y, 0.0f, 1.0f, 0.0f, sa.y, 0.0f, ca.y) ,
                        to_mat3 (1.0f, 0.0f, 0.0f, 0.0f, ca.x, - sa.x, 0.0f, sa.x, ca.x));
  ro = mul_mat3_f3(vuMat , to_float3 (0.0f, 0.0f, -6.0f));
  dstFar = 20.0f;
  ltDir = mul_mat3_f3(vuMat , normalize (to_float3 (0.1f, 0.1f, -1.0f)));
  rd = mul_mat3_f3(vuMat , normalize (to_float3_aw (uv, 3.5f)));
  fragColor = to_float4_aw (ShowScene (ro, rd,dstFar,ltDir,txBuf), 1.0f);
  

  SetFragmentShaderComputedColor(fragColor);  
}

