
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/ec8a6ea755d34600547a5353f21f0a453f9f55ff95514383b2d80b8d71283eda.mp3' to iChannel0


// Created by mrange/2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// 
// Messing around with refractions. I am quite new to shader programming
//  and raymarching so while I think the code might not be very good I also think that
//  what makes ShaderToy.com so good is that programmers are sharing what they do, 
//  big and small. I too want to share in the hope it might help someone on my level.
// 
// Inpiration and code from shaders:
//  https://www.shadertoy.com/view/4ds3zn (iq, inspirations and various functions)
//  https://www.shadertoy.com/view/XljGDz (otaviogood, "skybox")
//  https://www.shadertoy.com/view/Xl2GDW (purton, inspiration for reflection)
// Music: Levi Patel - As she passes (Soundcloud)
// Blogs:
//  Raymarching explained: http://9bitscience.blogspot.com/2013/07/raymarching-distance-fields_14.html
//  Distance Estimators: iquilezles.org/articles/distfunctions
//  Cool primitives: http://mercury.sexy/hg_sdf/
#define TOLERANCE       0.000001f
#define MAX_RAY_LENGTH  16.0f
#define MAX_BOUNCES     8
#define MAX_RAY_MARCHES 90
#define ABSORB_COLOR    1.0f,2.0f,3.0f
#define PI              3.141592654f
#define TAU             (2.0f*PI)

#define DEG2RAD         (PI/180.0f)

#define PERIODTIME      10.0f
#define FADE_RATE        0.5f
#define ITIME           (iTime*2.0f)
#define TIMEINPERIOD    (mod_f(ITIME, PERIODTIME))
#define PERIOD          (mod_f(_floor(ITIME / PERIODTIME), 9.0f))
//#define PERIOD          8

#define AA              0

__DEVICE__ float sgn(float x)
{
  return (x<0.0f)?-1.0f:1.0f;
}

__DEVICE__ float smin(float a, float b, float k)
{
  float res = _expf( -k*a ) + _expf( -k*b );
  return -_logf( res )/k;
}

__DEVICE__ float pReflect(inout float3 *p, float3 planeNormal, float offset)
{
  float t = dot(*p, planeNormal)+offset;
  if (t < 0.0f)
  {
    *p = *p - (2.0f*t)*planeNormal;
  }
  return sgn(t);
}

__DEVICE__ float2 pR(float2 p, float a)
{
  p = _cosf(a)*p + _sinf(a)*to_float2(p.y, -p.x);
  return p;
}

__DEVICE__ float maxComp(in float3 p)
{
  return _fmaxf(p.x,_fmaxf(p.y,p.z));
}

__DEVICE__ float lengthN(in float3 v, in float n)
{
  v = abs_f3(v);
  v = pow_f3(v, to_float3_s(n));
  return _powf(v.x + v.y + v.z, 1.0f/n);
}

__DEVICE__ float sdRoundCube(in float3 p, float r)
{
  return lengthN(p, 8.0f) - r;
}

__DEVICE__ float3 pMod3(inout float3 *p, float3 size)
{
  float3 c = _floor((*p + size*0.5f)/size);
  *p = mod_f3f3(*p + size*0.5f, size) - size*0.5f;
  return c;
}

__DEVICE__ float sdBox(float3 p, float3 b)
{
  float3  di = abs_f3(p) - b;
  float mc = maxComp(di);
  return _fminf(mc,length(_fmaxf(di,to_float3_s(0.0f))));
}

__DEVICE__ float sdSphere(float3 p, float r)
{
  return length(p) - r;
}

__DEVICE__ float impulse1(in float3 p, out float3 *col, out float *ref, out float *trans, out float3 *absorb)
{
  *col    = to_float3_s(0.3f);
  *ref    = 0.2f;
  *trans  = 0.9f;
  *absorb = 0.5f*to_float3(ABSORB_COLOR);

  float s  = sdSphere(p, 0.6f);
  float is = sdSphere(p, 0.8f);
  float rc = sdRoundCube(p, 1.0f);

  float d = rc;
  d = _fmaxf(d, -is);
  d = _fminf(d, s);
  if (d == s)
  {
    *absorb = 1.0f*to_float3(1.0f, -1.0f, -2.0f);
  }
  return d;
}

