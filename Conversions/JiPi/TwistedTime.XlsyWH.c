
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Font 1' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// "Twisted Time" by dr2 - 2017
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

#define txFnt iChannel0

//__DEVICE__ float4 dateCur;
//__DEVICE__ float3 sunDir, vnCylIn;
__DEVICE__ float2 gVec[7], hVec[7], fntSize, qnFnt, qnTxt;
//__DEVICE__ float dstFar, tCur, grDep, rngBlk, bCylRad, bCylHt, dCylIn, dCylOut;
__DEVICE__ int idTxt;

#define pi  3.14159f


__DEVICE__ float PrTorusDf (float3 p, float ri, float rc)
{
  return length (to_float2 (length (swi2(p,x,y)) - rc, p.z)) - ri;
}

#define C(c) c

#define _SP    C(0x20)
#define _EXCL  C(0x21)
#define _QUOT  C(0x22)
#define _NUM   C(0x23)
#define _DOLLR C(0x24)
#define _PCENT C(0x25)
#define _AMP   C(0x26)
#define _SQUOT C(0x27)
#define _LPAR  C(0x28)
#define _RPAR  C(0x29)
#define _AST   C(0x2A)
#define _PLUS  C(0x2B)
#define _COMMA C(0x2C)
#define _MINUS C(0x2D)
#define _PER   C(0x2E)
#define _SLASH C(0x2F)
#define _0     C(0x30)
#define _1     C(0x31)
#define _2     C(0x32)
#define _3     C(0x33)
#define _4     C(0x34)
#define _5     C(0x35)
#define _6     C(0x36)
#define _7     C(0x37)
#define _8     C(0x38)
#define _9     C(0x39)
#define _COLON C(0x3A)
#define _SEMI  C(0x3B)
#define _LT    C(0x3C)
#define _EQUAL C(0x3D)
#define _GT    C(0x3E)
#define _QUEST C(0x3F)
#define _AT    C(0x40)
#define _A     C(0x41)
#define _B     C(0x42)
#define _C     C(0x43)
#define _D     C(0x44)
#define _E     C(0x45)
#define _F     C(0x46)
#define _G     C(0x47)
#define _H     C(0x48)
#define _I     C(0x49)
#define _J     C(0x4A)
#define _K     C(0x4B)
#define _L     C(0x4C)
#define _M     C(0x4D)
#define _N     C(0x4E)
#define _O     C(0x4F)
#define _P     C(0x50)
#define _Q     C(0x51)
#define _R     C(0x52)
#define _S     C(0x53)
#define _T     C(0x54)
#define _U     C(0x55)
#define _V     C(0x56)
#define _W     C(0x57)
#define _X     C(0x58)
#define _Y     C(0x59)
#define _Z     C(0x5A)
#define _LSQB  C(0x5B)
#define _BSLSH C(0x5C)
#define _RSQB  C(0x5D)
#define _CARET C(0x5E)
#define _USCOR C(0x5F)
#define _GRAVE C(0x60)
#define _a     C(0x61)
#define _b     C(0x62)
#define _c     C(0x63)
#define _d     C(0x64)
#define _e     C(0x65)
#define _f     C(0x66)
#define _g     C(0x67)
#define _h     C(0x68)
#define _i     C(0x69)
#define _j     C(0x6A)
#define _k     C(0x6B)
#define _l     C(0x6C)
#define _m     C(0x6D)
#define _n     C(0x6E)
#define _o     C(0x6F)
#define _p     C(0x70)
#define _q     C(0x71)
#define _r     C(0x72)
#define _s     C(0x73)
#define _t     C(0x74)
#define _u     C(0x75)
#define _v     C(0x76)
#define _w     C(0x77)
#define _x     C(0x78)
#define _y     C(0x79)
#define _z     C(0x7A)
#define _LBRC  C(0x7B)
#define _VBAR  C(0x7C)
#define _RBRC  C(0x7D)
#define _TILDE C(0x7E)

