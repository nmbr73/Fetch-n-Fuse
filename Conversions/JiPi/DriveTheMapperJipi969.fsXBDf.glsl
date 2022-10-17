

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "Drive the Mapper" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

/*
  No. 7 in "Driving" series; others are listed in "Truck Driving 2" (ftt3Ds)
*/

#define AA  0   // (= 0/1) optional antialiasing (can be slow)

#if 0
#define VAR_ZERO min (iFrame, 0)
#else
#define VAR_ZERO 0
#endif

float PrBoxDf (vec3 p, vec3 b);
float PrBox2Df (vec2 p, vec2 b);
float PrRoundBoxDf (vec3 p, vec3 b, float r);
float PrRound4BoxDf (vec3 p, vec3 b, float r);
float PrRoundBox2Df (vec2 p, vec2 b, float r);
float PrCylDf (vec3 p, float r, float h);
float PrRoundCylDf (vec3 p, float r, float rt, float h);
float PrCapsDf (vec3 p, float r, float h);
float PrConCapsDf (vec3 p, vec2 cs, float r, float h);
float PrTorusDf (vec3 p, float ri, float rc);
float Minv2 (vec2 p);
float Maxv2 (vec2 p);
float SmoothMax (float a, float b, float r);
mat3 StdVuMat (float el, float az);
vec2 Rot2D (vec2 q, float a);
vec2 Rot2Cs (vec2 q, vec2 cs);
vec3 HsvToRgb (vec3 c);
float ShowIntPZ (vec2 q, vec2 cBox, float mxChar, float val);
vec3 Hashv3v2 (vec2 p);
float Fbm1 (float p);
float Fbm2 (vec2 p);
vec3 VaryNf (vec3 p, vec3 n, float f);
vec4 Loadv4 (int idVar);

const int npTrail = 32;  // same for image and buffer
const int npFst = 7;

vec4 carPos, wlRot;
vec3 sunDir, qHit, wlBase, carSz, bldSz;
vec2 axRot[6], wlRotCs[6], wlAngCs[6], gId, mOff;
float dstFar, tCur, wlRad, strRot, spd, onPath, gSize, rdWid, trHt;
int idObj;
bool isSh, isTran, inCab;
const int idBod = 1, idAx = 2, idWhl = 3, idStr = 4, idSeat = 5, idStk = 6, idWLit = 7,
   idLit = 8, idCam = 9, idBld = 10, idTree = 11;
const float pi = 3.1415927;

#define DMINQ(id) if (d < dMin) { dMin = d;  idObj = id;  qHit = q; }

float ObjDf (vec3 p)
{
  vec3 q, qa;
  float dMin, d, r, db, dw;
  p -= carPos.xyz;
  p.xz = Rot2D (p.xz, carPos.w);
  dMin = dstFar;
  if (! isSh) d = PrRoundBoxDf (p, vec3 (carSz.x, carSz.y + 1.2, carSz.z), 0.5);
  if (isSh || d < 0.1) {
    q = p;
    dw = PrRoundCylDf (vec3 (abs (q.x) - wlBase.x, q.y,
       wlBase.z * (fract (q.z / wlBase.z + 0.5) - 0.5)).yzx, wlRad + 0.02, 0.05, 0.35);
    q.y -= 1.1;
    db = PrRound4BoxDf (q, carSz - 1., 1.);
    d = min (abs (db) - 0.05, max (db, dw - 0.05));
    d = SmoothMax (d, - max (abs (abs (q.z) - wlBase.z - wlRad - 0.2) - 0.01,
       0.02 - abs (db)), 0.02);
    d = SmoothMax (d, - min (min (PrRoundBox2Df (vec2 (q.y - 0.3, abs (q.z) - 1.05),
       vec2 (0.3, 0.75), 0.2), PrRoundBox2Df (vec2 (abs (q.x) - 0.6, q.y - 0.3),
       vec2 (0.3, 0.25), 0.2)), max (PrRoundBox2Df (vec2 (q.x, q.z - 0.8),
       vec2 (0.8, 0.5), 0.2), - q.y)), 0.02);
    d = max (d, - max (dw, max (abs (q.z) - wlBase.z - (wlRad + 0.1),
       wlBase.x - 0.7 - abs (q.x))));
    DMINQ (idBod);
    q = p;
    q.yz -= vec2 (carSz.y + 1.33 + 0.05, -1.);
    d = PrRoundCylDf (q.xzy, 0.15, 0.05, 0.22);
    q.y -= max (0.5 * spd, 0.) - 0.05;
    d = min (d, PrRoundCylDf (q.xzy, 0.6, 0.15, 0.03));
    DMINQ (idCam);
    q = p;
    q.xz = abs (q.xz) - wlBase.xz - vec2 (-0.2, -0.5 * wlBase.z);
    q.z = abs (q.z) - 0.5 * wlBase.z;
    d = PrCylDf (q.yzx, 0.1, 0.2);
    DMINQ (idAx);
    for (int k = VAR_ZERO; k < 6; k ++) {
      q = p;
      q.xz = Rot2Cs (q.xz - axRot[k] * wlBase.xz, wlRotCs[k]);
      r = wlRad - length (q.yz);
      q.yz = Rot2Cs (q.yz, wlAngCs[k]);
      qa = abs (q);
      d = min (max (length (max (vec2 (0.1 - r, qa.x - 0.1), 0.)) - 0.1, r - 0.2),
         max (0.2 - r, qa.x - 0.12));
      d = max (d, - max (abs (fract (4. * q.x / 0.24) - 0.5) - 0.2, r - 0.05));
      d = min (d, PrCylDf (vec3 (qa.x - 0.1, ((qa.y < qa.z) ? q.yz : q.zy)), 0.06, wlRad - 0.15));
      q.x *= sign (p.x);
      DMINQ (idWhl);
    }
    q = p;
    d = PrRoundBoxDf (q - vec3 (-0.4, 0.5, carSz.z - 0.4), vec3 (0.1, 0.5, 0.03), 0.05);
    q.yz = Rot2D (q.yz - vec2 (0.72, -0.2), -0.1 * pi);
    q -= vec3 (-0.4, 0.9, 1.8);
    d = min (d, PrRoundCylDf (q, 0.03, 0.03, 0.35));
    q.z -= -0.35;
    q.xy = Rot2D (q.xy, -8. * strRot + pi / 6.);
    q.xy = Rot2D (q.xy, 2. * pi * floor (3. * atan (q.y, - q.x) / (2. * pi) + 0.5) / 3.);
    d = min (d, PrTorusDf (q, 0.025, 0.35));
    q.x += 0.17;
    d = min (d, PrCylDf (q.yzx, 0.02, 0.17));
    DMINQ (idStr);
    q = p;
    q.yz -= vec2 (0.5, 0.8);
    d = PrRoundBoxDf (q, vec3 (0.98, 0.2, 0.35) - 0.05, 0.05);
    q.yz = Rot2D (q.yz - vec2 (0.5, -0.45), 0.1 * pi);
    q.x = abs (q.x) - 0.49;
    d = min (d, PrRoundBoxDf (q, vec3 (0.48, 0.35, 0.1) - 0.05, 0.05));
    DMINQ (idSeat);
    q = p;
    q.yz -= vec2 (0.6, -1.);
    d = PrRoundBoxDf (q, vec3 (0.98, 0.2, 0.35) - 0.05, 0.05);
    q.yz = Rot2D (q.yz - vec2 (0.5, -0.45), 0.1 * pi);
    q.x = abs (q.x) - 0.49;
    d = min (d, PrRoundBoxDf (q, vec3 (0.48, 0.35, 0.1) - 0.05, 0.05));
    DMINQ (idSeat);
    q = p;
    q -= vec3 (0.1, -0.1, 1.5);
    q.yz = Rot2D (q.yz, pi * (0.02 + 0.06 * sign (spd)));
    q.y -= 0.5;
    d = PrCapsDf (q.xzy, 0.04, 0.5);
    DMINQ (idStk);
    q = p;
    q.yz -= vec2 (1.1, 2.4);
    d = PrCapsDf (q.xzy, 0.06, 0.05);
    DMINQ (idWLit);
    q = p;
    d = PrCapsDf ((vec3 (abs (q.xz), q.y).xzy - vec3 (0.7, 0.4, carSz.z + 0.02)).yzx, 0.05, 0.3);
    DMINQ (idLit);
  } else dMin = min (dMin, d);
  return dMin;
}

