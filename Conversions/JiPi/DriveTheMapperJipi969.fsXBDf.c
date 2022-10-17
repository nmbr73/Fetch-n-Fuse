
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// "Drive the Mapper" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

#define pi 3.1415927f

  __DEVICE__ const int npTrail = 32;  // same for image and buffer
  __DEVICE__ const int npFst = 7;
  
  __DEVICE__ const float txRow = 128.0f;


__DEVICE__ float PrRoundBox2Df (float2 p, float2 b, float r)
{
  return length (_fmaxf (abs_f2 (p) - b, to_float2_s(0.0f))) - r;
}

__DEVICE__ float SmoothBump (float lo, float hi, float w, float x)
{
  return (1.0f - smoothstep (hi - w, hi + w, x)) * smoothstep (lo - w, lo + w, x);
}

__DEVICE__ float2 Rot2D (float2 q, float a)
{
  float2 cs;
  cs = sin_f2 (a + to_float2 (0.5f * pi, 0.0f));
  return to_float2 (dot (q, to_float2 (cs.x, - cs.y)), dot (swi2(q,y,x), cs));
}

#define txBuf iChannel0
#define txSize iChannelResolution[0].xy

__DEVICE__ float4 Loadv4 (int idVar, float2 R, __TEXTURE2D__ iChannel0)
{
  float fi;
  fi = (float) (idVar);
  return texture (txBuf, (to_float2 (mod_f (fi, txRow), _floor (fi / txRow)) + 0.5f) / R); //txSize
}

__DEVICE__ void Savev4 (int idVar, float4 val, inout float4 *fCol, float2 fCoord)
{
  float2 d;
  float fi;
  fi = (float) (idVar);
  d = abs_f2 (fCoord - to_float2 (mod_f (fi, txRow), _floor (fi / txRow)) - 0.5f);
  if (_fmaxf (d.x, d.y) < 0.5f) *fCol = val;
}




__DEVICE__ bool OnPath (float4 carPos, float3 wlBase, float gSize, float rdWid)
{
  float2 p;
  bool onP;
  onP = true;
  for (int k = 0; k < 6; k ++) {
    if (k <= 1 || k >= 4) {
      p = swi2(carPos,x,z) - Rot2D ((to_float2 (2.0f * mod_f (float (k), 2.0f), float (k / 2)) - 1.0f) *
          swi2(wlBase,x,z), - carPos.w);
      onP = (PrRoundBox2Df (mod_f2 (p + 0.5f * gSize, gSize) - 0.5f * gSize, to_float2_s (0.5f * gSize) -
         2.0f * rdWid, rdWid) > 0.0f);
      if (! onP) break;
    }
  }

  return onP;
}



__KERNEL__ void DriveTheMapperJipi969Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, float3 iChannelResolution[], float4 iDate, sampler2D iChannel0)
{
  
  CONNECT_CHECKBOX0(Steering, 0);
  CONNECT_SLIDER0(Direction, -0.25f, 0.25f, 0.0f);
  CONNECT_SLIDER1(Speed, -0.5f, 0.5f, 0.0f);
  
  fragCoord+=0.5f;

  float4 carPos;
  float3 wlBase;
  float gSize, rdWid;

  

  float4 mPtr, mPtrP, stDat, carPosP, wlRot, wr;
  float3 wgBox;
  float2 iFrag, canvas, cnPos, cp, ud, w, rTurn;
  float todCur, tCur, autoMd, cRotN, wlRad, strRot, spd, msOff, trvDist,
        onPath, tc, nStep, az, el, asp, s;
  int pxId, wgSel, nFrame;
  bool init;
  iFrag = _floor (fragCoord);
  pxId = (int) (iFrag.x + txRow * iFrag.y);
  if (pxId >= npFst + npTrail) { SetFragmentShaderComputedColor(fragColor);  return; } //discard;
  nFrame = iFrame;
  canvas = iResolution;
  tCur = iTime;
  todCur = iTime; //iDate.w;
  mPtr = iMouse;
  swi2S(mPtr,x,y, swi2(mPtr,x,y) / canvas - 0.5f);
  init = (nFrame <= 1);
  asp = canvas.x / canvas.y;
  if (init) {
    gSize = 80.0f;
    msOff = mod_f (_floor (1000.0f * todCur), 10000.0f);
    carPos = to_float4 (0.5f * gSize + 3.0f, 0.0f, 0.0f, 0.0f);
    strRot = 0.0f;
    wlRad = 0.5f;
    wlBase = to_float3 (1.4f, wlRad, 1.5f);
    wlRot = to_float4_s (0.0f);
    az = -0.3f * pi;
    el = -0.1f * pi;
    nStep = 0.0f;
    cnPos = to_float2_s (0.0f);
    mPtrP = mPtr;
    wgSel = 0;
    autoMd = 1.0f;
    rdWid = 5.0f;
    onPath = 0.0f;
    trvDist = 0.0f;
  } else {
    stDat = Loadv4 (0, R, iChannel0);       // Maus
    swi3S(mPtrP,x,y,z, swi3(stDat,x,y,z));
    wgSel = (int) (stDat.w);
    stDat = Loadv4 (1, R, iChannel0);
    wlBase = swi3(stDat,x,y,z);
    stDat = Loadv4 (2, R, iChannel0);
    gSize = stDat.x;
    rdWid = stDat.y;
    onPath = stDat.z;
    trvDist = stDat.w;
    stDat = Loadv4 (3, R, iChannel0);
    az = stDat.x;
    el = stDat.y;
    cnPos = swi2(stDat,z,w);
    stDat = Loadv4 (4, R, iChannel0);
    nStep = stDat.x;
    strRot = stDat.y;
    autoMd = stDat.z;
    msOff = stDat.w;
    stDat = Loadv4 (5, R, iChannel0);
    carPos = stDat;
    stDat = Loadv4 (6, R, iChannel0);
    wlRot = stDat;
  }
  
  if (pxId < npFst) {
    wgBox = to_float3 (0.41f, -0.32f, 0.135f);
    
    

    
    
    if (mPtr.z > 0.0f) {
      if (wgSel == 0 && mPtrP.z > 0.0f) {
        az = -2.0f * pi * mPtr.x;
        el = - pi * mPtr.y;
      } else {
        ud = swi2(mPtr,x,y) * to_float2 (asp, 1.0f) - swi2(wgBox,x,y) * to_float2 (asp, 1.0f);
        if (wgSel == 1) {
          autoMd = - tCur;
          cnPos = ud;
          s = length (cnPos);
          if (s > 0.0f)   cnPos = _fminf (s, wgBox.z) * cnPos / s;
        } else if (mPtrP.z <= 0.0f && length (ud) < wgBox.z) wgSel = 1;
      }
      
    } else {
      wgSel = 0;
      cnPos *= to_float2 (1.0f - 1e-2, 1.0f - 2e-3);
    }
    
    if (Steering)
    {
      //autoMd = - tCur;
      autoMd = - 1.0;//tCur;
      cnPos = to_float2(Direction,Speed);
    }   
    
    
    
    wlRad = wlBase.y;
    ++ nStep;
    if (autoMd > 0.0f) {
      tc = mod_f (0.004f * nStep, 4.0f);
      strRot = 0.04f * pi * SmoothBump (0.3f, 0.7f, 0.1f, mod_f (tc, 1.0f)) * sign_f (mod_f (tc, 2.0f) - 1.0f) *
               sign_f (tc - 2.0f);
      spd = 0.7f * (0.12f - 0.06f * _fabs (strRot / (0.15f * pi)));
      w = to_float2 (- strRot / (0.15f * pi), spd / 0.5f);
      w = pow_f2 (abs_f2 (w), 1.0f / to_float2_s (1.5f)) * sign_f2 (w);
      cnPos = w * wgBox.z;
    } else {
      w = cnPos / wgBox.z;
      w = pow_f2 (abs_f2 (w), to_float2_s (1.5f)) * sign_f2 (w);
      strRot = -0.15f * pi * w.x;
      spd = 0.2f * w.y * smoothstep (0.01f, 0.02f, _fabs (w.y));
      if (tCur + autoMd > 20.0f) autoMd = 1.0f;
    }
    carPosP = carPos;
    cp = swi2(carPos,x,z);
    wr = to_float4_s (1.0f);
    if (_fabs (strRot) > 1e-4) {
      cRotN = carPos.w - strRot * spd / pi;
      rTurn.x = wlBase.z / _asinf(strRot);
      s = wlBase.z / rTurn.x;
      rTurn.y = rTurn.x * _sqrtf (1.0f - s * s);
      swi2S(carPos,x,z, swi2(carPos,x,z) + 2.0f * rTurn.x * (sin_f2 (carPos.w - to_float2 (0.5f * pi, 0.0f)) -
                        sin_f2 (cRotN - to_float2 (0.5f * pi, 0.0f))));
      carPos.w = mod_f (cRotN, 2.0f * pi);
      wr += wlBase.x * to_float4 (-1.0f, 1.0f, -1.0f, 1.0f) / swi4(rTurn,x,x,y,y);
    } else {
      swi2S(carPos,x,z, swi2(carPos,x,z) + spd * sin_f2 (carPos.w + to_float2 (0.0f, 0.5f * pi)));
    }
    onPath = OnPath (carPos, wlBase, gSize, rdWid) ? 1.0f : 0.0f;

    if (onPath > 0.0f) {
      float4 _wlRot = wlRot; //
      wlRot += wr * spd / wlRad;
      if (isnan(wlRot.z)==1) wlRot.z = _wlRot.z;
      if (isnan(wlRot.w)==1) wlRot.w = _wlRot.w;
      
      if (spd > 0.0f) trvDist += length (swi2(carPos,x,z) - cp);
    } else {
      carPos = carPosP;
      spd = 0.0f;
      trvDist = 0.0f;
    }
  }
  
  if (! init) {
    if (mod_f ((float) (nFrame), 12.0f) == 0.0f) {
      
      if (pxId == npFst)                                stDat = to_float4_aw(swi3(Loadv4 (5, R, iChannel0),x,z,w), tCur);
      else if (pxId < npFst + npTrail)                  stDat = Loadv4 (pxId - 1, R, iChannel0);
    } else if (pxId >= npFst && pxId < npFst + npTrail) stDat = Loadv4 (pxId, R, iChannel0);
  } else {
    stDat = to_float4 (0.0f, 0.0f, -1.0f, 0.0f);
  }
  
  if      (pxId == 0) stDat = to_float4_aw(swi3(mPtr,x,y,z), (float) (wgSel));
  else if (pxId == 1) stDat = to_float4_aw(wlBase, spd);
  else if (pxId == 2) stDat = to_float4 (gSize, rdWid, onPath, trvDist);
  else if (pxId == 3) stDat = to_float4 (az, el, cnPos.x, cnPos.y);
  else if (pxId == 4) stDat = to_float4 (nStep, strRot, autoMd, msOff);
  else if (pxId == 5) stDat = carPos;
  else if (pxId == 6) stDat = wlRot;
  Savev4 (pxId, stDat, &fragColor, fragCoord);
  
  //fragColor = to_float4_s(1.0f);
  
  SetFragmentShaderComputedColor(fragColor);  
}



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// "Drive the Mapper" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

