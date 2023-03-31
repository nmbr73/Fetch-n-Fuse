
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// CC0: Wednesday messing around
// Tinkered a bit with an earlier shader
// Thought while similar it turned out distinct enough to share

#define COLORBURN
#define SKYDOME
#define PERIOD        10.0f

#define PI            3.141592654f
#define ROT(a)        to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a))

#define TAU         (2.0f*PI)
#define TIME        iTime
#define RESOLUTION  iResolution

#define MAX_RAY_LENGTH  15.0f
#define MAX_RAY_MARCHES 70


// License: WTFPL, author: sam hocevar, found: https://stackoverflow.com/a/17897228/418488
__DEVICE__ float3 hsv2rgb(float3 c) {
  const float4 hsv2rgb_K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);

  float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(hsv2rgb_K,x,y,z)) * 6.0f - swi3(hsv2rgb_K,w,w,w));
  return c.z * _mix(swi3(hsv2rgb_K,x,x,x), clamp(p - swi3(hsv2rgb_K,x,x,x), 0.0f, 1.0f), c.y);
}

__DEVICE__ float3 band_color(float ny) {
  float3 hsv = to_float3_s(0.0f);
  float ramp = 1.0f/_fabs(ny);
  if (_fabs(ny) < 4.0f) {
    hsv = to_float3(0.0f, 0.0f, 0.0f);
  } else if (ny > 0.0f) {
    hsv = to_float3(0.88f, 2.5f*ramp,0.8f);
  } else {
    hsv = to_float3(0.53f, 4.0f*ramp, 0.8f);
  }

  return hsv2rgb(hsv);
}

// License: MIT, author: Inigo Quilez, found: https://iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
__DEVICE__ float box(float2 p, float2 b, float4 r) {
  swi2S(r,x,y, (p.x>0.0f)?swi2(r,x,y) : swi2(r,z,w));
  r.x  = (p.y>0.0f)?r.x  : r.y;
  float2 q = abs_f2(p)-b+r.x;
  return _fminf(_fmaxf(q.x,q.y),0.0f) + length(_fmaxf(q,to_float2_s(0.0f))) - r.x;
}

__DEVICE__ float fadeIn(float x, float iTime) {
  return _mix(-0.1f, 1.0f, smoothstep(-0.9f, -0.5f, -_cosf(-0.1f*x+TAU*TIME/PERIOD)));
}

__DEVICE__ float df_bars1(float3 p, float iTime, inout int *g_hit, float twist, float dist, int bars, float rounding) {
  p.y += dist*_sinf(0.5f*p.x+0.5f*p.z+TIME);
  float2 bp = swi2(p,z,y);
  
  float d = 1E6;

  float bs = 0.25f*fadeIn(p.x,iTime);
  float2 bsz = to_float2_s(bs);
  float4 brd = to_float4_s(bs*rounding);

  for (int i = 0; i < bars; ++i) {
    float ii = (float)(i);
    float2 pp = bp;
    float a = -TIME+0.5f*ii;
    float b = ii+p.x-2.0f*TIME;
    pp.y += _sinf(a);
    mat2 rot = ROT(-PI/4.0f*_cosf(a+twist*b));
    pp.x -= bsz.x*_sqrtf(2.0f)*ii; 
    pp = mul_f2_mat2(pp,rot);
    float dd = box(pp, bsz, brd);
    if (dd < d) {
      *g_hit = i;
      d = dd;
    }
  }
  
  return d; 
}

__DEVICE__ float df_bars2(float3 p, float iTime, inout int *g_hit, float rounding, float dist) {
  p.y += 0.5f*dist*_sinf(-0.9f*p.x+TIME);
  float2 p2 = swi2(p,y,z);
  p2 = mul_f2_mat2(p2,ROT(TIME+p.x));  
  float2 s2 = sign_f2(p2);
  p2 = abs_f2(p2);
  p2 -= 0.3f;
  *g_hit = 3+(int)(s2.y+2.0f*s2.x)-1;
  float bs = 0.25f*fadeIn(p.x,iTime);
  float2 bsz = to_float2_s(bs);
  float4 brd = to_float4_s(bs*rounding);
  return length(p2)-bs;
}

