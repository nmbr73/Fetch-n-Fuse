

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "Stomper" by dr2 - 2021
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

#if 0
#define VAR_ZERO min (iFrame, 0)
#else
#define VAR_ZERO 0
#endif

float PrBoxDf (vec3 p, vec3 b);
float PrBox2Df (vec2 p, vec2 b);
float PrSphDf (vec3 p, float r);
float PrCylDf (vec3 p, float r, float h);
float PrCapsDf (vec3 p, float r, float h);
mat3 StdVuMat (float el, float az);
vec2 Rot2D (vec2 q, float a);
vec2 Rot2Cs (vec2 q, vec2 cs);
vec2 Hashv2f (float p);
float Fbm2 (vec2 p);

vec3 ltDir, qHit, pStomp;
vec2 pAct, pActP;
float tCur, dstFar, boxSz, tCyc, tPhs;
int idObj;
int idPlat = 1, idBallR = 2, idBallM = 3, idStomp = 4, idTube = 5, idGrnd = 6;
const float pi = 3.1415927;

struct TbCon {
  vec3 pLo, pHi;
  vec2 aLimCs, tRotCs[2], pRotCs[2];
  float chLen, chDist, ang, rad;
};
TbCon tbCon;

#define DMINQ(id) if (d < dMin) { dMin = d;  idObj = id;  qHit = q; }

float ObjDf (vec3 p)
{
  vec3 q;
  float dMin, d, db, a;
  dMin = dstFar;
  q = p;
  db = PrBox2Df (q.xz, vec2 (boxSz));
  d = max (PrBoxDf (q, vec3 (vec2 (boxSz + 0.5), 0.5).xzy), - max (db,
     PrCylDf (vec3 (fract (q.xz) - 0.5, q.y - 0.5), 0.4, 0.8)));
  DMINQ (idPlat);
  d = max (db, - PrBox2Df (q.xz - pAct, vec2 (0.5)));
  q.xz = fract (q.xz) - 0.5;
  d = max (PrSphDf (vec3 (q.xz, q.y - 0.15).xzy, 0.4), d);
  DMINQ (idBallR);
  q = p;
  q.xz -= pAct;
  q.y -= 0.15;
  if (tPhs < 0.8) q.y -= 0.4 * (1. - cos (10. * pi * tCyc));
  d = PrSphDf (q, 0.4);
  DMINQ (idBallM);
  q = p;
  q -= pStomp;
  d = min (PrCylDf (q.xzy, 0.35, 0.05), max (PrCapsDf (q.xzy, 0.1, 0.3), - q.y));
  DMINQ (idStomp);
  q = p;
  q.xz -= tbCon.pLo.xz;
  d = PrCylDf (q.xzy, 0.4, 0.5);
  DMINQ (idPlat);
  q.y -= 0.5;
  d = max (PrCapsDf (q.xzy, 0.1, 0.3), - q.y);
  DMINQ (idTube);
  d = min (PrSphDf (p - tbCon.pLo, 0.1), PrSphDf (p - tbCon.pHi, 0.1));
  DMINQ (idTube);
  q = p - tbCon.pLo;
  q.xz = Rot2Cs (q.xz, tbCon.tRotCs[0]);
  q.yz = Rot2Cs (q.yz, tbCon.tRotCs[1]) - vec2 (tbCon.chLen, tbCon.chDist);
  a = fract ((256. / tbCon.ang) * atan (q.y, - q.z) / (2. * pi));
  d = max (dot (vec2 (abs (q.y), - q.z), tbCon.aLimCs), length (vec2 (length (q.yz) -
     tbCon.rad, q.x)) - (0.1 - 0.015 * smoothstep (0.15, 0.35, 0.5 - abs (0.5 - a))));
  DMINQ (idTube);
  q = p;
  d = max (q.y - 0.4, - db);
  DMINQ (idGrnd);
  return dMin;
}