/*
  No. 7 in "Driving" series; others are listed in "Truck Driving 2" (ftt3Ds)
*/

#define AA  0   // (= 0/1) optional antialiasing (can be slow)

#if 0
#define VAR_ZERO _fminf (iFrame, 0)
#else
#define VAR_ZERO 0
#endif


__DEVICE__ float PrBoxDf (float3 p, float3 b)
{
  float3 d;
  d = abs_f3 (p) - b;
  return _fminf (_fmaxf (d.x, _fmaxf (d.y, d.z)), 0.0f) + length (_fmaxf (d, to_float3_s(0.0f)));
}

__DEVICE__ float PrBox2Df (float2 p, float2 b)
{
  float2 d;
  d = abs_f2 (p) - b;
  return _fminf (_fmaxf (d.x, d.y), 0.0f) + length (_fmaxf (d, to_float2_s(0.0f)));
}

__DEVICE__ float PrRoundBoxDf (float3 p, float3 b, float r)
{
  return length(_fmaxf (abs_f3 (p) - b, to_float3_s(0.0f))) - r;
}

__DEVICE__ float PrRound4BoxDf (float3 p, float3 b, float r)
{
  p = _fmaxf (abs_f3 (p) - b, to_float3_s(0.0f));
  return _sqrtf (length (p * p)) - r;
}

/*
__DEVICE__ float PrRoundBox2Df (float2 p, float2 b, float r)
{
  return length (_fmaxf (_fabs (p) - b, 0.0f)) - r;
}
*/
__DEVICE__ float PrCylDf (float3 p, float r, float h)
{

  return _fmaxf (length (swi2(p,x,y)) - r, _fabs (p.z) - h);
}

__DEVICE__ float PrRoundCylDf (float3 p, float r, float rt, float h)
{
  return length (_fmaxf (to_float2 (length (swi2(p,x,y)) - r, _fabs (p.z) - h), to_float2_s(0.0f))) - rt;
}

__DEVICE__ float PrCapsDf (float3 p, float r, float h)
{
  return length (p - to_float3 (0.0f, 0.0f, clamp (p.z, - h, h))) - r;
}

__DEVICE__ float PrConCapsDf (float3 p, float2 cs, float r, float h)
{
  float2 b;
  float d;
  d = _fmaxf (dot (to_float2 (length (swi2(p,x,y)) - r, p.z), cs), _fabs (p.z) - h);
  h /= cs.x * cs.x;
  r /= cs.x;
  b = to_float2 (r, h);
  b *= cs.y;
  p.z += b.x;
  return _fminf (d, _fminf (length (p - to_float3 (0.0f, 0.0f, h)) - r + b.y,
         length (p - to_float3 (0.0f, 0.0f, - h)) - r  - b.y));
}

__DEVICE__ float PrTorusDf (float3 p, float ri, float rc)
{
  return length (to_float2 (length (swi2(p,x,y)) - rc, p.z)) - ri;
}

__DEVICE__ float Minv2 (float2 p)
{
  return _fminf (p.x, p.y);
}

__DEVICE__ float Maxv2 (float2 p)
{
  return _fmaxf (p.x, p.y);
}

__DEVICE__ float SmoothMin (float a, float b, float r)
{
  float h;
  h = clamp (0.5f + 0.5f * (b - a) / r, 0.0f, 1.0f);
  return _mix (b - h * r, a, h);
}

__DEVICE__ float SmoothMax (float a, float b, float r)
{
  return - SmoothMin (- a, - b, r);
}

__DEVICE__ mat3 StdVuMat (float el, float az)
{
  float2 ori, ca, sa;
  ori = to_float2 (el, az);
  ca = cos_f2 (ori);
  sa = sin_f2 (ori);
  //return mul_mat3_mat3(to_mat3 (ca.y, 0.0f, - sa.y, 0.0f, 1.0f, 0.0f, sa.y, 0.0f, ca.y) ,
  //                     to_mat3 (1.0f, 0.0f, 0.0f, 0.0f, ca.x, - sa.x, 0.0f, sa.x, ca.x));
  
  return mul_mat3_mat3(to_mat3 (1.0f, 0.0f, 0.0f, 0.0f, ca.x, - sa.x, 0.0f, sa.x, ca.x) ,
                       to_mat3 (ca.y, 0.0f, - sa.y, 0.0f, 1.0f, 0.0f, sa.y, 0.0f, ca.y) );
}

/*
__DEVICE__ float2 Rot2D (float2 q, float a)
{
  float2 cs;
  cs = _sinf (a + to_float2 (0.5f * pi, 0.0f));
  return to_float2 (dot (q, to_float2 (cs.x, - cs.y)), dot (swi2(q,y,x), cs));
}
*/
__DEVICE__ float2 Rot2Cs (float2 q, float2 cs)
{
  return to_float2 (dot (q, to_float2 (cs.x, - cs.y)), dot (swi2(q,y,x), cs));
}

