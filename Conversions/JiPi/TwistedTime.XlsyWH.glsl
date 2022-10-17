

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "Twisted Time" by dr2 - 2017
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

#define txFnt iChannel0

float PrTorusDf (vec3 p, float ri, float rc);
void DTimeSet (vec4 d);
int GetTxChar (int _nc);
float Hashfv2 (vec2 p);
vec2 Hashv2v2 (vec2 p);
float Fbm2 (vec2 p);
vec3 VaryNf (vec3 p, vec3 n, float f);
vec3 HsvToRgb (vec3 c);
float SmoothMin (float a, float b, float r);
float SmoothBump (float lo, float hi, float w, float x);
vec2 Rot2D (vec2 q, float a);

vec4 dateCur;
vec3 sunDir, vnCylIn;
vec2 gVec[7], hVec[7], fntSize, qnFnt, qnTxt;
float dstFar, tCur, grDep, rngBlk, bCylRad, bCylHt, dCylIn, dCylOut;
int idTxt;
ivec3 inm[2];
ivec2 idt[6];
const float pi = 3.14159;

void CylHit (vec3 ro, vec3 rd)
{
  vec3 s;
  float a, ai, b, w, ws, srdy;
  dCylIn = dstFar;
  dCylOut = dstFar;
  vnCylIn = vec3 (0.);
  a = dot (rd.xz, rd.xz);
  b = dot (rd.xz, ro.xz);
  w = b * b - a * (dot (ro.xz, ro.xz) - bCylRad * bCylRad);
  if (w > 0.) {
    ws = sqrt (w);
    srdy = sign (rd.y);
    if (a > 0.) {
      ai =  1. / a;
      dCylIn = (- b - ws) * ai;
      dCylOut = (- b + ws) * ai;
    }
    if (a > 0.) s = ro + dCylIn * rd;
    else s.y = bCylHt;
    if (abs (s.y) < bCylHt) vnCylIn.xz = s.xz / bCylRad;
    else if (srdy * ro.y < - bCylHt) {
      dCylIn = - (srdy * ro.y + bCylHt) / abs (rd.y);
      if (length (ro.xz + dCylIn * rd.xz) < bCylRad) vnCylIn.y = - srdy;
      else dCylIn = dstFar;
    } else dCylIn = dstFar;
    if (dCylIn < dstFar) {
      if (a > 0.) s = ro + dCylOut * rd;
      else s.y = bCylHt;
      if (abs (s.y) > bCylHt && srdy * ro.y < bCylHt)
         dCylOut = (- srdy * ro.y + bCylHt) / abs (rd.y);
    }
  }
}

float FontTexDf (vec2 p)
{
  vec3 tx;
  ivec2 ip;
  float d;
  int ic;
  ic = 0;
  ip = ivec2 (floor (p));
  if (ip.x == 0 && ip.y == 0) ic = GetTxChar (idTxt);
  if (ic != 0) {
    tx = texture (txFnt, mod ((vec2 (mod (float (ic), 16.),
       15. - floor (float (ic) / 16.)) + fract (p)) * (1. / 16.), 1.)).gba - 0.5;
    qnFnt = vec2 (tx.r, - tx.g);
    d = tx.b + 1. / 256.;
  } else d = 1.;
  return d;
}

float ObjRayT (vec3 ro, vec3 rd)
{
  vec3 p, rdi;
  vec2 srd, h;
  float dHit, dLim, d;
  if (rd.x == 0.) rd.x = 0.001;
  if (rd.y == 0.) rd.y = 0.001;
  if (rd.z == 0.) rd.z = 0.001;
  srd = - sign (rd.xy);
  rdi = 1. / abs (rd.xyz);
  dHit = 0.;
  dLim = rngBlk;
  ro.xy /= fntSize.x;
  rd.xy /= fntSize.x;
  ro.xy += 0.5;
  for (int j = 0; j < 150; j ++) {
    p = ro + dHit * rd;
    h = rdi.xy * fract (srd * p.xy);
    d = max (min (fntSize.x, 1.) * FontTexDf (p.xy), abs (p.z) - 0.5 * fntSize.y);
    dHit += min (d, 0.01 + fntSize.x * min (h.x, h.y));
    if (d < 0.0001 || dHit > dLim) break;
  }
  if (d >= 0.0001) dHit = dstFar;
  return dHit;
}