__DEVICE__ float impulse2(in float3 p, out float3 *col, out float *ref, out float *trans, out float3 *absorb)
{
  *col    = to_float3_s(0.3f);
  *ref    = 0.2f;
  *trans  = 0.9f;
  *absorb = 0.5f*to_float3(ABSORB_COLOR);

  float s1 = sdBox(p, to_float3_s(1.0f));
  float s3 = sdSphere(p, 0.4f);
  pMod3(&p, to_float3_s(1.0f));
  float s2 = sdSphere(p, 0.48f);
  float s = _fmaxf(s1, -s2);
  s = _fminf(s, s3);
  if (s == s3)
  {
    *absorb = 1.0f*to_float3(1.0f, -1.0f, -2.0f);
  }

  return s;
}

__DEVICE__ float mandelbulb(in float3 p)
{
  float3 w = p;
  float m = dot(w,w);

  float dz = 1.0f;

  dz = 8.0f*_powf(sqrt(m),7.0f)*dz + 1.0f;

  float r = length(w);
  float b = 8.0f*_acosf(w.y/r);
  float a = 8.0f*_atan2f(w.x, w.z);
  float3 v  = to_float3(_sinf(b)*_sinf(a), _cosf(b), _sinf(b)*_cosf(a));
  w = p + _powf(r,8.0f)*v;

  m = dot(w,w);

  return 0.25f*_logf(m)*_sqrtf(m)/dz;
}


__DEVICE__ float impulse3(in float3 p, out float3 *col, out float *ref, out float *trans, out float3 *absorb)
{
  *col    = to_float3_s(0.3f);
  *ref    = 0.2f;
  *trans  = 0.9f;
  *absorb = 0.5f*to_float3(ABSORB_COLOR);

  float rc = sdRoundCube(p, 1.0f);
  float s  = 0.9f;
  float mb = mandelbulb(p/s)*s;
  float s1 = sdSphere(p, 0.4f);
  float d = rc;
  d = _fmaxf(d, -mb);
  d = _fminf(d, s1);
  if (d == s1)
  {
    *absorb = 1.0f*to_float3(1.0f, -1.0f, -2.0f);
  }
  return d;
}

__DEVICE__ float bubbles(in float3 p, float iTime)
{
  float3 pp = p - to_float3_s(0.1f) - to_float3(0.0f, ITIME*0.15f + 10.0f, 0.0f);
  pReflect(&pp, normalize(to_float3(1.0f, 0.5f, 0.2f)), 0.3f);
  pReflect(&pp, normalize(to_float3(0.2f, 0.5f, 1.0f)), 0.2f);
  pMod3(&pp, to_float3(0.5f, 0.3f, 0.4f));

  float3 ppp = p - to_float3_s(0.2f) - to_float3(0.0f, ITIME*0.05f + 10.0f, 0.0f);
  pReflect(&ppp, normalize(to_float3(0.7f, 0.5f, 0.4f)), 0.3f);
  pReflect(&ppp, normalize(to_float3(0.5f, 0.4f, 0.7f)), 0.1f);
  pMod3(&ppp, to_float3(0.7f, 0.6f, 0.4f));

  float ss = sdSphere(pp, 0.05f);
  float sss = sdSphere(ppp, 0.1f);

  return smin(ss, sss, 20.0f);
}
__DEVICE__ float impulse5(in float3 p, float t, out float3 *col, out float *ref, out float *trans, out float3 *absorb, float iTime)
{
  *col    = to_float3_s(0.3f);
  *ref    = 0.2f;
  *trans  = 0.9f;
  *absorb = 0.5f*to_float3(ABSORB_COLOR);

  float sb = sdRoundCube(p, 1.0f);
  
  float s  = 25.0f / (1.0f + 9.0f*t/PERIODTIME);
  float bs = bubbles(p/s, iTime)*s;

  return _fmaxf(sb, -bs);
}

__DEVICE__ float impulse7(in float3 p, out float3 *col, out float *ref, out float *trans, out float3 *absorb, float iTime)
{
  *col    = to_float3_s(0.3f);
  *ref    = 0.2f;
  *trans  = 0.9f;
  *absorb = 0.5f*to_float3(ABSORB_COLOR);

  float sb = sdRoundCube(p, 1.0f);
  float s = 2.5f;
  float bs = bubbles(p/s, iTime)*s;

  return _fmaxf(sb, -bs);
}