__DEVICE__ float3 HsvToRgb (float3 c)
{
  return c.z * _mix (to_float3_s (1.0f), clamp (abs_f3 (fract_f3 (swi3(c,x,x,x) + to_float3 (1.0f, 2.0f/3.0f, 1.0f/3.0f)) * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f), c.y);
}

__DEVICE__ float DigSeg (float2 q)
{
  return (1.0f - smoothstep (0.13f, 0.17f, _fabs (q.x))) *
         (1.0f - smoothstep (0.5f, 0.57f, _fabs (q.y)));
}

#define DSG(q) k = kk;  kk = k / 2;  if (kk * 2 != k) d += DigSeg (q)

__DEVICE__ float ShowDig (float2 q, int iv)
{
  float d;
  int k, kk;
  const float2 vp = to_float2 (0.5f, 0.5f), vm = to_float2 (-0.5f, 0.5f), vo = to_float2 (1.0f, 0.0f);
  if (iv == -1) k = 8;
  else if (iv < 2) k = (iv == 0) ? 119 : 36;
  else if (iv < 4) k = (iv == 2) ? 93 : 109;
  else if (iv < 6) k = (iv == 4) ? 46 : 107;
  else if (iv < 8) k = (iv == 6) ? 122 : 37;
  else             k = (iv == 8) ? 127 : 47;
  q = (q - 0.5f) * to_float2 (1.8f, 2.3f);
  d = 0.0f;
  kk = k;
  DSG (swi2(q,y,x) - vo);  DSG (swi2(q,x,y) - vp);  DSG (swi2(q,x,y) - vm);  DSG (swi2(q,y,x));
  DSG (swi2(q,x,y) + vm);  DSG (swi2(q,x,y) + vp);  DSG (swi2(q,y,x) + vo);
  return d;
}

__DEVICE__ float ShowIntPZ (float2 q, float2 cBox, float mxChar, float val)
{
  float nDig, idChar, s, v;
  q = to_float2 (- q.x, q.y) / cBox;
  s = 0.0f;
  if (_fminf (q.x, q.y) >= 0.0f && _fmaxf (q.x, q.y) < 1.0f) {
    q.x *= mxChar;
    nDig = mxChar;
    idChar = mxChar - 1.0f - _floor (q.x);
    q.x = fract (q.x);
    val = _fmaxf (val, 0.0f);
    v = val / _powf (10.0f, mxChar - idChar - 1.0f);
    if (idChar >= mxChar - nDig) s = ShowDig (q, (int) (mod_f (_floor (v), 10.0f)));
  }
  return s;
}

#define cHashM  43758.54f

__DEVICE__ float2 Hashv2f (float p)
{
  return fract (sin_f2 (p + to_float2 (0.0f, 1.0f)) * cHashM);
}

__DEVICE__ float2 Hashv2v2 (float2 p)
{
  float2 cHashVA2 = to_float2 (37.0f, 39.0f);
  return fract_f2 (sin_f2 (dot (p, cHashVA2) + to_float2 (0.0f, cHashVA2.x)) * cHashM);
}

__DEVICE__ float3 Hashv3v2 (float2 p)
{
  float2 cHashVA2 = to_float2 (37.0f, 39);
  return fract_f3 (sin_f3 (dot (p, cHashVA2) + to_float3 (0.0f, cHashVA2.x,cHashVA2.y)) * cHashM);
}

__DEVICE__ float Noiseff (float p)
{
  float2 t;
  float ip, fp;
  ip = _floor (p);
  fp = fract (p);
  fp = fp * fp * (3.0f - 2.0f * fp);
  t = Hashv2f (ip);
  return _mix (t.x, t.y, fp);
}

__DEVICE__ float Noisefv2 (float2 p)
{
  float2 t, ip, fp;
  ip = _floor (p);  
  fp = fract_f2 (p);
  fp = fp * fp * (3.0f - 2.0f * fp);
  t = _mix (Hashv2v2 (ip), Hashv2v2 (ip + to_float2 (0.0f, 1.0f)), fp.y);
  return _mix (t.x, t.y, fp.x);
}

__DEVICE__ float Fbm1 (float p)
{
  float f, a;
  f = 0.0f;
  a = 1.0f;
  for (int j = 0; j < 5; j ++) {
    f += a * Noiseff (p);
    a *= 0.5f;
    p *= 2.0f;
  }
  return f * (1.0f / 1.9375f);
}

__DEVICE__ float Fbm2 (float2 p)
{
  float f, a;
  f = 0.0f;
  a = 1.0f;
  for (int j = 0; j < 5; j ++) {
    f += a * Noisefv2 (p);
    a *= 0.5f;
    p *= 2.0f;
  }
  return f * (1.0f / 1.9375f);
}

__DEVICE__ float Fbmn (float3 p, float3 n)
{
  float3 s;
  float a;
  s = to_float3_s (0.0f);
  a = 1.0f;
  for (int j = 0; j < 5; j ++) {
    s += a * to_float3 (Noisefv2 (swi2(p,y,z)), Noisefv2 (swi2(p,z,x)), Noisefv2 (swi2(p,x,y)));
    a *= 0.5f;
    p *= 2.0f;
  }
  return dot (s, abs_f3 (n));
}

__DEVICE__ float3 VaryNf (float3 p, float3 n, float f)
{
  
  float v[4];
  float3 g;
  float2 e;
  e = to_float2 (0.1f, 0.0f);
  for (int j = VAR_ZERO; j < 4; j ++)
     v[j] = Fbmn (p + ((j < 2) ? ((j == 0) ? swi3(e,x,y,y) : swi3(e,y,x,y)) : ((j == 2) ? swi3(e,y,y,x) : swi3(e,y,y,y))), n);
  //g = swi3(v,x,y,z) - v.w;
  g = to_float3(v[0],v[1],v[2]) - v[3];
  return normalize (n + f * (g - n * dot (n, g)));
}

/*
#define txBuf iChannel0
#define txSize iChannelResolution[0].xy

//const float txRow = 128.0f;

__DEVICE__ float4 Loadv4 (int idVar)
{
  float fi;
  fi = float (idVar);
  return texture (txBuf, (to_float2 (mod_f (fi, txRow), _floor (fi / txRow)) + 0.5f) / txSize);
}
*/

//const int npTrail = 32;  // same for image and buffer
//const int npFst = 7;

/*
__DEVICE__ float4 carPos, wlRot;
__DEVICE__ float3 sunDir, wlBase, carSz;//, bldSz;   //qHit
__DEVICE__ float2 axRot[6], wlRotCs[6], wlAngCs[6], mOff; //gId
__DEVICE__ float dstFar, tCur, wlRad, strRot, spd, onPath, gSize, rdWid;//, trHt;
*/

//int idObj;
__DEVICE__ bool isSh, isTran, inCab;
//__DEVICE__const int idBod = 1, idAx = 2, idWhl = 3, idStr = 4, idSeat = 5, idStk = 6, idWLit = 7, idLit = 8, idCam = 9, idBld = 10, idTree = 11;
#define idBod   1
#define idAx    2
#define idWhl   3
#define idStr   4
#define idSeat  5
#define idStk   6
#define idWLit  7
#define idLit   8
#define idCam   9
#define idBld  10
#define idTree 11

//const float pi = 3.1415927f;

#define DMINQ(id) if (d < dMin) { dMin = d;  *idObj = id;  *qHit = q; }

__DEVICE__ float ObjDf (float3 p, float3 *qHit, int *idObj, bool isSh,
                        float4 carPos, float dstFar, float3 carSz, float3 wlBase, float wlRad, float spd, float2 axRot[6], float2 wlRotCs[6], float2 wlAngCs[6], float strRot )
{
  
  float3 q, qa;
  float dMin, d, r, db, dw;
  p -= swi3(carPos,x,y,z);
  swi2S(p,x,z, Rot2D (swi2(p,x,z), carPos.w));
  dMin = dstFar;
  if (! isSh) d = PrRoundBoxDf (p, to_float3 (carSz.x, carSz.y + 1.2f, carSz.z), 0.5f);
  if (isSh || d < 0.1f) {
    q = p;
    dw = PrRoundCylDf (swi3(to_float3 (_fabs (q.x) - wlBase.x, q.y,
         wlBase.z * (fract (q.z / wlBase.z + 0.5f) - 0.5f)),y,z,x), wlRad + 0.02f, 0.05f, 0.35f);
    q.y -= 1.1f;
    db = PrRound4BoxDf (q, carSz - 1.0f, 1.0f);
    d = _fminf (_fabs (db) - 0.05f, _fmaxf (db, dw - 0.05f));
    d = SmoothMax (d, - _fmaxf (_fabs (abs (q.z) - wlBase.z - wlRad - 0.2f) - 0.01f,
        0.02f - _fabs (db)), 0.02f);
    d = SmoothMax (d, - _fminf (min (PrRoundBox2Df (to_float2 (q.y - 0.3f, _fabs (q.z) - 1.05f),
       to_float2 (0.3f, 0.75f), 0.2f), PrRoundBox2Df (to_float2 (_fabs (q.x) - 0.6f, q.y - 0.3f),
       to_float2 (0.3f, 0.25f), 0.2f)), _fmaxf (PrRoundBox2Df (to_float2 (q.x, q.z - 0.8f),
       to_float2 (0.8f, 0.5f), 0.2f), - q.y)), 0.02f);
    d = _fmaxf (d, - _fmaxf (dw, _fmaxf (_fabs (q.z) - wlBase.z - (wlRad + 0.1f),
       wlBase.x - 0.7f - _fabs (q.x))));
    DMINQ (idBod);
    q = p;
    swi2S(q,y,z, swi2(q,y,z) - to_float2 (carSz.y + 1.33f + 0.05f, -1.0f));
    d = PrRoundCylDf (swi3(q,x,z,y), 0.15f, 0.05f, 0.22f);
    q.y -= _fmaxf (0.5f * spd, 0.0f) - 0.05f;
    d = _fminf (d, PrRoundCylDf (swi3(q,x,z,y), 0.6f, 0.15f, 0.03f));
    DMINQ (idCam);
    q = p;
    swi2S(q,x,z, abs_f2(swi2(q,x,z)) - swi2(wlBase,x,z) - to_float2 (-0.2f, -0.5f * wlBase.z));
    q.z = _fabs (q.z) - 0.5f * wlBase.z;
    d = PrCylDf (swi3(q,y,z,x), 0.1f, 0.2f);
    DMINQ (idAx);
    for (int k = VAR_ZERO; k < 6; k ++) {
      q = p;
      swi2S(q,x,z, Rot2Cs (swi2(q,x,z) - axRot[k] * swi2(wlBase,x,z), wlRotCs[k]));
      r = wlRad - length (swi2(q,y,z));
      swi2S(q,y,z, Rot2Cs (swi2(q,y,z), wlAngCs[k]));
      qa = abs_f3 (q);
      d = _fminf (_fmaxf (length (_fmaxf (to_float2 (0.1f - r, qa.x - 0.1f), to_float2_s(0.0f))) - 0.1f, r - 0.2f),
                  _fmaxf (0.2f - r, qa.x - 0.12f));
      d = _fmaxf (d, - _fmaxf (_fabs (fract (4.0f * q.x / 0.24f) - 0.5f) - 0.2f, r - 0.05f));
      
      //d = _fminf (d, PrCylDf (to_float3(qa.x - 0.1f, ((qa.y < qa.z) ? swi2(q,y,z) : swi2(q,z,y))), 0.06f, wlRad - 0.15f));
      d = _fminf (d, PrCylDf ((qa.y < qa.z) ? to_float3(qa.x - 0.1f,q.y,q.z) : to_float3(qa.x - 0.1f,q.z,q.y), 0.06f, wlRad - 0.15f));
      q.x *= sign_f(p.x);
      DMINQ (idWhl);
    }
    q = p;
    d = PrRoundBoxDf (q - to_float3 (-0.4f, 0.5f, carSz.z - 0.4f), to_float3 (0.1f, 0.5f, 0.03f), 0.05f);
    swi2S(q,y,z, Rot2D (swi2(q,y,z) - to_float2 (0.72f, -0.2f), -0.1f * pi));
    q -= to_float3 (-0.4f, 0.9f, 1.8f);
    d = _fminf (d, PrRoundCylDf (q, 0.03f, 0.03f, 0.35f));
    q.z -= -0.35f;
    swi2S(q,x,y, Rot2D (swi2(q,x,y), -8.0f * strRot + pi / 6.0f));
    swi2S(q,x,y, Rot2D (swi2(q,x,y), 2.0f * pi * _floor (3.0f * _atan2f (q.y, - q.x) / (2.0f * pi) + 0.5f) / 3.0f));
    d = _fminf (d, PrTorusDf (q, 0.025f, 0.35f));
    q.x += 0.17f;
    d = _fminf (d, PrCylDf (swi3(q,y,z,x), 0.02f, 0.17f));
    DMINQ (idStr);
    q = p;
    //swi2(q,y,z) -= to_float2 (0.5f, 0.8f);
    q.y -= 0.5f;
    q.z -= 0.8f;
    
    d = PrRoundBoxDf (q, to_float3 (0.98f, 0.2f, 0.35f) - 0.05f, 0.05f);
    swi2S(q,y,z, Rot2D (swi2(q,y,z) - to_float2 (0.5f, -0.45f), 0.1f * pi));
    q.x = _fabs (q.x) - 0.49f;
    d = _fminf (d, PrRoundBoxDf (q, to_float3 (0.48f, 0.35f, 0.1f) - 0.05f, 0.05f));
    DMINQ (idSeat);
    q = p;
    //swi2(q,y,z) -= to_float2 (0.6f, -1.0f);
    q.y -=  0.6f;
    q.z -= -1.0f;
    
    d = PrRoundBoxDf (q, to_float3 (0.98f, 0.2f, 0.35f) - 0.05f, 0.05f);
    swi2S(q,y,z, Rot2D (swi2(q,y,z) - to_float2 (0.5f, -0.45f), 0.1f * pi));
    q.x = _fabs (q.x) - 0.49f;
    d = _fminf (d, PrRoundBoxDf (q, to_float3 (0.48f, 0.35f, 0.1f) - 0.05f, 0.05f));
    DMINQ (idSeat);
    q = p;
    q -= to_float3 (0.1f, -0.1f, 1.5f);
    swi2S(q,y,z, Rot2D (swi2(q,y,z), pi * (0.02f + 0.06f * sign_f (spd))));
    q.y -= 0.5f;
    d = PrCapsDf (swi3(q,x,z,y), 0.04f, 0.5f);
    DMINQ (idStk);
    q = p;
    //swi2(q,y,z) -= to_float2 (1.1f, 2.4f);
    q.y -= 1.1f;
    q.z -= 2.4f;
    
    d = PrCapsDf (swi3(q,x,z,y), 0.06f, 0.05f);
    DMINQ (idWLit);
    q = p;
    d = PrCapsDf (swi3((swi3(to_float3_aw(abs_f2 (swi2(q,x,z)), q.y),x,z,y) - to_float3(0.7f, 0.4f, carSz.z + 0.02f)),y,z,x), 0.05f, 0.3f);
    DMINQ (idLit);
  } else dMin = _fminf (dMin, d);

  return dMin;
}

__DEVICE__ float ObjRay (float3 ro, float3 rd, float3 *qHit, int *idObj, bool isSh,
                         float4 carPos, float dstFar, float3 carSz, float3 wlBase, float wlRad, float spd, float2 axRot[6], float2 wlRotCs[6], float2 wlAngCs[6], float strRot)
{
  float3 p;
  float dHit, d;
  dHit = 0.0f;
  for (int j = VAR_ZERO; j < 160; j ++) {
    p = ro + dHit * rd;
    
    d = ObjDf (p, qHit, idObj, isSh, carPos, dstFar,carSz,wlBase,wlRad,spd,axRot,wlRotCs,wlAngCs,strRot);
    dHit += d;
    if (d < 0.001f || dHit > dstFar || p.y < 0.0f) break;
  }
  if (p.y < 0.0f) dHit = dstFar;
  return dHit;
}

__DEVICE__ float3 ObjNf (float3 p, float3 *qHit, int *idObj, bool isSh,
                         float4 carPos, float dstFar, float3 carSz, float3 wlBase, float wlRad, float spd, float2 axRot[6], float2 wlRotCs[6], float2 wlAngCs[6], float strRot)
{
  float v[4];
  float2 e;
  e = to_float2 (0.001f, -0.001f);
  for (int j = VAR_ZERO; j < 4; j ++) {
    v[j] = ObjDf (p + ((j < 2) ? ((j == 0) ? swi3(e,x,x,x) : swi3(e,x,y,y)) : ((j == 2) ? swi3(e,y,x,y) : swi3(e,y,y,x))),qHit,idObj, isSh, carPos, dstFar,carSz,wlBase,wlRad,spd,axRot,wlRotCs,wlAngCs,strRot);
  }
  v[0] = - v[0];
  float4 _v = to_float4(v[0],v[1],v[2],v[3]);
  return normalize (2.0f * swi3(_v,y,z,w) - dot (_v, to_float4_s (1.0f)));
}

__DEVICE__ float TrObjDf (float3 p, float4 carPos, float3 carSz)
{
  
  float3 q;
  float d;
  p -= swi3(carPos,x,y,z);
  swi2S(p,x,z, Rot2D (swi2(p,x,z), carPos.w));
  q = p;
  q.y -= 1.1f;
  return _fmaxf (PrRound4BoxDf (q, carSz - 1.0f, 1.0f), -0.5f - q.y);
}

__DEVICE__ float TrObjRay (float3 ro, float3 rd, float4 carPos, float3 carSz, float dstFar)
{
  float3 p;
  float dHit, d;
  dHit = 0.0f;
  for (int j = VAR_ZERO; j < 60; j ++) {
    p = ro + dHit * rd;
    d = TrObjDf (p, carPos,carSz);
    dHit += d;
    if (d < 0.001f || dHit > dstFar || p.y < 0.0f) break;
  }
  if (p.y < 0.0f) dHit = dstFar;
  return dHit;
}

__DEVICE__ float3 TrObjNf (float3 p, float4 carPos, float3 carSz)
{
  float v[4];
  float2 e;
  e = to_float2 (0.001f, -0.001f);
  for (int j = VAR_ZERO; j < 4; j ++) {
    v[j] = TrObjDf (p + ((j < 2) ? ((j == 0) ? swi3(e,x,x,x) : swi3(e,x,y,y)) : ((j == 2) ? swi3(e,y,x,y) : swi3(e,y,y,x))),carPos,carSz);
  }
  
  v[0] = - v[0];
  float4 _v = to_float4(v[0],v[1],v[2],v[3]);
  return normalize (2.0f * swi3(_v,y,z,w) - dot (_v, to_float4_s (1.0f)));
}

__DEVICE__ float ObjSShadow (float3 ro, float3 rd, inout float3 *qHit, inout int *idObj, inout bool *isSh,
                             float4 carPos, float dstFar, float3 carSz, float3 wlBase, float wlRad, float spd, float2 axRot[6], float2 wlRotCs[6], float2 wlAngCs[6], float strRot)
{
  
  float sh, d, h;
  int idObjT;
  idObjT = *idObj;
  sh = 1.0f;
  d = 0.02f;
  *isSh = true;
  for (int j = VAR_ZERO; j < 30; j ++) {
    h = ObjDf (ro + d * rd,qHit,idObj, isSh,carPos,dstFar,carSz,wlBase,wlRad,spd,axRot,wlRotCs,wlAngCs,strRot);
    sh = _fminf (sh, smoothstep (0.0f, 0.1f * d, h));
    d += h;
    if (sh < 0.05f || d > dstFar) break;
  }
  *isSh = false;
  *idObj = idObjT;
  return 0.5f + 0.5f * sh;
}

__DEVICE__ void SetBldParm (inout float3 *bldSz, inout float *trHt, float2 gId, float gSize, float2 mOff)
{
  *bldSz = 4.0f * _floor (to_float3 (0.08f * gSize, 4.0f, 0.08f * gSize) * (0.4f +
           0.6f * Hashv3v2 (gId + mOff))) - 0.5f;
  *trHt = 10.0f + 2.0f * (*bldSz).y;
}

__DEVICE__ float GObjDf (float3 p, float3 *qHit, int *idObj, float3 bldSz, float trHt, float2 gId, bool isTran, float dstFar, float gSize)
{
  float3 q, qq, qa;
  float dMin, d, db;
  dMin = dstFar;
  q = p;
  swi2S(q,x,z, swi2(q,x,z) - gSize * (gId + 0.5f));
  q.y -= bldSz.y;
  db = PrBoxDf (q, bldSz);
  if (isTran) {
    d = db;
  } else {
    qq = mod_f3 (q + 2.0f, 4.0f) - 2.0f;
    qq.y -= 0.2f;
    qa = abs_f3 (q) - bldSz + 1.0f;
    d = SmoothMax (_fabs (db) - 0.1f, - _fminf (max (PrBox2Df (swi2(qq,x,y), to_float2 (1.3f, 1.0f)), Maxv2 (swi2(qa,x,y))),
       _fmaxf (PrBox2Df (swi2(qq,z,y), to_float2 (1.3f, 1.0f)), Maxv2 (swi2(qa,z,y)))), 0.05f);
    d = _fminf (d, _fmaxf (_fabs (qq.y - 1.1f) - 0.05f, db));
  }
  DMINQ (idBld);
  if (! isTran) {
    q.y -= - bldSz.y;
    q = to_float3_aw (abs_f2 (swi2(q,x,z)) - swi2(bldSz,x,z) - 4.0f, q.y - 0.1f * trHt);
    d = trHt * PrConCapsDf (q / trHt, sin_f2 (0.06f * pi + to_float2 (0.5f * pi, 0.0f)), 0.03f, 0.06f);
    DMINQ (idTree);
  }
  return dMin;
}

__DEVICE__ float GObjRay (float3 ro, float3 rd, float3 *qHit, int *idObj, inout float3 *bldSz, inout float *trHt, inout float2 *gId, float gSize, float2 mOff, bool isTran, float dstFar)
{
  float3 p, rdi;
  float2 gIdP;
  float dHit, d, eps;
  eps = 0.01f;
  if (rd.x == 0.0f) rd.x = 0.0001f;
  if (rd.z == 0.0f) rd.z = 0.0001f;
  swi2S(rdi,x,z, 1.0f / swi2(rd,x,z));
  gIdP = to_float2_s (-999.0f);
  dHit = eps;
  for (int j = VAR_ZERO; j < 120; j ++) {
    p = ro + dHit * rd;
    //swi2(p,x,z) -= 0.5f * gSize;
    p.x -= 0.5f * gSize;
    p.z -= 0.5f * gSize;
    
    *gId = _floor (swi2(p,x,z) / gSize);
    if ((*gId).x != gIdP.x || (*gId).y != gIdP.y ) {
      gIdP = *gId;
      SetBldParm (bldSz,trHt,*gId,gSize,mOff);
    }

    d = GObjDf (p,qHit,idObj,*bldSz,*trHt,*gId,isTran,dstFar,gSize);
    dHit += _fminf (d, eps + _fmaxf (0.0f, Minv2 ((gSize * (*gId + step (to_float2_s(0.0f), swi2(rd,x,z))) - swi2(p,x,z)) * swi2(rdi,x,z))));
    if (d < eps || dHit > dstFar || p.y < 0.0f) break;
  }
  if (d >= eps || p.y < 0.0f) dHit = dstFar;
  return dHit;
}

__DEVICE__ float3 GObjNf (float3 p, float3 *qHit, int *idObj, float3 bldSz, float trHt, float2 gId, bool isTran, float gSize, float dstFar)
{
  float v[4];
  float2 e = to_float2 (0.001f, -0.001f);
  swi2S(p,x,z, swi2(p,x,z) - 0.5f * gSize);
  for (int j = VAR_ZERO; j < 4; j ++) {
    v[j] = GObjDf (p + ((j < 2) ? ((j == 0) ? swi3(e,x,x,x) : swi3(e,x,y,y)) : ((j == 2) ? swi3(e,y,x,y) : swi3(e,y,y,x))),qHit,idObj,bldSz,trHt,gId,isTran,dstFar,gSize);
  }
  v[0] = - v[0];
  float4 _v = to_float4(v[0],v[1],v[2],v[3]);
  return normalize (2.0f * swi3(_v,y,z,w) - dot (_v, to_float4_s (1.0f)));
}

__DEVICE__ float GObjSShadow (float3 ro, float3 rd, float3 *qHit, int *idObj, inout float3 *bldSz, inout float *trHt, inout float2 *gId, float gSize, float2 mOff, bool isTran, float dstFar)
{
  float3 p;
  float2 gIdP;
  float sh, d, h;
  int idObjT;
  idObjT = *idObj;
  sh = 1.0f;
  gIdP = to_float2_s (-999.0f);
  d = 0.01f;
  for (int j = VAR_ZERO; j < 30; j ++) {
    p = ro + d * rd;
    swi2S(p,x,z, swi2(p,x,z) - 0.5f * gSize);
    *gId = _floor (swi2(p,x,z) / gSize);
    if ((*gId).x != gIdP.x || (*gId).y != gIdP.y) {
      gIdP = *gId;
      SetBldParm (bldSz,trHt,*gId,gSize,mOff);
    }
    h = GObjDf (p, qHit,idObj, *bldSz,*trHt,*gId,isTran,dstFar,gSize);
    sh = _fminf (sh, smoothstep (0.0f, 0.1f * d, h));
    d += _fmaxf (h, 0.01f);
    if (h < 0.001f || d > dstFar) break;
  }
  *idObj = idObjT;
  return 0.5f + 0.5f * sh;
}

__DEVICE__ float TrailShd (float2 p, float2 R, __TEXTURE2D__ iChannel0, float4 carPos, float3 wlBase, float tCur)
{
  float4 u;
  float2 gB[2], gF[2], dg, q;
  float st, s, gLen;
  st = 1.0f;
  for (float kz = -1.0f + (float) (VAR_ZERO); kz <= 1.0f; kz ++) {
    gB[0] = swi2(carPos,x,z);
    gF[0] = gB[0] + Rot2D (to_float2 (0.0f, kz * wlBase.z), - carPos.w);
    for (int j = VAR_ZERO; j < npTrail; j ++) {
      gB[1] = gB[0];
      gF[1] = gF[0];
      u = Loadv4 (npFst + j,R,iChannel0);
      if (u.z >= 0.0f) {
        gB[0] = swi2(u,x,y);
        gF[0] = gB[0] + Rot2D (to_float2 (0.0f, kz * wlBase.z), - u.z);
        s = 1.0f;
        if (kz == 0.0f) {
          dg = gF[0] - gF[1];
          gLen = length (dg);
          if (gLen > 0.0f) {
            q = Rot2Cs (p - 0.5f * (gF[0] + gF[1]), swi2(dg,y,x) / gLen);
            q.x = _fabs (q.x) - wlBase.x;
            s = _fminf (s, PrRoundBox2Df (q, to_float2 (0.1f, 0.5f * gLen), 0.05f));
          }
        } else {
          for (float k = -1.0f; k <= 1.0f; k += 2.0f) {
            dg = gF[0] - gF[1];
            gLen = length (dg);
            if (gLen > 0.0f) {
              q = Rot2Cs (p - 0.5f * (gF[0] + gF[1]) + Rot2D (to_float2 (k * wlBase.x, 0.0f), - u.z),
                  swi2(dg,y,x) / gLen);
              s = _fminf (s, PrRoundBox2Df (q, to_float2 (0.1f, 0.5f * gLen), 0.05f));
            }
          }
        }
        st = _fminf (st, 1.0f - 0.3f * _sqrtf (1.0f - _fminf (1.0f, ((float) (j) + 20.0f * (tCur - u.w)) /
             (float) (npTrail))) * (1.0f - smoothstep (0.0f, 0.05f, s)));
        st = _fminf (st, 1.0f - 0.3f * _sqrtf (1.0f - (float) (j) / (float) (npTrail)) *
           (1.0f - smoothstep (0.0f, 0.05f, s)));
      } else break;
    }
  }
  return st;
}

__DEVICE__ float3 GrndNf (float2 p)
{
  float2 e;
  e = to_float2 (0.01f, 0.0f);
  p *= 0.5f;
  
  return (swi3(normalize (to_float3_aw(Fbm2 (p) - to_float2 (Fbm2 (p + swi2(e,x,y)), Fbm2 (p + swi2(e,y,x))), 8.0f * e.x)),x,z,y));
}

__DEVICE__ float PathEdge (float2 p, float dw, float gSize, float rdWid)
{
  return PrRoundBox2Df (mod_f2(p + 0.5f * gSize, gSize) - 0.5f * gSize,
                        to_float2_s (0.5f * gSize) - 2.0f * rdWid - dw, rdWid);
}

__DEVICE__ float GrndWhlShd (float2 p, float dstFar, float4 carPos, float3 wlBase)
{
  float d;
  d = dstFar;
  for (int k = 0; k < 6; k ++) d = _fminf (d, length (swi2(carPos,x,z) -
     Rot2D ((to_float2 (2.0f * mod_f ((float) (k), 2.0f), (float) (k / 2)) - 1.0f) * swi2(wlBase,x,z), - carPos.w) - p));
  return 0.7f + 0.3f * smoothstep (0.2f, 0.3f, d);
}

__DEVICE__ float3 GrndCol (float2 p, float dstGrnd, float sh,float2 R, __TEXTURE2D__ iChannel0, inout float3 *bldSz, inout float *trHt, inout float2 *gId, float gSize, float2 mOff, float dstFar,
                           float4 carPos, float3 wlBase, float tCur, float rdWid, float3 sunDir )
{
  float3 col, colG, vn;
  float2 vf, q;
  float f, st;
  colG = 0.4f * _mix (to_float3 (0.8f, 1.0f, 0.5f), to_float3 (0.7f, 0.9f, 0.5f), 0.2f +
         0.6f * smoothstep (0.3f, 0.7f, Fbm2 (0.5f * p)));
  vf = to_float2_s (0.0f);
  if (PathEdge (p, 0.4f,gSize,rdWid) > 0.0f) {
    f = smoothstep (0.0f, 0.1f, _fabs (PathEdge (p, 0.2f,gSize,rdWid)) - 0.2f);
    col = _mix (to_float3 (0.4f, 0.4f, 0.1f), to_float3_s (0.3f), f);
    vn = to_float3 (0.0f, 1.0f, 0.0f);
    vf = to_float2 (4.0f, 0.2f + 0.5f * f);
  } else {
    q = p - 0.5f * gSize;
    *gId = _floor (q / gSize);
    SetBldParm (bldSz,trHt,*gId,gSize,mOff);
    q -= gSize * (*gId + 0.5f);
    if (PrRoundBox2Df (q, swi2(*bldSz,x,z) + 2.0f, 0.5f) < 0.0f) {
      col = to_float3_s (0.25f);
      vn = to_float3 (0.0f, 1.0f, 0.0f);
      vf = to_float2 (4.0f, 1.0f);
    } else {
      col = colG;
      vn = GrndNf (p);
    }
  }
  
  col *= GrndWhlShd (p,dstFar,carPos,wlBase);
  st = TrailShd (p,R,iChannel0,carPos,wlBase,tCur);
  vf *= 1.0f + 2.0f * step (st, 0.99f);
  vf.y *= 1.0f - smoothstep (0.5f, 0.8f, dstGrnd / dstFar);
  if (vf.y > 0.0f) vn = VaryNf (vf.x * to_float3 (p.x, 0.0f, p.y), vn, vf.y);
  col *= (0.2f + 0.8f * sh * _fmaxf (dot (vn, sunDir), 0.0f)) * (0.5f + 0.5f * st);
  return col;
}

__DEVICE__ float3 SkyBgCol (float3 ro, float3 rd, float tCur, float3 sunDir)
{
  
  float3 col, clCol, skCol;
  float2 q;
  float f, fd, ff, sd;
  if (rd.y > -0.02f && rd.y < 0.03f * Fbm1 (16.0f * _atan2f (rd.z, - rd.x))) {
    col = to_float3 (0.3f, 0.4f, 0.5f);
  } else {
    q = 0.01f * (swi2(ro,x,z) + 2.0f * tCur + ((100.0f - ro.y) / rd.y) * swi2(rd,x,z));
    ff = Fbm2 (q);
    f = smoothstep (0.2f, 0.8f, ff);
    fd = smoothstep (0.2f, 0.8f, Fbm2 (q + 0.01f * swi2(sunDir,x,z))) - f;
    clCol = (0.7f + 0.5f * ff) * (to_float3_s (0.7f) - 0.7f * to_float3 (0.3f, 0.3f, 0.2f) * sign_f (fd) *
            smoothstep (0.0f, 0.05f, _fabs (fd)));
    sd = _fmaxf (dot (rd, sunDir), 0.0f);
    skCol = to_float3 (0.3f, 0.4f, 0.8f) + step (0.1f, sd) * to_float3 (1.0f, 1.0f, 0.9f) *
            _fminf (0.3f * _powf (sd, 64.0f) + 0.5f * _powf (sd, 2048.0f), 1.0f);
    col = _mix (skCol, clCol, 0.1f + 0.9f * f * smoothstep (0.01f, 0.1f, rd.y));
  }
  return col;
}

__DEVICE__ float4 CarCol (out float *refFac, inout float3 *qHit, inout int *idObj, bool inCab, float3 wlBase, float wlRad, float onPath, float strRot, float tCur)
{
  float4 col4, colB4;
  float r;
  colB4 = to_float4 (0.95f, 0.9f, 0.9f, 0.2f);
  col4 = colB4;
  if (*idObj == idBod) {
    if (! inCab) {
      if (_fabs ((*qHit).z) < wlBase.z + wlRad + 0.2f) swi3S(col4,x,y,z, swi3(col4,x,y,z) * 0.7f + 0.3f *
                                                       smoothstep (0.0f, 0.02f, _fabs (length (to_float2 ((*qHit).y + 1.1f, wlBase.z *
                                                       (fract ((*qHit).z / wlBase.z + 0.5f) - 0.5f))) - wlRad - 0.14f) - 0.02f));
      swi3S(col4,x,y,z, swi3(col4,x,y,z) * 0.7f + 0.3f * smoothstep (0.0f, 0.02f, PrRoundBox2Df (to_float2 ((*qHit).y - 0.3f,
                       _fabs ((*qHit).z) - 1.05f), to_float2 (0.3f, 0.75f), 0.2f) - 0.04f));
      swi3S(col4,x,y,z, swi3(col4,x,y,z) * 0.7f + 0.3f * smoothstep (0.0f, 0.02f, PrRoundBox2Df (to_float2 (_fabs ((*qHit).x) - 0.6f,
                       (*qHit).y - 0.3f), to_float2 (0.3f, 0.25f), 0.2f) - 0.04f));
      if ((*qHit).y > 0.0f) swi3S(col4,x,y,z, swi3(col4,x,y,z) * 0.7f + 0.3f * smoothstep (0.0f, 0.02f,
                                              PrRoundBox2Df (to_float2 ((*qHit).x, (*qHit).z - 0.8f), to_float2 (0.8f, 0.5f), 0.2f) - 0.04f));
    } else {
      col4 *= 0.5f;
    }
  } else if (*idObj == idCam) {
    swi3S(col4,x,y,z, swi3(col4,x,y,z) * 0.1f + 0.9f * smoothstep (0.0f, 0.02f, length (to_float2 ((*qHit).y, dot (swi2(*qHit,z,x),
                      sin_f2 (2.0f * pi * _floor (12.0f * _atan2f ((*qHit).z, - (*qHit).x) / (2.0f * pi) + 0.5f) / 12.0f +
                      to_float2 (0.5f * pi, 0.0f))))) - 0.12f))  
  } else if (*idObj == idAx) {
    col4 = to_float4 (0.3f, 0.3f, 0.4f, 0.1f);
  } else if (*idObj == idWhl) {
    r = wlRad - length (swi2(*qHit,y,z));
    if (r < 0.17f) {
      col4 = to_float4 (0.3f, 0.3f, 0.3f, 0.0f);
      if (r < 0.07f) col4 *= 1.0f - 0.5f * _fabs (step (0.0f, _cosf (32.0f * pi * (*qHit).x)) -
         step (0.5f, mod_f (32.0f * _atan2f ((*qHit).z, - (*qHit).y) / (2.0f * pi) + 0.5f, 1.0f)));
    } else if (r < 0.2f || (*qHit).x < 0.0f) {
      col4 *= 0.5f;
    }
  } else if (*idObj == idStr) {
    col4 = to_float4 (0.9f, 0.9f, 0.7f, 0.2f);
  } else if (*idObj == idStk) {
    col4 = to_float4 (0.9f, 0.9f, 0.7f, 0.2f);
  } else if (*idObj == idSeat) {
    col4 = to_float4 (0.9f, 0.7f, 0.4f, 0.05f) * (0.95f + 0.05f * _cosf (64.0f * (*qHit).x));
  } else if (*idObj == idWLit) {
    col4 = (onPath > 0.0f) ? to_float4 (0.0f, 0.8f, 0.0f, -1.0f) : to_float4 (0.8f, 0.0f, 0.0f, -1.0f);
  } else if (*idObj == idLit) {
    if (_fabs (strRot) > 0.03f * pi && strRot * (*qHit).x < 0.0f && _fabs ((*qHit).x) > 0.95f &&
       mod_f (2.0f * tCur, 1.0f) > 0.5f) col4 = to_float4 (0.7f, 0.7f, 0.0f, -1.0f);
    else col4 = ((*qHit).z > 0.0f) ? to_float4 (1.0f, 1.0f, 0.95f, -1.0f) : to_float4 (0.9f, 0.0f, 0.0f, -1.0f);
    swi3S(col4,x,y,z, swi3(col4,x,y,z) * 0.8f + 0.2f * smoothstep (0.0f, 0.02f, _fabs (abs ((*qHit).x) - 0.95f)))
  }
  *refFac = 0.0f;
  //if (col4 == colB4) *refFac = 0.3f;
  if (col4.x==colB4.x && col4.y==colB4.y && col4.z==colB4.z && col4.w==colB4.w) *refFac = 0.3f;

  return col4;
}

__DEVICE__ float3 HorizCol (float3 col, float3 rd)
{
  return _mix (col, to_float3 (0.3f, 0.4f, 0.5f), _powf (1.0f + rd.y, 16.0f));
}

__DEVICE__ float4 BldCol (inout float3 *vn, inout float3 *qHit, inout int *idObj, float3 bldSz, float2 gId, float2 mOff)
{
  
  float4 col4;
  float2 g, vf;
  float h, s, bn;
  h = mod_f (dot (gId + mOff, to_float2 (17.11f, 21.11f)), 1.0f);
  vf = to_float2_s (0.0f);
  if (*idObj == idBld) {
    col4 = to_float4_aw (HsvToRgb (to_float3 (h, 0.4f, 0.8f)), 0.1f);
    if (PrBoxDf (*qHit, bldSz) < 0.0f) {
      col4 *= 0.3f;
      if ((*vn).y < -0.95f) col4 = _mix (to_float4 (1.0f, 1.0f, 0.5f, -1.0f), col4, 
         step (0.5f, length (mod_f (swi2(*qHit,x,z) + 2.0f, 4.0f) - 2.0f)));
    } else {
      s = 0.0f;
      g = to_float2 (dot (swi2(*qHit,x,z), normalize (to_float2 (- (*vn).z, (*vn).x))) - 1.3f, (*qHit).y + bldSz.y - 0.8f);
      if (Maxv2 (abs_f2 (swi2(*vn,x,z))) > 0.99f && PrBox2Df (g, to_float2 (3.0f, 1.0f)) < 0.1f) {
        bn = dot (mod_f (gId + 31.0f + _floor (0.4f * mOff), 100.0f), to_float2 (100.0f, 1.0f));
        s = ShowIntPZ (g, to_float2 (2.4f, 0.6f), 4.0f, bn);
        if (s > 0.0f) col4 = to_float4 (1.0f, 1.0f, 0.8f, -1.0f);
      }
      if (s == 0.0f) vf = to_float2 (16.0f, 0.5f);
    }
  } else if (*idObj == idTree) {
    col4 = to_float4_aw (HsvToRgb (to_float3 (mod_f (0.25f + 0.2f * h, 1.0f), 0.6f, 0.9f)), 0.0f);
    vf = to_float2 (32.0f, 1.0f);
  }
  if (vf.y > 0.0f) *vn = VaryNf (vf.x * *qHit, *vn, vf.y);
  return col4;
}

__DEVICE__ float3 ShowScene (float3 ro, float3 rd, float3 *qHit, int *idObj, float2 R, __TEXTURE2D__ iChannel0,
                             float4 carPos, float dstFar, float3 wlBase, float wlRad, float spd, float strRot, float gSize, float2 mOff, float tCur, float rdWid, float3 sunDir, float onPath, float4 wlRot)
{
  
 
  float4 col4;
  float3 roo, rdo, col, colR, vn, qHitG, q;
  float dstObjG, dstObj, dstTrObj, dstGrnd=0.0f, nDotL, refFac=0.0f, sh;
  int idObjG;
  
  float3 bldSz;
  float trHt;
  float2 gId;
  
  float2 axRot[6], wlRotCs[6], wlAngCs[6];
  
  roo = ro;
  rdo = rd;
  bool isSh = false;
  bool isTran = false;
  bool inCab  = false;
  
  float3 carSz = to_float3 (wlBase.x + 0.05f, 1.2f, wlBase.z + 1.0f);
  for (int k = VAR_ZERO; k < 6; k ++) {
    axRot[k] = to_float2 (2.0f * mod_f ((float) (k), 2.0f), (float) (k / 2)) - 1.0f;
    wlRotCs[k] = sin_f2 (- strRot * axRot[k].y + to_float2 (0.5f * pi, 0.0f));
    wlAngCs[k] = sin_f2 (- ((axRot[k].y != 1.0f) ? ((axRot[k].x < 0.0f) ? wlRot.x : wlRot.y) :
                           ((axRot[k].x < 0.0f) ? wlRot.z : wlRot.w)) + to_float2 (0.5f * pi, 0.0f));
  }
  dstObjG = GObjRay (ro, rd,qHit,idObj,&bldSz,&trHt,&gId,gSize,mOff,isTran,dstFar);
  idObjG = *idObj;
  qHitG = *qHit;
  dstObj = ObjRay (ro, rd,qHit,idObj,isSh,carPos,dstFar,carSz,wlBase,wlRad,spd,axRot,wlRotCs,wlAngCs,strRot);
  if (dstObjG < _fminf (dstObj, dstFar)) {
    *idObj = idObjG;
    *qHit = qHitG;
    dstObj = dstObjG;
  }
  if (dstObj < dstFar) {
    ro += dstObj * rd;
    vn = (*idObj == idObjG) ? GObjNf (ro,qHit,idObj,bldSz,trHt,gId,isTran,gSize,dstFar) : ObjNf (ro,qHit,idObj,isSh,carPos,dstFar,carSz,wlBase,wlRad,spd,axRot,wlRotCs,wlAngCs,strRot);
    if (*idObj == idBod) inCab = (PrRound4BoxDf (*qHit, carSz - 1.0f, 1.0f) < 0.03f);
    refFac = 0.0f;
      
    col4 = (*idObj == idObjG) ? BldCol (&vn,qHit,idObj,bldSz,gId,mOff) : CarCol (&refFac,qHit,idObj,inCab,wlBase,wlRad,onPath,strRot, tCur);
    if (col4.w >= 0.0f) {
      nDotL = _fmaxf (dot (vn, sunDir), 0.0f);
      if (*idObj != idObjG) {
        sh = ObjSShadow (ro + 0.01f * vn, sunDir,qHit,idObj, &isSh,carPos,dstFar,carSz,wlBase,wlRad,spd,axRot,wlRotCs,wlAngCs,strRot);
      } else {
        sh = 1.0f;
        nDotL *= nDotL;
      }

      col = swi3(col4,x,y,z) * (0.2f + 0.8f * sh * nDotL) + col4.w * step (0.95f, sh) *
         _powf (_fmaxf (dot (sunDir, reflect (rd, vn)), 0.0f), 32.0f);
      if (*idObj == idObjG) col = _mix (col, 0.9f * HorizCol (GrndCol (swi2(ro,x,z), dstGrnd, sh,R,iChannel0,&bldSz,&trHt,&gId,gSize,mOff,dstFar,carPos,wlBase,tCur,rdWid,sunDir),
         rd), smoothstep (0.85f, 0.95f, dstObj / dstFar));
      if (refFac > 0.0f) rd = reflect (rd, vn);
    } else if (col4.w == -1.0f) {
      col = swi3(col4,x,y,z) * (0.5f - 0.5f * dot (rd, vn));
    }
  }
  if (dstObj >= dstFar || refFac > 0.0f) {
    if (rd.y < 0.0f) {
      dstGrnd = - ro.y / rd.y;
      ro += dstGrnd * rd;
  
      sh = (dstGrnd < dstFar) ? _fminf (ObjSShadow (ro + 0.01f * to_float3 (0.0f, 1.0f, 0.0f), sunDir,qHit,idObj, &isSh,carPos,dstFar,carSz,wlBase,wlRad,spd,axRot,wlRotCs,wlAngCs,strRot),
                                        GObjSShadow (ro + 0.01f * to_float3 (0.0f, 1.0f, 0.0f), sunDir,qHit,idObj,&bldSz,&trHt,&gId,gSize,mOff,isTran,dstFar)) : 1.0f;
      colR = HorizCol (GrndCol (swi2(ro,x,z), dstGrnd, sh,R,iChannel0,&bldSz,&trHt,&gId,gSize,mOff,dstFar,carPos,wlBase,tCur,rdWid,sunDir), rd);
    } else {
      colR = SkyBgCol (ro, rd, tCur,sunDir);
    }
    col = (refFac > 0.0f) ? _mix (col, 0.9f * colR, refFac) : colR;
  }
  dstTrObj = TrObjRay (roo, rdo,carPos,carSz,dstFar);
  if (dstTrObj < _fminf (dstObj, dstFar)) {
    ro = roo + dstTrObj * rdo;
    vn = TrObjNf (ro,carPos,carSz);
    col *= to_float3 (0.9f, 1.0f, 0.9f);
    rd = reflect (rdo, vn);
    col = _mix (col, SkyBgCol (ro, rd,tCur,sunDir), 0.2f + 0.8f * _powf (1.0f - _fabs (dot (vn, rd)), 5.0f));
  }
  
  isTran = true;
  dstObjG = GObjRay (roo, rdo,qHit,idObj,&bldSz,&trHt,&gId,gSize,mOff,isTran,dstFar);
  if (dstObjG < _fminf (dstObj, dstFar)) {
    ro = roo + dstObjG * rdo;
    vn = GObjNf (ro,qHit,idObj,bldSz,trHt,gId,isTran,gSize,dstFar);
    rd = reflect (rdo, vn);
    col = _mix (0.8f * col, SkyBgCol (ro, rd, tCur,sunDir), 0.6f);
  }
  return clamp (col, 0.0f, 1.0f);
}

__KERNEL__ void DriveTheMapperJipi969Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, float3 iChannelResolution[], sampler2D iChannel0)
{
  
  fragCoord+=0.5f;

  float4 carPos, wlRot;
  float3 sunDir, wlBase;//, carSz;//, bldSz;   //qHit
  float2  mOff; //axRot[6], wlRotCs[6], wlAngCs[6], mOff; //gId
  float dstFar, tCur, wlRad, strRot, spd, onPath, gSize, rdWid;//, trHt;

  float3 qHit;
  int idObj;

  mat3 vuMat;
  float4 stDat;
  float3 rd, ro, col, c, wgBox;
  float2 canvas, uv, ud, cnPos;
  float el, az, asp, zmFac, trvDist, s, sr;
  canvas = iResolution;
  uv = 2.0f * fragCoord / canvas - 1.0f;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  asp = canvas.x / canvas.y;
  stDat = Loadv4 (1,R,iChannel0);
  wlBase = swi3(stDat,x,y,z);
  wlRad = wlBase.y;
  spd = stDat.w;
  stDat = Loadv4 (2,R,iChannel0);
  gSize = stDat.x;
  rdWid = stDat.y;
  onPath = stDat.z;
  trvDist = stDat.w;
  stDat = Loadv4 (3,R,iChannel0);
  az = stDat.x;
  el = stDat.y;

  cnPos = swi2(stDat,z,w);
  stDat = Loadv4 (4,R,iChannel0);
  strRot = stDat.y;
  mOff = mod_f (to_float2 (_floor (stDat.w), _floor (stDat.w / 100.0f)), 100.0f);
  stDat = Loadv4 (5,R,iChannel0);
  carPos = stDat;
  stDat = Loadv4 (6,R,iChannel0);
  wlRot = stDat;
  el = clamp (el - 0.1f * pi, -0.49f * pi, 0.02f * pi);
  vuMat = StdVuMat (el, az);
  ro = swi3(carPos,x,y,z) + mul_mat3_f3(vuMat , to_float3 (0.0f, 1.0f, -12.0f));
  ro.y += 2.0f;
  carPos.y += wlRad;
  zmFac = 3.5f + 2.0f * el;
  dstFar = 400.0f;
  sunDir = normalize (to_float3 (1.0f, 2.0f, -1.0f));
  swi2S(sunDir,x,z, Rot2D (swi2(sunDir,x,z), - az)); //mul_mat3_f3(vuMat , normalize (to_float3 (1.0f, 1.5f, -1.0f)));
#if ! AA
  const float naa = 1.0f;
#else
  const float naa = 3.0f;
#endif  
  col = to_float3_s (0.0f);
  sr = 2.0f * mod_f (dot (mod_f2 (_floor (0.5f * (uv + 1.0f) * canvas), 2.0f), to_float2_s (1.0f)), 2.0f) - 1.0f;
  for (float a = (float) (VAR_ZERO); a < naa; a ++) {
    rd = mul_mat3_f3(vuMat , normalize (to_float3_aw (uv + step (1.5f, naa) * Rot2D (to_float2 (0.5f / canvas.y, 0.0f),
         sr * (0.667f * a + 0.5f) * pi), zmFac)));
        
    col += (1.0f / naa) * ShowScene (ro, rd, &qHit, &idObj, R, iChannel0,carPos,dstFar,wlBase,wlRad,spd,strRot,gSize,mOff,tCur,rdWid,sunDir,onPath,wlRot);
  }
  
  wgBox = to_float3 (0.41f, -0.32f, 0.135f);
  ud = 0.5f * uv - swi2(wgBox,x,y) * to_float2 (asp, 1.0f);
  s = (length (ud) - wgBox.z) * canvas.y;
  col = _mix (to_float3 (0.0f, 1.0f, 1.0f), col, smoothstep (0.0f, 1.0f, _fabs (s) - 1.0f));
  if (s < 0.0f) {
  
    col = _mix (to_float3 (0.0f, 1.0f, 1.0f), col, step (1.0f, Minv2 (abs_f2 (ud)) * canvas.y));
    c = (onPath > 0.0f) ? to_float3 (0.0f, 1.0f, 0.0f) : to_float3 (0.8f, 0.0f, 0.0f);
    col = _mix (c, col, smoothstep (2.5f, 3.5f, _fabs (length (ud - cnPos) * canvas.y - 10.0f))); //Bedienring
    ud = Rot2D (ud, _atan2f (cnPos.y, - cnPos.x));
    if (ud.x < 0.0f && (length (cnPos) - length (ud)) * canvas.y > 10.0f)                       //Linie zum Bedienring
       col = _mix (c, col, smoothstep (1.5f, 2.5f, _fabs (ud.y) * canvas.y));
  }
  col = _mix (col, to_float3 (0.0f, 1.0f, 1.0f), ShowIntPZ (0.5f * uv - to_float2 (0.44f * asp, -0.15f),
              to_float2 (0.06f * asp, 0.03f), 4.0f, mod_f (_floor (trvDist / (2.0f * wlRad)), 1e4)));
  fragColor = to_float4_aw (col, 1.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}
