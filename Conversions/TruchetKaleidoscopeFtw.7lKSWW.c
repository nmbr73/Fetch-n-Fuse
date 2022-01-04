
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// CC0: Truchet + Kaleidoscope FTW
//  Bit of experimenting with kaleidoscopes and truchet turned out nice
//  Quite similar to an earlier shader I did but I utilized a different truchet pattern this time
#define PI              3.141592654
#define TAU             (2.0f*PI)
#define RESOLUTION      iResolution
#define TIME            iTime
#define ROT(a)          to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a))
#define PCOS(x)         (0.5f+0.5f*_cosf(x))

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float4 alphaBlend(float4 back, float4 front) {
  float w = front.w + back.w*(1.0f-front.w);
  float3 xyz = (swi3(front,x,y,z)*front.w + swi3(back,x,y,z)*back.w*(1.0f-front.w))/w;
  return w > 0.0f ? to_float4_aw(xyz, w) : to_float4_s(0.0f);
}

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float3 alphaBlend(float3 back, float4 front) {
  return _mix(back, swi3(front,x,y,z), front.w);
}

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float hash(float co) {
  return fract(_sinf(co*12.9898f) * 13758.5453f);
}

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float hash(float2 p) {
  float a = dot(p, to_float2 (127.1f, 311.7f));
  return fract(_sinf (a)*43758.5453123f);
}

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float tanh_approx(float x) {
  //  Found this somewhere on the interwebs
  //  return _tanhf(x);
  float x2 = x*x;
  return clamp(x*(27.0f + x2)/(27.0f+9.0f*x2), -1.0f, 1.0f);
}

// License: MIT, author: Inigo Quilez, found: https://www.iquilezles.org/www/articles/smin/smin.htm
__DEVICE__ float pmin(float a, float b, float k) {
  float h = clamp(0.5f+0.5f*(b-a)/k, 0.0f, 1.0f);
  return _mix(b, a, h) - k*h*(1.0f-h);
}

// License: MIT, author: Inigo Quilez, found: https://www.iquilezles.org/www/index.htm
__DEVICE__ float3 postProcess(float3 col, float2 q) {
  col = clamp(col, 0.0f, 1.0f);
  col = pow_f3(col, to_float3_s(1.0f/2.2f));
  col = col*0.6f+0.4f*col*col*(3.0f-2.0f*col);
  col = _mix(col, to_float3_s(dot(col, to_float3_s(0.33f))), -0.4f);
  col *=0.5f+0.5f*_powf(19.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y),0.7f);
  return col;
}

__DEVICE__ float pmax(float a, float b, float k) {
  return -pmin(-a, -b, k);
}

__DEVICE__ float pabs(float a, float k) {
  return pmax(a, -a, k);
}

__DEVICE__ float2 toPolar(float2 p) {
  return to_float2(length(p), _atan2f(p.y, p.x));
}

__DEVICE__ float2 toRect(float2 p) {
  return to_float2(p.x*_cosf(p.y), p.x*_sinf(p.y));
}

// License: MIT OR CC-BY-NC-4.0f, author: mercury, found: https://mercury.sexy/hg_sdf/
__DEVICE__ float modMirror1(inout float *p, float size) {
  float halfsize = size*0.5f;
  float c = _floor((*p + halfsize)/size);
  *p = mod_f(*p + halfsize,size) - halfsize;
  *p *= mod_f(c, 2.0f)*2.0f - 1.0f;
  return c;
}

__DEVICE__ float smoothKaleidoscope(inout float2 *p, float sm, float rep) {
  float2 hp = *p;

  float2 hpp = toPolar(hp);
  float rn = modMirror1(&hpp.y, TAU/rep);
float zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz;
  float sa = PI/rep - pabs(PI/rep - _fabs(hpp.y), sm);
  hpp.y = sign_f(hpp.y)*(sa);

  hp = toRect(hpp);

  *p = hp;

  return rn;
}

