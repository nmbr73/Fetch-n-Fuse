
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// "Dynamic Block Grid" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

// Constant gap between adjacent blocks (full 3D extension of "Random Rectangular Tiling",
// the fixed-width channels were used in, e.g., "Green Mercury")

#if 0
#define VAR_ZERO _fminf (iFrame, 0)
#else
#define VAR_ZERO 0
#endif

#define pi 3.1415927f

__DEVICE__ float PrRound4BoxDf (float3 p, float3 b, float r)
{
  p = _fmaxf (abs_f3 (p) - b, to_float3_s(0.0f));
  return _sqrtf (length (p * p)) - r;
}

__DEVICE__ float PrRound4Box2Df (float2 p, float2 b, float r)
{
  p = _fmaxf (abs_f2(p) - b, to_float2_s(0.0f));
  return _sqrtf (length (p * p)) - r;
}

__DEVICE__ float Minv2 (float2 p)
{
  return _fminf (p.x, p.y);
}

__DEVICE__ mat3 StdVuMat (float el, float az)
{
  float2 ori, ca, sa;
  ori = to_float2 (el, az);
  ca = cos_f2 (ori);
  sa = sin_f2 (ori);
  return mul_mat3_mat3(to_mat3 (ca.y, 0.0f, - sa.y, 0.0f, 1.0f, 0.0f, sa.y, 0.0f, ca.y) ,
                       to_mat3 (1.0f, 0.0f, 0.0f, 0.0f, ca.x, - sa.x, 0.0f, sa.x, ca.x));
}

__DEVICE__ float2 Rot2D (float2 q, float a)
{
  
  float2 cs;
  cs = sin_f2 (a + to_float2 (0.5f * pi, 0.0f));
  return to_float2 (dot (q, to_float2 (cs.x, - cs.y)), dot (swi2(q,y,x), cs));
}

