
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// "Stomper" by dr2 - 2021
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

#if 0
#define VAR_ZERO _fminf (iFrame, 0)
#else
#define VAR_ZERO 0
#endif

#define  idPlat  1
#define  idBallR 2
#define  idBallM 3
#define  idStomp 4
#define  idTube  5 
#define  idGrnd  6
#define pi  3.1415927f



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

__DEVICE__ float PrSphDf (float3 p, float r)
{
  return length (p) - r;
}

__DEVICE__ float PrCylDf (float3 p, float r, float h)
{
  return _fmaxf (length (swi2(p,x,y)) - r, _fabs (p.z) - h);
}

__DEVICE__ float PrCapsDf (float3 p, float r, float h)
{
  return length (p - to_float3 (0.0f, 0.0f, clamp (p.z, - h, h))) - r;
}

__DEVICE__ mat3 StdVuMat (float el, float az)
{
  float2 ori, ca, sa;
  ori = to_float2 (el, az);
  ca = cos_f2 (ori);
  sa = sin_f2 (ori);
  return mul_mat3_mat3(to_mat3(ca.y, 0.0f, - sa.y, 0.0f, 1.0f, 0.0f, sa.y, 0.0f, ca.y) ,
                       to_mat3(1.0f, 0.0f, 0.0f, 0.0f, ca.x, - sa.x, 0.0f, sa.x, ca.x));
}

__DEVICE__ float2 Rot2D (float2 q, float a)
{
  float2 cs;
  cs = sin_f2 (a + to_float2 (0.5f * pi, 0.0f));
  return to_float2 (dot (q, to_float2 (cs.x, - cs.y)), dot (swi2(q,y,x), cs));
}

__DEVICE__ float2 Rot2Cs (float2 q, float2 cs)
{
  return to_float2 (dot (q, to_float2 (cs.x, - cs.y)), dot (swi2(q,y,x), cs));
}

__DEVICE__ const float cHashM = 43758.54f;

__DEVICE__ float2 Hashv2f (float p)
{
  return fract_f2 (sin_f2 (p + to_float2 (0.0f, 1.0f)) * cHashM);
}

__DEVICE__ float2 Hashv2v2 (float2 p)
{
  float2 cHashVA2 = to_float2 (37.0f, 39.0f);
  return fract_f2(sin_f2(dot (p, cHashVA2) + to_float2 (0.0f, cHashVA2.x)) * cHashM);
}

