
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/ec8a6ea755d34600547a5353f21f0a453f9f55ff95514383b2d80b8d71283eda.mp3' to iChannel0


// Constants ----------
#define PI 3.14159265358979f
#define P2 6.28318530717959f

// -- tracing parameters
const int   MAX_TRACE_STEP = 90;
const float MAX_TRACE_DIST = 80.0f;
const float NO_HIT_DIST    = 100.0f; // must be NO_HIT_DIST > MAX_TRACE_DIST
const float TRACE_PRECISION = 0.001f;
const float FUDGE_FACTOR = 0.9f;
const int   RAY_TRACE_COUNT = 5;
const float3  GAMMA = to_float3(1.0f/2.2f);

// -- rendering parameters
const int   GI_TRACE_STEP = 5;
const float GI_LENGTH = 1.6f;
const float GI_STRENGTH = 0.2f;
const float AO_STRENGTH = 0.4f;
const int   SS_MAX_TRACE_STEP = 4;
const float SS_MAX_TRACE_DIST = 10.0f;
const float SS_MIN_MARCHING = 0.4f;
const float SS_SHARPNESS = 1.0f;
const float CS_STRENGTH = 0.4f;
const float CS_SHARPNESS = 0.3f;


// Structures ----------
struct Surface {
  float d;              // distance
  float3  kd, tc, rl, rr; // diffusion, transparent-color, reflectance, refractive index
};
__DEVICE__ Surface near(Surface s,Surface t) { if (s.d<t.d) return s; return t; }

struct Ray {
  float3  org, dir, col;     // origin, direction, color
  float len, stp, rr, sgn; // length, marching step, refractive index of current media, sign of distance function
};
__DEVICE__ Ray ray(float3 o, float3 d) { return Ray(o,d,to_float3(1),0.0f,0.0f,1.0f,1.0f); }
__DEVICE__ Ray ray(float3 o, float3 d, float3 c, float rr, float s) { return Ray(o,d,c,0.0f,0.0f,rr,s); }
__DEVICE__ float3 _pos(Ray r) { return r.org+r.dir*r.len; }

struct Hit {
  float3 pos,nml; // position, normal
  Ray ray;      // ray
  Surface srf;  // surface
  bool isTransparent, isReflect;  // = (len2(srf.tc) > 0.001f, len2(srf.rl) > 0.001f)
};
__DEVICE__ Hit nohit(Ray r) { 
Surface surf = {NO_HIT_DIST, to_float3_a(1)};
Hit ret = {to_float3_s(0), to_float3_s(0), r, surf, to_float3_s(0), to_float3_s(0), to_float3_s(0)), false, false};
return ret;//Hit(to_float3_aw(0), to_float3(0), r, Surface(NO_HIT_DIST, to_float3_aw(1), to_float3(0), to_float3_aw(0), to_float3(0)), false, false); 
}

struct Camera {
  float3 pos, tgt;  // position, target
  float rol, fcs; // roll, focal length
};

__DEVICE__ mat3 _mat3(Camera c) {
  float3 w = normalize(c.pos-c.tgt);
  float3 u = normalize(cross(w,to_float3_aw(_sinf(c.rol),_cosf(c.rol),0)));
  return to_mat3(u,normalize(cross(u,w)),w);
}

struct AABB { float3 bmin, bmax; };
struct Light { float3 dir, col; };

__DEVICE__ float3 _diff(float3 n, Light l){ return clamp((dot(n, l.dir)+1.0f)*0.5f, 0.0f, 1.0f)*l.col; }


// Grobal valiables ----------
const float bpm = 126.0f;
const Light amb = {to_float3(0,-1,0), to_float3_s(0.4f)};
const Light dif = {normalize(to_float3(0,-1,0)), to_float3_s(1)};
float phase;