__DEVICE__ float df_bars3(float3 p, float iTime, inout int *g_hit, float rounding, float dist) {
  const float r = 0.25f;
  p.y += 0.5f*dist*_sinf(-0.9f*p.x+TIME);
  mat2 rot = ROT(TIME+p.x);
  float2 p2 = swi2(p,y,z);
  float2 s2 = to_float2_s(0.0f);

  p2 = mul_f2_mat2(p2,rot);
  s2 += 2.0f*sign_f2(p2);
  p2 = abs_f2(p2);
  p2 -= 2.0f*r;

  p2 = mul_f2_mat2(p2,rot);
  s2 += 1.0f*sign_f2(p2);
  p2 = abs_f2(p2);
  p2 -= 1.0f*r;

  *g_hit = 3+(int)(s2.y+2.0f*s2.x)-1;

  float bs = (0.9f*r)*fadeIn(p.x,iTime);
  float2 bsz = to_float2_s(bs);
  float4 brd = to_float4_s(bs*rounding);
  float d0 = length(p2)-bs;
  float d1 = box(p2, bsz, brd);
  float d = d0;
  return d;
}

__DEVICE__ float df_bars4(float3 p, float iTime, inout int *g_hit, float rounding, float dist) {
  p.y += 0.5f*dist*_sinf(-0.9f*p.x+TIME);
  float2 p2 = swi2(p,y,z);
  p2 = mul_f2_mat2(p2,ROT(TIME+p.x));  
  float2 s2 = sign_f2(p2);
  p2 = abs_f2(p2);
  p2 -= 0.3f;
  *g_hit = 3+(int)(s2.y+2.0f*s2.x)-1;

  float bs = 0.25f*fadeIn(p.x,iTime);

  float2 bsz = to_float2_s(bs);
  float4 brd = to_float4_s(bs*rounding);
  return box(p2, bsz, brd);
}

__DEVICE__ float df(float3 p, float iTime, inout int *g_hit, float twist, int bars, float rounding, float dist, mat2 trans, int g_period) {
  swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , trans));
  switch(g_period) {
  case 0:
    return df_bars1(p,iTime,g_hit,twist,dist,bars,rounding);
  case 1:
    return df_bars2(p,iTime,g_hit,rounding,dist);
  case 2:
    return df_bars3(p,iTime,g_hit,rounding,dist);
  case 3:
    return df_bars4(p,iTime,g_hit,rounding,dist);
  default:
    return length(p) - 0.5f;
  }
}

__DEVICE__ float rayMarch(float3 ro, float3 rd, float ti, float iTime, inout int *g_hit, float twist, int bars, float rounding, float dist, mat2 trans, int g_period, float TOLERANCE, float raymarchFactor) {
  float t = ti;
  int i = 0;
  float2 dti = to_float2(1e10,0.0f);
  for (i = 0; i < MAX_RAY_MARCHES; i++) {
    float d = df(ro + rd*t,iTime,g_hit,twist,bars,rounding,dist,trans,g_period);
    if (d < TOLERANCE || t > MAX_RAY_LENGTH) break;
    if (d<dti.x) { dti=to_float2(d,t); }
    t += raymarchFactor*d;
  }
  if(i==MAX_RAY_MARCHES) { t=dti.y; }
  return t;
}

