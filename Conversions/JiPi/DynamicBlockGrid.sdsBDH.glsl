

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "Dynamic Block Grid" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

// Constant gap between adjacent blocks (full 3D extension of "Random Rectangular Tiling",
// the fixed-width channels were used in, e.g., "Green Mercury")

#if 0
#define VAR_ZERO min (iFrame, 0)
#else
#define VAR_ZERO 0
#endif

float PrRound4BoxDf (vec3 p, vec3 b, float r);
float PrRound4Box2Df (vec2 p, vec2 b, float r);
float Minv2 (vec2 p);
mat3 StdVuMat (float el, float az);
vec2 Rot2D (vec2 q, float a);
vec3 HsvToRgb (vec3 c);
float Hashfv2 (vec2 p);
vec3 VaryNf (vec3 p, vec3 n, float f);

vec3 ltDir;
vec2 pMid[4], pEdge[4], qcMin, ip;
float tCur, dstFar, eFac, eRound, gSize;
const float pi = 3.1415927;

#define H(z) (0.5 + 0.5 * cos (1.5 * tCur * (0.1 + 0.9 * Hashfv2 (ip + z))))

void CellParms ()
{
  vec4 hm, hc;
  vec3 e;
  vec2 ee[4];
  float hp;
  e = vec3 (-1., 0., 1.);
  ee[0] = e.xz;
  ee[1] = e.zz;
  ee[2] = e.zx;
  ee[3] = e.xx;
  hp = H(0.);
  hm = vec4 (H(e.zy), H(e.xy), H(e.yz), H(e.yx));
  hc = vec4 (H(e.zz), H(e.xx), H(e.xz), H(e.zx));
  if (mod (ip.x + ip.y, 2.) < 0.5) {
    pEdge[0] = vec2 (hm.z - hm.y, hc.z - hp);
    pEdge[1] = vec2 (hm.x - hm.z, hc.x - hp);
    pEdge[2] = vec2 (hm.x - hm.w, hp - hc.w);
    pEdge[3] = vec2 (hm.w - hm.y, hp - hc.y);
    pMid[0] = vec2 (hm.z, hp);
    pMid[1] = pMid[0];
    pMid[2] = vec2 (hm.w, hp);
    pMid[3] = pMid[2];
  } else {
    pEdge[0] = vec2 (hp - hc.z, hm.z - hm.y);
    pEdge[1] = vec2 (hc.x - hp, hm.z - hm.x);
    pEdge[2] = vec2 (hc.w - hp, hm.x - hm.w);
    pEdge[3] = vec2 (hp - hc.y, hm.y - hm.w);
    pMid[0] = vec2 (hp, hm.y);
    pMid[1] = vec2 (hp, hm.x);
    pMid[2] = pMid[1];
    pMid[3] = pMid[0];
  }
  for (int k = 0; k < 4; k ++) {
    pEdge[k] = eFac * pEdge[k] + 0.5;
    pMid[k] = 2. * eFac * (pMid[k] - 0.5) + pEdge[k] * ee[k];
  }
}

float ObjDf (vec3 p)
{
  vec3 q, bs;
  vec2 qc;
  float dMin, d;
  dMin = dstFar;
  for (int k = 0; k < 4; k ++) {
    qc = ip + pMid[k];
    q.xz = p.xz - gSize * qc;
    qc = floor (qc);
    bs.xz = pEdge[k] - eFac + 0.05;
    bs.y = 0.4 * (bs.x + bs.z) + 0.1 * Hashfv2 (qc);
    q.y = p.y - gSize * bs.y;
    d = gSize * PrRound4BoxDf (q / gSize, bs - eRound, eRound);
    if (d < dMin) {
      dMin = d;
      qcMin = qc;
    }
  }
  return dMin;
}

#if 1

float ObjRay (vec3 ro, vec3 rd)  // (cell-based ray-marching)
{
  vec3 p, rdi;
  vec2 fp, ipP;
  float dHit, d, eps;
  eps = 0.001;
  if (rd.x == 0.) rd.x = 0.0001;
  if (rd.z == 0.) rd.z = 0.0001;
  rdi.xz = 1. / rd.xz;
  dHit = eps;
  ipP = vec2 (0.5);
  for (int j = VAR_ZERO; j < 160; j ++) {
    p = ro + dHit * rd;
    fp = p.xz + 0.5;
    ip = floor (fp / gSize);
    if (ip != ipP) {
      ipP = ip;
      CellParms ();
    }
    d = ObjDf (p);
    dHit += min (d, eps + max (0., Minv2 ((gSize * (ip + step (0., rd.xz)) - fp) * rdi.xz)));
    if (d < eps || dHit > dstFar || p.y < 0.) break;
  }
  if (d >= eps || p.y < 0.) dHit = dstFar;
  return dHit;
}