__DEVICE__ int3 MName (int i)
{
  int3 m;
  if      (i == 0)  m = to_int3 (_J, _a, _n);
  else if (i == 1)  m = to_int3 (_F, _e, _b);
  else if (i == 2)  m = to_int3 (_M, _a, _r);
  else if (i == 3)  m = to_int3 (_A, _p, _r);
  else if (i == 4)  m = to_int3 (_M, _a, _y);
  else if (i == 5)  m = to_int3 (_J, _u, _n);
  else if (i == 6)  m = to_int3 (_J, _u, _l);
  else if (i == 7)  m = to_int3 (_A, _u, _g);
  else if (i == 8)  m = to_int3 (_S, _e, _p);
  else if (i == 9)  m = to_int3 (_O, _c, _t);
  else if (i == 10) m = to_int3 (_N, _o, _v);
  else if (i == 11) m = to_int3 (_D, _e, _c);
  return m;
}

__DEVICE__ int3 DName (int i)
{
  int3 d;
  if      (i == 0)  d = to_int3 (_S, _u, _n);
  else if (i == 1)  d = to_int3 (_M, _o, _n);
  else if (i == 2)  d = to_int3 (_T, _u, _e);
  else if (i == 3)  d = to_int3 (_W, _e, _d);
  else if (i == 4)  d = to_int3 (_T, _h, _u);
  else if (i == 5)  d = to_int3 (_F, _r, _i);
  else if (i == 6)  d = to_int3 (_S, _a, _t);
  return d;
}

__DEVICE__ int DWk (float3 ymd)
{
  float dddddddddddddddddddd;
  int3 d;
  float2 cy;
  ymd.y += 1.0f;
  if (ymd.y <= 2.0f) {
    ymd.x -= 1.0f;
    ymd.y += 12.0f;
  }
  cy = to_float2 (_floor (ymd.x / 100.0f), mod_f (ymd.x, 100.0f));
  return (int) (mod_f (mod_f (ymd.z + _floor (13.0f * (ymd.y + 1.0f) / 5.0f) + cy.y + _floor (cy.y / 4.0f) +
                _floor (cy.x / 4.0f) + 5.0f * cy.x, 7.0f) + 6.0f, 7.0f));
}

#define DIG2(v) _0 + to_int2_cfloat(to_float2(_floor ((v) / 10.0f), mod_f ((v), 10.0f)))
#define T(c) _ic = (_nc -- == 0) ? (c) : _ic;

__DEVICE__ void DTimeSet (float4 d, int2 idt[6], int3 inm[2])
{
  idt[0] = DIG2 (_floor (d.x / 100.0f));
  idt[1] = DIG2 (mod_f (d.x, 100.0f));
  idt[2] = DIG2 (d.z);
  idt[3] = DIG2 (_floor (d.w / 3600.0f));
  idt[4] = DIG2 (_floor (mod_f (d.w, 3600.0f) / 60.0f));
  idt[5] = DIG2 (_floor (mod_f (d.w, 60.0f)));
  inm[0] = MName ((int) (d.y));
  inm[1] = DName (DWk (swi3(d,x,y,z)));
}

__DEVICE__ int GetTxChar (int _nc, int2 idt[6], int3 inm[2])
{
  int _ic;
  float ggggggggggggggggg;
  _ic = 0;
  T(_SP) T(inm[1].x) T(inm[1].y) T(inm[1].z)T(_SP) T(idt[2].x) T(idt[2].y) T(_MINUS) 
  T(inm[0].x) T(inm[0].y) T(inm[0].z) T(_MINUS) T(idt[0].x) T(idt[0].y) T(idt[1].x) T(idt[1].y)
  T(_SP) T(idt[3].x) T(idt[3].y) T(_COLON) T(idt[4].x) T(idt[4].y) T(_COLON) T(idt[5].x) T(idt[5].y)
  return _ic;
}





__DEVICE__ float Hashfv2 (float2 p)
{
  const float cHashM = 43758.54f;
  const float3 cHashA3 = to_float3 (1.0f, 57.0f, 113.0f);
  return fract (_sinf (dot (p, swi2(cHashA3,x,y))) * cHashM);
}

__DEVICE__ float2 Hashv2v2 (float2 p)
{
  const float cHashM = 43758.54f;
  const float2 cHashVA2 = to_float2 (37.1f, 61.7f);
  const float2 e = to_float2 (1.0f, 0.0f);
  return fract_f2(sin_f2(to_float2 (dot (p + swi2(e,y,y), cHashVA2),
                                  dot (p + swi2(e,x,y), cHashVA2))) * cHashM);
}

