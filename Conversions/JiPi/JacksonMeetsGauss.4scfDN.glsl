

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "Jackson Meets Gauss" by dr2 - 2018
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

vec4 LoadTx (vec2 uv);

vec3 ltDir, qnBlk;
vec2 qBlk;
float dstFar, tCur;
const float pi = 3.14159;

float BlkHit (vec3 ro, vec3 rd, float bSize)
{
  vec3 v, tm, tp, u;
  float dMin, dn, df;
  if (rd.x == 0.) rd.x = 0.001;
  if (rd.y == 0.) rd.y = 0.001;
  if (rd.z == 0.) rd.z = 0.001;
  v = ro / rd;
  tp = bSize / abs (rd) - v;
  tm = - tp - 2. * v;
  dn = max (max (tm.x, tm.y), tm.z);
  df = min (min (tp.x, tp.y), tp.z);
  dMin = dstFar;
  if (df > 0. && dn < df) {
    dMin = dn;
    qnBlk = - sign (rd) * step (tm.zxy, tm) * step (tm.yzx, tm);
    u = (v + dn) * rd;
    qBlk = vec2 (dot (u.zxy, qnBlk), dot (u.yzx, qnBlk)) / bSize;
  }
  return dMin;
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec3 col;
  if (BlkHit (ro, rd, 1.) < dstFar) col = LoadTx (0.5 * qBlk + 0.5).rgb *
     (0.2 + 0.8 * max (dot (qnBlk, ltDir), 0.)) +
     0.1 * pow (max (dot (normalize (ltDir - rd), qnBlk), 0.), 32.);
  else col = vec3 (0.6, 0.6, 1.) * (0.3 + 0.15 * (rd.y + 1.) * (rd.y + 1.));
  return clamp (col, 0., 1.);
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
  vec4 mPtr;
  vec3 ro, rd, col;
  vec2 canvas, uv, ori, ca, sa;
  float el, az;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  az = 0.;
  el = 0.;
  if (mPtr.z > 0.) {
    az += 2. * pi * mPtr.x;
    el += pi * mPtr.y;
  } else {
    az += 0.1 *  pi * tCur;
    el += 0.073 *  pi * tCur;
  }
  ori = vec2 (el, az);
  ca = cos (ori);
  sa = sin (ori);
  vuMat = mat3 (ca.y, 0., - sa.y, 0., 1., 0., sa.y, 0., ca.y) *
          mat3 (1., 0., 0., 0., ca.x, - sa.x, 0., sa.x, ca.x);
  ro = vuMat * vec3 (0., 0., -6.);
  dstFar = 20.;
  ltDir = vuMat * normalize (vec3 (0.1, 0.1, -1.));
  rd = vuMat * normalize (vec3 (uv, 3.5));
  fragColor = vec4 (ShowScene (ro, rd), 1.);
}

#define txBuf iChannel0