#else

float ObjRay (vec3 ro, vec3 rd)  // (simple ray-marching - visual artifacts)
{
  vec3 p;
  vec2 ipP;
  float dHit, d;
  dHit = 0.;
  ipP = vec2 (0.5);
  for (int j = VAR_ZERO; j < 160; j ++) {
    p = ro + dHit * rd;
    ip = floor ((p.xz + 0.5) / gSize);
    if (ip != ipP) {
      ipP = ip;
      CellParms ();
    }
    d = ObjDf (p);
    if (d < 0.001 || dHit > dstFar || p.y < 0.) break;
    dHit += d;
  }
  if (p.y < 0.) dHit = dstFar;
  return dHit;
}

#endif

vec3 ObjNf (vec3 p)
{
  vec4 v;
  vec2 e;
  e = vec2 (0.001, -0.001);
  for (int j = VAR_ZERO; j < 4; j ++) {
    v[j] = ObjDf (p + ((j < 2) ? ((j == 0) ? e.xxx : e.xyy) : ((j == 2) ? e.yxy : e.yyx)));
  }
  v.x = - v.x;
  return normalize (2. * v.yzw - dot (v, vec4 (1.)));
}

float ObjSShadow (vec3 ro, vec3 rd)
{
  vec3 p;
  vec2 ipP;
  float sh, d, h;
  sh = 1.;
  d = 0.01;
  ipP = vec2 (0.5);
  for (int j = VAR_ZERO; j < 24; j ++) {
    p = ro + d * rd;
    ip = floor ((p.xz + 0.5) / gSize);
    if (ip != ipP) {
      ipP = ip;
      CellParms ();
    }
    h = ObjDf (p);
    sh = min (sh, smoothstep (0., 0.1 * d, h));
    d += max (h, 0.01);
    if (h < 0.001 || d > dstFar) break;
  }
  return 0.5 + 0.5 * sh;
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec4 col4;
  vec3 col, vn;
  vec2 q, b;
  float dstObj, dstGrnd, sh, nDotL, s;
  eFac = 0.2;
  eRound = 0.2;
  gSize = 1.;
  dstObj = ObjRay (ro, rd);
  dstGrnd = dstFar;
  if (dstObj < dstFar || rd.y < 0.) {
    if (dstObj < dstFar) {
      ro += dstObj * rd;
      vn = ObjNf (ro);
      col4 = vec4 (HsvToRgb (vec3 (Hashfv2 (qcMin), 0.7, 1.)), 0.2);
      nDotL = max (dot (vn, ltDir), 0.);
      nDotL *= nDotL;
      s = 1.;
    } else {
      dstGrnd = - ro.y / rd.y;
      ro += dstGrnd * rd;
      vn = vec3 (0., 1., 0.);
      vn = VaryNf (16. * ro, vn, 0.5);
      col4 = vec4 (0.4, 0.4, 0.4, 0.);
      ip = floor ((ro.xz + 0.5) / gSize);
      CellParms ();
      s = 1.;
      for (int k = 0; k < 4; k ++) {
        q = ro.xz - gSize * (ip + pMid[k]);
        b = pEdge[k] - eFac + 0.1;
        s = min (s, gSize * PrRound4Box2Df (q / gSize, b - eRound, eRound));
      }
      col4 *= 1. + 0.7 * (1. - smoothstep (0.01, 0.02, abs (s)));
      s = 1.;
      for (int k = 0; k < 4; k ++) {
        q = ro.xz - gSize * (ip + pMid[k]);
        b = pEdge[k] - eFac;
        s = min (s, gSize * PrRound4Box2Df (q / gSize, b - eRound, eRound));
      }
      s = 0.5 + 0.5 * smoothstep (0., 0.02, s - 0.02);
      nDotL = max (dot (vn, ltDir), 0.);
    }
    sh = (dstGrnd <= dstFar) ? min (s, ObjSShadow (ro + 0.01 * vn, ltDir)) : 1.;
    col = col4.rgb * (0.1 + 0.1 * max (vn.y, 0.) + 0.8 * sh * nDotL) +
       col4.a * step (0.95, sh) * pow (max (dot (ltDir, reflect (rd, vn)), 0.), 32.);
  } else {
    col = vec3 (0.6, 0.6, 1.);
  }
  return clamp (col, 0., 1.);
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
  vec4 mPtr;
  vec3 ro, rd, col;
  vec2 canvas, uv;
  float el, az, zmFac;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  tCur += 1.;
  az = 0.;
  el = -0.15 * pi;
  if (mPtr.z > 0.) {
    az += 2. * pi * mPtr.x;
    el += pi * mPtr.y;
  } else {
    az += 0.03 * pi * sin (0.1 * pi * tCur);
  }
  el = clamp (el, -0.4 * pi, -0.12 * pi);
  ltDir = normalize (vec3 (0.7, 1.5, -1.));
  vuMat = StdVuMat (el, az);
  ro = vec3 (0.07 * tCur, 6., 0.2 * tCur);
  zmFac = 4.;
  dstFar = 120.;
  rd = vuMat * normalize (vec3 (uv, zmFac));
  col = ShowScene (ro, rd);
  fragColor = vec4 (col, 1.);
}