__DEVICE__ float4 Hashv4f (float p)
{
  const float cHashM = 43758.54f;
  const float4 cHashA4 = to_float4 (0.0f, 1.0f, 57.0f, 58.0f);
  return fract_f4(sin_f4(p + cHashA4) * cHashM);
}

__DEVICE__ float Noisefv2 (float2 p)
{
  const float3 cHashA3 = to_float3 (1.0f, 57.0f, 113.0f);
  float4 t;
  float2 ip, fp;
  ip = _floor (p);
  fp = fract_f2(p);
  fp = fp * fp * (3.0f - 2.0f * fp);
  t = Hashv4f (dot (ip, swi2(cHashA3,x,y)));
  return _mix (_mix (t.x, t.y, fp.x), _mix (t.z, t.w, fp.x), fp.y);
}

__DEVICE__ float Fbm2 (float2 p)
{
  float f, a;
  f = 0.0f;
  a = 1.0f;
  for (int i = 0; i < 5; i ++) {
    f += a * Noisefv2 (p);
    a *= 0.5f;
    p *= 2.0f;
  }
  return f * (1.0f / 1.9375f);
}

__DEVICE__ float Fbmn (float3 p, float3 n)
{
  float3 s = to_float3_s (0.0f);
  float a = 1.0f;
  for (int i = 0; i < 5; i ++) {
    s += a * to_float3 (Noisefv2 (swi2(p,y,z)), Noisefv2 (swi2(p,z,x)), Noisefv2 (swi2(p,x,y)));
    a *= 0.5f;
    p *= 2.0f;
  }
  return dot (s, abs_f3 (n));
}

__DEVICE__ float3 VaryNf (float3 p, float3 n, float f)
{
  float3 g;
  float s;
  const float3 e = to_float3 (0.1f, 0.0f, 0.0f);
  s = Fbmn (p, n);
  g = to_float3 (Fbmn (p + swi3(e,x,y,y), n) - s, Fbmn (p + swi3(e,y,x,y), n) - s,
                 Fbmn (p + swi3(e,y,y,x), n) - s);
  return normalize (n + f * (g - n * dot (n, g)));
}

__DEVICE__ float3 HsvToRgb (float3 c)
{
  float3 p;
  p = abs_f3 (fract_f3 (swi3(c,x,x,x) + to_float3 (1.0f, 2.0f/3.0f, 1.0f/3.0f)) * 6.0f - 3.0f);
  return c.z * _mix (to_float3_s (1.0f), clamp (p - 1.0f, 0.0f, 1.0f), c.y);
}

__DEVICE__ float SmoothMin (float a, float b, float r)
{
  float h;
  h = clamp (0.5f + 0.5f * (b - a) / r, 0.0f, 1.0f);
  return _mix (b, a, h) - r * h * (1.0f - h);
}

__DEVICE__ float SmoothBump (float lo, float hi, float w, float x)
{
  return (1.0f - smoothstep (hi - w, hi + w, x)) * smoothstep (lo - w, lo + w, x);
}

__DEVICE__ float2 Rot2D (float2 q, float a)
{
  return q * _cosf (a) + swi2(q,y,x) * _sinf (a) * to_float2 (-1.0f, 1.0f);
}



__DEVICE__ void CylHit (float3 ro, float3 rd, inout float *dCylIn, inout float *dCylOut, inout float3 *vnCylIn, float dstFar, float bCylRad, float bCylHt)
{
  float3 s;
  float a, ai, b, w, ws, srdy;
  *dCylIn = dstFar;
  *dCylOut = dstFar;
  *vnCylIn = to_float3_s (0.0f);
  a = dot (swi2(rd,x,z), swi2(rd,x,z));
  b = dot (swi2(rd,x,z), swi2(ro,x,z));
  w = b * b - a * (dot (swi2(ro,x,z), swi2(ro,x,z)) - bCylRad * bCylRad);
  if (w > 0.0f) {
    ws = _sqrtf (w);
    srdy = sign_f (rd.y);
    if (a > 0.0f) {
      ai =  1.0f / a;
      *dCylIn = (- b - ws) * ai;
      *dCylOut = (- b + ws) * ai;
    }
    float cccccccccccccccccccccc;
    if (a > 0.0f)  s = ro + *dCylIn * rd;
    else s.y = bCylHt;
    if (_fabs (s.y) < bCylHt) { (*vnCylIn).x = s.x / bCylRad; (*vnCylIn).z = s.z / bCylRad; } //swi2S(*vnCylIn,x,z, swi2(s,x,z) / bCylRad);
    else if (srdy * ro.y < - bCylHt) {
      *dCylIn = - (srdy * ro.y + bCylHt) / _fabs (rd.y);
      if (length (swi2(ro,x,z) + *dCylIn * swi2(rd,x,z)) < bCylRad)   (*vnCylIn).y = - srdy;
      else *dCylIn = dstFar;
    } else *dCylIn = dstFar;
    if (*dCylIn < dstFar) {
      if (a > 0.0f)       s = ro + *dCylOut * rd;
      else                s.y = bCylHt;
      if (_fabs (s.y) > bCylHt && srdy * ro.y < bCylHt)
         *dCylOut = (- srdy * ro.y + bCylHt) / _fabs (rd.y);
    }
  }
}