// Utilities ----------
__DEVICE__ float3  _hsv(float h, float s, float v) { return ((clamp(_fabs(fract(h+to_float3(0,2,1)/3.0f)*6.0f-3.0f)-1.0f,0.0f,1.0f)-1.0f)*s+1.0f)*v; }
__DEVICE__ mat3  _smat(float2 a) { float x=_cosf(a.y),y=_cosf(a.x),z=_sinf(a.y),w=_sinf(a.x); return mat3(y,w*z,-w*x,0,x,z,w,-y*z,y*x); }
__DEVICE__ mat3  _pmat(float3 a) {
  float sx=_sinf(a.x),sy=_sinf(a.y),sz=_sinf(a.z),cx=_cosf(a.x),cy=_cosf(a.y),cz=_cosf(a.z);
  return mat3(cz*cy,sz*cy,-sy,-sz*cx+cz*sy*sx,cz*cx+sz*sy*sx,cy*sx,sz*sx+cz*sy*cx,-cz*sx+sz*sy*cx,cy*cx);
}
__DEVICE__ float _checker(float2 uv, float2 csize) { return mod_f(_floor(uv.x/csize.x)+_floor(uv.y/csize.y),2.0f); }
__DEVICE__ float _checker3(float3 uvt, float3 csize) { return mod_f(_floor(uvt.x/csize.x)+_floor(uvt.y/csize.y)+_floor(uvt.z/csize.z),2.0f); }
__DEVICE__ float len2(float3 v) { return dot(v,v); }
__DEVICE__ float smin(float a, float b, float k) { return -_logf(exp(-k*a)+_expf(-k*b))/k; }
__DEVICE__ float smax(float a, float b, float k) { return _logf(exp(k*a)+_expf(k*b))/k; }
__DEVICE__ float vmin(float3 v) { return _fminf(v.x, _fminf(v.y, v.z)); }
__DEVICE__ float vmax(float3 v) { return _fmaxf(v.x, _fmaxf(v.y, v.z)); }
__DEVICE__ float2  cycl(float t, float2 f, float2 r) { return to_float2(_cosf(t*f.x)*r.x+_cosf(t*f.y)*r.y,_sinf(t*f.x)*r.x+_sinf(t*f.y)*r.y); }
__DEVICE__ float3  fresnel(float3 f0, float dp) { return f0+(1.0f-f0)*_powf(1.0f-_fabs(dp),5.0f); }
__DEVICE__ float rr2rl(float rr) { float v=(rr-1.0f)/(rr+1.0f); return v*v; }
__DEVICE__ float rand(float2 co){ return fract(_sinf(dot(swi2(co,x,y) ,to_float2(12.9898f,78.233f))) * 43758.5453f); }

// Intersect functions ----------
__DEVICE__ float2 hitAABB(Ray r, AABB b) { 
  float3 t1=(b.bmin-r.org)/r.dir, t2=(b.bmax-r.org)/r.dir;
  return to_float2(vmax(_fminf(t1, t2)), vmin(_fmaxf(t1, t2)));
}

__DEVICE__ float ifBox(Ray r, float3 b) {
  float2 v = hitAABB(r, AABB(-b,b));
  return (0.0f<=v.y&&v.x<=v.y)?v.x:NO_HIT_DIST;
}

// Distance functions ----------
__DEVICE__ float dfPln(float3 p, float3 n, float d) { return dot(p,n) + d; }
__DEVICE__ float dfBox(float3 p, float3 b, float r) { return r-length(_fmaxf(_fabs(p)-b,0.0f));}

const float vtx[11] = float[11](1.10f, 1.39f, 1.75f, 2.32f, 2.68f, 2.08f, 1.44f, 1.12f, 0.98f, 0.95f, 1.00f);
const float vcnt = 5.0f;
__DEVICE__ float dfFreeDSBC(in float3 p, in float r, in float t) {
  p = to_float3_aw(p.x, _fabs(p.y)-t, _fabs(p.z*2.0f));
  float at = _atan2f(p.z,p.x)/(PI/vcnt);
  float a = _floor(at+0.5f)*(PI/vcnt), c = _cosf(a), s = _sinf(a);
  int   i = int(at*2.0f);
  float v0 = vtx[i]   + textureLod(iChannel0, to_float2(1.05f-float(i)  /12.0f, 0), 0.0f).x*2.0f;
  float v1 = vtx[i+1] + textureLod(iChannel0, to_float2(1.05f-float(i+1)/12.0f, 0), 0.0f).x*2.0f;
  float rf = _mix(v0,v1,fract(at*2.0f)) * r;
  float3  q = to_float3((c*p.x+s*p.z)/rf, p.y/r, _fabs(-c*p.z+s*p.x)/rf);
  float fcBezel = dot(q, to_float3(0.544639035f, 0.8386705679f, 0))           - 0.544639035f;
  float fcUGird = dot(q, to_float3(0.636291199f, 0.7609957358f, 0.1265661887f)) - 0.636291199f;
  float fcStar  = dot(q, to_float3(0.332894535f, 0.9328278154f, 0.1378894313f)) - 0.448447409f;
  float fcTable =   q.y - 0.2727511892f - 0.05f;
  float fcCulet = - q.y - 0.8692867378f * 0.96f;
  float fcGirdl = length(swi2(q,x,z)) - 0.975f;
  return _fmaxf(fcGirdl, _fmaxf(fcCulet, _fmaxf(fcTable, _fmaxf(fcBezel, _fmaxf(fcStar, fcUGird)))));
}