float BlkHit (vec3 ro, vec3 rd, vec3 bSize)
{
  vec3 v, tm, tp;
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
    rngBlk = df - dn;
  }
  return dMin;
}

float ObjDf (vec3 p)
{
  vec3 q;
  q = p;
  q.y = abs (q.y) - bCylHt;
  return PrTorusDf (q.xzy, 0.02, bCylRad);
}

float ObjRay (vec3 ro, vec3 rd)
{
  float dHit, d;
  dHit = 0.;
  for (int j = 0; j < 100; j ++) {
    d = ObjDf (ro + dHit * rd);
    dHit += d;
    if (d < 0.001 || dHit > dstFar) break;
  }
  return dHit;
}

vec3 ObjNf (vec3 p)
{
  vec4 v;
  const vec3 e = vec3 (0.001, -0.001, 0.);
  v = vec4 (ObjDf (p + e.xxx), ObjDf (p + e.xyy), ObjDf (p + e.yxy),
     ObjDf (p + e.yyx));
  return normalize (vec3 (v.x - v.y - v.z - v.w) + 2. * v.yzw);
}

#define SQRT3 1.73205

vec2 PixToHex (vec2 p)
{
  vec3 c, r, dr;
  c.xz = vec2 ((1./SQRT3) * p.x - (1./3.) * p.y, (2./3.) * p.y);
  c.y = - c.x - c.z;
  r = floor (c + 0.5);
  dr = abs (r - c);
  r -= step (dr.yzx, dr) * step (dr.zxy, dr) * dot (r, vec3 (1.));
  return r.xz;
}

vec2 HexToPix (vec2 h)
{
  return vec2 (SQRT3 * (h.x + 0.5 * h.y), (3./2.) * h.y);
}

void HexVorInit ()
{
  vec3 e = vec3 (1., 0., -1.);
  gVec[0] = e.yy;
  gVec[1] = e.xy;
  gVec[2] = e.yx;
  gVec[3] = e.xz;
  gVec[4] = e.zy;
  gVec[5] = e.yz;
  gVec[6] = e.zx;
  for (int k = 0; k < 7; k ++) hVec[k] = HexToPix (gVec[k]);
}

vec4 HexVor (vec2 p)
{
  vec4 sd, udm;
  vec2 ip, fp, d, u;
  float amp, a;
  amp = 0.7;
  ip = PixToHex (p);
  fp = p - HexToPix (ip);
  sd = vec4 (4.);
  udm = vec4 (4.);
  for (int k = 0; k < 7; k ++) {
    u = Hashv2v2 (ip + gVec[k]);
    a = 2. * pi * (u.y - 0.5);
    d = hVec[k] + amp * (0.4 + 0.6 * u.x) * vec2 (cos (a), sin (a)) - fp;
    sd.w = dot (d, d);
    if (sd.w < sd.x) {
      sd = sd.wxyw;
      udm = vec4 (d, u);
    } else sd = (sd.w < sd.y) ? sd.xwyw : ((sd.w < sd.z) ? sd.xyww : sd);
  }
  sd.xyz = sqrt (sd.xyz);
  return vec4 (SmoothMin (sd.y, sd.z, 0.3) - sd.x, udm.xy, Hashfv2 (udm.zw));
}