__DEVICE__ float FontTexDf (float2 p, __TEXTURE2D__ txFnt, int2 idt[6], int3 inm[2])
{
  float3 tx;
  int2 ip;
  float d;
  int ic;
  ic = 0;
  ip = make_int2(_floor (p));
  if (ip.x == 0 && ip.y == 0) ic = GetTxChar (idTxt, idt, inm);
  if (ic != 0) {
    tx = swi3(texture (txFnt, mod_f2 ((to_float2 (mod_f ((float)(ic), 16.0f),
       15.0f - _floor ((float) (ic) / 16.0f)) + fract_f2 (p)) * (1.0f / 16.0f), 1.0f)),y,z,w) - 0.5f;
    qnFnt = to_float2 (tx.x, - tx.y);
    d = tx.z + 1.0f / 256.0f;
  } else d = 1.0f;
  return d;
}

__DEVICE__ float ObjRayT (float3 ro, float3 rd, __TEXTURE2D__ txFnt, int2 idt[6], int3 inm[2], float rngBlk, float2 fntSize, float dstFar)
{
  float3 p, rdi;
  float2 srd, h;
  float dHit, dLim, d;
  if (rd.x == 0.0f) rd.x = 0.001f;
  if (rd.y == 0.0f) rd.y = 0.001f;
  if (rd.z == 0.0f) rd.z = 0.001f;
  srd = -1.0f * sign_f2(swi2(rd,x,y));
  rdi = 1.0f / abs_f3(swi3(rd,x,y,z));
  dHit = 0.0f;
  dLim = rngBlk;
  //swi2(ro,x,y) /= fntSize.x;
  ro.x /= fntSize.x;
  ro.y /= fntSize.x;
  //swi2(rd,x,y) /= fntSize.x;
  rd.x /= fntSize.x;
  rd.y /= fntSize.x;
    
  //swi2(ro,x,y) += 0.5f;
  ro.x += 0.5f;
  ro.y += 0.5f;
  for (int j = 0; j < 150; j ++) {
    p = ro + dHit * rd;
    h = swi2(rdi,x,y) * fract (srd * swi2(p,x,y));
    d = _fmaxf (min (fntSize.x, 1.0f) * FontTexDf (swi2(p,x,y),txFnt,idt,inm), _fabs (p.z) - 0.5f * fntSize.y);
    dHit += _fminf (d, 0.01f + fntSize.x * _fminf (h.x, h.y));
    if (d < 0.0001f || dHit > dLim) break;
  }
  if (d >= 0.0001f) dHit = dstFar;
  return dHit;
}

__DEVICE__ float BlkHit (float3 ro, float3 rd, float3 bSize, float rngBlk, float dstFar)
{
  float3 v, tm, tp;
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
    rngBlk = df - dn;
  }
  return dMin;
}

__DEVICE__ float ObjDf (float3 p, float bCylHt, float bCylRad)
{
  float3 q;
  q = p;
  q.y = _fabs (q.y) - bCylHt;
  return PrTorusDf (swi3(q,x,z,y), 0.02f, bCylRad);
}

__DEVICE__ float ObjRay (float3 ro, float3 rd, float bCylHt, float bCylRad, float dstFar)
{
  float dHit, d;
  dHit = 0.0f;
  for (int j = 0; j < 100; j ++) {
    d = ObjDf (ro + dHit * rd,bCylHt,bCylRad);
    dHit += d;
    if (d < 0.001f || dHit > dstFar) break;
  }
  return dHit;
}

