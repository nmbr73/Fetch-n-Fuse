

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "Fireworks 3d" by dr2 - 2018
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

// Simple 3d fireworks - happy new year

vec2 Rot2D (vec2 q, float a);
vec3 HsvToRgb (vec3 c);
float Hashff (float p);
float Hashfv3 (vec3 p);
float Noisefv3 (vec3 p);

float tCur, dstFar;
const float pi = 3.14159, phi = 1.618034;

vec2 BallHit (vec3 ro, vec3 rd, vec3 p, float s)
{
  vec3 v;
  float dbIn, dbOut, b, d;
  v = ro - p;
  b = dot (rd, v);
  d = b * b + s * s - dot (v, v);
  dbIn = dstFar;
  dbOut = dstFar;
  if (d > 0.) {
    d = sqrt (d);
    dbIn = - b - d;
    dbOut = - b + d;
  }
  return vec2 (dbIn, dbOut);
}

vec4 SphFib (vec3 v, float n)
{   // Keinert et al's inverse spherical Fibonacci mapping
  vec4 b;
  vec3 vf, vfMin;
  vec2 ff, c;
  float fk, ddMin, dd, a, z, ni;
  ni = 1. / n;
  fk = pow (phi, max (2., floor (log (n * pi * sqrt (5.) * dot (v.xy, v.xy)) /
     log (phi + 1.)))) / sqrt (5.);
  ff = vec2 (floor (fk + 0.5), floor (fk * phi + 0.5));
  b = vec4 (ff * ni, pi * (fract ((ff + 1.) * phi) - (phi - 1.)));
  c = floor ((0.5 * mat2 (b.y, - b.x, b.w, - b.z) / (b.y * b.z - b.x * b.w)) *
     vec2 (atan (v.y, v.x), v.z - (1. - ni)));
  ddMin = 4.1;
  for (int j = 0; j < 4; j ++) {
    a = dot (ff, vec2 (j - 2 * (j / 2), j / 2) + c);
    z = 1. - (2. * a + 1.) * ni;
    vf = vec3 (sin (2. * pi * fract (phi * a) + vec2 (0.5 * pi, 0.)) * sqrt (1. - z * z), z);
    dd = dot (vf - v, vf - v);
    if (dd < ddMin) {
      ddMin = dd;
      vfMin = vf;
    }
  }
  return vec4 (sqrt (ddMin), vfMin);
}