__DEVICE__ float distanceField(in float3 p, out float3 *col, out float *ref, out float *trans, out float3 *absorb, float iTime)
{
  int period = (int)(PERIOD);
 
  float i = 10000000.0f;

  if (period == 0)      i = impulse1(p, col, ref, trans, absorb);
  else if (period == 1) i = impulse5(p,TIMEINPERIOD, col, ref, trans, absorb, iTime);
  else if (period == 2) i = impulse7(p, col, ref, trans, absorb, iTime);
  else if (period == 3) i = impulse3(p, col, ref, trans, absorb);
  else if (period == 4) i = impulse2(p, col, ref, trans, absorb);
  else if (period == 5) i = impulse3(p, col, ref, trans, absorb);
  else if (period == 6) i = impulse2(p, col, ref, trans, absorb);
  else if (period == 7) i = impulse7(p, col, ref, trans, absorb, iTime);
  else if (period == 8) i = impulse2(p, col, ref, trans, absorb);

  float rr=TIMEINPERIOD/PERIODTIME;
  float rat=_sinf(rr/FADE_RATE*PI/2.0f);
  if (rr<FADE_RATE)
  {
      float3 col2,absorb2;
      float trans2,ref2;
      float i2 = 10000000.0f;
      if (period == 0)      i2= impulse2(p, &col2, &ref2, &trans2, &absorb2);
      else if (period == 1) i2= impulse1(p, &col2, &ref2, &trans2, &absorb2);
      else if (period == 2) i2= impulse5(p,TIMEINPERIOD+PERIODTIME, &col2, &ref2, &trans2, &absorb2, iTime);
      else if (period == 3) i2= impulse7(p, &col2, &ref2, &trans2, &absorb2, iTime);
      else if (period == 4) i2= impulse3(p, &col2, &ref2, &trans2, &absorb2);
      else if (period == 5) i2= impulse2(p, &col2, &ref2, &trans2, &absorb2);
      else if (period == 6) i2= impulse3(p, &col2, &ref2, &trans2, &absorb2);
      else if (period == 7) i2= impulse2(p, &col2, &ref2, &trans2, &absorb2);
      else if (period == 8) i2= impulse7(p, &col2, &ref2, &trans2, &absorb2, iTime);
    //i=i2*(rr-0.8f)/0.2f +i*(1-(rr-0.8f)/0.2f);
      float inv=1.0f-rat;
      i       = i*rat+       i2*inv;
      *col    = *col*rat+    col2*inv;
      *absorb = *absorb*rat+ absorb2*inv;
      *ref    = *ref*rat+    ref2*inv;
      *trans  = *trans*rat+  trans2*inv;
  }
  float rc = sdRoundCube(p - to_float3(0.0f, -5.05f, 0.0f), 4.0f);

  float d = _fminf(rc, i);
  if (d == rc)
  {
    *col    = to_float3_s(1.0f);
    *ref    = 0.2f;
    *trans  = 0.0f;
  }
  return d;
}

__DEVICE__ float3 _saturatef3(in float3 a)   { return clamp(a, 0.0f, 1.0f); }
__DEVICE__ float2 _saturatef2(in float2 a)   { return clamp(a, 0.0f, 1.0f); }
__DEVICE__ float _saturatef(in float a) { return clamp(a, 0.0f, 1.0f); }


__DEVICE__ float3 getSkyColor(float3 rayDir)
{

  const float3 lightPos1 = 100.0f*to_float3(-0.3f, 0.0f, 1.0f);
  const float3 lightPos2 = 100.0f*to_float3(-0.5f, -0.1f, -1.2f);

  const float3 lightCol1 = to_float3(8.0f/8.0f,7.0f/8.0f,6.0f/8.0f);
  const float3 lightCol2 = to_float3(8.0f/8.0f,6.0f/8.0f,7.0f/8.0f);
  
  float3 lightDir1 = normalize(lightPos1);
  float3 lightDir2 = normalize(lightPos2);

  float ld1      = _fmaxf(dot(lightDir1, rayDir), 0.0f);
  float ld2      = _fmaxf(dot(lightDir2, rayDir), 0.0f);
  float3 final     = to_float3_s(0.125f);

  if ((rayDir.y > _fabs(rayDir.x)*1.0f) && (rayDir.y > _fabs(rayDir.z*0.25f))) final = to_float3_s(2.0f)*rayDir.y;
  float roundBox = length(_fmaxf(abs_f2(swi2(rayDir,x,z)/_fmaxf(0.0f,rayDir.y))-to_float2(0.9f, 4.0f),to_float2_s(0.0f)))-0.1f;
  final += to_float3_s(0.8f)* _powf(saturate(1.0f - roundBox*0.5f), 6.0f);

  final += pow_f3(lightCol1, to_float3(2.0f, 1.5f, 1.5f)) * _powf(ld1, 8.0f);
  final += lightCol1 * _powf(ld1, 200.0f);
  final += pow_f3(lightCol2, to_float3(2.0f, 1.5f, 1.5f)) * _powf(ld2, 8.0f);
  final += lightCol2 * _powf(ld2, 200.0f);
  return final;
}