__DEVICE__ float sdChain(float3 p, float r, float l, float w){
  float a=(l+r-w)*2.0f, x0=mod_f(p.x, a*2.0f), x1=mod_f(p.x+a, a*2.0f);
  return _fminf(length(to_float2(length(to_float2(_fmaxf(_fabs(x0-a)-l,0.0f),p.z))-r,p.y))-w,
             length(to_float2(length(to_float2(_fmaxf(_fabs(x1-a)-l,0.0f),p.y))-r,p.z))-w);
}


// Domain Operations ----------
__DEVICE__ float3 doXZ(float3 p, float2 r){
  float2 hr = r*0.5f;
  return to_float3_aw(mod_f(p.x+hr.x, r.x)-hr.x, p.y, mod_f(p.z+hr.y, r.y)-hr.y);
}


// Deforimng function ----------
__DEVICE__ float3 foXZCircle(float3 p, float l, float r){
  return to_float3_aw(_atan2f(p.z,p.x)*l/P2,p.y,length(swi2(p,x,z))-r);
}


// Easing Functions ----------


// Ray calcuatoins ----------
Ray rayScreen(in float2 p, in Camera c) {
  return ray(c.pos, normalize(_mat3(c) * to_float3(swi2(p,x,y), -c.fcs)));
}

Ray rayReflect(in Hit h, in float3 rl) {
  return ray(h.pos + h.nml*0.01f, reflect(h.ray.dir, h.nml), h.ray.col*rl, h.ray.rr, h.ray.sgn);
}

Ray rayRefract(in Hit h, in float rr) {
  float3 r = refract(h.ray.dir, h.nml, h.ray.rr/rr);
  if (len2(r)<0.001f) return rayReflect(h, to_float3(1));
  return ray(h.pos - h.nml*0.01f, r, h.ray.col*h.srf.tc, rr, -h.ray.sgn);
}


// Sphere tracing ----------.
__DEVICE__ Surface map(in float3 p, float phase, float phase){
  float t = _expf(cos(phase/2.0f)*5.0f)*0.001f;
  float isGlass = sign(p.x) * 0.5f + 0.5f; // glass=1, mirror=0
  float3 rr = to_float3_aw(isGlass*1.8f),
       rl = to_float3(rr2rl(rr.x)),
       col = to_float3(0),
       tc = to_float3(1.0f,0.2f,0.2f)*isGlass;
  return near(
    Surface(dfFreeDSBC(to_float3(_fabs(p.x)-2.7f,p.y-2.0f,p.z)*_pmat(to_float3(phase/32.0f,0.0f,0.5f*PI)), 1.6f, 0.1f), col, tc, rl, rr),
    Surface(sdChain(foXZCircle((p-to_float3(0,2,0))*_pmat(to_float3_aw(0,_sinf(phase/32.0f)*P2,phase/32.0f)),30.6f,6.0f), 0.35f, 0.1f+t, 0.1f+t), 
            to_float3_s(0), to_float3_s(1.0f), rl, rr)
  );
}
//doXZ(p, to_float2(1.5f*8.0f-0.4f*16.0f))*mat

// Lighting ----------
__DEVICE__ float4 _cs(in float3 pos, in float3 dir) {
  float4 col;
  float len = SS_MIN_MARCHING;
  for (int i=SS_MAX_TRACE_STEP; i!=0; --i) {
    Surface s = map(pos + dir*len);
    col = to_float4(s.tc, _fminf(col.w, SS_SHARPNESS*s.d/len));
    len += _fmaxf(s.d, SS_MIN_MARCHING);
    if (s.d<TRACE_PRECISION || len>SS_MAX_TRACE_DIST) break;
  }
  col.w = clamp(col.w, 0.0f, 1.0f);
  swi3(col,x,y,z) = _powf((1.0f-col.w), CS_SHARPNESS) * swi3(col,x,y,z) * CS_STRENGTH;
  return col;
}

__DEVICE__ float4 _gi(in float3 pos, in float3 nml) {
  float4 col = to_float4_aw(0);
  for (int i=GI_TRACE_STEP; i!=0; --i) {
    float hr = 0.01f + float(i) * GI_LENGTH / 4.0f;
    Surface s = map(nml * hr + pos);
    col += to_float4(s.kd, 1.0f) * (hr - s.d);
  }
  swi3(col,x,y,z) *= GI_STRENGTH / GI_LENGTH;
  col.w = clamp(1.0f-col.w * AO_STRENGTH / GI_LENGTH, 0.0f, 1.0f);
  return col;
}

__DEVICE__ float3 _back(in Ray ray, float phase) {
  ray.len = ifBox(ray, to_float3_aw(22));
  float3 p = _pos(ray);
  float3 ap = _fabs(p);
  float c = len2(fract(p*0.5f)-0.5f)-0.25f;
  float ct = 0.7f + p.y/33.0f;
  return (fract(c*_cosf(phase/16.0f)*16.0f)*0.2f+ct)*to_float3(1.0f-(ap.x+ap.y+ap.z-vmin(ap))/44.2f);
}

