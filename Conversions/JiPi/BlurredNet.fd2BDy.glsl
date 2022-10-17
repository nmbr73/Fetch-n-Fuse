

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "Blurred Net" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

#if 0
#define VAR_ZERO min (iFrame, 0)
#else
#define VAR_ZERO 0
#endif

#define txBuf iChannel0

const float pi = 3.1415927;

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  vec4 txVal, col4;
  vec2 canvas, uv, duv;
  float dstFoc, dPix, r;
  float ga = pi * (3. - sqrt(5.));
  const float NP = 32.;
  canvas = iResolution.xy;
  uv = fragCoord / canvas;
  dstFoc = 5.; // (from "Losing Focus 2")
  dPix = abs (dstFoc - texture (txBuf, uv).a);
  duv = 0.0008 * dPix * vec2 (canvas.y / canvas.x, 1.);
  col4 = vec4 (0.);
  for (float n = float (VAR_ZERO); n < NP; n ++) {
    r = sqrt (n / NP);
    txVal = texture (txBuf, uv + duv * r * sin (n * ga + vec2 (0.5 * pi, 0.)));
    col4 += vec4 (txVal.rgb, 1.) * exp (-1. * r * r) *
       clamp (1. + (1. + 0.1 * txVal.a * txVal.a) * (abs (dstFoc - txVal.a) - dPix), 0., 1.);
  }
  col4.rgb /= col4.a;
  fragColor = vec4 (col4.rgb, 1.);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// "Blurred Net" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

#if 0
#define VAR_ZERO min (iFrame, 0)
#else
#define VAR_ZERO 0
#endif

mat3 StdVuMat (float el, float az);
vec2 Rot2D (vec2 q, float a);
float Fbm3 (vec3 p);

vec3 ltDir;
vec2 trkAx, trkAy, trkFx, trkFy;
float tCur, dstFar;
const float pi = 3.1415927;

vec3 TrackPath (float t)
{
  return vec3 (dot (trkAx, sin (trkFx * t)), dot (trkAy, sin (trkFy * t)), t);
}

vec3 TrackVel (float t)
{
  return vec3 (dot (trkAx * trkFx, cos (trkFx * t)), dot (trkAy * trkFy, cos (trkFy * t)), 1);
}

float ObjDf (vec3 p)
{
  vec4 q;
  q = vec4 (p, 1.);
  q.xy -= TrackPath (p.z).xy;
  for (float j = 0.; j < 8.; j ++) {
    q.xyz = 2. * fract (0.5 * q.xyz + 0.5) - 1.;
    q *= 1.3 / dot (q.xyz, q.xyz);
  }
  return 0.25 * (length (q.xyz) / q.w - 0.01);
}

