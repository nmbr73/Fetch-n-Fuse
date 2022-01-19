// ACHTUNG - DEN HIER BENUTZE ICH, UM DIE FUSE-GENERIERUNG ZU TESTEN
// BITTE NICHT AN DIESER DATEI HIER ARBEITEN - UND AUCH NICHT DAS
// FRAGMENT IN DEM ANDEREN REPO IN EINE FUSE VERPFLANZEN ... DAS SOLL
// EBEN HALBWEGS AUSTOMATISIERT GESCHEHEN - HEISST, DAS ES ZU KONFLIKTEN
// UND UEBERSCHREIBUNGEN MIT MEINEN EXPERIMENTEN FUEREN WUERDE!!!

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// License CC0: Apollian with a twist
// Playing around with apollian fractal

#define L2(x)           dot(x, x)
#define ROT(a)          to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a))
#define PSIN(x)         (0.5f+0.5f*_sinf(x))

// mat2 ... hack, otherwise it's not seen

__DEVICE__ float3 hsv2rgb(float3 c) {
  const float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
  float3 p = _fabs(fract(swi3(c,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
  return c.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), c.y);
}

__DEVICE__ float apollian(float4 p, float s, float scale) {
  //float scale = 1.0f;

  for(int i=0; i<7; ++i) {
    p        = -1.0f + 2.0f*fract(0.5f*p+0.5f);

    float r2 = dot(p,p);

    float k  = s/r2;
    p       *= k;
    scale   *= k;
  }

  return _fabs(p.y)/scale;
}

__DEVICE__ float weird(float2 p, float iTime, float scale) {
  float z = 4.0f;
  p *= ROT(iTime*0.1f);
  float tm = 0.2f*iTime;
  float r = 0.5f;
  float4 off = to_float4(r*PSIN(tm*_sqrtf(3.0f)), r*PSIN(tm*_sqrtf(1.5f)), r*PSIN(tm*_sqrtf(2.0f)), 0.0f);
  float4 pp = to_float4(p.x, p.y, 0.0f, 0.0f)+off;
  pp.w = 0.125f*(1.0f-_tanhf(length(swi3(pp,x,y,z))));

  float2 tmp;
  tmp = mul_f2_mat2(swi2(pp,y,z), ROT(tm));               pp.y=tmp.x; pp.z=tmp.y; // swi2(pp,y,z) *= ROT(tm);
  tmp = mul_f2_mat2(swi2(pp,x,z), ROT(tm*_sqrtf(0.5f)));  pp.x=tmp.x; pp.z=tmp.y; // swi2(pp,x,z) *= ROT(tm*_sqrtf(0.5f));

  pp /= z;
  float d = apollian(pp, 1.2f, scale);
  return d*z;
}

__DEVICE__ float df(float2 p, float iTime, float scale) {
  const float zoom = 0.5f;
  p /= zoom;
  float d0 = weird(p,iTime,scale);
  return d0*zoom;
}

__DEVICE__ float3 color(float2 p, float iTime, float iResolution_y, float scale) {
  float aa   = 2.0f/iResolution_y;
  const float lw = 0.0235f;
  const float lh = 1.25f;

  const float3 lp1 = to_float3(0.5f, lh, 0.5f);
  const float3 lp2 = to_float3(-0.5f, lh, 0.5f);

  float d = df(p,iTime,scale);

  float b = -0.125f;
  float t = 10.0f;

  float3 ro = to_float3(0.0f, t, 0.0f);
  float3 pp = to_float3(p.x, 0.0f, p.y);

  float3 rd = normalize(pp - ro);

//float3 ld1 = normalize(lp1 - pp);
//float3 ld2 = normalize(lp2 - pp);

  float bt = -(t-b)/rd.y;

  float3 bp   = ro + bt*rd;
  float3 srd1 = normalize(lp1-bp);
  float3 srd2 = normalize(lp2-bp);
  float bl21= L2(lp1-bp);
  float bl22= L2(lp2-bp);

  float st1= (0.0f-b)/srd1.y;
//float st2= (0.0f-b)/srd2.y;
  float3 sp1 = bp + srd1*st1;
  float3 sp2 = bp + srd2*st1;

//float bd = df(swi2(bp,x,z),iTime,scale);
  float sd1= df(swi2(sp1,x,z),iTime,scale);
  float sd2= df(swi2(sp2,x,z),iTime,scale);

  float3 col  = to_float3_s(0.0f);
  const float ss =15.0f;

  col       += to_float3(1.0f, 1.0f, 1.0f)*(1.0f-_expf(-ss*(_fmaxf((sd1+0.0f*lw), 0.0f))))/bl21;
  col       += to_float3_s(0.5f)*(1.0f-_expf(-ss*(_fmaxf((sd2+0.0f*lw), 0.0f))))/bl22;
  float l   = length(p);
  float hue = fract(0.75f*l-0.3f*iTime)+0.3f+0.15f;
  float sat = 0.75f*_tanhf(2.0f*l);
  float3 hsv  = to_float3(hue, sat, 1.0f);
  float3 bcol = hsv2rgb(hsv);
  col       *= (1.0f-_tanhf(0.75f*l))*0.5f;
  col       = _mix(col, bcol, smoothstep(-aa, aa, -d));
  col       += 0.5f*_sqrtf(swi3(bcol,z,x,y))*(_expf(-(10.0f+100.0f*_tanhf(l))*_fmaxf(d, 0.0f)));

  return col;
}


__KERNEL__ void ApollianWithATwistFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
  CONNECT_SLIDER0(Alpha,0.0f,1.0f,1.0f);
  CONNECT_SLIDER1(Scale,0.0f,10.0f,1.0f);
  CONNECT_SLIDER2(Contrast,0.0f,1.0f,0.6f);
  CONNECT_SLIDER3(Saturation,0.0f,1.0f,0.33f);
  CONNECT_SLIDER4(Vigneting,0.0f,1.0f,0.7f);

  float2 q = fragCoord/swi2(iResolution,x,y);
  float2 p = -1.0f + 2.0f * q;
  p.x *= iResolution.x/iResolution.y;

  float3 col = color(p,iTime,iResolution.y,Scale);

  #if 0
    col=_powf(clamp(col,0.0f,1.0f),to_float3_s(1.0f/2.2f));
    col=col*0.6f+0.4f*col*col*(3.0f-2.0f*col);  // contrast
    col=_mix(col, to_float3_s( dot(col, to_float3_s(0.33f)) ), -0.4f);  // saturation
    col*=0.5f+0.5f*_powf(19.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y),0.7f);  // vigneting
    fragColor = to_float4_aw(col, 1.0f);
  #else
    col=_powf(clamp(col,0.0f,1.0f),to_float3_s(1.0f/2.2f));
    col=col*Contrast+0.4f*col*col*(3.0f-2.0f*col);  // contrast
    col=_mix(col, to_float3_s( dot(col, to_float3_s(Saturation)) ), -0.4f);  // saturation
    col*=0.5f+0.5f*_powf(19.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y),Vigneting);  // vigneting
    fragColor = to_float4_aw(col, Alpha);
  #endif



  SetFragmentShaderComputedColor(fragColor);
}