
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// "Fireworks 3d" by dr2 - 2018
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License

// Simple 3d fireworks - happy new year

//float tCur, dstFar;
#define pi  3.14159f
#define phi 1.618034f


__DEVICE__ float2 Rot2D (float2 q, float a)
{
  float2 cs;
  cs = sin_f2(a + to_float2 (0.5f * pi, 0.0f));
  return to_float2 (dot (q, to_float2 (cs.x, - cs.y)), dot (swi2(q,y,x), cs));
}

__DEVICE__ float3 HsvToRgb (float3 c)
{
  float hhhhhhhhhhhhhhhh;
  return c.z * _mix (to_float3_s(1.0f), clamp (abs_f3 (fract_f3(swi3(c,x,x,x) + to_float3 (1.0f, 2.0f/3.0f, 1.0f/3.0f)) * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f), c.y);
}

#define cHashM 43758.54f

__DEVICE__ float Hashff (float p)
{
  return fract (_sinf(p) * cHashM);
}

__DEVICE__ float Hashfv3 (float3 p)
{
  return fract(_sinf(dot (p, to_float3 (37.0f, 39.0f, 41.0f))) * cHashM);
}

__DEVICE__ float4 Hashv4v3 (float3 p)
{
  float3 cHashVA3 = to_float3 (37.0f, 39.0f, 41.0f);
  float2 e = to_float2 (1.0f, 0.0f);
  return fract_f4 (sin_f4(to_float4 (dot (p + swi3(e,y,y,y), cHashVA3), dot (p + swi3(e,x,y,y), cHashVA3),
                                     dot (p + swi3(e,y,x,y), cHashVA3), dot (p + swi3(e,x,x,y), cHashVA3))) * cHashM);
}

__DEVICE__ float Noisefv3 (float3 p)
{
  float4 t;
  float3 ip, fp;
  ip = _floor (p);
  fp = fract_f3(p);
  fp *= fp * (3.0f - 2.0f * fp);
  t = _mix (Hashv4v3 (ip), Hashv4v3 (ip + to_float3 (0.0f, 0.0f, 1.0f)), fp.z);
  return _mix(_mix (t.x, t.y, fp.x), _mix (t.z, t.w, fp.x), fp.y);
}

__DEVICE__ float2 BallHit (float3 ro, float3 rd, float3 p, float s, float dstFar)
{
  float3 v;
  float dbIn, dbOut, b, d;
  v = ro - p;
  b = dot (rd, v);
  d = b * b + s * s - dot (v, v);
  dbIn = dstFar;
  dbOut = dstFar;
  if (d > 0.0f) {
    d = _sqrtf (d);
    dbIn = - b - d;
    dbOut = - b + d;
  }
  return to_float2 (dbIn, dbOut);
}

__DEVICE__ float4 SphFib (float3 v, float n)
{   // Keinert et al's inverse spherical Fibonacci mapping
  float4 b;
  float3 vf, vfMin;
  float2 ff, c;
  float fk, ddMin, dd, a, z, ni;
  ni = 1.0f / n;
  fk = _powf (phi, _fmaxf (2.0f, _floor (_logf (n * pi * _sqrtf (5.0f) * dot (swi2(v,x,y), swi2(v,x,y))) /
     _logf (phi + 1.0f)))) / _sqrtf (5.0f);
  ff = to_float2 (_floor (fk + 0.5f), _floor (fk * phi + 0.5f));
  b = to_float4_f2f2(ff * ni, pi * (fract_f2((ff + 1.0f) * phi) - (phi - 1.0f)));
  c = _floor (0.5f * mul_mat2_f2(to_mat2(b.y, - b.x, b.w, - b.z),
                                  to_float2 (_atan2f (v.y, v.x), v.z - (1.0f - ni))) / (b.y * b.z - b.x * b.w) );
                                  
                                 
  ddMin = 4.1f;
  for (int j = 0; j < 4; j ++) {
    a = dot (ff, to_float2 (j - 2 * (j / 2), j / 2) + c);
    z = 1.0f - (2.0f * a + 1.0f) * ni;
    vf = to_float3_aw (sin_f2(2.0f * pi * fract (phi * a) + to_float2 (0.5f * pi, 0.0f)) * _sqrtf (1.0f - z * z), z);
    dd = dot (vf - v, vf - v);
    if (dd < ddMin) {
      ddMin = dd;
      vfMin = vf;
    }
  }
  return to_float4(_sqrtf (ddMin), vfMin.x,vfMin.y,vfMin.z);
}

__DEVICE__ float3 SkyCol(float3 ro, float3 rd, float tCur)
{
  
  float3 col, rds, mDir, vn;
  float mRad, bs, ts, f;
  swi2(rd,x,z) = Rot2D (swi2(rd,x,z), 0.001f * tCur);
  mDir = normalize (to_float3 (0.0f, 1.0f, 1.0f));
  mRad = 0.02f;
  col = to_float3 (0.02f, 0.02f, 0.04f) + to_float3 (0.06f, 0.04f, 0.02f) *
        _powf (clamp (dot (rd, mDir), 0.0f, 1.0f), 16.0f);
  bs = dot (rd, mDir);
  ts = bs * bs - dot (mDir, mDir) + mRad * mRad;
  if (ts > 0.0f) {
    ts = bs - _sqrtf (ts);
    if (ts > 0.0f) {
      vn = normalize ((ts * rd - mDir) / mRad);
      col += 0.8f * to_float3 (1.0f, 0.9f, 0.5f) * clamp (dot (to_float3 (-0.77f, 0.4f, 0.5f), vn) *
         (1.0f - 0.3f * Noisefv3 (4.0f * vn)), 0.0f, 1.0f);
    }
    col *= 1.3f;
  } else {
    rds = _floor (2000.0f * rd);
    rds = 0.00015f * rds + 0.1f * Noisefv3 (0.0005f * swi3(rds,y,z,x));
    for (int j = 0; j < 19; j ++) rds = abs_f3(rds) / dot (rds, rds) - 0.9f;
    col += 0.5f * smoothstep (0.01f, 0.04f, rd.y) * to_float3 (0.8f, 0.8f, 0.6f) *
       _fminf (1.0f, 0.5e-3 * _powf (_fminf (6.0f, length (rds)), 5.0f));
  }
  return col;
}

__DEVICE__ float3 ShowScene (float3 ro, float3 rd, float tCur, float dstFar)
{
  float4 f4;
  float3 col, bPos;
  float2 dSph;
  float nCyc, tCyc, iFib, phs, hm, hr, h, s, r, a;
  bool isBg;
  tCyc = 3.0f;
  nCyc = _floor (tCur / tCyc) + 1.0f;
  hm = 0.2f * _fmaxf (Hashff (17.0f * nCyc) - 0.2f, 0.0f);
  hr = 0.8f * _fminf (2.0f * Hashff (27.0f * nCyc), 1.0f);
  iFib = 500.0f + _floor (6000.0f * Hashff (37.0f * nCyc));
  col = SkyCol (ro, rd, tCur);
  isBg = true;
  for (float k = 0.0f; k < 40.0f; k ++) {
    phs = fract (tCur / tCyc) - 0.005f * k;
    bPos = to_float3 (0.0f, 0.0f, 0.0f);
    if (phs > 0.1f) {
      a = smoothstep (0.1f, 0.15f, phs) - 0.7f * smoothstep (0.3f, 1.0f, phs);
      h = hm + hr * _fmaxf (phs - 0.2f, 0.0f);
      dSph = BallHit (ro, rd, bPos, 0.5f + 8.5f * _sqrtf (phs - 0.1f), dstFar);
      if (dSph.x < dstFar) {
        if (k == 0.0f) col = _mix (col, to_float3 (1.0f, 1.0f, 0.0f), 0.03f * a);
        r = 0.015f * (0.5f + 0.5f * a);
        ro += dSph.x * rd;
        f4 = SphFib (normalize (ro), iFib);
        s = Hashfv3 (73.0f * swi3(f4,y,z,w) + 87.0f * nCyc);
        if (s > 0.5f && f4.x < r * s) {
          col = _mix (col, HsvToRgb (to_float3 (h, 1.0f, a)), a);
          isBg = false;
        } else {
          ro += (dSph.y - dSph.x) * rd;
          f4 = SphFib (normalize (ro), iFib);
          s = Hashfv3 (73.0f * swi3(f4,y,z,w) + 87.0f * nCyc);
          if (s > 0.5f && f4.x < r * s) {
            col = _mix (col, HsvToRgb (to_float3 (h, 1.0f, 0.7f * a)), a);
            isBg = false;
          }
        }
      }
      if (! isBg) break;
    }
  }
  return clamp (col, 0.0f, 1.0f);
}

#define AA  1   // optional antialiasing

__KERNEL__ void Fireworks3DFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

  mat3 vuMat;
  float4 mPtr;
  float3 ro, rd, col;
  float2 canvas, uv, ori, ca, sa;
  float el, az, sr;
  canvas = iResolution;
  uv = 2.0f * fragCoord / canvas - 1.0f;
  uv.x *= canvas.x / canvas.y;
  float tCur = iTime;
  mPtr = iMouse;
  //swi2(mPtr,x,y) = swi2(mPtr,x,y) / canvas - 0.5f;
  mPtr.x = mPtr.x / canvas.x - 0.5f;
  mPtr.y = mPtr.y / canvas.y - 0.5f;
  az = -0.1f * pi;
  el = 0.2f * pi;
  if (mPtr.z > 0.0f) {
    az += 0.5f * pi * mPtr.x;
    el += 0.2f * pi * mPtr.y;
  } else {
    az += 0.001f * pi * tCur;
  }
  ori = to_float2 (el, az);
  ca = cos_f2(ori);
  sa = sin_f2(ori);
  vuMat = mul_mat3_mat3(to_mat3 (ca.y, 0.0f, - sa.y, 0.0f, 1.0f, 0.0f, sa.y, 0.0f, ca.y) ,
                        to_mat3 (1.0f, 0.0f, 0.0f, 0.0f, ca.x, - sa.x, 0.0f, sa.x, ca.x));
  ro = mul_mat3_f3(vuMat , to_float3 (0.0f, 0.0f, -40.0f));
  float dstFar = 100.0f;
#if ! AA
  const float naa = 1.0f;
#else
  const float naa = 3.0f;
#endif  
  col = to_float3_s (0.0f);
  sr = 2.0f * mod_f (dot (mod_f (_floor (0.5f * (uv + 1.0f) * canvas), 2.0f), to_float2_s (1.0f)), 2.0f) - 1.0f;
  for (float a = 0.0f; a < naa; a +=1.0f) {
    rd = mul_mat3_f3(vuMat , normalize (to_float3_aw (uv + step (1.5f, naa) * Rot2D (to_float2 (0.5f / canvas.y, 0.0f),
                                          sr * (0.667f * a + 0.5f) * pi), 4.5f)));
    col += (1.0f / naa) * ShowScene (ro, rd, tCur, dstFar);
  }
  fragColor = to_float4_aw(col, 1.0f);
  
  SetFragmentShaderComputedColor(fragColor);
}