float ObjRay (vec3 ro, vec3 rd)
{
  float dHit, d;
  dHit = 0.;
  for (int j = VAR_ZERO; j < 220; j ++) {
    d = ObjDf (ro + dHit * rd);
    if (d < 0.001 || dHit > dstFar) break;
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

vec3 BgCol (vec3 rd)
{
  float t, gd, b;
  t = tCur * 4.;
  b = dot (vec2 (atan (rd.x, rd.y), 0.5 * pi - acos (rd.z)), vec2 (2., sin (rd.x)));
  gd = clamp (sin (5. * b + t), 0., 1.) * clamp (sin (3.5 * b - t), 0., 1.) +
     clamp (sin (21. * b - t), 0., 1.) * clamp (sin (17. * b + t), 0., 1.);
  return mix (vec3 (0.6, 0.5, 0.), vec3 (0.9, 0.4, 0.2), 0.5 + 0.5 * rd.z) *
     (0.24 + 0.44 * (rd.z + 1.) * (rd.z + 1.)) * (1. + 0.2 * gd);
}

vec4 ShowScene (vec3 ro, vec3 rd)
{
  vec4 col4;
  vec3 col, bgCol, vn;
  float dstObj;
  dstObj = ObjRay (ro, rd);
  bgCol = BgCol (rd);
  if (dstObj < dstFar) {
    ro += dstObj * rd;
    vn = ObjNf (ro);
    col4 = mix (vec4 (0.6, 0.8, 0.6, 0.1), vec4 (0.8, 0.8, 0.9, 0.5), smoothstep (0.5, 0.55,
       Fbm3 (16. * ro)));
    col = col4.rgb * (0.2 + 0.8 * max (dot (ltDir, vn), 0.)) +
       col4.a * vec3 (1., 1., 0.) * pow (max (dot (ltDir, vn), 0.), 32.);
    col = mix (col, bgCol, smoothstep (0.2, 1., dstObj / dstFar));
  } else col = bgCol;
  return vec4 (clamp (col, 0., 1.), dstObj);
}

#define txSize iChannelResolution[0].xy

#define AA  0

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
  vec4 mPtr, col4;
  vec3 ro, rd, vd;
  vec2 canvas, uv, uvv;
  float el, az, zmFac, sr, asp, vFly;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  asp = canvas.x / canvas.y;
  zmFac = 3.;
  az = 0.;
  el = 0.;
  if (mPtr.z > 0.) {
    az += 2. * pi * mPtr.x;
    el += pi * mPtr.y;
  }
  trkAx = 0.25 * vec2 (2., 0.9);
  trkAy = 0.25 * vec2 (1.3, 0.66);
  trkFx = 2. * vec2 (0.2, 0.23);
  trkFy = 2. * vec2 (0.17, 0.24);
  vFly = 0.5;
  ro = TrackPath (vFly * tCur) + 1.;
  vd = normalize (TrackVel (vFly * tCur));
  az += atan (vd.x, vd.z);
  vuMat = StdVuMat (el, az);
  ltDir = vuMat * normalize (vec3 (-1., 1., -1.));
  dstFar = 50.;
#if ! AA
  const float naa = 1.;
#else
  const float naa = 3.;
#endif  
  col4 = vec4 (0.);
  sr = 2. * mod (dot (mod (floor (0.5 * (uv + 1.) * canvas), 2.), vec2 (1.)), 2.) - 1.;
  for (float a = float (VAR_ZERO); a < naa; a ++) {
    uvv = (uv + step (1.5, naa) * Rot2D (vec2 (0.5 / canvas.y, 0.), sr * (0.667 * a + 0.5) *
       pi)) / zmFac;
    rd = vuMat * normalize (vec3 (2. * tan (0.5 * atan (uvv.x / asp)) * asp, uvv.y, 1.));
    col4 += (1. / naa) * ShowScene (ro, rd);
  }
  fragColor = col4;
}

vec2 Rot2D (vec2 q, float a)
{
  vec2 cs;
  cs = sin (a + vec2 (0.5 * pi, 0.));
  return vec2 (dot (q, vec2 (cs.x, - cs.y)), dot (q.yx, cs));
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

const float cHashM = 43758.54;

vec4 Hashv4v3 (vec3 p)
{
  vec3 cHashVA3 = vec3 (37., 39., 41.);
  return fract (sin (dot (p, cHashVA3) + vec4 (0., cHashVA3.xy, cHashVA3.x + cHashVA3.y)) * cHashM);
}

float Noisefv3 (vec3 p)
{
  vec4 t;
  vec3 ip, fp;
  ip = floor (p);
  fp = fract (p);
  fp *= fp * (3. - 2. * fp);
  t = mix (Hashv4v3 (ip), Hashv4v3 (ip + vec3 (0., 0., 1.)), fp.z);
  return mix (mix (t.x, t.y, fp.x), mix (t.z, t.w, fp.x), fp.y);
}

float Fbm3 (vec3 p)
{
  float f, a;
  f = 0.;
  a = 1.;
  for (int j = 0; j < 5; j ++) {
    f += a * Noisefv3 (p);
    a *= 0.5;
    p *= 2.;
  }
  return f * (1. / 1.9375);
}