__DEVICE__ float3 HsvToRgb (float3 c)
{
  return c.z * _mix (to_float3_s (1.0f), clamp (abs_f3 (fract_f3 (swi3(c,x,x,x) + to_float3 (1.0f, 2.0f/3.0f, 1.0f/3.0f)) * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f), c.y);
}

#define cHashM 43758.54f

__DEVICE__ float Hashfv2 (float2 p)
{
  return fract (_sinf (dot (p, to_float2 (37.0f, 39.0f))) * cHashM);
}

__DEVICE__ float2 Hashv2v2 (float2 p)
{
  float2 cHashVA2 = to_float2 (37.0f, 39.0f);
  return fract_f2 (sin_f2 (dot (p, cHashVA2) + to_float2 (0.0f, cHashVA2.x)) * cHashM);
}

__DEVICE__ float Noisefv2 (float2 p)
{
  float2 t, ip, fp;
  ip = _floor (p);  
  fp = fract (p);
  fp = fp * fp * (3.0f - 2.0f * fp);
  t = _mix (Hashv2v2 (ip), Hashv2v2 (ip + to_float2 (0.0f, 1.0f)), fp.y);
  return _mix (t.x, t.y, fp.x);
}

__DEVICE__ float Fbmn (float3 p, float3 n)
{
  float3 s;
  float a;
  s = to_float3_s (0.0f);
  a = 1.0f;
  for (int j = 0; j < 5; j ++) {
    s += a * to_float3(Noisefv2 (swi2(p,y,z)), Noisefv2 (swi2(p,z,x)), Noisefv2 (swi2(p,x,y)));
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
  g = to_float3(v[0],v[1],v[2]) - v[3];
  return normalize (n + f * (g - n * dot (n, g)));
}





#define H(z) (0.5f + 0.5f * _cosf (1.5f * tCur * (0.1f + 0.9f * Hashfv2 (ip + z))))

__DEVICE__ void CellParms (float2 ip, float tCur, float2 pEdge[4], float2 pMid[4], float eFac)
{
  float4 hm, hc;
  float3 e;
  float2 ee[4];
  float hp;
  e = to_float3 (-1.0f, 0.0f, 1.0f);
  ee[0] = swi2(e,x,z);
  ee[1] = swi2(e,z,z);
  ee[2] = swi2(e,z,x);
  ee[3] = swi2(e,x,x);
  hp = H(0.0f);
  hm = to_float4 (H(swi2(e,z,y)), H(swi2(e,x,y)), H(swi2(e,y,z)), H(swi2(e,y,x)));
  hc = to_float4 (H(swi2(e,z,z)), H(swi2(e,x,x)), H(swi2(e,x,z)), H(swi2(e,z,x)));
  if (mod_f (ip.x + ip.y, 2.0f) < 0.5f) {
    pEdge[0] = to_float2 (hm.z - hm.y, hc.z - hp);
    pEdge[1] = to_float2 (hm.x - hm.z, hc.x - hp);
    pEdge[2] = to_float2 (hm.x - hm.w, hp - hc.w);
    pEdge[3] = to_float2 (hm.w - hm.y, hp - hc.y);
    pMid[0] = to_float2 (hm.z, hp);
    pMid[1] = pMid[0];
    pMid[2] = to_float2 (hm.w, hp);
    pMid[3] = pMid[2];
  } else {
    pEdge[0] = to_float2 (hp - hc.z, hm.z - hm.y);
    pEdge[1] = to_float2 (hc.x - hp, hm.z - hm.x);
    pEdge[2] = to_float2 (hc.w - hp, hm.x - hm.w);
    pEdge[3] = to_float2 (hp - hc.y, hm.y - hm.w);
    pMid[0] = to_float2 (hp, hm.y);
    pMid[1] = to_float2 (hp, hm.x);
    pMid[2] = pMid[1];
    pMid[3] = pMid[0];
  }
  for (int k = 0; k < 4; k ++) {
    pEdge[k] = eFac * pEdge[k] + 0.5f;
    pMid[k] = 2.0f * eFac * (pMid[k] - 0.5f) + pEdge[k] * ee[k];
  }
}

__DEVICE__ float ObjDf (float3 p, float dstFar, float2 ip, float2 pMid[4], float2 pEdge[4], float gSize, float eRound, float2 *qcMin, float eFac  )
{
  float3 q, bs;
  float2 qc;
  float dMin, d;
  dMin = dstFar;
  for (int k = 0; k < 4; k ++) {
    qc = ip + pMid[k];
    swi2S(q,x,z, swi2(p,x,z) - gSize * qc);
    qc = _floor (qc);
    swi2S(bs,x,z, pEdge[k] - eFac + 0.05f);
    bs.y = 0.4f * (bs.x + bs.z) + 0.1f * Hashfv2 (qc);
    q.y = p.y - gSize * bs.y;
    d = gSize * PrRound4BoxDf (q / gSize, bs - eRound, eRound);
    if (d < dMin) {
      dMin = d;
      *qcMin = qc;
    }
  }
  return dMin;
}

#if 1

__DEVICE__ float ObjRay (float3 ro, float3 rd, float dstFar, float2 *ip, float2 pMid[4], float gSize, float eRound, float2 *qcMin,
                         float tCur, float2 pEdge[4], float eFac )  // (cell-based ray-marching)
{
  
  float3 p, rdi;
  float2 fp, ipP;
  float dHit, d, eps;
  eps = 0.001f;
  if (rd.x == 0.0f) rd.x = 0.0001f;
  if (rd.z == 0.0f) rd.z = 0.0001f;
  swi2(rdi,x,z) = 1.0f / swi2(rd,x,z);
  dHit = eps;
  ipP = to_float2_s (0.5f);
  for (int j = VAR_ZERO; j < 160; j ++) {
    p = ro + dHit * rd;
    fp = swi2(p,x,z) + 0.5f;
    *ip = _floor (fp / gSize);
    //if (*ip != ipP) {
    if ((*ip).x != ipP.x || (*ip).y != ipP.y ) {
      ipP = *ip;
      CellParms (*ip,tCur, pEdge, pMid, eFac);
    }
    d = ObjDf (p, dstFar, *ip, pMid,pEdge, gSize, eRound, qcMin,eFac);
    dHit += _fminf (d, eps + _fmaxf (0.0f, Minv2 ((gSize * (*ip + step (to_float2_s(0.0f), swi2(rd,x,z))) - fp) * swi2(rdi,x,z))));
    if (d < eps || dHit > dstFar || p.y < 0.0f) break;
  }
  if (d >= eps || p.y < 0.0f) dHit = dstFar;
  return dHit;
}

#else

__DEVICE__ float ObjRay (float3 ro, float3 rd, float dstFar, float2 *ip, float2 pMid[4], float gSize, float eRound, float2 *qcMin,
                         float tCur, float2 pEdge[4], float eFac )  // (simple ray-marching - visual artifacts)
{
  float3 p;
  float2 ipP;
  float dHit, d;
  dHit = 0.0f;
  ipP = to_float2_s (0.5f);
  for (int j = VAR_ZERO; j < 160; j ++) {
    p = ro + dHit * rd;
    *ip = _floor ((swi2(p,x,z) + 0.5f) / gSize);
    //if (*ip != ipP) {
    if ((*ip).x != ipP.x || (*ip).y != ipP.y ) {
      ipP = *ip;
      CellParms (ip,tCur, pEdge, pMid, eFac);
    }
    d = ObjDf (p, dstFar, *ip, pMid,pEdge, gSize, eRound, qcMin,eFac );
    if (d < 0.001f || dHit > dstFar || p.y < 0.0f) break;
    dHit += d;
  }
  if (p.y < 0.0f) dHit = dstFar;
  return dHit;
}

#endif

__DEVICE__ float3 ObjNf (float3 p, float dstFar, float2 ip, float2 pMid[4], float2 pEdge[4], float gSize, float eRound, float2 *qcMin, float eFac )
{
  
  float v[4];
  float2 e;
  e = to_float2 (0.001f, -0.001f);
  for (int j = VAR_ZERO; j < 4; j ++) {
    v[j] = ObjDf (p + ((j < 2) ? ((j == 0) ? swi3(e,x,x,x) : swi3(e,x,y,y)) : ((j == 2) ? swi3(e,y,x,y) : swi3(e,y,y,x))), dstFar, ip, pMid,pEdge, gSize, eRound, qcMin, eFac );
  }
  v[0] = - v[0];
  return normalize (2.0f * to_float3(v[1],v[2],v[3]) - dot (to_float4(v[0],v[1],v[2],v[3]), to_float4_s (1.0f)));
}

__DEVICE__ float ObjSShadow (float3 ro, float3 rd, float dstFar, float2 *ip, float2 pMid[4], float gSize, float eRound, float2 *qcMin,
           float tCur, float2 pEdge[4], float eFac )
{
  float3 p;
  float2 ipP;
  float sh, d, h;
  sh = 1.0f;
  d = 0.01f;
  ipP = to_float2_s (0.5f);

  for (int j = VAR_ZERO; j < 24; j ++) {
    p = ro + d * rd;
    *ip = _floor ((swi2(p,x,z) + 0.5f) / gSize);
    //if (ip != ipP) {
    if ((*ip).x != ipP.x || (*ip).y != ipP.y ) {
      ipP = *ip;
      CellParms (*ip,tCur, pEdge, pMid, eFac);
    }
    h = ObjDf (p, dstFar, *ip, pMid,pEdge, gSize, eRound, qcMin,eFac );
    sh = _fminf (sh, smoothstep (0.0f, 0.1f * d, h));
    d += _fmaxf (h, 0.01f);
    if (h < 0.001f || d > dstFar) break;
  }
  return 0.5f + 0.5f * sh;
}

__DEVICE__ float3 ShowScene (float3 ro, float3 rd, float dstFar, float2 *ip, float2 pMid[4], float2 *qcMin,
           float tCur, float2 pEdge[4], float3 ltDir)
{
  
  float4 col4;
  float3 col, vn;
  float2 q, b;
  float dstObj, dstGrnd, sh, nDotL, s;
  float eFac = 0.2f;
  float eRound = 0.2f;
  float gSize = 1.0f;
  dstObj = ObjRay (ro, rd, dstFar, ip, pMid, gSize, eRound, qcMin, tCur, pEdge, eFac );
  dstGrnd = dstFar;
  if (dstObj < dstFar || rd.y < 0.0f) {
    if (dstObj < dstFar) {
      ro += dstObj * rd;
      vn = ObjNf (ro, dstFar, *ip, pMid,pEdge, gSize, eRound, qcMin,eFac );
      col4 = to_float4_aw(HsvToRgb (to_float3(Hashfv2 (*qcMin), 0.7f, 1.0f)), 0.2f);
      
// Textur einfügen      
//      if (qcMin.x == 2.0 && qcMin.y == 2.0) col4 = texture(iChannel0, vn.xz/R);//col4 = vec4(0.0f);
//      col4 = texture(iChannel0, ro.xz*1.);
    
      
      
      nDotL = _fmaxf (dot (vn, ltDir), 0.0f);
      nDotL *= nDotL;
      s = 1.0f;
    } else {
      
      dstGrnd = - ro.y / rd.y;
      ro += dstGrnd * rd;
      vn = to_float3 (0.0f, 1.0f, 0.0f);
      vn = VaryNf (16.0f * ro, vn, 0.5f);
      col4 = to_float4 (0.4f, 0.4f, 0.4f, 0.0f);
      *ip = _floor ((swi2(ro,x,z) + 0.5f) / gSize);
      CellParms (*ip,tCur, pEdge, pMid, eFac);
      s = 1.0f;
      for (int k = 0; k < 4; k ++) {
        q = swi2(ro,x,z) - gSize * (*ip + pMid[k]);
        b = pEdge[k] - eFac + 0.1f;
        s = _fminf (s, gSize * PrRound4Box2Df (q / gSize, b - eRound, eRound));
      }
      col4 *= 1.0f + 0.7f * (1.0f - smoothstep (0.01f, 0.02f, _fabs (s)));
      s = 1.0f;
      for (int k = 0; k < 4; k ++) {
        q = swi2(ro,x,z) - gSize * (*ip + pMid[k]);
        b = pEdge[k] - eFac;
        s = _fminf (s, gSize * PrRound4Box2Df (q / gSize, b - eRound, eRound));
      }
      s = 0.5f + 0.5f * smoothstep (0.0f, 0.02f, s - 0.02f);
      nDotL = _fmaxf (dot (vn, ltDir), 0.0f);
    }
    sh = (dstGrnd <= dstFar) ? _fminf (s, ObjSShadow(ro + 0.01f * vn, ltDir,dstFar,ip,pMid,gSize,eRound,qcMin,tCur,pEdge,eFac)) : 1.0f;
    col = swi3(col4,x,y,z) * (0.1f + 0.1f * _fmaxf (vn.y, 0.0f) + 0.8f * sh * nDotL) +
          col4.w * step (0.95f, sh) * _powf (_fmaxf (dot (ltDir, reflect (rd, vn)), 0.0f), 32.0f);
  } else {
    col = to_float3 (0.6f, 0.6f, 1.0f);
  }
  return clamp (col, 0.0f, 1.0f);
}

__KERNEL__ void DynamicBlockGridFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

  float3 ltDir;
  float2 pMid[4], pEdge[4], qcMin, ip;
  float tCur, dstFar;

  mat3 vuMat;
  float4 mPtr;
  float3 ro, rd, col;
  float2 canvas, uv;
  float el, az, zmFac;
  canvas = iResolution;
  uv = 2.0f * fragCoord / canvas - 1.0f;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  mPtr = iMouse;
  //swi2(mPtr,x,y) = swi2(mPtr,x,y) / canvas - 0.5f;
  mPtr.x = mPtr.x / canvas.x - 0.5f;
  mPtr.y = mPtr.y / canvas.y - 0.5f;
  
  tCur += 1.0f;
  az = 0.0f;
  el = -0.15f * pi;
  if (mPtr.z > 0.0f) {
    az += 2.0f * pi * mPtr.x;
    el += pi * mPtr.y;
  } else {
    az += 0.03f * pi * _sinf (0.1f * pi * tCur);
  }
  el = clamp (el, -0.4f * pi, -0.12f * pi);
  ltDir = normalize (to_float3 (0.7f, 1.5f, -1.0f));
  vuMat = StdVuMat (el, az);
  ro = to_float3 (0.07f * tCur, 6.0f, 0.2f * tCur);
  zmFac = 4.0f;
  dstFar = 120.0f;
  rd = mul_mat3_f3(vuMat , normalize (to_float3_aw (uv, zmFac)));
    
  col = ShowScene (ro, rd, dstFar, &ip, pMid, &qcMin,tCur, pEdge,ltDir );
  fragColor = to_float4_aw (col, 1.0f);
  
  SetFragmentShaderComputedColor(fragColor);
}