vec3 BgCol (vec3 ro, vec3 rd)
{
  vec4 vc;
  vec3 vn, col;
  float s;
  if (rd.y >= 0.) {
    rd.y += 0.0001;
    ro.z += 2. * tCur;
    col = mix (vec3 (0.1, 0.2, 0.4), vec3 (1.),
       clamp (0.1 + 0.8 * Fbm2 (0.05 * (ro.xz + rd.xz * (100. - ro.y) / rd.y)) * rd.y, 0., 1.));
  } else {
    ro -= ((grDep + ro.y) / rd.y) * rd;
    vc = HexVor (2.5 * ro.zx);
    s = step (0.06 + 0.03 * vc.w, vc.x);
    col = mix (vec3 (0.8), HsvToRgb (vec3 (37. * vc.w, 0.7, 1.)) *
       (1. - 0.05 * step (0.5, Fbm2 (15. * ro.xz))), s);
    vn = VaryNf (15. * ro, vec3 (0., 1., 0.), 2. - 1.8 * s);
    col *= (0.1 + 0.9 * max (dot (vn, sunDir), 0.)) *
       (1. - 0.003 * length (ro.xz));
    col = mix (col, vec3 (0.2, 0.3, 0.5), pow (1. + rd.y, 5.));
  }
  return col;
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec3 col, vn, qtTxt, tro, trd, trdd, q, qt, ds;
  float dstTxt, dstBlk, dstObj, db, d, ang, angTxt, angRot, aTw, rad, twFac, sdTxt;
  fntSize = vec2 (1.7, 0.6);
  HexVorInit ();
  dstTxt = dstFar;
  tro = ro;
  trd = rd;
  rad = 7.;
  bCylRad = rad + 0.6 * fntSize.x;
  bCylHt = 1.3 * fntSize.y;
  dstObj = ObjRay (ro, rd);
  CylHit (ro, rd);
  if (dCylIn < dstFar) {
    twFac = 3.;
    angRot = 0.05 * pi * tCur;
    aTw = angRot * twFac;
    tro.xz = Rot2D (tro.xz, angRot);
    trd.xz = Rot2D (trd.xz, angRot);
    for (float k = 0.; k < 50.; k ++) {
      idTxt = int (mod (k, 25.));
      ang = 2. * pi * (1. - k / 50.);
      qt = - vec3 (rad * sin (ang), 0., rad * cos (ang));
      q = tro - qt;
      q.xz = Rot2D (q.xz, ang);
      q.yz = Rot2D (q.yz, twFac * ang + aTw);
      trdd = trd;
      trdd.xz = Rot2D (trdd.xz, ang);
      trdd.yz = Rot2D (trdd.yz, twFac * ang + aTw);
      db = BlkHit (q, trdd, vec3 (0.5 * fntSize.x, 0.55 * fntSize.x, 0.55 * fntSize.y));
      if (db < dstFar) {
        d = db + ObjRayT (q + db * trdd, trdd);
        if (d < dstTxt) {
          dstTxt = d;
          qtTxt = qt;
          angTxt = ang;
          qnTxt = qnFnt;
          sdTxt = floor (k / 25.);
        }
      }
    }
  }
  if (dstTxt < min (dstObj, dstFar)) {
    tro += trd * dstTxt;
    ds = tro - qtTxt;
    ds.xz = Rot2D (ds.xz, angTxt);
    ds.yz = Rot2D (ds.yz, twFac * angTxt + aTw);
    if (abs (ds.z) < 0.49 * fntSize.y) {
      vn = normalize (vec3 (qnTxt, 0.00001));
      col = vec3 (0.8, 0.8, 1.);
    } else {
      vn = vec3 (0., 0., sign (ds.z));
      col = (sdTxt == 0.) ? vec3 (1., 0.3, 0.3) : vec3 (1., 1., 0.);
    }
    vn.yz = Rot2D (vn.yz, - twFac * angTxt - aTw);
    vn.xz = Rot2D (vn.xz, - angRot - angTxt);
    col = col * (0.2 + 0.8 * max (dot (sunDir, vn), 0.)) +
       0.5 * pow (max (dot (normalize (sunDir - rd), vn), 0.), 128.);
    col = mix (col, BgCol (tro, reflect (rd, vn)), 0.7 - 0.5 * abs (dot (rd, vn)));
  } else if (dstObj < min (dstTxt, dstFar)) {
    vn = ObjNf (ro + dstObj * rd);
    col = vec3 (0.2, 0.5, 0.7) * (0.2 + 0.8 * max (dot (sunDir, vn), 0.)) +
       0.5 * pow (max (dot (normalize (sunDir - rd), vn), 0.), 128.);
  } else col = BgCol (ro, rd);
  if (dCylIn < dstFar) {
    col = mix (col,  vec3 (1.) * (0.2 + 0.8 * max (dot (sunDir, vnCylIn), 0.) +
       0.5 * pow (max (dot (normalize (sunDir - rd), vnCylIn), 0.), 128.)), 0.15);
    col = mix (col, BgCol (ro + dCylIn * rd,
       reflect (rd, vnCylIn)), pow (1. - abs (dot (rd, vnCylIn)), 4.));
  }
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
  dateCur = iDate;
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  az = 0.;
  el = 0.08 * pi * sin (0.1 * pi * tCur);
  if (mPtr.z > 0.) {
    az += 2. * pi * mPtr.x;
    el += 0.4 * pi * mPtr.y;
  }
  el = clamp (el, -0.2 * pi, 0.1 * pi);
  ori = vec2 (el, az);
  ca = cos (ori);
  sa = sin (ori);
  vuMat = mat3 (ca.y, 0., - sa.y, 0., 1., 0., sa.y, 0., ca.y) *
          mat3 (1., 0., 0., 0., ca.x, - sa.x, 0., sa.x, ca.x);
  rd = vuMat * normalize (vec3 (uv, 2.1));
  ro = vuMat * vec3 (0., 0., -14.);
  sunDir = vuMat * normalize (vec3 (0.5, 1., -1.));
  dstFar = 100.;
  grDep = 5.;
  DTimeSet (dateCur);
  col = ShowScene (ro, rd);
  fragColor = vec4 (clamp (col, 0., 1.), 1.);
}