__DEVICE__ float3 normal(float3 pos, float iTime, inout int *g_hit, float twist, int bars, float rounding, float dist, mat2 trans, int g_period, float NORM_OFF) {
  float2  eps = to_float2(NORM_OFF,0.0f);
  float3 nor;
  nor.x = df(pos+swi3(eps,x,y,y),iTime,g_hit,twist,bars,rounding,dist,trans,g_period) - df(pos-swi3(eps,x,y,y),iTime,g_hit,twist,bars,rounding,dist,trans,g_period);
  nor.y = df(pos+swi3(eps,y,x,y),iTime,g_hit,twist,bars,rounding,dist,trans,g_period) - df(pos-swi3(eps,y,x,y),iTime,g_hit,twist,bars,rounding,dist,trans,g_period);
  nor.z = df(pos+swi3(eps,y,y,x),iTime,g_hit,twist,bars,rounding,dist,trans,g_period) - df(pos-swi3(eps,y,y,x),iTime,g_hit,twist,bars,rounding,dist,trans,g_period);
  return normalize(nor);
}



__DEVICE__ float3 skyColor(float3 ro, float3 rd, float3 lightPos, float3 lightCol, float3 overCol) {
  float3  ld    = normalize(lightPos - ro);
  float dif   = _fmaxf(dot(ld, rd), 0.0f);

  float3  col   = to_float3_s(0.0f);

  if ((rd.y > _fabs(rd.x)*1.0f) && (rd.y > _fabs(rd.z*0.25f))) { 
    col = 2.0f*overCol*rd.y;
  }
  float rb = length(_fmaxf(abs_f2(swi2(rd,x,z)/_fmaxf(0.0f,rd.y))-to_float2(0.9f, 4.0f),to_float2_s(0.0f)))-0.1f;

  col += overCol*_powf(clamp(1.0f - rb*0.5f, 0.0f, 1.0f), 6.0f);
  col += lightCol*_powf(dif, 8.0f);
  col += 4.0f*lightCol*_powf(dif, 40.0f);
  return col;
}

__DEVICE__ float3 effect(float2 p, float iTime, inout int *g_hit, float twist, int bars, float rounding, float dist, mat2 trans, int g_period, float TOLERANCE, float raymarchFactor, float NORM_OFF,
                         float3 lightPos, float3 lightCol, float3 overCol, inout float *Hit) {
  float3 ro = to_float3(0.0f, 0.0f, -5.0f);
  float3 la = to_float3(0.0f, 0.0f, 0.0f);
  float3 ww = normalize(la-ro);
  float3 uu = normalize(cross(to_float3(0.0f,1.0f,0.0f), ww ));
  float3 vv = normalize(cross(ww,uu));
  const float fov = 3.0f;
  float3 rd = normalize(-p.x*uu + p.y*vv + fov*ww );

  *g_hit = -1;
  float t = rayMarch(ro, rd, 3.0f, iTime, g_hit, twist, bars, rounding, dist, trans, g_period, TOLERANCE, raymarchFactor);
  int hit = *g_hit;
  *Hit = t;

  float3 col = to_float3_s(1.0f);
  float3 bcol = band_color(-4.0f*(float)(hit-(bars-1)/2));
  bcol *= bcol;
  if (t < MAX_RAY_LENGTH) {
    float3 p = ro + rd*t;
    float3 n = normal(p, iTime, g_hit, twist, bars, rounding, dist, trans, g_period, NORM_OFF);
    float3 r = reflect(rd, n);
    float3 ld= normalize(lightPos-p);
  
    float dif = _fmaxf(dot(ld, n), 0.0f);
    col = bcol*_mix(0.5f, 1.0f, dif);
#ifdef SKYDOME    
    float3 rs= skyColor(p, r, lightPos, lightCol, overCol);
    float fre = 1.0f+dot(rd, n);
    fre *= fre;
    float rf  = _mix(0.05f, 1.0f, fre);
    col += rf*rs;
    // Just some fine-tuning, don't judge me
    col += smoothstep(0.5f, 1.0f, fre)*_fmaxf(n.y, 0.0f);
#else   
    float spe = _powf(_fmaxf(dot(ld, r), 0.0f), 30.0f);
    col += spe;
#endif    
  }
  return col;
}