__DEVICE__ float3 lighting(in Hit h, float phase) {
  if (h.ray.len > MAX_TRACE_DIST) return _back(h.ray, phase);
  float4 fgi = _gi(h.pos, h.nml);    // Fake Global Illumination
  float4 fcs = _cs(h.pos, dif.dir);  // Fake Caustic Shadow
  //   lin = ([Ambient]        + [Diffuse]        * [SS]  + [CS])    * [AO]  + [GI]
  float3 lin = (_diff(h.nml, amb) + _diff(h.nml, dif) * fcs.w + swi3(fcs,x,y,z)) * fgi.w + swi3(fgi,x,y,z);
  return  h.srf.kd * lin;
}


// Ray tracing ----------
__DEVICE__ float3 _calcNormal(in float3 p){
  float3 v=to_float3(0.001f,0,map(p).d);
  return normalize(to_float3(map(p+swi3(v,x,y,y)).d-v.z,map(p+swi3(v,y,x,y)).d-v.z,map(p+swi3(v,y,y,x)).d-v.z));
}

Hit sphereTrace(in Ray r) {
  Surface s;
  for(int i=0; i<MAX_TRACE_STEP; i++) {
    s = map(_pos(r));
    s.d *= r.sgn;
    r.len += s.d * FUDGE_FACTOR;
    r.stp = float(i);
    if (s.d < TRACE_PRECISION) break;
    if (r.len > MAX_TRACE_DIST) return nohit(r);
  }
  float3 p = _pos(r);
  float interior = 0.5f-r.sgn*0.5f;
  s.rr = _mix(s.rr, to_float3_aw(1), interior);
  s.tc = _fmaxf(s.tc, interior);
  return Hit(p, _calcNormal(p)*r.sgn, r, s, (len2(s.tc)>0.001f), (len2(s.rl)>0.001f));
}

Hit trace(in Ray r) {
  return sphereTrace(r);
}


// Rendering ----------
__DEVICE__ float3 _difColor(inout Hit h) {
  if (len2(h.srf.kd) < 0.001f) return to_float3_aw(0);
  float3 col = lighting(h) * h.ray.col;
  h.ray.col *= 1.0f - h.srf.kd;
  return col;
}

Ray _nextRay(Hit h) {
  if (h.isTransparent) return rayRefract(h, h.srf.rr.x);
  return rayReflect(h, fresnel(h.srf.rl, dot(h.ray.dir, h.nml)));
}

__DEVICE__ float4 render(in Ray r){
  float3 col = to_float3(0), rl, c;
  Hit h0, h1;
  float l0;

  // first trace
  h0 = trace(r);
  l0 = h0.ray.len;

  // first diffusion surface
  col += _difColor(h0);
  if (!h0.isReflect) return to_float4_aw(col, l0);

  // first reflection
  rl = fresnel(h0.srf.rl, dot(h0.ray.dir, h0.nml));
  h1 = trace(rayReflect(h0, rl));
  col += _difColor(h1);
  h0.ray.col *= 1.0f - rl;
  if (!h0.isTransparent) h0 = h1;
 
  // repeating trace
  for (int i=RAY_TRACE_COUNT; i!=0; --i) {
    if (!h0.isReflect) return to_float4_aw(col, l0);
    h0 = trace(_nextRay(h0));
    col += _difColor(h0);
  }

  // cheap trick
  c = h0.ray.col;
  if (len2(c) >= 0.25f) col += _back(h0.ray) * c * c;

  return to_float4_aw(col, l0);
}

__DEVICE__ float4 gamma(in float4 i) {
  return to_float4(_powf(swi3(i,x,y,z), GAMMA), i.w);
}


// Entry point ----------
__KERNEL__ void GlassAndMetalHeartsFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

  float phase = iTime * bpm / 60.0f * P2;

  float2   m = to_float2(_cosf(phase/128.0f), _sinf(phase/128.0f))*15.0f;
  Camera c = Camera(to_float3_aw(m.x,_sinf(phase/32.0f)*10.0f,m.y), to_float3(0,2,0), 0.0f, 1.73205081f);
  //Camera c = Camera(to_float3(0,10,1),to_float3(0,1,0), 0.0f, 1.73205081f);
  Ray    r = rayScreen((fragCoord * 2.0f - iResolution) / iResolution.x, c);

  float4 res = render(r);
  res.w = _fminf(_fabs(res.w - length(c.pos)+15.0f)/100.0f, 1.0f);

  fragColor = gamma(res);


  SetFragmentShaderComputedColor(fragColor);
}