float ObjRay (vec3 ro, vec3 rd)
{
  vec3 p;
  float dHit, d;
  dHit = 0.;
  for (int j = VAR_ZERO; j < 160; j ++) {
    p = ro + dHit * rd;
    d = ObjDf (p);
    dHit += d;
    if (d < 0.001 || dHit > dstFar || p.y < 0.) break;
  }
  if (p.y < 0.) dHit = dstFar;
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

float TrObjDf (vec3 p)
{
  vec3 q;
  float d;
  p -= carPos.xyz;
  p.xz = Rot2D (p.xz, carPos.w);
  q = p;
  q.y -= 1.1;
  return max (PrRound4BoxDf (q, carSz - 1., 1.), -0.5 - q.y);
}

float TrObjRay (vec3 ro, vec3 rd)
{
  vec3 p;
  float dHit, d;
  dHit = 0.;
  for (int j = VAR_ZERO; j < 60; j ++) {
    p = ro + dHit * rd;
    d = TrObjDf (p);
    dHit += d;
    if (d < 0.001 || dHit > dstFar || p.y < 0.) break;
  }
  if (p.y < 0.) dHit = dstFar;
  return dHit;
}

vec3 TrObjNf (vec3 p)
{
  vec4 v;
  vec2 e;
  e = vec2 (0.001, -0.001);
  for (int j = VAR_ZERO; j < 4; j ++) {
    v[j] = TrObjDf (p + ((j < 2) ? ((j == 0) ? e.xxx : e.xyy) : ((j == 2) ? e.yxy : e.yyx)));
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
  isSh = true;
  for (int j = VAR_ZERO; j < 30; j ++) {
    h = ObjDf (ro + d * rd);
    sh = min (sh, smoothstep (0., 0.1 * d, h));
    d += h;
    if (sh < 0.05 || d > dstFar) break;
  }
  isSh = false;
  idObj = idObjT;
  return 0.5 + 0.5 * sh;
}

void SetBldParm ()
{
  bldSz = 4. * floor (vec3 (0.08 * gSize, 4., 0.08 * gSize) * (0.4 +
     0.6 * Hashv3v2 (gId + mOff))) - 0.5;
  trHt = 10. + 2. * bldSz.y;
}

float GObjDf (vec3 p)
{
  vec3 q, qq, qa;
  float dMin, d, db;
  dMin = dstFar;
  q = p;
  q.xz -= gSize * (gId + 0.5);
  q.y -= bldSz.y;
  db = PrBoxDf (q, bldSz);
  if (isTran) {
    d = db;
  } else {
    qq = mod (q + 2., 4.) - 2.;
    qq.y -= 0.2;
    qa = abs (q) - bldSz + 1.;
    d = SmoothMax (abs (db) - 0.1, - min (max (PrBox2Df (qq.xy, vec2 (1.3, 1.)), Maxv2 (qa.xy)),
       max (PrBox2Df (qq.zy, vec2 (1.3, 1.)), Maxv2 (qa.zy))), 0.05);
    d = min (d, max (abs (qq.y - 1.1) - 0.05, db));
  }
  DMINQ (idBld);
  if (! isTran) {
    q.y -= - bldSz.y;
    q = vec3 (abs (q.xz) - bldSz.xz - 4., q.y - 0.1 * trHt);
    d = trHt * PrConCapsDf (q / trHt, sin (0.06 * pi + vec2 (0.5 * pi, 0.)), 0.03, 0.06);
    DMINQ (idTree);
  }
  return dMin;
}

float GObjRay (vec3 ro, vec3 rd)
{
  vec3 p, rdi;
  vec2 gIdP;
  float dHit, d, eps;
  eps = 0.01;
  if (rd.x == 0.) rd.x = 0.0001;
  if (rd.z == 0.) rd.z = 0.0001;
  rdi.xz = 1. / rd.xz;
  gIdP = vec2 (-999.);
  dHit = eps;
  for (int j = VAR_ZERO; j < 120; j ++) {
    p = ro + dHit * rd;
    p.xz -= 0.5 * gSize;
    gId = floor (p.xz / gSize);
    if (gId != gIdP) {
      gIdP = gId;
      SetBldParm ();
    }
    d = GObjDf (p);
    dHit += min (d, eps + max (0., Minv2 ((gSize * (gId + step (0., rd.xz)) - p.xz) * rdi.xz)));
    if (d < eps || dHit > dstFar || p.y < 0.) break;
  }
  if (d >= eps || p.y < 0.) dHit = dstFar;
  return dHit;
}

vec3 GObjNf (vec3 p)
{
  vec4 v;
  vec2 e = vec2 (0.001, -0.001);
  p.xz -= 0.5 * gSize;
  for (int j = VAR_ZERO; j < 4; j ++) {
    v[j] = GObjDf (p + ((j < 2) ? ((j == 0) ? e.xxx : e.xyy) : ((j == 2) ? e.yxy : e.yyx)));
  }
  v.x = - v.x;
  return normalize (2. * v.yzw - dot (v, vec4 (1.)));
}

float GObjSShadow (vec3 ro, vec3 rd)
{
  vec3 p;
  vec2 gIdP;
  float sh, d, h;
  int idObjT;
  idObjT = idObj;
  sh = 1.;
  gIdP = vec2 (-999.);
  d = 0.01;
  for (int j = VAR_ZERO; j < 30; j ++) {
    p = ro + d * rd;
    p.xz -= 0.5 * gSize;
    gId = floor (p.xz / gSize);
    if (gId != gIdP) {
      gIdP = gId;
      SetBldParm ();
    }
    h = GObjDf (p);
    sh = min (sh, smoothstep (0., 0.1 * d, h));
    d += max (h, 0.01);
    if (h < 0.001 || d > dstFar) break;
  }
  idObj = idObjT;
  return 0.5 + 0.5 * sh;
}

float TrailShd (vec2 p)
{
  vec4 u;
  vec2 gB[2], gF[2], dg, q;
  float st, s, gLen;
  st = 1.;
  for (float kz = -1. + float (VAR_ZERO); kz <= 1.; kz ++) {
    gB[0] = carPos.xz;
    gF[0] = gB[0] + Rot2D (vec2 (0., kz * wlBase.z), - carPos.w);
    for (int j = VAR_ZERO; j < npTrail; j ++) {
      gB[1] = gB[0];
      gF[1] = gF[0];
      u = Loadv4 (npFst + j);
      if (u.z >= 0.) {
        gB[0] = u.xy;
        gF[0] = gB[0] + Rot2D (vec2 (0., kz * wlBase.z), - u.z);
        s = 1.;
        if (kz == 0.) {
          dg = gF[0] - gF[1];
          gLen = length (dg);
          if (gLen > 0.) {
            q = Rot2Cs (p - 0.5 * (gF[0] + gF[1]), dg.yx / gLen);
            q.x = abs (q.x) - wlBase.x;
            s = min (s, PrRoundBox2Df (q, vec2 (0.1, 0.5 * gLen), 0.05));
          }
        } else {
          for (float k = -1.; k <= 1.; k += 2.) {
            dg = gF[0] - gF[1];
            gLen = length (dg);
            if (gLen > 0.) {
              q = Rot2Cs (p - 0.5 * (gF[0] + gF[1]) + Rot2D (vec2 (k * wlBase.x, 0.), - u.z),
                 dg.yx / gLen);
              s = min (s, PrRoundBox2Df (q, vec2 (0.1, 0.5 * gLen), 0.05));
            }
          }
        }
        st = min (st, 1. - 0.3 * sqrt (1. - min (1., (float (j) + 20. * (tCur - u.w)) /
           float (npTrail))) * (1. - smoothstep (0., 0.05, s)));
        st = min (st, 1. - 0.3 * sqrt (1. - float (j) / float (npTrail)) *
           (1. - smoothstep (0., 0.05, s)));
      } else break;
    }
  }
  return st;
}

vec3 GrndNf (vec2 p)
{
  vec2 e;
  e = vec2 (0.01, 0.);
  p *= 0.5;
  return vec3 (normalize (vec3 (Fbm2 (p) - vec2 (Fbm2 (p + e.xy), Fbm2 (p + e.yx)), 8. * e.x)).xzy);
}

float PathEdge (vec2 p, float dw)
{
  return PrRoundBox2Df (mod (p + 0.5 * gSize, gSize) - 0.5 * gSize,
     vec2 (0.5 * gSize) - 2. * rdWid - dw, rdWid);
}

float GrndWhlShd (vec2 p)
{
  float d;
  d = dstFar;
  for (int k = 0; k < 6; k ++) d = min (d, length (carPos.xz -
     Rot2D ((vec2 (2. * mod (float (k), 2.), float (k / 2)) - 1.) * wlBase.xz, - carPos.w) - p));
  return 0.7 + 0.3 * smoothstep (0.2, 0.3, d);
}

vec3 GrndCol (vec2 p, float dstGrnd, float sh)
{
  vec3 col, colG, vn;
  vec2 vf, q;
  float f, st;
  colG = 0.4 * mix (vec3 (0.8, 1., 0.5), vec3 (0.7, 0.9, 0.5), 0.2 +
     0.6 * smoothstep (0.3, 0.7, Fbm2 (0.5 * p)));
  vf = vec2 (0.);
  if (PathEdge (p, 0.4) > 0.) {
    f = smoothstep (0., 0.1, abs (PathEdge (p, 0.2)) - 0.2);
    col = mix (vec3 (0.4, 0.4, 0.1), vec3 (0.3), f);
    vn = vec3 (0., 1., 0.);
    vf = vec2 (4., 0.2 + 0.5 * f);
  } else {
    q = p - 0.5 * gSize;
    gId = floor (q / gSize);
    SetBldParm ();
    q -= gSize * (gId + 0.5);
    if (PrRoundBox2Df (q, bldSz.xz + 2., 0.5) < 0.) {
      col = vec3 (0.25);
      vn = vec3 (0., 1., 0.);
      vf = vec2 (4., 1.);
    } else {
      col = colG;
      vn = GrndNf (p);
    }
  }
  col *= GrndWhlShd (p);
  st = TrailShd (p);
  vf *= 1. + 2. * step (st, 0.99);
  vf.y *= 1. - smoothstep (0.5, 0.8, dstGrnd / dstFar);
  if (vf.y > 0.) vn = VaryNf (vf.x * vec3 (p.x, 0., p.y), vn, vf.y);
  col *= (0.2 + 0.8 * sh * max (dot (vn, sunDir), 0.)) * (0.5 + 0.5 * st);
  return col;
}

vec3 SkyBgCol (vec3 ro, vec3 rd)
{
  vec3 col, clCol, skCol;
  vec2 q;
  float f, fd, ff, sd;
  if (rd.y > -0.02 && rd.y < 0.03 * Fbm1 (16. * atan (rd.z, - rd.x))) {
    col = vec3 (0.3, 0.4, 0.5);
  } else {
    q = 0.01 * (ro.xz + 2. * tCur + ((100. - ro.y) / rd.y) * rd.xz);
    ff = Fbm2 (q);
    f = smoothstep (0.2, 0.8, ff);
    fd = smoothstep (0.2, 0.8, Fbm2 (q + 0.01 * sunDir.xz)) - f;
    clCol = (0.7 + 0.5 * ff) * (vec3 (0.7) - 0.7 * vec3 (0.3, 0.3, 0.2) * sign (fd) *
       smoothstep (0., 0.05, abs (fd)));
    sd = max (dot (rd, sunDir), 0.);
    skCol = vec3 (0.3, 0.4, 0.8) + step (0.1, sd) * vec3 (1., 1., 0.9) *
       min (0.3 * pow (sd, 64.) + 0.5 * pow (sd, 2048.), 1.);
    col = mix (skCol, clCol, 0.1 + 0.9 * f * smoothstep (0.01, 0.1, rd.y));
  }
  return col;
}

vec4 CarCol (out float refFac)
{
  vec4 col4, colB4;
  float r;
  colB4 = vec4 (0.95, 0.9, 0.9, 0.2);
  col4 = colB4;
  if (idObj == idBod) {
    if (! inCab) {
      if (abs (qHit.z) < wlBase.z + wlRad + 0.2) col4.rgb *= 0.7 + 0.3 *
         smoothstep (0., 0.02, abs (length (vec2 (qHit.y + 1.1, wlBase.z *
         (fract (qHit.z / wlBase.z + 0.5) - 0.5))) - wlRad - 0.14) - 0.02);
      col4.rgb *= 0.7 + 0.3 * smoothstep (0., 0.02, PrRoundBox2Df (vec2 (qHit.y - 0.3,
         abs (qHit.z) - 1.05), vec2 (0.3, 0.75), 0.2) - 0.04);
      col4.rgb *= 0.7 + 0.3 * smoothstep (0., 0.02, PrRoundBox2Df (vec2 (abs (qHit.x) - 0.6,
         qHit.y - 0.3), vec2 (0.3, 0.25), 0.2) - 0.04);
      if (qHit.y > 0.) col4.rgb *= 0.7 + 0.3 * smoothstep (0., 0.02,
         PrRoundBox2Df (vec2 (qHit.x, qHit.z - 0.8), vec2 (0.8, 0.5), 0.2) - 0.04);
    } else {
      col4 *= 0.5;
    }
  } else if (idObj == idCam) {
    col4.rgb *= 0.1 + 0.9 * smoothstep (0., 0.02, length (vec2 (qHit.y, dot (qHit.zx,
       sin (2. * pi * floor (12. * atan (qHit.z, - qHit.x) / (2. * pi) + 0.5) / 12. +
       vec2 (0.5 * pi, 0.))))) - 0.12); 
  } else if (idObj == idAx) {
    col4 = vec4 (0.3, 0.3, 0.4, 0.1);
  } else if (idObj == idWhl) {
    r = wlRad - length (qHit.yz);
    if (r < 0.17) {
      col4 = vec4 (0.3, 0.3, 0.3, 0.);
      if (r < 0.07) col4 *= 1. - 0.5 * abs (step (0., cos (32. * pi * qHit.x)) -
         step (0.5, mod (32. * atan (qHit.z, - qHit.y) / (2. * pi) + 0.5, 1.)));
    } else if (r < 0.2 || qHit.x < 0.) {
      col4 *= 0.5;
    }
  } else if (idObj == idStr) {
    col4 = vec4 (0.9, 0.9, 0.7, 0.2);
  } else if (idObj == idStk) {
    col4 = vec4 (0.9, 0.9, 0.7, 0.2);
  } else if (idObj == idSeat) {
    col4 = vec4 (0.9, 0.7, 0.4, 0.05) * (0.95 + 0.05 * cos (64. * qHit.x));
  } else if (idObj == idWLit) {
    col4 = (onPath > 0.) ? vec4 (0., 0.8, 0., -1.) : vec4 (0.8, 0., 0., -1.);
  } else if (idObj == idLit) {
    if (abs (strRot) > 0.03 * pi && strRot * qHit.x < 0. && abs (qHit.x) > 0.95 &&
       mod (2. * tCur, 1.) > 0.5) col4 = vec4 (0.7, 0.7, 0., -1.);
    else col4 = (qHit.z > 0.) ? vec4 (1., 1., 0.95, -1.) : vec4 (0.9, 0., 0., -1.);
    col4.rgb *= 0.8 + 0.2 * smoothstep (0., 0.02, abs (abs (qHit.x) - 0.95));
  }
  refFac = 0.;
  if (col4 == colB4) refFac = 0.3;
  return col4;
}

vec3 HorizCol (vec3 col, vec3 rd)
{
  return mix (col, vec3 (0.3, 0.4, 0.5), pow (1. + rd.y, 16.));
}

vec4 BldCol (inout vec3 vn)
{
  vec4 col4;
  vec2 g, vf;
  float h, s, bn;
  h = mod (dot (gId + mOff, vec2 (17.11, 21.11)), 1.);
  vf = vec2 (0.);
  if (idObj == idBld) {
    col4 = vec4 (HsvToRgb (vec3 (h, 0.4, 0.8)), 0.1);
    if (PrBoxDf (qHit, bldSz) < 0.) {
      col4 *= 0.3;
      if (vn.y < -0.95) col4 = mix (vec4 (1., 1., 0.5, -1.), col4, 
         step (0.5, length (mod (qHit.xz + 2., 4.) - 2.)));
    } else {
      s = 0.;
      g = vec2 (dot (qHit.xz, normalize (vec2 (- vn.z, vn.x))) - 1.3, qHit.y + bldSz.y - 0.8);
      if (Maxv2 (abs (vn.xz)) > 0.99 && PrBox2Df (g, vec2 (3., 1.)) < 0.1) {
        bn = dot (mod (gId + 31. + floor (0.4 * mOff), 100.), vec2 (100., 1.));
        s = ShowIntPZ (g, vec2 (2.4, 0.6), 4., bn);
        if (s > 0.) col4 = vec4 (1., 1., 0.8, -1.);
      }
      if (s == 0.) vf = vec2 (16., 0.5);
    }
  } else if (idObj == idTree) {
    col4 = vec4 (HsvToRgb (vec3 (mod (0.25 + 0.2 * h, 1.), 0.6, 0.9)), 0.);
    vf = vec2 (32., 1.);
  }
  if (vf.y > 0.) vn = VaryNf (vf.x * qHit, vn, vf.y);
  return col4;
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec4 col4;
  vec3 roo, rdo, col, colR, vn, qHitG, q;
  float dstObjG, dstObj, dstTrObj, dstGrnd, nDotL, refFac, sh;
  int idObjG;
  roo = ro;
  rdo = rd;
  isSh = false;
  isTran = false;
  carSz = vec3 (wlBase.x + 0.05, 1.2, wlBase.z + 1.);
  for (int k = VAR_ZERO; k < 6; k ++) {
    axRot[k] = vec2 (2. * mod (float (k), 2.), float (k / 2)) - 1.;
    wlRotCs[k] = sin (- strRot * axRot[k].y + vec2 (0.5 * pi, 0.));
    wlAngCs[k] = sin (- ((axRot[k].y != 1.) ? ((axRot[k].x < 0.) ? wlRot.x : wlRot.y) :
       ((axRot[k].x < 0.) ? wlRot.z : wlRot.w)) + vec2 (0.5 * pi, 0.));
  }
  dstObjG = GObjRay (ro, rd);
  idObjG = idObj;
  qHitG = qHit;
  dstObj = ObjRay (ro, rd);
  if (dstObjG < min (dstObj, dstFar)) {
    idObj = idObjG;
    qHit = qHitG;
    dstObj = dstObjG;
  }
  if (dstObj < dstFar) {
    ro += dstObj * rd;
    vn = (idObj == idObjG) ? GObjNf (ro) : ObjNf (ro);
    if (idObj == idBod) inCab = (PrRound4BoxDf (qHit, carSz - 1., 1.) < 0.03);
    refFac = 0.;
    col4 = (idObj == idObjG) ? BldCol (vn) : CarCol (refFac);
    if (col4.a >= 0.) {
      nDotL = max (dot (vn, sunDir), 0.);
      if (idObj != idObjG) {
        sh = ObjSShadow (ro + 0.01 * vn, sunDir);
      } else {
        sh = 1.;
        nDotL *= nDotL;
      }
      col = col4.rgb * (0.2 + 0.8 * sh * nDotL) + col4.a * step (0.95, sh) *
         pow (max (dot (sunDir, reflect (rd, vn)), 0.), 32.);
      if (idObj == idObjG) col = mix (col, 0.9 * HorizCol (GrndCol (ro.xz, dstGrnd, sh),
         rd), smoothstep (0.85, 0.95, dstObj / dstFar));
      if (refFac > 0.) rd = reflect (rd, vn);
    } else if (col4.a == -1.) {
      col = col4.rgb * (0.5 - 0.5 * dot (rd, vn));
    }
  }
  if (dstObj >= dstFar || refFac > 0.) {
    if (rd.y < 0.) {
      dstGrnd = - ro.y / rd.y;
      ro += dstGrnd * rd;
      sh = (dstGrnd < dstFar) ? min (ObjSShadow (ro + 0.01 * vec3 (0., 1., 0.), sunDir),
         GObjSShadow (ro + 0.01 * vec3 (0., 1., 0.), sunDir)) : 1.;
      colR = HorizCol (GrndCol (ro.xz, dstGrnd, sh), rd);
    } else {
      colR = SkyBgCol (ro, rd);
    }
    col = (refFac > 0.) ? mix (col, 0.9 * colR, refFac) : colR;
  }
  dstTrObj = TrObjRay (roo, rdo);
  if (dstTrObj < min (dstObj, dstFar)) {
    ro = roo + dstTrObj * rdo;
    vn = TrObjNf (ro);
    col *= vec3 (0.9, 1., 0.9);
    rd = reflect (rdo, vn);
    col = mix (col, SkyBgCol (ro, rd), 0.2 + 0.8 * pow (1. - abs (dot (vn, rd)), 5.));
  }
  isTran = true;
  dstObjG = GObjRay (roo, rdo);
  if (dstObjG < min (dstObj, dstFar)) {
    ro = roo + dstObjG * rdo;
    vn = GObjNf (ro);
    rd = reflect (rdo, vn);
    col = mix (0.8 * col, SkyBgCol (ro, rd), 0.6);
  }
  return clamp (col, 0., 1.);
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
  vec4 stDat;
  vec3 rd, ro, col, c, wgBox;
  vec2 canvas, uv, ud, cnPos;
  float el, az, asp, zmFac, trvDist, s, sr;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iTime;
  asp = canvas.x / canvas.y;
  stDat = Loadv4 (1);
  wlBase = stDat.xyz;
  wlRad = wlBase.y;
  spd = stDat.w;
  stDat = Loadv4 (2);
  gSize = stDat.x;
  rdWid = stDat.y;
  onPath = stDat.z;
  trvDist = stDat.w;
  stDat = Loadv4 (3);
  az = stDat.x;
  el = stDat.y;
  cnPos = stDat.zw;
  stDat = Loadv4 (4);
  strRot = stDat.y;
  mOff = mod (vec2 (floor (stDat.w), floor (stDat.w / 100.)), 100.);
  stDat = Loadv4 (5);
  carPos = stDat;
  stDat = Loadv4 (6);
  wlRot = stDat;
  el = clamp (el - 0.1 * pi, -0.49 * pi, 0.02 * pi);
  vuMat = StdVuMat (el, az);
  ro = carPos.xyz + vuMat * vec3 (0., 1., -12.);
  ro.y += 2.;
  carPos.y += wlRad;
  zmFac = 3.5 + 2. * el;
  dstFar = 400.;
  sunDir = normalize (vec3 (1., 2., -1.));
  sunDir.xz = Rot2D (sunDir.xz, - az); //vuMat * normalize (vec3 (1., 1.5, -1.));
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
  wgBox = vec3 (0.41, -0.32, 0.135);
  ud = 0.5 * uv - wgBox.xy * vec2 (asp, 1.);
  s = (length (ud) - wgBox.z) * canvas.y;
  col = mix (vec3 (0., 1., 1.), col, smoothstep (0., 1., abs (s) - 1.));
  if (s < 0.) {
    col = mix (vec3 (0., 1., 1.), col, step (1., Minv2 (abs (ud)) * canvas.y));
    c = (onPath > 0.) ? vec3 (0., 1., 0.) : vec3 (0.8, 0., 0.);
    col = mix (c, col,
       smoothstep (2.5, 3.5, abs (length (ud - cnPos) * canvas.y - 10.)));
    ud = Rot2D (ud, atan (cnPos.y, - cnPos.x));
    if (ud.x < 0. && (length (cnPos) - length (ud)) * canvas.y > 10.)
       col = mix (c, col, smoothstep (1.5, 2.5, abs (ud.y) * canvas.y));
  }
  col = mix (col, vec3 (0., 1., 1.), ShowIntPZ (0.5 * uv - vec2 (0.44 * asp, -0.15),
     vec2 (0.06 * asp, 0.03), 4., mod (floor (trvDist / (2. * wlRad)), 1e4)));
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

float PrRoundBoxDf (vec3 p, vec3 b, float r)
{
  return length (max (abs (p) - b, 0.)) - r;
}

float PrRound4BoxDf (vec3 p, vec3 b, float r)
{
  p = max (abs (p) - b, 0.);
  return sqrt (length (p * p)) - r;
}

float PrRoundBox2Df (vec2 p, vec2 b, float r)
{
  return length (max (abs (p) - b, 0.)) - r;
}

float PrCylDf (vec3 p, float r, float h)
{
  return max (length (p.xy) - r, abs (p.z) - h);
}

float PrRoundCylDf (vec3 p, float r, float rt, float h)
{
  return length (max (vec2 (length (p.xy) - r, abs (p.z) - h), 0.)) - rt;
}

float PrCapsDf (vec3 p, float r, float h)
{
  return length (p - vec3 (0., 0., clamp (p.z, - h, h))) - r;
}

float PrConCapsDf (vec3 p, vec2 cs, float r, float h)
{
  vec2 b;
  float d;
  d = max (dot (vec2 (length (p.xy) - r, p.z), cs), abs (p.z) - h);
  h /= cs.x * cs.x;
  r /= cs.x;
  b = vec2 (r, h);
  b *= cs.y;
  p.z += b.x;
  return min (d, min (length (p - vec3 (0., 0., h)) - r + b.y,
     length (p - vec3 (0., 0., - h)) - r  - b.y));
}

float PrTorusDf (vec3 p, float ri, float rc)
{
  return length (vec2 (length (p.xy) - rc, p.z)) - ri;
}

float Minv2 (vec2 p)
{
  return min (p.x, p.y);
}

float Maxv2 (vec2 p)
{
  return max (p.x, p.y);
}

float SmoothMin (float a, float b, float r)
{
  float h;
  h = clamp (0.5 + 0.5 * (b - a) / r, 0., 1.);
  return mix (b - h * r, a, h);
}

float SmoothMax (float a, float b, float r)
{
  return - SmoothMin (- a, - b, r);
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

vec3 HsvToRgb (vec3 c)
{
  return c.z * mix (vec3 (1.), clamp (abs (fract (c.xxx + vec3 (1., 2./3., 1./3.)) * 6. - 3.) - 1., 0., 1.), c.y);
}

float DigSeg (vec2 q)
{
  return (1. - smoothstep (0.13, 0.17, abs (q.x))) *
     (1. - smoothstep (0.5, 0.57, abs (q.y)));
}

#define DSG(q) k = kk;  kk = k / 2;  if (kk * 2 != k) d += DigSeg (q)

float ShowDig (vec2 q, int iv)
{
  float d;
  int k, kk;
  const vec2 vp = vec2 (0.5, 0.5), vm = vec2 (-0.5, 0.5), vo = vec2 (1., 0.);
  if (iv == -1) k = 8;
  else if (iv < 2) k = (iv == 0) ? 119 : 36;
  else if (iv < 4) k = (iv == 2) ? 93 : 109;
  else if (iv < 6) k = (iv == 4) ? 46 : 107;
  else if (iv < 8) k = (iv == 6) ? 122 : 37;
  else             k = (iv == 8) ? 127 : 47;
  q = (q - 0.5) * vec2 (1.8, 2.3);
  d = 0.;
  kk = k;
  DSG (q.yx - vo);  DSG (q.xy - vp);  DSG (q.xy - vm);  DSG (q.yx);
  DSG (q.xy + vm);  DSG (q.xy + vp);  DSG (q.yx + vo);
  return d;
}

float ShowIntPZ (vec2 q, vec2 cBox, float mxChar, float val)
{
  float nDig, idChar, s, v;
  q = vec2 (- q.x, q.y) / cBox;
  s = 0.;
  if (min (q.x, q.y) >= 0. && max (q.x, q.y) < 1.) {
    q.x *= mxChar;
    nDig = mxChar;
    idChar = mxChar - 1. - floor (q.x);
    q.x = fract (q.x);
    val = max (val, 0.);
    v = val / pow (10., mxChar - idChar - 1.);
    if (idChar >= mxChar - nDig) s = ShowDig (q, int (mod (floor (v), 10.)));
  }
  return s;
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

vec3 Hashv3v2 (vec2 p)
{
  vec2 cHashVA2 = vec2 (37., 39);
  return fract (sin (dot (p, cHashVA2) + vec3 (0., cHashVA2.xy)) * cHashM);
}

float Noiseff (float p)
{
  vec2 t;
  float ip, fp;
  ip = floor (p);
  fp = fract (p);
  fp = fp * fp * (3. - 2. * fp);
  t = Hashv2f (ip);
  return mix (t.x, t.y, fp);
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

float Fbm1 (float p)
{
  float f, a;
  f = 0.;
  a = 1.;
  for (int j = 0; j < 5; j ++) {
    f += a * Noiseff (p);
    a *= 0.5;
    p *= 2.;
  }
  return f * (1. / 1.9375);
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

#define txBuf iChannel0
#define txSize iChannelResolution[0].xy

const float txRow = 128.;

vec4 Loadv4 (int idVar)
{
  float fi;
  fi = float (idVar);
  return texture (txBuf, (vec2 (mod (fi, txRow), floor (fi / txRow)) + 0.5) / txSize);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// "Drive the Mapper" by dr2 - 2022
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

float PrRoundBox2Df (vec2 p, vec2 b, float r);
float SmoothBump (float lo, float hi, float w, float x);
vec2 Rot2D (vec2 q, float a);
vec4 Loadv4 (int idVar);
void Savev4 (int idVar, vec4 val, inout vec4 fCol, vec2 fCoord);

const int npTrail = 32;  // same for image and buffer
const int npFst = 7;

vec4 carPos;
vec3 wlBase;
float gSize, rdWid;
const float pi = 3.1415927;
const float txRow = 128.;

bool OnPath ()
{
  vec2 p;
  bool onP;
  onP = true;
  for (int k = 0; k < 6; k ++) {
    if (k <= 1 || k >= 4) {
      p = carPos.xz - Rot2D ((vec2 (2. * mod (float (k), 2.), float (k / 2)) - 1.) *
         wlBase.xz, - carPos.w);
      onP = (PrRoundBox2Df (mod (p + 0.5 * gSize, gSize) - 0.5 * gSize, vec2 (0.5 * gSize) -
         2. * rdWid, rdWid) > 0.);
      if (! onP) break;
    }
  }
  return onP;
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  vec4 mPtr, mPtrP, stDat, carPosP, wlRot, wr;
  vec3 wgBox;
  vec2 iFrag, canvas, cnPos, cp, ud, w, rTurn;
  float todCur, tCur, autoMd, cRotN, wlRad, strRot, spd, msOff, trvDist,
     onPath, tc, nStep, az, el, asp, s;
  int pxId, wgSel, nFrame;
  bool init;
  iFrag = floor (fragCoord);
  pxId = int (iFrag.x + txRow * iFrag.y);
  if (pxId >= npFst + npTrail) discard;
  nFrame = iFrame;
  canvas = iResolution.xy;
  tCur = iTime;
  todCur = iDate.w;
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  init = (nFrame <= 1);
  asp = canvas.x / canvas.y;
  if (init) {
    gSize = 80.;
    msOff = mod (floor (1000. * todCur), 10000.);
    carPos = vec4 (0.5 * gSize + 3., 0., 0., 0.);
    strRot = 0.;
    wlRad = 0.5;
    wlBase = vec3 (1.4, wlRad, 1.5);
    wlRot = vec4 (0.);
    az = -0.3 * pi;
    el = -0.1 * pi;
    nStep = 0.;
    cnPos = vec2 (0.);
    mPtrP = mPtr;
    wgSel = 0;
    autoMd = 1.;
    rdWid = 5.;
    onPath = 0.;
    trvDist = 0.;
  } else {
    stDat = Loadv4 (0);
    mPtrP.xyz = stDat.xyz;
    wgSel = int (stDat.w);
    stDat = Loadv4 (1);
    wlBase = stDat.xyz;
    stDat = Loadv4 (2);
    gSize = stDat.x;
    rdWid = stDat.y;
    onPath = stDat.z;
    trvDist = stDat.w;
    stDat = Loadv4 (3);
    az = stDat.x;
    el = stDat.y;
    cnPos = stDat.zw;
    stDat = Loadv4 (4);
    nStep = stDat.x;
    strRot = stDat.y;
    autoMd = stDat.z;
    msOff = stDat.w;
    stDat = Loadv4 (5);
    carPos = stDat;
    stDat = Loadv4 (6);
    wlRot = stDat;
  }
  if (pxId < npFst) {
    wgBox = vec3 (0.41, -0.32, 0.135);
    if (mPtr.z > 0.) {
      if (wgSel == 0 && mPtrP.z > 0.) {
        az = -2. * pi * mPtr.x;
        el = - pi * mPtr.y;
      } else {
        ud = mPtr.xy * vec2 (asp, 1.) - wgBox.xy * vec2 (asp, 1.);
        if (wgSel == 1) {
          autoMd = - tCur;
          cnPos = ud;
          s = length (cnPos);
          if (s > 0.) cnPos = min (s, wgBox.z) * cnPos / s;
        } else if (mPtrP.z <= 0. && length (ud) < wgBox.z) wgSel = 1;
      }
    } else {
      wgSel = 0;
      cnPos *= vec2 (1. - 1e-2, 1. - 2e-3);
    }
    wlRad = wlBase.y;
    ++ nStep;
    if (autoMd > 0.) {
      tc = mod (0.004 * nStep, 4.);
      strRot = 0.04 * pi * SmoothBump (0.3, 0.7, 0.1, mod (tc, 1.)) * sign (mod (tc, 2.) - 1.) *
         sign (tc - 2.);
      spd = 0.7 * (0.12 - 0.06 * abs (strRot / (0.15 * pi)));
      w = vec2 (- strRot / (0.15 * pi), spd / 0.5);
      w = pow (abs (w), 1. / vec2 (1.5)) * sign (w);
      cnPos = w * wgBox.z;
    } else {
      w = cnPos / wgBox.z;
      w = pow (abs (w), vec2 (1.5)) * sign (w);
      strRot = -0.15 * pi * w.x;
      spd = 0.2 * w.y * smoothstep (0.01, 0.02, abs (w.y));
      if (tCur + autoMd > 20.) autoMd = 1.;
    }
    carPosP = carPos;
    cp = carPos.xz;
    wr = vec4 (1.);
    if (abs (strRot) > 1e-4) {
      cRotN = carPos.w - strRot * spd / pi;
      rTurn.x = wlBase.z / asin (strRot);
      s = wlBase.z / rTurn.x;
      rTurn.y = rTurn.x * sqrt (1. - s * s);
      carPos.xz += 2. * rTurn.x * (sin (carPos.w - vec2 (0.5 * pi, 0.)) -
         sin (cRotN - vec2 (0.5 * pi, 0.)));
      carPos.w = mod (cRotN, 2. * pi);
      wr += wlBase.x * vec4 (-1., 1., -1., 1.) / rTurn.xxyy;
    } else {
      carPos.xz += spd * sin (carPos.w + vec2 (0., 0.5 * pi));
    }
    onPath = OnPath () ? 1. : 0.;
    if (onPath > 0.) {
      wlRot += wr * spd / wlRad;
      if (spd > 0.) trvDist += length (carPos.xz - cp);
    } else {
      carPos = carPosP;
      spd = 0.;
      trvDist = 0.;
    }
  }
  if (! init) {
    if (mod (float (nFrame), 12.) == 0.) {
      if (pxId == npFst) stDat = vec4 (Loadv4 (5).xzw, tCur);
      else if (pxId < npFst + npTrail) stDat = Loadv4 (pxId - 1);
    } else if (pxId >= npFst && pxId < npFst + npTrail) stDat = Loadv4 (pxId);
  } else {
    stDat = vec4 (0., 0., -1., 0.);
  }
  if      (pxId == 0) stDat = vec4 (mPtr.xyz, float (wgSel));
  else if (pxId == 1) stDat = vec4 (wlBase, spd);
  else if (pxId == 2) stDat = vec4 (gSize, rdWid, onPath, trvDist);
  else if (pxId == 3) stDat = vec4 (az, el, cnPos);
  else if (pxId == 4) stDat = vec4 (nStep, strRot, autoMd, msOff);
  else if (pxId == 5) stDat = carPos;
  else if (pxId == 6) stDat = wlRot;
  Savev4 (pxId, stDat, fragColor, fragCoord);
}

float PrRoundBox2Df (vec2 p, vec2 b, float r)
{
  return length (max (abs (p) - b, 0.)) - r;
}

float SmoothBump (float lo, float hi, float w, float x)
{
  return (1. - smoothstep (hi - w, hi + w, x)) * smoothstep (lo - w, lo + w, x);
}

vec2 Rot2D (vec2 q, float a)
{
  vec2 cs;
  cs = sin (a + vec2 (0.5 * pi, 0.));
  return vec2 (dot (q, vec2 (cs.x, - cs.y)), dot (q.yx, cs));
}

#define txBuf iChannel0
#define txSize iChannelResolution[0].xy

vec4 Loadv4 (int idVar)
{
  float fi;
  fi = float (idVar);
  return texture (txBuf, (vec2 (mod (fi, txRow), floor (fi / txRow)) + 0.5) / txSize);
}

void Savev4 (int idVar, vec4 val, inout vec4 fCol, vec2 fCoord)
{
  vec2 d;
  float fi;
  fi = float (idVar);
  d = abs (fCoord - vec2 (mod (fi, txRow), floor (fi / txRow)) - 0.5);
  if (max (d.x, d.y) < 0.5) fCol = val;
}