__KERNEL__ void WednesdayMessingAroundFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  CONNECT_COLOR0(LightPosition, 2.0f, 3.0f, -5.0f, 1.0f);
  CONNECT_COLOR1(LightColor, 0.53f, 0.5f, 1.0f, 1.0f);
  CONNECT_COLOR2(OverColor, 0.88f, 0.25f, 0.8f, 1.0f);
  CONNECT_COLOR3(ColorBurn, 0.2f, 0.3f, 0.2f, 1.0f);
  CONNECT_COLOR4(Background, 0.9f, 0.84f, 0.9f, 1.0f);

  CONNECT_INTSLIDER0(bars, 1, 12, 7);

  CONNECT_SLIDER0(twist, -1.0f, 10.0f, 1.0f);
  CONNECT_SLIDER1(dist, -1.0f, 2.0f, 0.5f);
  CONNECT_SLIDER2(rounding, -1.0f, 1.0f, 0.125f);
  //CONNECT_SLIDER3(raymarchFactor, -1.0f, 2.0f, 0.8f);
    
  //CONNECT_SLIDER4(TOLERANCE, -1.0f, 1.0f, 0.001f);
  //CONNECT_SLIDER5(NORM_OFF, -1.0f, 1.0f, 0.005f);
  
  //CONNECT_SLIDER6(Hit_Thres, -10.0f, 10.0f, 0.0f);
  
  CONNECT_CHECKBOX0(AutoPeriod, 1);
  CONNECT_INTSLIDER1(Period, 0, 4, 1);
  
  CONNECT_CHECKBOX1(ChangeBackground, 0);
  
  
  //const int   bars     = 7;
  const mat2  trans    = ROT(PI/9.0f);
  //const float twist    = 1.0f;
  //const float dist     = 0.5f;
  //const float rounding = 0.125f;

  const float raymarchFactor = 0.8f;

  const float4 hsv2rgb_K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);

  //#define TOLERANCE       0.001f
  float TOLERANCE = 0.001f;
  //#define NORM_OFF        0.005f
  float NORM_OFF = 0.005f;

  int g_hit     = 0;
  int g_period  = 0;
  float Hit     = 0;
  
  const float3 lightPos = swi3(LightPosition,x,y,z);//to_float3(2.0f, 3.0f, -5.0f); 
  const float3 lightCol = swi3(LightColor,x,y,z);//(hsv2rgb(to_float3(0.53f, 0.5f, 1.0f)));
  const float3 overCol  = swi3(OverColor,x,y,z);//(hsv2rgb(to_float3(0.88f, 0.25f, 0.8f)));

  float2 q = fragCoord/swi2(RESOLUTION,x,y);
  float2 p  = -1.0f + 2.0f * q;
  p.x     *= RESOLUTION.x/RESOLUTION.y;
  g_period = (int)(mod_f(1.0f+_floor(TIME/PERIOD), 4.0f));

  if(AutoPeriod == false)
    g_period = Period;

  float3 col  = effect(p,iTime, &g_hit,twist,bars,rounding,dist,trans,g_period,TOLERANCE,raymarchFactor,NORM_OFF,lightPos,lightCol,overCol, &Hit);
#if defined(COLORBURN)  
  col -= swi3(ColorBurn,x,y,z);//to_float3(0.2f, 0.3f, 0.2f);
#endif  
  col = clamp(col, 0.0f, 1.0f);
  col = sqrt_f3(col);
  
  fragColor = to_float4_aw(col, Background.w);

  //if(ChangeBackground) fragColor = (col.x == col.z && col.x != 0.0f) ? Background : to_float4_aw(col,1.0f); 
  if(ChangeBackground) fragColor = (Hit < 10.0f) ? to_float4_aw(col,1.0f) : Background; 

  SetFragmentShaderComputedColor(fragColor);
}