__DEVICE__ float3 ObjNf (float3 p, float bCylHt, float bCylRad, float dstFar)
{
  float4 v;
  const float3 e = to_float3 (0.001f, -0.001f, 0.0f);
  v = to_float4 (ObjDf (p + swi3(e,x,x,x),bCylHt,bCylRad), ObjDf (p + swi3(e,x,y,y),bCylHt,bCylRad), ObjDf (p + swi3(e,y,x,y),bCylHt,bCylRad),
                 ObjDf (p + swi3(e,y,y,x),bCylHt,bCylRad));
  return normalize (to_float3_s(v.x - v.y - v.z - v.w) + 2.0f * swi3(v,y,z,w));
}

#define SQRT3 1.73205

__DEVICE__ float2 PixToHex (float2 p)
{
  float pppppppppppppppppppppppp;
  float3 c, r, dr;
  swi2S(c,x,z, to_float2 ((1.0f/SQRT3) * p.x - (1.0f/3.0f) * p.y, (2.0f/3.0f) * p.y));
  c.y = - c.x - c.z;
  r = _floor (c + 0.5f);
  dr = abs_f3 (r - c);
  r -= step (swi3(dr,y,z,x), dr) * step (swi3(dr,z,x,y), dr) * dot (r, to_float3_s(1.0f));
  return swi2(r,x,z);
}

__DEVICE__ float2 HexToPix (float2 h)
{
  return to_float2 (SQRT3 * (h.x + 0.5f * h.y), (3.0f/2.0f) * h.y);
}

__DEVICE__ void HexVorInit ()
{
  float3 e = to_float3 (1.0f, 0.0f, -1.0f);
  gVec[0] = swi2(e,y,y);
  gVec[1] = swi2(e,x,y);
  gVec[2] = swi2(e,y,x);
  gVec[3] = swi2(e,x,z);
  gVec[4] = swi2(e,z,y);
  gVec[5] = swi2(e,y,z);
  gVec[6] = swi2(e,z,x);
  for (int k = 0; k < 7; k ++) hVec[k] = HexToPix (gVec[k]);
}

__DEVICE__ float4 HexVor (float2 p)
{
  float4 sd, udm;
  float2 ip, fp, d, u;
  float amp, a;
  amp = 0.7f;
  ip = PixToHex (p);
  fp = p - HexToPix (ip);
  sd = to_float4_s (4.0f);
  udm = to_float4_s (4.0f);
  for (int k = 0; k < 7; k ++) {
    u = Hashv2v2 (ip + gVec[k]);
    a = 2.0f * pi * (u.y - 0.5f);
    d = hVec[k] + amp * (0.4f + 0.6f * u.x) * to_float2 (_cosf (a), _sinf (a)) - fp;
    sd.w = dot (d, d);
    if (sd.w < sd.x) {
      sd = swi4(sd,w,x,y,w);
      udm = to_float4_f2f2(d, u);
    } else sd = (sd.w < sd.y) ? swi4(sd,x,w,y,w) : ((sd.w < sd.z) ? swi4(sd,x,y,w,w) : sd);
  }
  swi3S(sd,x,y,z, sqrt_f3 (swi3(sd,x,y,z)));
  return to_float4 (SmoothMin (sd.y, sd.z, 0.3f) - sd.x, udm.x, udm.y, Hashfv2 (swi2(udm,z,w)));
}

__DEVICE__ float3 BgCol (float3 ro, float3 rd, float tCur, float grDep, float3 sunDir)
{
  float4 vc;
  float3 vn, col;
  float s;
  if (rd.y >= 0.0f) {
    rd.y += 0.0001f;
    ro.z += 2.0f * tCur;
    col = _mix (to_float3 (0.1f, 0.2f, 0.4f), to_float3_s (1.0f),
                clamp (0.1f + 0.8f * Fbm2 (0.05f * (swi2(ro,x,z) + swi2(rd,x,z) * (100.0f - ro.y) / rd.y)) * rd.y, 0.0f, 1.0f));
  } else {
    ro -= ((grDep + ro.y) / rd.y) * rd;
    vc = HexVor (2.5f * swi2(ro,z,x));
    s = step (0.06f + 0.03f * vc.w, vc.x);
    col = _mix (to_float3_s (0.8f), HsvToRgb (to_float3 (37.0f * vc.w, 0.7f, 1.0f)) *
       (1.0f - 0.05f * step (0.5f, Fbm2 (15.0f * swi2(ro,x,z)))), s);
    vn = VaryNf (15.0f * ro, to_float3 (0.0f, 1.0f, 0.0f), 2.0f - 1.8f * s);
    col *= (0.1f + 0.9f * _fmaxf (dot (vn, sunDir), 0.0f)) *
       (1.0f - 0.003f * length (swi2(ro,x,z)));
    col = _mix (col, to_float3 (0.2f, 0.3f, 0.5f), _powf (1.0f + rd.y, 5.0f));
  }
  return col;
}