// The path function
__DEVICE__ float3 offset(float z) {
  float a = z;
  float2 p = -0.075f*(to_float2(_cosf(a), _sinf(a*_sqrtf(2.0f))) + to_float2(_cosf(a*_sqrtf(0.75f)), _sinf(a*_sqrtf(0.5f))));
  return to_float3_aw(p, z);
}

// The derivate of the path function
//  Used to generate where we are looking
__DEVICE__ float3 doffset(float z) {
  float eps = 0.1f;
  return 0.5f*(offset(z + eps) - offset(z - eps))/eps;
}

// The second derivate of the path function
//  Used to generate tilt
__DEVICE__ float3 ddoffset(float z) {
  float eps = 0.1f;
  return 0.125f*(doffset(z + eps) - doffset(z - eps))/eps;
}

__DEVICE__ float2 cell_df(float r, float2 np, float2 mp, float2 off) {
  const float2 n0 = normalize(to_float2(1.0f, 1.0f));
  const float2 n1 = normalize(to_float2(1.0f, -1.0f));

  np += off;
  mp -= off;
  
  float hh = hash(np);
  float h0 = hh;

  float2  p0 = mp;  
  p0 = abs_f2(p0);
  p0 -= 0.5f;
  float d0 = length(p0);
  float d1 = _fabs(d0-r); 

  float dot0 = dot(n0, mp);
  float dot1 = dot(n1, mp);

  float d2 = _fabs(dot0);
  float t2 = dot1;
  d2 = _fabs(t2) > _sqrtf(0.5f) ? d0 : d2;

  float d3 = _fabs(dot1);
  float t3 = dot0;
  d3 = _fabs(t3) > _sqrtf(0.5f) ? d0 : d3;


  float d = d0;
  d = _fminf(d, d1);
  if (h0 > 0.85f)
  {
    d = _fminf(d, d2);
    d = _fminf(d, d3);
  }
  else if(h0 > 0.5f)
  {
    d = _fminf(d, d2);
  }
  else if(h0 > 0.15f)
  {
    d = _fminf(d, d3);
  }
  
  return to_float2(d, d0-r);
}

__DEVICE__ float2 truchet_df(float r, float2 p) {
  float2 np = _floor(p+0.5f);
  float2 mp = fract_f2(p+0.5f) - 0.5f;
  return cell_df(r, np, mp, to_float2_s(0.0f));
}

__DEVICE__ float4 plane(float3 ro, float3 rd, float3 pp, float3 off, float aa, float n,float iTime) {
  float h_ = hash(n);
  float h0 = fract_f(1777.0f*h_);
  float h1 = fract_f(2087.0f*h_);
  float h2 = fract_f(2687.0f*h_);
  float h3 = fract_f(3167.0f*h_);
  float h4 = fract_f(3499.0f*h_);

  float l = length(pp - ro);

  float3 hn;
  float2 p = swi2((pp-off*to_float3(1.0f, 1.0f, 0.0f)),x,y);
  p = mul_f2_mat2(p,ROT(0.5f*(h4 - 0.5f)*TIME));
  float rep = 2.0f*_round(_mix(5.0f, 30.0f, h2));
  float sm = 0.05f*20.0f/rep;
  float sn = smoothKaleidoscope(&p, sm, rep);
  p = mul_f2_mat2(p,ROT(TAU*h0+0.025f*TIME));
  float z = _mix(0.2f, 0.4f, h3);
  p /= z;
  p+=0.5f+_floor(h1*1000.0f);
  float tl = tanh_approx(0.33f*l);
  float r = _mix(0.30f, 0.45f, PCOS(0.1f*n));
  float2 d2 = truchet_df(r, p);
  d2 *= z;
  float d = d2.x;
  float lw =0.025f*z; 
  d -= lw;
  
  float3 col = _mix(to_float3_s(1.0f), to_float3_s(0.0f), smoothstep(aa, -aa, d));
  col = _mix(col, to_float3_s(0.0f), smoothstep(_mix(1.0f, -0.5f, tl), 1.0f, _sinf(PI*100.0f*d)));
//  float t0 = smoothstep(aa, -aa, -d2.y-lw);
  col = _mix(col, to_float3_s(0.0f), step(d2.y, 0.0f));
  //float t = smoothstep(3.0f*lw, 0.0f, -d2.y);
//  float t = smoothstep(aa, -aa, -d2.y-lw);
  float t = smoothstep(aa, -aa, -d2.y-3.0f*lw)*_mix(0.5f, 1.0f, smoothstep(aa, -aa, -d2.y-lw));
  return to_float4_aw(col, t);
}