__DEVICE__ float3 normal(in float3 pos, float iTime)
{
  float3  eps = to_float3(0.0001f,0.0f,0.0f);
  float3 col;
  float ref;
  float trans;
  float3 nor;
  float3 absorb;
  nor.x = distanceField(pos+swi3(eps,x,y,y), &col, &ref, &trans, &absorb, iTime) - distanceField(pos-swi3(eps,x,y,y), &col, &ref, &trans, &absorb, iTime);
  nor.y = distanceField(pos+swi3(eps,y,x,y), &col, &ref, &trans, &absorb, iTime) - distanceField(pos-swi3(eps,y,x,y), &col, &ref, &trans, &absorb, iTime);
  nor.z = distanceField(pos+swi3(eps,y,y,x), &col, &ref, &trans, &absorb, iTime) - distanceField(pos-swi3(eps,y,y,x), &col, &ref, &trans, &absorb, iTime);
  return normalize(nor);
}

__DEVICE__ float rayMarch(in float dmod, in float3 ro, inout float3 *rd, float mint, float minstep, out int *rep, out float3 *col, out float *ref, out float *trans, out float3 *absorb, float iTime)
{
  float t = mint;
  for (int i = 0; i < MAX_RAY_MARCHES; i++)
  {
    float distance_ = distanceField(ro + *rd*t, col, ref, trans, absorb, iTime);
    float distance = dmod*distance_;
    if (distance < TOLERANCE*t || t > MAX_RAY_LENGTH) break;
    t += _fmaxf(distance, minstep);
    *rep = i;
  }
  return t;
}

__DEVICE__ float softShadow(in float3 pos, in float3 ld, in float ll, float mint, float k, float iTime)
{
  const float minShadow = 0.25f;
  float res = 1.0f;
  float t = mint;
  float3 col;
  float ref;
  float trans;
  float3 absorb;
  for (int i=0; i<24; i++)
  {
    float distance = distanceField(pos + ld*t, &col, &ref, &trans, &absorb, iTime);
    res = _fminf(res, k*distance/t);
    if (ll <= t) break;
    if(res <= minShadow) break;
    t += _fmaxf(mint*0.2f, distance);
  }
  return clamp(res,minShadow,1.0f);
}

__DEVICE__ float3 postProcess(in float3 col, in float2 q)
{
  col = pow_f3(clamp(col,0.0f,1.0f),to_float3_s(0.75f));
  col = col*0.6f+0.4f*col*col*(3.0f-2.0f*col);  // contrast
  col = _mix(col, to_float3_s(dot(col, to_float3_s(0.33f))), -0.4f);  // satuation
  col *= 0.5f+0.5f*_powf(19.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y),0.7f);  // vigneting
  return col;
}

__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}