float ObjRay (vec3 ro, vec3 rd)
{
  float dHit, d;
  dHit = 0.;
  for (int j = VAR_ZERO; j < 120; j ++) {
    d = ObjDf (ro + dHit * rd);
    if (d < 0.0005 || dHit > dstFar) break;
    dHit += d;
  }
  return dHit;
}

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
  float sh, d, h;
  int idObjT;
  idObjT = idObj;
  sh = 1.;
  d = 0.02;
  for (int j = VAR_ZERO; j < 30; j ++) {
    h = ObjDf (ro + d * rd);
    sh = min (sh, smoothstep (0., 0.05 * d, h));
    d += h;
    if (sh < 0.05) break;
  }
  idObj = idObjT;
  return 0.5 + 0.5 * sh;
}

#define F(x) (sin (x) / x - b)

float SecSolve (float b)
{  // (from "Bucking Bronco")
  vec3 t;
  vec2 f;
  float x;
  if (b < 0.95) {
    t.yz = vec2 (0.7, 1.2);
    f = vec2 (F(t.y), F(t.z));
    for (int nIt = 0; nIt < 4; nIt ++) {
      t.x = (t.z * f.x - t.y * f.y) / (f.x - f.y);
      t.zy = t.yx;
      f = vec2 (F(t.x), f.x);
    }
    x = t.x;
  } else if (b < 1.) {
    x = sqrt (10. * (1. - sqrt (1. - 1.2 * (1. - b))));
  } else {
    x = 0.;
  }
  return x;
}

void SetConf ()
{
  vec3 vp;
  float tubeLen;
  boxSz = 4.;
  tCyc = tCur / 2. + 1.;
  tPhs = fract (tCyc);
  pAct = floor (boxSz * (2. * Hashv2f (1.11 + 17.33 * floor (tCyc)) - 1.)) + 0.5;
  pActP = floor (boxSz * (2. * Hashv2f (1.11 + 17.33 * floor (tCyc - 1.)) - 1.)) + 0.5;
  if (tPhs < 0.8) {
    pStomp.xz = mix (pActP, pAct, tPhs / 0.8);
    pStomp.y = 1.7;
  } else {
    pStomp.xz = pAct;
    pStomp.y = mix (0.5, 2., max (2. * abs (tPhs - 0.9) / 0.1 - 1., 0.));
  }
  tbCon.pLo = vec3 (1.5 * boxSz, 0.9, 0.);
  tbCon.pHi = pStomp;
  tbCon.pHi.y += 0.4;
  vp = tbCon.pHi - tbCon.pLo;
  tbCon.chLen = 0.5 * length (vp);
  tbCon.tRotCs[0] = sin (atan (vp.x, vp.z) + vec2 (0.5 * pi, 0.));
  tbCon.tRotCs[1] = sin (- asin (length (vp.xz) / length (vp)) + vec2 (0.5 * pi, 0.));
  tubeLen = 1.4 * boxSz;
  tbCon.ang = SecSolve (tbCon.chLen / tubeLen);
  tbCon.chDist = tbCon.chLen / tan (tbCon.ang);
  tbCon.rad = length (vec2 (tbCon.chDist, tbCon.chLen));
  tbCon.aLimCs = sin (- tbCon.ang + vec2 (0.5 * pi, 0.));
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec4 col4;
  vec3 col, vn;
  float dstObj, sh, nDotL;
  SetConf ();
  dstObj = ObjRay (ro, rd);
  if (dstObj < dstFar) {
    ro += dstObj * rd;
    vn = ObjNf (ro);
    if (idObj == idPlat) {
      col4 = vec4 (0.5, 0.2, 0., 0.2);
    } else if (idObj == idGrnd) {
      col4 = vec4 (0.5, 0.2, 0., 0.2) * (0.8 + 0.2 * Fbm2 (4. * ro.xz));
    } else if (idObj == idBallR) {
      col4 = vec4 (0., 1., 0., 0.2);
      if (PrBox2Df (ro.xz - pActP, vec2 (0.5)) < 0.) col4 = mix (vec4 (1., 0., 0., 0.2) *
         (0.7 + 0.3 * sin (16. * pi * tCur)), col4, smoothstep (0.2, 0.6, tPhs));
    } else if (idObj == idBallM) {
      col4 = vec4 (1., 0., 0., 0.2);
      if (tPhs > 0.9) col4 = vec4 (0.7, 0.5, 0., 0.2);
    } else if (idObj == idStomp) {
      col4 = mix (vec4 (1., 1., 1., 0.2), vec4 (0., 0., 1., -1.),
         smoothstep (0., 0.02, length (qHit.xz) - 0.25));
    } else if (idObj == idTube) {
      col4 = vec4 (1., 1., 1., 0.2);
    }
    if (idObj == idBallR || idObj == idBallM) col4 *= 0.7 + 0.3 * smoothstep (0., 0.02,
       abs (length (qHit.xz) - 0.2) - 0.02);
    if (col4.a >= 0.) {
      nDotL = max (dot (vn, ltDir), 0.);
      if (idObj == idTube) nDotL *= nDotL;
      sh = ObjSShadow (ro + 0.01 * vn, ltDir);
      col = col4.rgb * (0.2 + 0.8 * sh * nDotL) +
         col4.a * step (0.95, sh) * pow (max (dot (ltDir, reflect (rd, vn)), 0.), 32.);
      col *= 1. - 0.9 * smoothstep (0.2, 0.3, length (ro.xz) / dstFar);
    } else col = col4.rgb * (0.5 + 0.5 * max (- dot (rd, vn), 0.));
  } else {
    col = vec3 (0.1);
  }
  return clamp (col, 0., 1.);
}