__DEVICE__ float3 ShowScene (float3 ro, float3 rd, __TEXTURE2D__ txFnt, int2 idt[6], int3 inm[2],
                             inout float *dCylIn, inout float *dCylOut, inout float3 *vnCylIn, float dstFar, float bCylRad, float bCylHt,
                             float rngBlk, float tCur, float grDep, float3 sunDir)
{
float zzzzzzzzzzzzzzzzzzz;  
  float3 col, vn, qtTxt, tro, trd, trdd, q, qt, ds;
  float dstTxt, dstBlk, dstObj, db, d, ang, angTxt, angRot, aTw, rad, twFac, sdTxt;
  float2 fntSize = to_float2 (1.7f, 0.6f);
  HexVorInit ();
  dstTxt = dstFar;
  tro = ro;
  trd = rd;
  rad = 7.0f;
  bCylRad = rad + 0.6f * fntSize.x;
  bCylHt = 1.3f * fntSize.y;
  dstObj = ObjRay (ro, rd,bCylHt,bCylRad,dstFar);
  CylHit (ro, rd,dCylIn,dCylOut,vnCylIn,dstFar,bCylRad,bCylHt);
  if (*dCylIn < dstFar) {
    twFac = 3.0f;
    angRot = 0.05f * pi * tCur;
    aTw = angRot * twFac;
    swi2S(tro,x,z, Rot2D (swi2(tro,x,z), angRot));
    swi2S(trd,x,z, Rot2D (swi2(trd,x,z), angRot));
    for (float k = 0.0f; k < 50.0f; k ++) {
      idTxt = (int) (mod_f (k, 25.0f));
      ang = 2.0f * pi * (1.0f - k / 50.0f);
      qt = -1.0f* to_float3 (rad * _sinf (ang), 0.0f, rad * _cosf (ang));
      q = tro - qt;
      swi2S(q,x,z, Rot2D (swi2(q,x,z), ang));
      swi2S(q,y,z, Rot2D (swi2(q,y,z), twFac * ang + aTw));
      trdd = trd;
      swi2S(trdd,x,z, Rot2D (swi2(trdd,x,z), ang));
      swi2S(trdd,y,z, Rot2D (swi2(trdd,y,z), twFac * ang + aTw));
      db = BlkHit (q, trdd, to_float3 (0.5f * fntSize.x, 0.55f * fntSize.x, 0.55f * fntSize.y),rngBlk,dstFar);
      if (db < dstFar) {
        d = db + ObjRayT (q + db * trdd, trdd, txFnt,idt,inm,rngBlk,fntSize,dstFar);
        if (d < dstTxt) {
          dstTxt = d;
          qtTxt = qt;
          angTxt = ang;
          qnTxt = qnFnt;
          sdTxt = _floor (k / 25.0f);
        }
      }
    }
  }
  if (dstTxt < _fminf (dstObj, dstFar)) {
    tro += trd * dstTxt;
    ds = tro - qtTxt;
    swi2S(ds,x,z, Rot2D (swi2(ds,x,z), angTxt));
    swi2S(ds,y,z, Rot2D (swi2(ds,y,z), twFac * angTxt + aTw));
    if (_fabs (ds.z) < 0.49f * fntSize.y) {
      vn = normalize (to_float3_aw (qnTxt, 0.00001f));
      col = to_float3 (0.8f, 0.8f, 1.0f);
    } else {
      vn = to_float3 (0.0f, 0.0f, sign_f(ds.z));
      col = (sdTxt == 0.0f) ? to_float3(1.0f, 0.3f, 0.3f) : to_float3 (1.0f, 1.0f, 0.0f);
    }
    swi2S(vn,y,z, Rot2D (swi2(vn,y,z), - twFac * angTxt - aTw));
    swi2S(vn,x,z, Rot2D (swi2(vn,x,z), - angRot - angTxt));
    col = col * (0.2f + 0.8f * _fmaxf (dot (sunDir, vn), 0.0f)) +
          0.5f * _powf (_fmaxf (dot (normalize (sunDir - rd), vn), 0.0f), 128.0f);
    col = _mix (col, BgCol (tro, reflect (rd, vn),tCur,grDep,sunDir), 0.7f - 0.5f * _fabs (dot (rd, vn)));
  } else if (dstObj < _fminf (dstTxt, dstFar)) {
    vn = ObjNf (ro + dstObj * rd,bCylHt,bCylRad,dstFar);
    col = to_float3 (0.2f, 0.5f, 0.7f) * (0.2f + 0.8f * _fmaxf (dot (sunDir, vn), 0.0f)) +
       0.5f * _powf (_fmaxf (dot (normalize (sunDir - rd), vn), 0.0f), 128.0f);
  } else col = BgCol (ro, rd,tCur,grDep,sunDir);
  if (*dCylIn < dstFar) {
    col = _mix (col,  to_float3_s (1.0f) * (0.2f + 0.8f * _fmaxf (dot (sunDir, *vnCylIn), 0.0f) +
          0.5f * _powf (_fmaxf (dot (normalize (sunDir - rd), *vnCylIn), 0.0f), 128.0f)), 0.15f);
    col = _mix (col, BgCol (ro + *dCylIn * rd,
          reflect (rd, *vnCylIn),tCur,grDep,sunDir), _powf (1.0f - _fabs (dot (rd, *vnCylIn)), 4.0f));
  }
  return clamp (col, 0.0f, 1.0f);
}