float PrRound4BoxDf (vec3 p, vec3 b, float r)
{
  p = max (abs (p) - b, 0.);
  return sqrt (length (p * p)) - r;
}

float PrRound4Box2Df (vec2 p, vec2 b, float r)
{
  p = max (abs (p) - b, 0.);
  return sqrt (length (p * p)) - r;
}

float Minv2 (vec2 p)
{
  return min (p.x, p.y);
}

mat3 StdVuMat (float el, float az)
{
  vec2 ori, ca, sa;
  ori = vec2 (el, az);
  ca = cos (ori);
  sa = sin (ori);
  return mat3 (ca.y, 0., - sa.y, 0., 1., 0., sa.y, 0., ca.y) *
         mat3 (1., 0., 0., 0., ca.x, - sa.x, 0., sa.x, ca.x);
}

vec2 Rot2D (vec2 q, float a)
{
  vec2 cs;
  cs = sin (a + vec2 (0.5 * pi, 0.));
  return vec2 (dot (q, vec2 (cs.x, - cs.y)), dot (q.yx, cs));
}

vec3 HsvToRgb (vec3 c)
{
  return c.z * mix (vec3 (1.), clamp (abs (fract (c.xxx + vec3 (1., 2./3., 1./3.)) * 6. - 3.) - 1., 0., 1.), c.y);
}

const float cHashM = 43758.54;

float Hashfv2 (vec2 p)
{
  return fract (sin (dot (p, vec2 (37., 39.))) * cHashM);
}

vec2 Hashv2v2 (vec2 p)
{
  vec2 cHashVA2 = vec2 (37., 39.);
  return fract (sin (dot (p, cHashVA2) + vec2 (0., cHashVA2.x)) * cHashM);
}

float Noisefv2 (vec2 p)
{
  vec2 t, ip, fp;
  ip = floor (p);  
  fp = fract (p);
  fp = fp * fp * (3. - 2. * fp);
  t = mix (Hashv2v2 (ip), Hashv2v2 (ip + vec2 (0., 1.)), fp.y);
  return mix (t.x, t.y, fp.x);
}

float Fbmn (vec3 p, vec3 n)
{
  vec3 s;
  float a;
  s = vec3 (0.);
  a = 1.;
  for (int j = 0; j < 5; j ++) {
    s += a * vec3 (Noisefv2 (p.yz), Noisefv2 (p.zx), Noisefv2 (p.xy));
    a *= 0.5;
    p *= 2.;
  }
  return dot (s, abs (n));
}

vec3 VaryNf (vec3 p, vec3 n, float f)
{
  vec4 v;
  vec3 g;
  vec2 e;
  e = vec2 (0.1, 0.);
  for (int j = VAR_ZERO; j < 4; j ++)
     v[j] = Fbmn (p + ((j < 2) ? ((j == 0) ? e.xyy : e.yxy) : ((j == 2) ? e.yyx : e.yyy)), n);
  g = v.xyz - v.w;
  return normalize (n + f * (g - n * dot (n, g)));
}