vec3 SkyCol (vec3 ro, vec3 rd)
{
  vec3 col, rds, mDir, vn;
  float mRad, bs, ts, f;
  rd.xz = Rot2D (rd.xz, 0.001 * tCur);
  mDir = normalize (vec3 (0., 1., 1.));
  mRad = 0.02;
  col = vec3 (0.02, 0.02, 0.04) + vec3 (0.06, 0.04, 0.02) *
     pow (clamp (dot (rd, mDir), 0., 1.), 16.);
  bs = dot (rd, mDir);
  ts = bs * bs - dot (mDir, mDir) + mRad * mRad;
  if (ts > 0.) {
    ts = bs - sqrt (ts);
    if (ts > 0.) {
      vn = normalize ((ts * rd - mDir) / mRad);
      col += 0.8 * vec3 (1., 0.9, 0.5) * clamp (dot (vec3 (-0.77, 0.4, 0.5), vn) *
         (1. - 0.3 * Noisefv3 (4. * vn)), 0., 1.);
    }
    col *= 1.3;
  } else {
    rds = floor (2000. * rd);
    rds = 0.00015 * rds + 0.1 * Noisefv3 (0.0005 * rds.yzx);
    for (int j = 0; j < 19; j ++) rds = abs (rds) / dot (rds, rds) - 0.9;
    col += 0.5 * smoothstep (0.01, 0.04, rd.y) * vec3 (0.8, 0.8, 0.6) *
       min (1., 0.5e-3 * pow (min (6., length (rds)), 5.));
  }
  return col;
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec4 f4;
  vec3 col, bPos;
  vec2 dSph;
  float nCyc, tCyc, iFib, phs, hm, hr, h, s, r, a;
  bool isBg;
  tCyc = 3.;
  nCyc = floor (tCur / tCyc) + 1.;
  hm = 0.2 * max (Hashff (17. * nCyc) - 0.2, 0.);
  hr = 0.8 * min (2. * Hashff (27. * nCyc), 1.);
  iFib = 500. + floor (6000. * Hashff (37. * nCyc));
  col = SkyCol (ro, rd);
  isBg = true;
  for (float k = 0.; k < 40.; k ++) {
    phs = fract (tCur / tCyc) - 0.005 * k;
    bPos = vec3 (0., 0., 0.);
    if (phs > 0.1) {
      a = smoothstep (0.1, 0.15, phs) - 0.7 * smoothstep (0.3, 1., phs);
      h = hm + hr * max (phs - 0.2, 0.);
      dSph = BallHit (ro, rd, bPos, 0.5 + 8.5 * sqrt (phs - 0.1));
      if (dSph.x < dstFar) {
        if (k == 0.) col = mix (col, vec3 (1., 1., 0.), 0.03 * a);
        r = 0.015 * (0.5 + 0.5 * a);
        ro += dSph.x * rd;
        f4 = SphFib (normalize (ro), iFib);
        s = Hashfv3 (73. * f4.yzw + 87. * nCyc);
        if (s > 0.5 && f4.x < r * s) {
          col = mix (col, HsvToRgb (vec3 (h, 1., a)), a);
          isBg = false;
        } else {
          ro += (dSph.y - dSph.x) * rd;
          f4 = SphFib (normalize (ro), iFib);
          s = Hashfv3 (73. * f4.yzw + 87. * nCyc);
          if (s > 0.5 && f4.x < r * s) {
            col = mix (col, HsvToRgb (vec3 (h, 1., 0.7 * a)), a);
            isBg = false;
          }
        }
      }
      if (! isBg) break;
    }
  }
  return clamp (col, 0., 1.);
}

#define AA  1   // optional antialiasing

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
  vec4 mPtr;
  vec3 ro, rd, col;
  vec2 canvas, uv, ori, ca, sa;
  float el, az, sr;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  az = -0.1 * pi;
  el = 0.2 * pi;
  if (mPtr.z > 0.) {
    az += 0.5 * pi * mPtr.x;
    el += 0.2 * pi * mPtr.y;
  } else {
    az += 0.001 * pi * tCur;
  }
  ori = vec2 (el, az);
  ca = cos (ori);
  sa = sin (ori);
  vuMat = mat3 (ca.y, 0., - sa.y, 0., 1., 0., sa.y, 0., ca.y) *
          mat3 (1., 0., 0., 0., ca.x, - sa.x, 0., sa.x, ca.x);
  ro = vuMat * vec3 (0., 0., -40.);
  dstFar = 100.;
  #if ! AA
  const float naa = 1.;
#else
  const float naa = 3.;
#endif  
  col = vec3 (0.);
  sr = 2. * mod (dot (mod (floor (0.5 * (uv + 1.) * canvas), 2.), vec2 (1.)), 2.) - 1.;
  for (float a = 0.; a < naa; a ++) {
    rd = vuMat * normalize (vec3 (uv + step (1.5, naa) * Rot2D (vec2 (0.5 / canvas.y, 0.),
       sr * (0.667 * a + 0.5) * pi), 4.5));
    col += (1. / naa) * ShowScene (ro, rd);
  }
  fragColor = vec4 (col, 1.);
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

float Hashff (float p)
{
  return fract (sin (p) * cHashM);
}

float Hashfv3 (vec3 p)
{
  return fract (sin (dot (p, vec3 (37., 39., 41.))) * cHashM);
}

vec4 Hashv4v3 (vec3 p)
{
  vec3 cHashVA3 = vec3 (37., 39., 41.);
  vec2 e = vec2 (1., 0.);
  return fract (sin (vec4 (dot (p + e.yyy, cHashVA3), dot (p + e.xyy, cHashVA3),
     dot (p + e.yxy, cHashVA3), dot (p + e.xxy, cHashVA3))) * cHashM);
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