__DEVICE__ float Noisefv2 (float2 p)
{
  float2 t, ip, fp;
  ip = _floor (p);  
  fp = fract_f2(p);
  fp = fp * fp * (3.0f - 2.0f * fp);
  t = _mix (Hashv2v2 (ip), Hashv2v2 (ip + to_float2 (0.0f, 1.0f)), fp.y);
  return _mix (t.x, t.y, fp.x);
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

__DEVICE__ float3 ltDir, pStomp; //, qHit
__DEVICE__ float2 pAct, pActP;
__DEVICE__ float tCur, dstFar, boxSz, tCyc, tPhs;
//__DEVICE__ int idObj;



struct TbCon {
  float3 pLo, pHi;
  float2 aLimCs, tRotCs[2], pRotCs[2];
  float chLen, chDist, ang, rad;
};

__DEVICE__ TbCon tbCon;

#define DMINQ(id) if (d < dMin) { dMin = d;  *idObj = id;  *qHit = q; }

__DEVICE__ float ObjDf (float3 p, float3 *qHit, float *idObj)
{
  float3 q;
  float dMin, d, db, a;
  dMin = dstFar;
  q = p;
  db = PrBox2Df (swi2(q,x,z), to_float2_s(boxSz));

  d = _fmaxf (PrBoxDf (q, swi3(to_float3_aw(to_float2_s(boxSz + 0.5f), 0.5f),x,z,y)), - _fmaxf (db,
              PrCylDf (to_float3_aw(fract_f2(swi2(q,x,z)) - 0.5f, q.y - 0.5f), 0.4f, 0.8f)));
  DMINQ (idPlat);
  d = _fmaxf (db, - PrBox2Df (swi2(q,x,z) - pAct, to_float2_s (0.5f)));
  swi2S(q,x,z, fract_f2(swi2(q,x,z)) - 0.5f);
  d = _fmaxf (PrSphDf (swi3(to_float3_aw(swi2(q,x,z), q.y - 0.15f),x,z,y), 0.4f), d);
  DMINQ (idBallR);
  q = p;
  //swi2(q,x,z) -= pAct;
  q.x -= pAct.x;
  q.z -= pAct.y;
  
  q.y -= 0.15f;
  if (tPhs < 0.8f) q.y -= 0.4f * (1.0f - _cosf (10.0f * pi * tCyc));
  d = PrSphDf (q, 0.4f);
  DMINQ (idBallM);
  q = p;
  q -= pStomp;
  d = _fminf (PrCylDf (swi3(q,x,z,y), 0.35f, 0.05f), _fmaxf (PrCapsDf (swi3(q,x,z,y), 0.1f, 0.3f), - q.y));
  DMINQ (idStomp);
  q = p;
  swi2S(q,x,z, swi2(q,x,z) - swi2(tbCon.pLo,x,z));
  d = PrCylDf (swi3(q,x,z,y), 0.4f, 0.5f);
  DMINQ (idPlat);
  q.y -= 0.5f;
  d = _fmaxf (PrCapsDf (swi3(q,x,z,y), 0.1f, 0.3f), - q.y);
  DMINQ (idTube);
  d = _fminf (PrSphDf (p - tbCon.pLo, 0.1f), PrSphDf (p - tbCon.pHi, 0.1f));
  DMINQ (idTube);
  q = p - tbCon.pLo;
  swi2S(q,x,z, Rot2Cs (swi2(q,x,z), tbCon.tRotCs[0]));
  swi2S(q,y,z, Rot2Cs (swi2(q,y,z), tbCon.tRotCs[1]) - to_float2 (tbCon.chLen, tbCon.chDist));
  a = fract ((256.0f / tbCon.ang) * _atan2f (q.y, - q.z) / (2.0f * pi));
  d = _fmaxf (dot (to_float2 (_fabs (q.y), - q.z), tbCon.aLimCs), length (to_float2 (length (swi2(q,y,z)) -
              tbCon.rad, q.x)) - (0.1f - 0.015f * smoothstep (0.15f, 0.35f, 0.5f - _fabs (0.5f - a))));
  
  DMINQ (idTube);
  q = p;
  d = _fmaxf (q.y - 0.4f, - db);
  DMINQ (idGrnd);
  return dMin;
}

__DEVICE__ float ObjRay (float3 ro, float3 rd, float3 *qHit, float *idObj)
{
  float dHit, d;
  dHit = 0.0f;
  for (int j = VAR_ZERO; j < 120; j ++) {
    d = ObjDf (ro + dHit * rd, qHit, idObj);
    if (d < 0.0005f || dHit > dstFar) break;
    dHit += d;
  }
  return dHit;
}

__DEVICE__ float3 ObjNf (float3 p, float3 *qHit, float *idObj)
{
  float v[4];
  float2 e;
  e = to_float2 (0.001f, -0.001f);
  for (int j = VAR_ZERO; j < 4; j ++) {
    v[j] = ObjDf (p + ((j < 2) ? ((j == 0) ? swi3(e,x,x,x) : swi3(e,x,y,y)) : ((j == 2) ? swi3(e,y,x,y) : swi3(e,y,y,x))),qHit,idObj);
  }
  v[0] = - v[0];
  float4 _v = to_float4(v[0],v[1],v[2],v[3]);
  return normalize (2.0f * swi3(_v,y,z,w) - dot (_v, to_float4_s (1.0f)));
}

__DEVICE__ float ObjSShadow (float3 ro, float3 rd, float3 *qHit, float *idObj)
{
  float sh, d, h;
  int idObjT;
  idObjT = *idObj;
  sh = 1.0f;
  d = 0.02f;
  for (int j = VAR_ZERO; j < 30; j ++) {
    h = ObjDf (ro + d * rd, qHit,idObj);
    sh = _fminf (sh, smoothstep (0.0f, 0.05f * d, h));
    d += h;
    if (sh < 0.05f) break;
  }
  *idObj = idObjT;
  return 0.5f + 0.5f * sh;
}

#define F(x) (_sinf (x) / x - b)

__DEVICE__ float SecSolve (float b)
{  // (from "Bucking Bronco")
  float3 t;
  float2 f;
  float x;
  if (b < 0.95f) {
    //swi2(t,y,z) = to_float2 (0.7f, 1.2f);
    t.y = 0.7f;
    t.z = 1.2f;
    
    f = to_float2 (F(t.y), F(t.z));
    for (int nIt = 0; nIt < 4; nIt ++) {
      t.x = (t.z * f.x - t.y * f.y) / (f.x - f.y);
      //swi2(t,z,y) = swi2(t,y,x);
      t.z = t.y;
      t.y = t.x;
            
      f = to_float2 (F(t.x), f.x);
    }
    x = t.x;
  } else if (b < 1.0f) {
    x = _sqrtf (10.0f * (1.0f - _sqrtf (1.0f - 1.2f * (1.0f - b))));
  } else {
    x = 0.0f;
  }
  return x;
}

__DEVICE__ void SetConf ()
{
  float3 vp;
  float tubeLen;
  boxSz = 4.0f;
  tCyc = tCur / 2.0f + 1.0f;
  tPhs = fract (tCyc);
  pAct = _floor (boxSz * (2.0f * Hashv2f (1.11f + 17.33f * _floor (tCyc)) - 1.0f)) + 0.5f;
  pActP = _floor (boxSz * (2.0f * Hashv2f (1.11f + 17.33f * _floor (tCyc - 1.0f)) - 1.0f)) + 0.5f;
  if (tPhs < 0.8f) {
    swi2S(pStomp,x,z, _mix (pActP, pAct, tPhs / 0.8f));
    pStomp.y = 1.7f;
  } else {
    swi2S(pStomp,x,z, pAct);
    pStomp.y = _mix (0.5f, 2.0f, _fmaxf (2.0f * _fabs (tPhs - 0.9f) / 0.1f - 1.0f, 0.0f));
  }
  tbCon.pLo = to_float3 (1.5f * boxSz, 0.9f, 0.0f);
  tbCon.pHi = pStomp;
  tbCon.pHi.y += 0.4f;
  vp = tbCon.pHi - tbCon.pLo;
  tbCon.chLen = 0.5f * length (vp);
  
  tbCon.tRotCs[0] = sin_f2(_atan2f (vp.x, vp.z) + to_float2 (0.5f * pi, 0.0f));
  tbCon.tRotCs[1] = sin_f2(- asin (length (swi2(vp,x,z)) / length (vp)) + to_float2 (0.5f * pi, 0.0f));
  tubeLen = 1.4f * boxSz;
  tbCon.ang = SecSolve (tbCon.chLen / tubeLen);
  tbCon.chDist = tbCon.chLen / _tanf (tbCon.ang);
  tbCon.rad = length (to_float2 (tbCon.chDist, tbCon.chLen));
  tbCon.aLimCs = sin_f2 (- tbCon.ang + to_float2 (0.5f * pi, 0.0f));
}

__DEVICE__ float3 ShowScene (float3 ro, float3 rd, float3 *qHit, float *idObj)
{
  float4 col4;
  float3 col, vn;
  float dstObj, sh, nDotL;
  SetConf ();
  dstObj = ObjRay (ro, rd, qHit, idObj);
  if (dstObj < dstFar) {
    ro += dstObj * rd;
    vn = ObjNf (ro,qHit, idObj);
    if (*idObj == idPlat) {
      col4 = to_float4 (0.5f, 0.2f, 0.0f, 0.2f);
    } else if (*idObj == idGrnd) {
      col4 = to_float4 (0.5f, 0.2f, 0.0f, 0.2f) * (0.8f + 0.2f * Fbm2 (4.0f * swi2(ro,x,z)));
    } else if (*idObj == idBallR) {
      col4 = to_float4 (0.0f, 1.0f, 0.0f, 0.2f);
      if (PrBox2Df (swi2(ro,x,z) - pActP, to_float2_s (0.5f)) < 0.0f) col4 = _mix (to_float4 (1.0f, 0.0f, 0.0f, 0.2f) *
         (0.7f + 0.3f * _sinf (16.0f * pi * tCur)), col4, smoothstep (0.2f, 0.6f, tPhs));
    } else if (*idObj == idBallM) {
      col4 = to_float4 (1.0f, 0.0f, 0.0f, 0.2f);
      if (tPhs > 0.9f) col4 = to_float4 (0.7f, 0.5f, 0.0f, 0.2f);
    } else if (*idObj == idStomp) {
      col4 = _mix (to_float4 (1.0f, 1.0f, 1.0f, 0.2f), to_float4 (0.0f, 0.0f, 1.0f, -1.0f),
         smoothstep (0.0f, 0.02f, length (swi2(*qHit,x,z)) - 0.25f));
    } else if (*idObj == idTube) {
      col4 = to_float4 (1.0f, 1.0f, 1.0f, 0.2f);
    }
    if (*idObj == idBallR || *idObj == idBallM) col4 *= 0.7f + 0.3f * smoothstep (0.0f, 0.02f,
       _fabs (length (swi2(*qHit,x,z)) - 0.2f) - 0.02f);
    if (col4.w >= 0.0f) {
      nDotL = _fmaxf (dot (vn, ltDir), 0.0f);
      if (*idObj == idTube) nDotL *= nDotL;
      sh = ObjSShadow (ro + 0.01f * vn, ltDir,qHit, idObj);
      col = swi3(col4,x,y,z) * (0.2f + 0.8f * sh * nDotL) +
            col4.w * step (0.95f, sh) * _powf (_fmaxf (dot (ltDir, reflect (rd, vn)), 0.0f), 32.0f);
      col *= 1.0f - 0.9f * smoothstep (0.2f, 0.3f, length (swi2(ro,x,z)) / dstFar);
    } else col = swi3(col4,x,y,z) * (0.5f + 0.5f * _fmaxf (- dot (rd, vn), 0.0f));
  } else {
    col = to_float3_s (0.1f);
  }
  return clamp (col, 0.0f, 1.0f);
}

#define AA  0   // optional antialiasing

__KERNEL__ void StomperFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

  float3 qHit;
  float idObj;
  
  mat3 vuMat;
  float4 mPtr;
  float3 ro, rd, col;
  float2 canvas, uv;
  float el, az, zmFac, sr;
  canvas = iResolution;
  uv = 2.0f * fragCoord / canvas - 1.0f;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  mPtr = iMouse;
  swi2S(mPtr,x,y, swi2(mPtr,x,y) / canvas - 0.5f);
  az = 0.0f;
  el = -0.2f * pi;
  if (mPtr.z > 0.0f) {
    az += 2.0f * pi * mPtr.x;
    el += pi * mPtr.y;
  }
  el = clamp (el, -0.4f * pi, -0.13f * pi);
  vuMat = StdVuMat (el, az);
  ro = mul_mat3_f3(vuMat , to_float3 (0.0f, 1.0f, -30.0f));
  zmFac = 5.5f;
  dstFar = 60.0f;
  ltDir = mul_mat3_f3(vuMat , normalize (to_float3 (1.0f, 1.0f, -1.0f)));
#if ! AA
  const float naa = 1.0f;
#else
  const float naa = 3.0f;
#endif  
  col = to_float3_s (0.0f);
  sr = 2.0f * mod_f (dot (mod_f (_floor (0.5f * (uv + 1.0f) * canvas), 2.0f), to_float2_s (1.0f)), 2.0f) - 1.0f;
  for (float a = (float) (VAR_ZERO); a < naa; a ++) {
    rd = mul_mat3_f3(vuMat , normalize (to_float3_aw (uv + step (1.5f, naa) * Rot2D (to_float2 (0.5f / canvas.y, 0.0f), sr * (0.667f * a + 0.5f) * pi), zmFac)));
    col += (1.0f / naa) * ShowScene (ro, rd,&qHit,&idObj);
  }
  fragColor = to_float4_aw (col, 1.0f);
  
  SetFragmentShaderComputedColor(fragColor);  
}

