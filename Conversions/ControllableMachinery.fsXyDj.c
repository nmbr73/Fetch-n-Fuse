
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// "Controllable Machinery" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

__DEVICE__ float Maxv2 (float2 p)
{
  return _fmaxf (p.x, p.y);
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

#define txBuf iChannel0
//#define txSize swi2(iChannelResolution[0],x,y)
#define txSize iResolution


__DEVICE__ float4 Loadv4 (__TEXTURE2D__ txBuf, int idVar, float2 iResolution, float txRow)
{
  float fi;
  fi = (float)(idVar);
  return texture (txBuf, (to_float2 (mod_f (fi, txRow), _floor (fi / txRow)) + 0.5f) / txSize);
}

__DEVICE__ void Savev4 (int idVar, float4 val, inout float4 *fCol, float2 fCoord, float txRow)
{
  float fi;
  fi = (float)(idVar);
//  if (Maxv2 (abs_f2(fCoord - to_float2 (mod_f (fi, txRow), _floor (fi / txRow)) - 0.5f)) < 0.5f) *fCol = val;

*fCol = val;

}

#define pi 3.1415927f


//**************************************************************************************************************************************************************************
__KERNEL__ void ControllableMachineryFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

  CONNECT_SLIDER1(Speed, -10.0f, 10.0f , 0.0f);

  const float txRow = 128.0f;
  

  
  mat3 vuMat;
  float4 mPtr, stDat, wgObj;
  float3 vCon, ro;
  float2 canvas, iFrag, ud;
  float tCur, tMov, az, el, asp, zmFac, spd, cnPos, mPtrPz, s;
  int pxId, wgSel, wgReg;
  bool init;
  canvas = iResolution;
  tCur = iTime;
  mPtr = iMouse;
  swi2S(mPtr,x,y, swi2(mPtr,x,y) / canvas - 0.5f);
  iFrag = _floor (fragCoord);
  
  pxId = (int)(iFrag.x + txRow * iFrag.y);
  if (pxId >= 2) { SetFragmentShaderComputedColor(fragColor); return;} // discard;
  
  wgSel = -1;
  wgReg = -2;
  asp = canvas.x / canvas.y;
  init = (iFrame <= 2);
  if (init) {
    tMov = 0.0f;
    spd = 0.5f;
    cnPos = 0.5f;
    az = 0.0f;
    el = -0.12f * pi;
    mPtrPz = mPtr.z;
  } else {
    stDat = Loadv4 (txBuf,0,iResolution,txRow);
    tMov = stDat.x;
    cnPos = stDat.y;
    spd = stDat.z;
    tMov += 0.02f * spd;
    stDat = Loadv4 (txBuf,1,iResolution,txRow);
    az = stDat.x;
    el = stDat.y;
    mPtrPz = stDat.z;
    wgSel = int (stDat.w);
  }
  if (! init) {
    if (mPtr.z > 0.0f) {
      vuMat = StdVuMat (el, az);
      ro = mul_mat3_f3(vuMat , to_float3 (0.0f, 0.0f, -8.0f));
      ro.z += 0.9f;
      zmFac = 4.0f;
      vCon = mul_mat3_f3(vuMat , normalize (to_float3_aw(2.0f * swi2(mPtr,x,y) * to_float2 (asp, 1.0f), zmFac)));
      wgObj = to_float4 (cnPos - 0.5f, -1.12f, -0.4f, 0.08f);
      ud = (swi2(ro,x,z) + (- (ro.y - wgObj.y) / vCon.y) * swi2(vCon,x,z));
      s = Maxv2 (abs_f2(ud - swi2(wgObj,x,z))) - wgObj.w - 0.1f;
      if (s < 0.0f || wgSel == 0) {
        if (s < 0.0f && wgSel == 0) cnPos = clamp (ud.x + 0.5f, 0.0f, 1.0f);
        wgReg = 0;
      } else if (wgReg < 0) {
        az = 2.0f * pi * mPtr.x;
        el = pi * mPtr.y;
        el = clamp (el, -0.4f * pi, 0.1f * pi);
      }
      if (mPtrPz <= 0.0f) wgSel = wgReg;
    } else {
      wgSel = -1;
      wgReg = -2;
      az = _mix (az, 0.0f, 0.003f + 0.05f * step (_fabs (az), 0.15f));
      el = _mix (el, -0.12f * pi, 0.003f + 0.05f * step (_fabs (el + 0.12f * pi), 0.15f));
    }
  }
  spd = 2.0f * cnPos + Speed;
  if      (pxId == 0) stDat = to_float4 (tMov, cnPos, spd, 0.0f);
  else if (pxId == 1) stDat = to_float4 (az, el, mPtr.z, (float)(wgSel));
  Savev4 (pxId, stDat, &fragColor, fragCoord, txRow);

  SetFragmentShaderComputedColor(fragColor);
}




// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// "Controllable Machinery" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

// (Extension of "Machinery"; control widget in world space - as in "Maze Ball Solved 2")

#define AA    0  // (= 0/1) optional antialiasing

#if 0
#define VAR_ZERO _fminf (iFrame, 0)
#else
#define VAR_ZERO 0
#endif

__DEVICE__ float PrBoxDf (float3 p, float3 b)
{
  float3 d;
  d = abs_f3(p) - b;
  return _fminf (max (d.x, _fmaxf (d.y, d.z)), 0.0f) + length (_fmaxf (d, to_float3_s(0.0f)));
}

__DEVICE__ float PrRoundBoxDf (float3 p, float3 b, float r)
{
  return length (_fmaxf (abs_f3(p) - b, to_float3_s(0.0f))) - r;
}

__DEVICE__ float PrCylDf (float3 p, float r, float h)
{
  return _fmaxf (length (swi2(p,x,y)) - r, _fabs (p.z) - h);
}

__DEVICE__ float PrRoundCylDf (float3 p, float r, float rt, float h)
{
  return length (_fmaxf (to_float2 (length (swi2(p,x,y)) - r, _fabs (p.z) - h), to_float2_s(0.0f))) - rt;
}

__DEVICE__ float PrCaps2Df (float2 p, float r, float h)
{
  return length (p - to_float2 (0.0f, clamp (p.y, - h, h))) - r;
}

__DEVICE__ float Minv3 (float3 p)
{
  return _fminf (p.x, _fminf (p.y, p.z));
}

__DEVICE__ float Maxv3 (float3 p)
{
  return _fmaxf (p.x, _fmaxf (p.y, p.z));
}

__DEVICE__ float Minv2 (float2 p)
{
  return _fminf (p.x, p.y);
}

//__DEVICE__ float Maxv2 (float2 p)
//{
//  return _fmaxf (p.x, p.y);
//}

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

__DEVICE__ float SmoothBump (float lo, float hi, float w, float x)
{
  return (1.0f - smoothstep (hi - w, hi + w, x)) * smoothstep (lo - w, lo + w, x);
}

//__DEVICE__ mat3 StdVuMat (float el, float az)
//{
//  float2 ori, ca, sa;
//  ori = to_float2 (el, az);
//  ca = cos_f2 (ori);
//  sa = sin_f2 (ori);
//  return mul_mat3_mat3(to_mat3 (ca.y, 0.0f, - sa.y, 0.0f, 1.0f, 0.0f, sa.y, 0.0f, ca.y) ,
//                       to_mat3 (1.0f, 0.0f, 0.0f, 0.0f, ca.x, - sa.x, 0.0f, sa.x, ca.x));
//}

__DEVICE__ float2 Rot2D (float2 q, float a)
{
  float2 cs;
  cs = sin_f2(a + to_float2 (0.5f * pi, 0.0f));
  return to_float2 (dot (q, to_float2 (cs.x, - cs.y)), dot (swi2(q,y,x), cs));
}