#define AA  0   // optional antialiasing

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
  vec4 mPtr;
  vec3 ro, rd, col;
  vec2 canvas, uv;
  float el, az, zmFac, sr;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  az = 0.;
  el = -0.2 * pi;
  if (mPtr.z > 0.) {
    az += 2. * pi * mPtr.x;
    el += pi * mPtr.y;
  }
  el = clamp (el, -0.4 * pi, -0.13 * pi);
  vuMat = StdVuMat (el, az);
  ro = vuMat * vec3 (0., 1., -30.);
  zmFac = 5.5;
  dstFar = 60.;
  ltDir = vuMat * normalize (vec3 (1., 1., -1.));
#if ! AA
  const float naa = 1.;
#else
  const float naa = 3.;
#endif  
  col = vec3 (0.);
  sr = 2. * mod (dot (mod (floor (0.5 * (uv + 1.) * canvas), 2.), vec2 (1.)), 2.) - 1.;
  for (float a = float (VAR_ZERO); a < naa; a ++) {
    rd = vuMat * normalize (vec3 (uv + step (1.5, naa) * Rot2D (vec2 (0.5 / canvas.y, 0.),
       sr * (0.667 * a + 0.5) * pi), zmFac));
    col += (1. / naa) * ShowScene (ro, rd);
  }
  fragColor = vec4 (col, 1.);
}

float PrBoxDf (vec3 p, vec3 b)
{
  vec3 d;
  d = abs (p) - b;
  return min (max (d.x, max (d.y, d.z)), 0.) + length (max (d, 0.));
}

float PrBox2Df (vec2 p, vec2 b)
{
  vec2 d;
  d = abs (p) - b;
  return min (max (d.x, d.y), 0.) + length (max (d, 0.));
}

float PrSphDf (vec3 p, float r)
{
  return length (p) - r;
}

float PrCylDf (vec3 p, float r, float h)
{
  return max (length (p.xy) - r, abs (p.z) - h);
}

float PrCapsDf (vec3 p, float r, float h)
{
  return length (p - vec3 (0., 0., clamp (p.z, - h, h))) - r;
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

vec2 Rot2Cs (vec2 q, vec2 cs)
{
  return vec2 (dot (q, vec2 (cs.x, - cs.y)), dot (q.yx, cs));
}

const float cHashM = 43758.54;

vec2 Hashv2f (float p)
{
  return fract (sin (p + vec2 (0., 1.)) * cHashM);
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

float Fbm2 (vec2 p)
{
  float f, a;
  f = 0.;
  a = 1.;
  for (int j = 0; j < 5; j ++) {
    f += a * Noisefv2 (p);
    a *= 0.5;
    p *= 2.;
  }
  return f * (1. / 1.9375);
}