__DEVICE__ float3 render(in float3 ro, in float3 rd, float iTime, float refmul, float refoff)
{
  float3 lightPos = 1.5f*to_float3(1.5f, 3.0f, 1.0f);

  float3 final  = to_float3_s(0.0f);

  float3 ragg   = to_float3_s(1.0f);
float zzzzzzzzzzzzzzzzz;
  float tdist = 0.0f;

  int period = (int)(PERIOD);

  float refraction = 0.9f;

  if (period == 4) refraction = 1.2f;
  if (period == 5) refraction = 0.75f;
  if (period == 6) refraction = -0.8f;

  bool inside = false;

  float mint    = 0.01f;
  float minstep = 0.001f;

  for (int i = 0; i < MAX_BOUNCES; ++i)
  {
    if (maxComp(ragg) <  0.01f) break;
    float dmod  = inside ? -1.0f : 1.0f;
    float3 absorb ;
    float3 col    ;
    float ref   ;
    float trans ;
    int rep     ;
    float t     = rayMarch(dmod, ro, &rd, mint, minstep, &rep, &col, &ref, &trans, &absorb, iTime);
    tdist       += t;

    float3 pos    = ro + t*rd;

    float3 nor = to_float3(0.0f, 1.0f, 0.0f);

    if (t < MAX_RAY_LENGTH)
    {
      // Ray intersected object
      nor = normal(pos, iTime);
    }
    else
    {
      // Ray intersected sky
      final += ragg*getSkyColor(rd);
      break;
    }

    float fresnel = _powf(1.0f - _fabs(dot(nor, rd)), 2.0f);

    ref = _mix(ref, 1.0f, fresnel);
    trans = _mix(trans, 0.0f, fresnel);

    float mref = refraction;

    if (inside)
    {
      nor = -nor;
      mref = 1.0f/refraction;
    }

    float3 refl = reflect(rd, nor);
    float3 refr = _refract_f3(rd, nor, mref,refmul,refoff);

    float3 lv   = lightPos - pos;
    float3  ld  = normalize(lv);
    float ll  = length(lv);
    // TODO: Rework shadow to "work" with transparent objects
    float sha = 1.0f;
    if (!inside)
    {
      sha = softShadow(pos, ld, ll, 0.01f, 64.0f, iTime);
    }

    float dif = _fmaxf(dot(nor,ld),0.0f);
    float occ = 1.0f - (float)(rep)/(float)(MAX_RAY_MARCHES);
    float l   = dif*sha*occ;


    float3 lr   = to_float3_s(0.0f);

    float lin = _mix(0.2f, 1.0f, l);

    float3 sky  = getSkyColor(refl);
    float3 mcol = _mix(lin*col + lr, sky, ref);

    float3 beer = to_float3_s(1.0f);

    if (inside)
    {
      beer = exp_f3(-absorb*t);
    }
    final      += (1.0f - trans)*ragg*beer*mcol;
    ragg       *= trans*beer;

    ro        = pos;

    //if (refr == to_float3_s(0.0f))
    if (refr.x == 0.0f && refr.y == 0.0f && refr.z == 0.0f)
    {
      rd = refl;
    }
    else
    {
      rd = refr;
      inside = !inside;
    }

    /* TODO: Fix visual artifacts on borders
    if (fresnel >)
    {
      //final += to_float3_s(1.0f);
      mint = 0.1f;
      minstep = 0.01f;
    }
    else
    {
      minstep = 0.001f;
    }
    */
  }


  return final;
}

__DEVICE__ float3 getSample(in float2 p, float iTime, float refmul, float refoff, float3 view)
{
  float time   = TIMEINPERIOD;
  int period   = (int)(PERIOD);

  float3 ro  = to_float3(3.0f, 0.1f, 0.0f) + view;
  if (period == 5) ro = 1.5f*to_float3(1.0f, 1.5f, 0.0f);

  float3 la  = to_float3_s(0.0f);

  swi2S(ro,x,z, pR(swi2(ro,x,z), ITIME/PERIODTIME));

  float3 ww = normalize(la - ro);
  float3 uu = normalize(cross(to_float3(0.0f,1.0f,0.0f), ww ));
  float3 vv = normalize(cross(ww,uu));
  float3 rd = normalize( p.x*uu + p.y*vv + 2.0f*ww );

  float3 col = render(ro, rd, iTime, refmul, refoff);

  return col;
}

__KERNEL__ void ImpulseGlassMorphJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    CONNECT_SLIDER1(refmul, -1.0f, 10.0f, 0.0f); 
    CONNECT_SLIDER2(refoff, -1.0f, 10.0f, 0.0f);
    
    CONNECT_POINT0(ViewXY, 0.0f, 0.0f );
    CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);

  float3 view = to_float3(ViewZ, ViewXY.y, ViewXY.x);

  float time = TIMEINPERIOD;
  float2 q = fragCoord/iResolution;
  float2 p = -1.0f + 2.0f*q;
  p.x *= iResolution.x/iResolution.y;

#if AA == 0
  float3 col = getSample(p, iTime, refmul, refoff, view);
#elif AA == 1
  float3 col  = to_float3_s(0.0f);
  float2 unit = 1.0f/iResolution;
  for(int y = 0; y < 2; ++y)
  {
    for(int x = 0; x < 2; ++x)
    {
      col += getSample(p - 0.5f*unit + unit*to_float2(x, y), iTime, refmul, refoff, view);
    }
  }

  col /= 4.0f;
#endif

  fragColor = to_float4_aw(postProcess(col, q), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}