__DEVICE__ float3 HsvToRgb (float3 c)
{
  return c.z * _mix(to_float3_s (1.0f), clamp (abs_f3(fract_f3(swi3(c,x,x,x) + to_float3 (1.0f, 2.0f/3.0f, 1.0f/3.0f)) * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f), c.y);
}

#define txBuf iChannel0
//#define txSize iChannelResolution[0].xy
#define txSize iResolution



//__DEVICE__ float4 Loadv4 (__TEXTURE2D__ txBuf, int idVar, float2 iResolution, float txRow)
//{
//  float fi;
//  fi = (float)(idVar);
//  return texture (txBuf, (to_float2 (mod_f (fi, txRow), _floor (fi / txRow)) + 0.5f) / txSize);
//}



//float4 wgObj;
//float3 ltDir, vnBlk;
//float2 qBlk;
//float dstFar, tCur, tMov, angRot, bEdge, tCyc, cnPos, hitBlk;
//int idObj;

//const int idGr = 1, idPln = 2, idConv = 3, idSup = 4, idAx = 5, idBas = 6,
//          idWhl = 7, idSpl = 8, idCon = 9, idBlk = 10;
#define idGr  1
#define idPln 2
#define idConv 3
#define idSup  4
#define idAx   5
#define idBas  6
#define idWhl  7
#define idSpl  8
#define idCon  9
#define idBlk  10

//const float pi = 3.1415927f;

//const float nBlk = 13.0f;
#define nBlk  13.0f

#define DMIN(id) if (d < dMin) { dMin = d;  *idObj = id; }

__DEVICE__ float GearWlDf (float3 p, float rad, float wlThk, float tWid, float nt, float aRot, 
   bool bev, float dMin)
{
  float3 q;
  float d, s;
  q = p;
  d = _fmaxf (length (swi2(q,x,y)) - rad, _fabs (q.z) - wlThk);
  if (d < dMin) {
    swi2S(q,x,y, Rot2D (swi2(q,x,y), aRot));
    swi2S(q,x,y, Rot2D (swi2(q,x,y), _floor (nt * _atan2f (q.y, - q.x) / (2.0f * pi) + 0.5f) * 2.0f * pi / nt));
    if (bev)  swi2S(q,x,y, swi2(q,x,y) * 1.2f - 0.2f * q.z / wlThk);
    s = q.x - 2.0f * clamp (1.5f * tWid + 0.5f * q.x * step (0.0f, q.x) - _fabs (q.y), 0.0f, tWid);
    d = _fmaxf (d, - rad - 0.95f * s);
  }
  return _fminf (dMin, d);
}

__DEVICE__ float4 BPos (float t, float tCyc, float bEdge)
{
  float3 p;
  float a;
  t = mod_f (t, tCyc);
  if (t < 5.0f) {
    a = 0.0f;
    p = to_float3 (-1.018f + 2.118f * t / 5.0f, bEdge, 0.0f);
  } else if (t < 10.0f) {
    a = 0.5f * pi * (t - 5.0f) / 5.0f;
    p = to_float3(1.1f, bEdge + 1.0f * _sinf (a), 1.0f - 1.0f * _cosf (a));
  } else if ( t < 15.0f) {
    a = 0.5f * pi;
    p = to_float3 (1.1f - 2.118f * (t - 10.0f) / 5.0f, 1.0f + bEdge, 1.0f);
  } else if (t < 17.5f) {
    a = 0.5f * pi;
    p = to_float3 (-1.018f, 1.0f + bEdge, 1.0f - 1.0f * (t - 15.0f) / 2.5f);
  } else {
    t -= 17.5f;
    a = -0.5f * pi * t;
    p = to_float3 (-1.018f, 1.0f + bEdge - t * t, 0.0f);
  }
  return to_float4_aw (p, a);
}

__DEVICE__ float GearDf (float3 p, float dstFar, float angRot, float bEdge)
{
  float3 q;
  float dMin, wlThk, tWid, nt, rad, gRat;
  dMin = dstFar / 0.3f;
  gRat = 2.0f;
  rad = 0.3f;
  wlThk = rad / 7.0f;
  tWid = rad / 10.0f;
  nt = 20.0f;
  q = p - to_float3 (-1.05f, -0.21f, 1.3f);
  dMin = GearWlDf (- q, rad, wlThk, tWid, nt, angRot * gRat, true, dMin);
  dMin = GearWlDf ( swi3(q - to_float3 (0.85f * rad, 0.0f, 0.85f * rad),y,z,x),
                    rad, wlThk, tWid, nt, angRot * gRat + pi / nt, true, dMin);
  rad = 0.43f;
  wlThk = rad / 15.0f;
  tWid = rad / 16.0f;
  nt = 32.0f;
  q = p -to_float3 (0.1f, 0.0f, 1.0f);
  dMin = GearWlDf (swi3(q - to_float3 (0.0f, bEdge, 0.0f),y,z,x), rad, wlThk, tWid, nt,
                   - angRot - 0.3f * pi / nt, false, dMin);
  dMin = GearWlDf (- 1.0f*swi3(q - to_float3 (0.0f, -0.21f, 0.555f),z,y,x), rad / gRat, wlThk, tWid,
                   nt / gRat, - angRot * gRat, false, dMin);
  rad = 0.32f;
  wlThk = rad / 15.0f;
  tWid = rad / 12.0f;
  nt = 24.0f;
  q = p - to_float3 (-1.05f, -0.21f, 0.6f);
  dMin = GearWlDf ((q - to_float3 (0.0f, 0.0f, 0.1f)), rad, wlThk, tWid, nt,
     angRot * gRat + pi / nt, false, dMin);
  dMin = GearWlDf ((q - to_float3 (0.0f, -0.47f, 0.1f)), rad / gRat, wlThk, tWid, nt / gRat,
     - angRot * gRat * gRat, false, dMin);
  dMin = GearWlDf ((q - to_float3 (0.0f, -0.47f, -0.1f)), rad, wlThk, tWid, nt,
     - angRot * gRat * gRat - pi / nt, false, dMin);
  dMin = GearWlDf ((q - to_float3 (0.0f, 0.0f, -0.1f)), rad / gRat, wlThk, tWid, nt / gRat,
     angRot * gRat * gRat * gRat, false, dMin);
  return dMin * 0.3f;
}

__DEVICE__ float ObjDf (float3 p, int *idObj, float dstFar, float angRot, float bEdge, float4 wgObj)
{
  float4 a4;
  float3 q, bPos;
  float dMin, d, r, a;
  dMin = dstFar;
  q = p - to_float3 (1.13f + bEdge, bEdge, 1.0f);
  r = length (swi2(q,y,z));
  swi2S(q,y,z, Rot2D (swi2(q,y,z), - angRot));
  a = (r > 0.0f) ? _atan2f (q.z, - q.y) / (2.0f * pi) : 0.0f;
  swi2S(q,y,z, Rot2D (swi2(q,y,z), 2.0f * pi * (_floor (8.0f * a + 0.5f)) / 8.0f));
  q.z = _fabs (q.z);
  d = SmoothMax (_fminf (min (_fabs (r - 1.01f) - 0.1f, r - 0.3f),
                 _fmaxf (r - 1.0f, dot (swi2(q,y,z), to_float2 (_sinf (0.8f * 2.0f * pi / 32.0f),
                 _cosf (0.8f * 2.0f * pi / 32.0f))))), _fabs (q.x) - 0.02f, 0.01f);
  DMIN (idWhl);
  d = _fminf (PrBoxDf (p - to_float3 (0.0f, 0.98f, 1.0f), to_float3 (1.12f, 0.02f, 0.1f)),
              PrBoxDf (p - to_float3 (-1.018f, 0.98f, 0.5f), to_float3 (0.1f, 0.02f, 0.5f - bEdge)));
  DMIN (idPln);
  d = SmoothMax (_fabs (PrCaps2Df (swi2(p - to_float3 (-0.05f, -0.21f, 0.0f),y,x), 0.2f, 1.0f)) - 0.01f,
                 _fabs (p.z) - 0.1f, 0.02f);
  DMIN (idConv);
  q = p - to_float3 (-0.05f, -0.21f, 0.0f);
  q.x = _fabs (q.x) - 1.0f;
  d = PrRoundCylDf (q, 0.18f, 0.01f, 0.11f);
  DMIN (idSpl);
  q = p - to_float3 (0.65f, -0.14f, 1.0f);
  q.x = _fabs (q.x) - 0.3f;
  d = PrRoundBoxDf (q, to_float3 (0.01f, 1.08f, 0.06f), 0.02f);
  q = p - to_float3 (-0.05f, -0.68f, 0.0f);
  swi2S(q,x,z, abs_f2(swi2(q,x,z)) - to_float2 (1.0f, 0.2f));
  d = _fminf (d, PrRoundBoxDf (q, to_float3 (0.04f, 0.55f, 0.01f), 0.02f));
  q = p - to_float3 (-1.05f, -0.14f, 1.0f);
  d = _fminf (d, PrRoundBoxDf (q, to_float3 (0.04f, 1.08f, 0.01f), 0.02f));
  q = p - to_float3 (-1.05f, -0.68f, 0.6f);
  q.z = _fabs (q.z) - 0.2f;
  d = _fminf (d, PrRoundBoxDf (q, to_float3 (0.04f, 0.55f, 0.01f), 0.02f));
  q = p - to_float3 (-0.33f, -0.68f, 1.555f);
  q.x = _fabs (q.x) - 0.3f;
  d = _fminf (d, PrRoundBoxDf (q, to_float3 (0.01f, 0.55f, 0.04f), 0.02f));
  DMIN (idSup);
  q = p - to_float3 (0.65f, bEdge, 1.0f);
  d = PrCylDf (swi3(q,y,z,x), 0.04f, 0.62f);
  q = p - to_float3 (-0.36f, -0.21f, 1.555f);
  d = _fminf (d, PrCylDf (swi3(q,y,z,x), 0.03f, 0.51f));
  q = p - to_float3 (-0.05f, -0.21f, 0.0f);
  q.x -= 1.0f;
  d = _fminf (d, PrCylDf (q, 0.03f, 0.27f));
  swi2S(q,x,z, swi2(q,x,z) - to_float2 (-2.0f, 0.14f));
  d = _fminf (d, PrCylDf (q, 0.03f, 0.4f));
  q.z -= 0.87f;
  d = _fminf (d, PrCylDf (q, 0.03f, 0.36f));
  q = p - to_float3 (-1.05f, -0.68f, 0.6f);
  d = _fminf (d, PrCylDf (q, 0.03f, 0.25f));
  DMIN (idAx);
  q = p - to_float3 (0.0f, -1.2f, 0.9f);
  d = PrRoundBoxDf (q, to_float3 (1.7f, 0.03f, 1.5f), 0.02f);
  DMIN (idBas);
  q = p - swi3(wgObj,x,y,z);
  d = PrRoundCylDf (swi3(q,x,z,y), wgObj.w, 0.02f, 0.02f);
  DMIN (idCon);
  return dMin;
}

__DEVICE__ float ObjRay (float3 ro, float3 rd, int *idObj, float dstFar, float angRot, float bEdge, float4 wgObj)
{
  float dHit, d;
  dHit = 0.0f;
  for (int j = VAR_ZERO; j < 150; j ++) {
    d = ObjDf (ro + dHit * rd, idObj,dstFar,angRot,bEdge,wgObj);
    dHit += d;
    if (d < 0.0005f || dHit > dstFar) break;
  }
  return dHit;
}

__DEVICE__ float GearRay (float3 ro, float3 rd, float dstFar, float angRot, float bEdge)
{
  float dHit, d;
  dHit = 0.0f;
  for (int j = VAR_ZERO; j < 250; j ++) {
    d = GearDf (ro + dHit * rd, dstFar, angRot, bEdge);
    dHit += d;
    if (d < 0.0005f || dHit > dstFar) break;
  }
  
  return dHit;
}

__DEVICE__ float3 GearNf (float3 p, float dstFar, float angRot, float bEdge)
{
  float _v[4];
  float2 e;
  e = to_float2 (0.0005f, -0.0005f);
  for (int j = VAR_ZERO; j < 4; j ++) {
    _v[j] = GearDf (p + ((j < 2) ? ((j == 0) ? swi3(e,x,x,x) : swi3(e,x,y,y)) : ((j == 2) ? swi3(e,y,x,y) : swi3(e,y,y,x))),dstFar,angRot,bEdge);
  }
  float4 v = to_float4(_v[0],_v[1],_v[2],_v[3]);
  v.x = - v.x;
  return normalize (2.0f * swi3(v,y,z,w) - dot (v, to_float4_s (1.0f)));
}

__DEVICE__ float3 ObjNf (float3 p, int *idObj, float dstFar, float angRot, float bEdge, float4 wgObj)
{
  float _v[4];
  float2 e;
  e = to_float2 (0.0005f, -0.0005f);
  for (int j = VAR_ZERO; j < 4; j ++) { 
    _v[j] = ObjDf (p + ((j < 2) ? ((j == 0) ? swi3(e,x,x,x) : swi3(e,x,y,y)) : ((j == 2) ? swi3(e,y,x,y) : swi3(e,y,y,x))),idObj,dstFar,angRot,bEdge,wgObj);
  }
  float4 v = to_float4(_v[0],_v[1],_v[2],_v[3]);
  v.x = - v.x;
  
  return normalize (2.0f * swi3(v,y,z,w) - dot (v, to_float4_s (1.0f)));
}

__DEVICE__ float BlkHit (float3 ro, float3 rd, float tCyc, float dstFar, float bEdge, float tMov, float *hitBlk, float3 *vnBlk, float2 *qBlk)
{
  float4 a4;
  float3 rm, rdm, u, v, tm, tp;
  float dMin, dn, df;
  dMin = dstFar;
  for (float k = float (VAR_ZERO); k < nBlk; k ++) {
    a4 = BPos (tMov + tCyc * k / nBlk, tCyc, bEdge);
    rm = ro - swi3(a4,x,y,z);
    rdm = rd;
    swi2(rm,z,y) = Rot2D (swi2(rm,z,y), a4.w);
    swi2(rdm,z,y) = Rot2D (swi2(rdm,z,y), a4.w);
    v = rm / rdm;
    tp = bEdge / abs_f3(rdm) - v;
    tm = - tp - 2.0f * v;
    dn = Maxv3 (tm);
    df = Minv3 (tp);
    
    if (df > 0.0f && dn < _fminf (df, dMin)) {
      dMin = dn;
      *hitBlk = k;
      *vnBlk = -1.0f * sign_f3(rdm) * step (swi3(tm,z,x,y), tm) * step (swi3(tm,y,z,x), tm);
      u = (v + dn) * rdm;
      *qBlk = to_float2 (dot (swi3(u,z,x,y), *vnBlk), dot (swi3(u,y,z,x), *vnBlk));
      swi2((*vnBlk),z,y) = Rot2D (swi2(*vnBlk,z,y), - a4.w);
    }
  }
  return dMin;
}

__DEVICE__ float BlkHitSh (float3 ro, float3 rd, float rng, float tCyc, float bEdge, float dstFar, float tMov)
{
  float4 a4;
  float3 rm, rdm, v, tm, tp;
  float dMin, dn, df;
  dMin = dstFar;
  for (float k = float (VAR_ZERO); k < nBlk; k ++) {
    a4 = BPos (tMov + tCyc * k / nBlk, tCyc,bEdge);
    rm = ro - swi3(a4,x,y,z);
    rdm = rd;
    swi2S(rm,z,y, Rot2D (swi2(rm,z,y), a4.w));
    swi2S(rdm,z,y, Rot2D (swi2(rdm,z,y), a4.w));
    v = rm / rdm;
    tp = bEdge / abs_f3(rdm) - v;
    tm = - tp - 2.0f * v;
    dn = Maxv3 (tm);
    df = Minv3 (tp);
    if (df > 0.0f && dn < _fminf (df, dMin)) dMin = dn;
  }
  
  return smoothstep (0.0f, rng, dMin);
}

__DEVICE__ float ObjSShadow (float3 ro, float3 rd, int *idObj, float dstFar, float angRot, float bEdge, float4 wgObj)
{
  float sh, d, h;
  sh = 1.0f;
  d = 0.02f;
  for (int j = VAR_ZERO; j < 30; j ++) {
    h = ObjDf (ro + rd * d,idObj,dstFar,angRot,bEdge,wgObj); 
    sh = _fminf (sh, smoothstep (0.0f, 0.05f * d, h));
    d += h;
    if (sh < 0.05f) break;
  }
  return sh;
}

__DEVICE__ float GearSShadow (float3 ro, float3 rd, float dstFar, float angRot, float bEdge)
{
  float sh, d, h;
  sh = 1.0f;
  d = 0.02f;
  for (int j = VAR_ZERO; j < 30; j ++) {
    h = GearDf (ro + rd * d, dstFar,angRot,bEdge);
    sh = _fminf (sh, smoothstep (0.0f, 0.05f * d, h));
    d += h;
    if (sh < 0.05f) break;
  }
  return sh;
}

__DEVICE__ float3 ShowScene (float3 ro, float3 rd, float dstFar, float4 wgObj, float tMov, float tCur, float3 ltDir, __TEXTURE2D__ iChannel1)
{
  float3 vnBlk;
  float2 qBlk;
  float  hitBlk;
  
  int idObj;
  float4 col4;
  float3 vn, col, q;
  float dstObj, dstGear, dstBlk, sh, s, r, a, nDotL;
  int idObjT;
  bool isMet;
  float tCyc = 18.5f;
  float bEdge = 0.08f;
  isMet = false;
  float angRot = 0.1f * pi * tMov;
  dstObj = ObjRay (ro, rd,&idObj,dstFar,angRot,bEdge,wgObj);
  idObjT = idObj;
  dstGear = GearRay (ro, rd, dstFar,angRot,bEdge);
  if (dstGear < _fminf (dstObj, dstFar)) {
    dstObj = dstGear;
    idObj = idGr;
  } else idObj = idObjT;
  dstBlk = BlkHit (ro, rd, tCyc,dstFar,bEdge,tMov, &hitBlk, &vnBlk, &qBlk);
  if (_fminf (dstBlk, dstObj) < dstFar) {
    if (dstBlk < dstObj) {
      dstObj = dstBlk;
      ro += dstObj * rd;
      idObj = idBlk;
      vn = vnBlk;
      col4 = to_float4_aw(HsvToRgb (to_float3 (hitBlk / nBlk, 1.0f, 1.0f)), 0.2f) *
             (1.0f - 0.4f * step (0.8f * bEdge, Maxv2 (abs_f2(qBlk))));
      if (hitBlk == 2.0f ) 
      {
        col4 = to_float4_s(1.0f);       
        col4 = texture(iChannel1,ro);
      }
             
    } else {
      ro += dstObj * rd;
      vn = (idObj == idGr) ? GearNf (ro,dstFar,angRot,bEdge) : ObjNf (ro,&idObj,dstFar,angRot,bEdge,wgObj);
      if (idObj == idWhl) {
        col4 = to_float4 (0.9f, 0.7f, 0.3f, 0.2f);
        q = ro - to_float3 (1.1f + bEdge + 0.03f, bEdge, 1.0f);
        r = length (swi2(q,y,z));
        swi2(q,y,z) = Rot2D (swi2(q,y,z), - angRot);
        a = fract (64.0f * _atan2f (q.z, - q.y) / (2.0f * pi) + 0.5f);
        if (r > 0.99f) swi2(vn,y,z) = Rot2D (swi2(vn,y,z), - _sinf (a - 0.5f));
        if (r > 0.92f) col4 *= 0.7f + 0.3f * SmoothBump (0.05f, 0.95f, 0.01f, a);
        isMet = true;
      } else if (idObj == idGr) {
        col4 = to_float4 (0.9f, 0.8f, 0.4f, 0.2f);
        isMet = true;
      } else if (idObj == idSpl) {
        col4 = to_float4 (0.8f, 0.8f, 0.85f, 0.2f) * (1.0f - 0.4f * step (_fabs (ro.z), 0.1f));
        isMet = true;
      } else if (idObj == idAx) {
        col4 = to_float4 (0.8f, 0.8f, 0.85f, 0.2f);
        isMet = true;
      } else if (idObj == idPln) {
        col4 = (_fabs (vn.y) > 0.99f) ? to_float4 (0.5f, 0.6f, 0.2f, 0.05f) : to_float4 (0.7f, 0.5f, 0.4f, 0.1f);
      } else if (idObj == idConv) {
        q = ro - to_float3 (-0.05f, -0.21f, 0.0f);
        col4 = to_float4 (0.8f, 0.8f, 0.4f, 0.0f);
        if (sign_f(vn.y) != sign_f(q.y)) {
          if (_fabs (q.x) < 1.0f && _fabs (vn.y) > 0.5f) col4 *= 1.0f - 0.1f * SmoothBump (0.45f, 0.55f, 0.03f,
           fract (10.0f * (q.x - sign_f(q.y) * mod_f (tMov, 20.0f) * 2.1f / 5.0f)));
        } else col4 *= 0.8f + 0.2f * smoothstep (0.0f, 0.01f, _fabs (abs (q.z) - 0.07f));
      } else if (idObj == idSup) {
        col4 = to_float4 (0.7f, 0.5f, 0.4f, 0.1f);
        isMet = true;
      } else if (idObj == idBas) {
        q = ro;
        q.z -= 0.9f;
        if (Maxv2 (abs_f2(swi2(q,x,z)) - to_float2 (1.65f, 1.45f)) > 0.0f) {
          col4 = to_float4 (0.9f, 0.9f, 0.9f, 0.2f);
          isMet = true;
        } else {
          col4 = to_float4 (0.3f, 0.5f, 0.4f, 0.0f);
        }
        col4 *= (0.5f + 0.5f * step (0.0f, Maxv2 (abs_f2(to_float2 (q.x, q.z + 1.3f)) - to_float2 (0.4f, 0.02f)))) *
           (0.7f + 0.3f * step (0.0f, _fabs (PrCaps2Df (to_float2 (q.z + 1.3f, q.x), 0.08f, 0.5f)) - 0.01f));
      } else if (idObj == idCon) {
        col4 = to_float4 (0.0f, 1.0f, 1.0f, 0.2f);
        if (length (swi2(ro,x,z) - swi2(wgObj,x,z)) < 0.6f * wgObj.w)
           col4 = _mix (0.8f * col4, to_float4 (1.0f, 0.0f, 1.0f, 0.2f), step (0.0f, _sinf (2.0f * pi * tCur)));
      }
    }
    sh = _fminf (ObjSShadow (ro, ltDir,&idObj,dstFar,angRot,bEdge,wgObj), GearSShadow (ro, ltDir,dstFar,angRot,bEdge));
    sh = 0.6f + 0.4f * _fminf (sh, BlkHitSh (ro + 0.01f * ltDir, ltDir, 6.0f,tCyc,bEdge,dstFar,tMov)); //, float dstFar, float tMov
    nDotL = _fmaxf (dot (vn, ltDir), 0.0f);
    if (isMet) nDotL *= nDotL;
    col = swi3(col4,x,y,z) * (0.1f + 0.1f * _fmaxf (- dot (vn, ltDir), 0.0f) + 0.9f * sh * nDotL) +
               col4.w * step (0.95f, sh) * sh * _powf (_fmaxf (0.0f, dot (ltDir, reflect (rd, vn))), 32.0f);
    if (isMet) {
      rd = reflect (rd, vn);
      col = _mix (col, to_float3_s (1.0f), 0.01f * step (0.1f, Minv2 (fract_f2 (8.0f * to_float2 (_atan2f (rd.z, rd.x),
                  2.0f * asin (rd.y)) + 0.5f) - 0.5f)));
    }
  } else col = to_float3 (0.0f, 0.0f, 0.1f) * (1.0f + 0.9f * rd.y);
  return clamp (col, 0.0f, 1.0f);
}
//**************************************************************************************************************************************************************************
__KERNEL__ void ControllableMachineryFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  const float txRow = 128.0f;
  

  mat3 vuMat;
  float4 stDat;
  float3 ro, rd, col;
  float2 canvas, uv;
  float el, az, zmFac, sr;
  canvas = iResolution;
  uv = 2.0f * fragCoord / canvas - 1.0f;
  uv.x *= canvas.x / canvas.y;
  float tCur = iTime;
  float dstFar = 30.0f;
  stDat = Loadv4 (txBuf,0,iResolution,txRow);
  float tMov = stDat.x;
  float cnPos = stDat.y;
  float4 wgObj = to_float4 (cnPos - 0.5f, -1.12f, -0.4f, 0.08f);
  stDat = Loadv4 (txBuf,1,iResolution,txRow);
  az = stDat.x;
  el = stDat.y;
  vuMat = StdVuMat (el, az);
  zmFac = 4.0f;
  ro = mul_mat3_f3(vuMat , to_float3 (0.0f, 0.0f, -8.0f));
  ro.z += 0.9f;
  rd = mul_mat3_f3(vuMat , normalize (to_float3_aw (uv, zmFac)));
  float3 ltDir = mul_mat3_f3(vuMat , normalize (to_float3 (-0.5f, 1.0f, -1.0f)));
#if ! AA
  const float naa = 1.0f;
#else
  const float naa = 3.0f;
#endif  
  col = to_float3_s (0.0f);
  sr = 2.0f * mod_f (dot (mod_f (_floor (0.5f * (uv + 1.0f) * canvas), 2.0f), to_float2_s (1.0f)), 2.0f) - 1.0f;
  for (float a = (float)(VAR_ZERO); a < naa; a ++) {
    rd = mul_mat3_f3(vuMat , normalize (to_float3_aw (uv + step (1.5f, naa) * Rot2D (to_float2 (0.5f / canvas.y, 0.0f),
                                        sr * (0.667f * a + 0.5f) * pi), zmFac)));
    col += (1.0f / naa) * ShowScene (ro, rd, dstFar,wgObj, tMov, tCur, ltDir, iChannel1);
  }
  fragColor = to_float4_aw (col, 1.0f);
  
  SetFragmentShaderComputedColor(fragColor);
}