__KERNEL__ void TwistedTimeFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float4 iDate, sampler2D iChannel0)
{
  
  float dstFar, tCur, grDep, rngBlk, bCylRad, bCylHt, dCylIn, dCylOut;
  
  float3 vnCylIn;
  
  int3 inm[2];
  int2 idt[6];
float IIIIIIIIIIIIIIIIIIIIIII;
  mat3 vuMat;
  float4 mPtr;
  float3 ro, rd, col;
  float2 canvas, uv, ori, ca, sa;
  float el, az;
  canvas = iResolution;
  uv = 2.0f * fragCoord / canvas - 1.0f;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  float4 dateCur = iDate;
  mPtr = iMouse;
  //swi2(mPtr,x,y) = swi2(mPtr,x,y) / canvas - 0.5f;
  mPtr.x = mPtr.x / canvas.x - 0.5f;
  mPtr.y = mPtr.y / canvas.y - 0.5f;
  az = 0.0f;
  el = 0.08f * pi * _sinf (0.1f * pi * tCur);
  if (mPtr.z > 0.0f) {
    az += 2.0f * pi * mPtr.x;
    el += 0.4f * pi * mPtr.y;
  }
  el = clamp (el, -0.2f * pi, 0.1f * pi);
  ori = to_float2 (el, az);
  ca = cos_f2 (ori);
  sa = sin_f2 (ori);
  vuMat = mul_mat3_mat3(to_mat3(ca.y, 0.0f, - sa.y, 0.0f, 1.0f, 0.0f, sa.y, 0.0f, ca.y) ,
                        to_mat3(1.0f, 0.0f, 0.0f, 0.0f, ca.x, - sa.x, 0.0f, sa.x, ca.x));
  rd = mul_mat3_f3(vuMat , normalize (to_float3_aw (uv, 2.1f)));
  ro = mul_mat3_f3(vuMat , to_float3 (0.0f, 0.0f, -14.0f));
  float3 sunDir = mul_mat3_f3(vuMat , normalize (to_float3 (0.5f, 1.0f, -1.0f)));
  dstFar = 100.0f;
  grDep = 5.0f;
  DTimeSet (dateCur,idt,inm);
  col = ShowScene (ro, rd, txFnt,idt,inm,&dCylIn,&dCylOut,&vnCylIn,dstFar,bCylRad,bCylHt,rngBlk,tCur,grDep,sunDir);
  fragColor = to_float4_aw(clamp (col, 0.0f, 1.0f), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}