__DEVICE__ float3 skyColor(float3 ro, float3 rd) {
  float d = _powf(_fmaxf(dot(rd, to_float3(0.0f, 0.0f, 1.0f)), 0.0f), 20.0f);
  return to_float3_s(d);
}

__DEVICE__ float3 color(float3 ww, float3 uu, float3 vv, float3 ro, float2 p, float iTime, float2 iResolution) {

  float lp = length(p);
  float2 np = p + 1.0f/RESOLUTION;
  float rdd = (2.0f+1.0f*tanh_approx(lp));
//  float rdd = 2.0f;
  float3 rd = normalize(p.x*uu + p.y*vv + rdd*ww);
  float3 nrd = normalize(np.x*uu + np.y*vv + rdd*ww);

  const float planeDist = 1.0f-0.25f;
  const int furthest = 6;
  const int fadeFrom = _fmaxf(furthest-5, 0);

  const float fadeDist = planeDist*(float)(furthest - fadeFrom);
  float nz = _floor(ro.z / planeDist);

  float3 skyCol = skyColor(ro, rd);


  float4 acol = to_float4_s(0.0f);
  const float cutOff = 0.95f;
  bool cutOut = false;

  // Steps from nearest to furthest plane and accumulates the color 
  for (int i = 1; i <= furthest; ++i) {
    float pz = planeDist*nz + planeDist*(float)(i);

    float pd = (pz - ro.z)/rd.z;

    if (pd > 0.0f && acol.w < cutOff) {
      float3 pp = ro + rd*pd;
      float3 npp = ro + nrd*pd;

      float aa = 3.0f*length(pp - npp);

      float3 off = offset(pp.z);

      float4 pcol = plane(ro, rd, pp, off, aa, nz+(float)(i),iTime);

      float nz = pp.z-ro.z;
      float fadeIn = smoothstep(planeDist*(float)(furthest), planeDist*(float)(fadeFrom), nz);
      float fadeOut = smoothstep(0.0f, planeDist*0.1f, nz);
      pcol = to_float4_aw(_mix(skyCol, swi3(pcol,x,y,z), fadeIn),pcol.w);
  
      pcol.w *= fadeOut;
      pcol = clamp(pcol, 0.0f, 1.0f);

      acol = alphaBlend(pcol, acol);
    } else {
      cutOut = true;
      break;
    }

  }

  float3 col = alphaBlend(skyCol, acol);
  // To debug cutouts due to transparency  
  //  col += cutOut ? to_float3(1.0f, -1.0f, 0.0f) : to_float3_s(0.0f);
  return col;
}

__DEVICE__ float3 effect(float2 p, float2 q, float iTime, float2 iResolution) {
  float tm  = TIME*0.25f;
  float3 ro   = offset(tm);
  float3 dro  = doffset(tm);
  float3 ddro = ddoffset(tm);

  float3 ww = normalize(dro);
  float3 uu = normalize(cross(normalize(to_float3(0.0f,1.0f,0.0f)+ddro), ww));
  float3 vv = normalize(cross(ww, uu));

  float3 col = color(ww, uu, vv, ro, p,iTime,iResolution);
  
  return col;
}

__KERNEL__ void TruchetKaleidoscopeFtwFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  float2 q = fragCoord/RESOLUTION;
  float2 p = -1.0f + 2.0f * q;
  p.x *= RESOLUTION.x/RESOLUTION.y;
  
  float3 col = effect(p, q,iTime,iResolution);
  col *= smoothstep(0.0f, 4.0f, TIME);
  col = postProcess(col, q);
 
  fragColor = to_float4_aw(col, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}