vec4 LoadTx (vec2 uv)
{
  return texture (txBuf, uv);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// "Jackson Meets Gauss" by dr2 - 2018
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

vec2 GaussRand (vec2 seed);
float SegDist (vec2 p, vec2 v1, vec2 v2);
float LineDist (vec2 p, vec2 v1, vec2 v2);
vec3 HsvToRgb (vec3 c);
vec2 Hashv2v2 (vec2 p);
vec4 LoadTx (vec2 uv);
vec4 Loadv4 (ivec2 idVar);
void Savev4 (ivec2 idVar, vec4 val, inout vec4 fCol, vec2 fCoord);

vec2 pPen, ppPen;
float tCur, nStep;
const float pi = 3.14159;

vec4 ShowScene (vec2 uv, vec4 col)
{
  vec2 p;
  p = 2. * (uv - 0.5);
  return (SegDist (p, ppPen, pPen) < 0.007) ? vec4 (HsvToRgb (vec3 (mod (0.001 * nStep, 1.),
     0.8 + 0.2 * sin (2. * pi * 0.01 * nStep), 1. - 0.5 * smoothstep (0.006, 0.007,
     LineDist (p, ppPen, pPen)))), 1.) : mix (col, vec4 (0.1), 0.0005);
}

#define txBuf iChannel0
#define txSize iChannelResolution[0].xy

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  vec4 stDat;
  vec2 txSizeP, uvtx, dp;
  ivec2 iFrag;
  bool doInit;
  tCur = iTime;
  uvtx = fragCoord / txSize;
  iFrag = ivec2 (fragCoord);
  doInit = false;
  if (iFrame <= 5) {
    doInit = true;
    txSizeP = txSize;
  } else {
    stDat = Loadv4 (ivec2 (0, 0));
    pPen = stDat.xy;
    ppPen = stDat.zw;
    stDat = Loadv4 (ivec2 (1, 0));
    txSizeP.x = stDat.x;
    nStep = stDat.y;
  }
  if (txSize.x != txSizeP.x) doInit = true;
  fragColor = doInit ? vec4 (0.1) : ShowScene (uvtx, LoadTx (uvtx));
  if (iFrag.y == 0 && iFrag.x <= 1) {
    if (doInit) {
      ppPen = vec2 (0.);
      pPen = ppPen;
      nStep = 0.;
    } else {
      ppPen = pPen;
      dp = 0.07 * GaussRand (pPen + tCur);
      if (abs (pPen.x + dp.x) > 0.98) dp.x *= -1.;
      if (abs (pPen.y + dp.y) > 0.98) dp.y *= -1.;
      pPen += dp;
      ++ nStep;
    }
    if (iFrag.x == 0) stDat = vec4 (pPen, ppPen);
    else if (iFrag.x == 1) stDat = vec4 (txSize.x, nStep, 0., 0.);
    Savev4 (iFrag, stDat, fragColor, fragCoord);
  }
}

vec2 GaussRand (vec2 seed)   // Box-Muller
{
  vec2 r;
  r = 0.001 + 0.999 * Hashv2v2 (seed + 0.001);
  return sqrt (-2. * log (r.x)) * sin (2. * pi * (r.y + vec2 (0., 0.25)));
}

float SegDist (vec2 p, vec2 v1, vec2 v2)
{
  vec2 a, b;
  float s;
  a = p - v1;
  b = v2 - v1;
  s = length (b);
  b = (s > 0.) ? b / s : vec2 (0.);
  return length (a - clamp (dot (a, b), 0., s) * b);
}

float LineDist (vec2 p, vec2 v1, vec2 v2)
{
  vec2 a, b;
  float s;
  a = p - v1;
  b = v2 - v1;
  s = length (b);
  b = (s > 0.) ? b / s : vec2 (0.);
  return length (a - dot (a, b) * b);
}

vec3 HsvToRgb (vec3 c)
{
  vec3 p;
  p = abs (fract (c.xxx + vec3 (1., 2./3., 1./3.)) * 6. - 3.);
  return c.z * mix (vec3 (1.), clamp (p - 1., 0., 1.), c.y);
}

const float cHashM = 43758.54;

vec2 Hashv2v2 (vec2 p)
{
  const vec2 cHashVA2 = vec2 (37.1, 61.7);
  const vec2 e = vec2 (1., 0.);
  return fract (sin (vec2 (dot (p + e.yy, cHashVA2), dot (p + e.xy, cHashVA2))) * cHashM);
}

vec4 LoadTx (vec2 uv)
{
  return texture (txBuf, uv);
}

vec4 Loadv4 (ivec2 idVar)
{
  return texture (txBuf, (vec2 (idVar) + 0.5) / txSize);
}

void Savev4 (ivec2 idVar, vec4 val, inout vec4 fCol, vec2 fCoord)
{
  vec2 d;
  d = abs (fCoord - vec2 (idVar) - 0.5);
  if (max (d.x, d.y) < 0.5) fCol = val;
}