float PrTorusDf (vec3 p, float ri, float rc)
{
  return length (vec2 (length (p.xy) - rc, p.z)) - ri;
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

ivec3 MName (int i)
{
  ivec3 m;
  if      (i == 0)  m = ivec3 (_J, _a, _n);
  else if (i == 1)  m = ivec3 (_F, _e, _b);
  else if (i == 2)  m = ivec3 (_M, _a, _r);
  else if (i == 3)  m = ivec3 (_A, _p, _r);
  else if (i == 4)  m = ivec3 (_M, _a, _y);
  else if (i == 5)  m = ivec3 (_J, _u, _n);
  else if (i == 6)  m = ivec3 (_J, _u, _l);
  else if (i == 7)  m = ivec3 (_A, _u, _g);
  else if (i == 8)  m = ivec3 (_S, _e, _p);
  else if (i == 9)  m = ivec3 (_O, _c, _t);
  else if (i == 10) m = ivec3 (_N, _o, _v);
  else if (i == 11) m = ivec3 (_D, _e, _c);
  return m;
}

ivec3 DName (int i)
{
  ivec3 d;
  if      (i == 0)  d = ivec3 (_S, _u, _n);
  else if (i == 1)  d = ivec3 (_M, _o, _n);
  else if (i == 2)  d = ivec3 (_T, _u, _e);
  else if (i == 3)  d = ivec3 (_W, _e, _d);
  else if (i == 4)  d = ivec3 (_T, _h, _u);
  else if (i == 5)  d = ivec3 (_F, _r, _i);
  else if (i == 6)  d = ivec3 (_S, _a, _t);
  return d;
}

int DWk (vec3 ymd)
{
  ivec3 d;
  vec2 cy;
  ymd.y += 1.;
  if (ymd.y <= 2.) {
    ymd.x -= 1.;
    ymd.y += 12.;
  }
  cy = vec2 (floor (ymd.x / 100.), mod (ymd.x, 100.));
  return int (mod (mod (ymd.z + floor (13. * (ymd.y + 1.) / 5.) + cy.y + floor (cy.y / 4.) +
     floor (cy.x / 4.) + 5. * cy.x, 7.) + 6., 7.));
}

#define DIG2(v) _0 + ivec2 (vec2 (floor ((v) / 10.), mod ((v), 10.)))
#define T(c) _ic = (_nc -- == 0) ? (c) : _ic;

void DTimeSet (vec4 d)
{
  idt[0] = DIG2 (floor (d.x / 100.));
  idt[1] = DIG2 (mod (d.x, 100.));
  idt[2] = DIG2 (d.z);
  idt[3] = DIG2 (floor (d.w / 3600.));
  idt[4] = DIG2 (floor (mod (d.w, 3600.) / 60.));
  idt[5] = DIG2 (floor (mod (d.w, 60.)));
  inm[0] = MName (int (d.y));
  inm[1] = DName (DWk (d.xyz));
}

int GetTxChar (int _nc)
{
  int _ic;
  _ic = 0;
  T(_SP) T(inm[1].x) T(inm[1].y) T(inm[1].z)T(_SP) T(idt[2].x) T(idt[2].y) T(_MINUS) 
  T(inm[0].x) T(inm[0].y) T(inm[0].z) T(_MINUS) T(idt[0].x) T(idt[0].y) T(idt[1].x) T(idt[1].y)
  T(_SP) T(idt[3].x) T(idt[3].y) T(_COLON) T(idt[4].x) T(idt[4].y) T(_COLON) T(idt[5].x) T(idt[5].y)
  return _ic;
}

const vec4 cHashA4 = vec4 (0., 1., 57., 58.);
const vec3 cHashA3 = vec3 (1., 57., 113.);
const float cHashM = 43758.54;

float Hashfv2 (vec2 p)
{
  return fract (sin (dot (p, cHashA3.xy)) * cHashM);
}

vec2 Hashv2v2 (vec2 p)
{
  const vec2 cHashVA2 = vec2 (37.1, 61.7);
  const vec2 e = vec2 (1., 0.);
  return fract (sin (vec2 (dot (p + e.yy, cHashVA2),
     dot (p + e.xy, cHashVA2))) * cHashM);
}

vec4 Hashv4f (float p)
{
  return fract (sin (p + cHashA4) * cHashM);
}

float Noisefv2 (vec2 p)
{
  vec4 t;
  vec2 ip, fp;
  ip = floor (p);
  fp = fract (p);
  fp = fp * fp * (3. - 2. * fp);
  t = Hashv4f (dot (ip, cHashA3.xy));
  return mix (mix (t.x, t.y, fp.x), mix (t.z, t.w, fp.x), fp.y);
}

float Fbm2 (vec2 p)
{
  float f, a;
  f = 0.;
  a = 1.;
  for (int i = 0; i < 5; i ++) {
    f += a * Noisefv2 (p);
    a *= 0.5;
    p *= 2.;
  }
  return f * (1. / 1.9375);
}

float Fbmn (vec3 p, vec3 n)
{
  vec3 s = vec3 (0.);
  float a = 1.;
  for (int i = 0; i < 5; i ++) {
    s += a * vec3 (Noisefv2 (p.yz), Noisefv2 (p.zx), Noisefv2 (p.xy));
    a *= 0.5;
    p *= 2.;
  }
  return dot (s, abs (n));
}

vec3 VaryNf (vec3 p, vec3 n, float f)
{
  vec3 g;
  float s;
  const vec3 e = vec3 (0.1, 0., 0.);
  s = Fbmn (p, n);
  g = vec3 (Fbmn (p + e.xyy, n) - s, Fbmn (p + e.yxy, n) - s,
     Fbmn (p + e.yyx, n) - s);
  return normalize (n + f * (g - n * dot (n, g)));
}

vec3 HsvToRgb (vec3 c)
{
  vec3 p;
  p = abs (fract (c.xxx + vec3 (1., 2./3., 1./3.)) * 6. - 3.);
  return c.z * mix (vec3 (1.), clamp (p - 1., 0., 1.), c.y);
}

float SmoothMin (float a, float b, float r)
{
  float h;
  h = clamp (0.5 + 0.5 * (b - a) / r, 0., 1.);
  return mix (b, a, h) - r * h * (1. - h);
}

float SmoothBump (float lo, float hi, float w, float x)
{
  return (1. - smoothstep (hi - w, hi + w, x)) * smoothstep (lo - w, lo + w, x);
}

vec2 Rot2D (vec2 q, float a)
{
  return q * cos (a) + q.yx * sin (a) * vec2 (-1., 1.);
}
