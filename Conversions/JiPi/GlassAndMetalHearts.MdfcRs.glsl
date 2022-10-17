

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Constants ----------
#define PI 3.14159265358979
#define P2 6.28318530717959

// -- tracing parameters
const int   MAX_TRACE_STEP = 90;
const float MAX_TRACE_DIST = 80.;
const float NO_HIT_DIST    = 100.; // must be NO_HIT_DIST > MAX_TRACE_DIST
const float TRACE_PRECISION = .001;
const float FUDGE_FACTOR = .9;
const int   RAY_TRACE_COUNT = 5;
const vec3  GAMMA = vec3(1./2.2);

// -- rendering parameters
const int   GI_TRACE_STEP = 5;
const float GI_LENGTH = 1.6;
const float GI_STRENGTH = .2;
const float AO_STRENGTH = .4;
const int   SS_MAX_TRACE_STEP = 4;
const float SS_MAX_TRACE_DIST = 10.;
const float SS_MIN_MARCHING = .4;
const float SS_SHARPNESS = 1.;
const float CS_STRENGTH = .4;
const float CS_SHARPNESS = .3;


// Structures ----------
struct Surface {
  float d;              // distance
  vec3  kd, tc, rl, rr; // diffusion, transparent-color, reflectance, refractive index
};
Surface near(Surface s,Surface t) { if (s.d<t.d) return s; return t; }

struct Ray {
  vec3  org, dir, col;     // origin, direction, color
  float len, stp, rr, sgn; // length, marching step, refractive index of current media, sign of distance function
};
Ray ray(vec3 o, vec3 d) { return Ray(o,d,vec3(1),0.,0.,1.,1.); }
Ray ray(vec3 o, vec3 d, vec3 c, float rr, float s) { return Ray(o,d,c,0.,0.,rr,s); }
vec3 _pos(Ray r) { return r.org+r.dir*r.len; }

struct Hit {
  vec3 pos,nml; // position, normal
  Ray ray;      // ray
  Surface srf;  // surface
  bool isTransparent, isReflect;  // = (len2(srf.tc) > 0.001, len2(srf.rl) > 0.001)
};
Hit nohit(Ray r) { return Hit(vec3(0), vec3(0), r, Surface(NO_HIT_DIST, vec3(1), vec3(0), vec3(0), vec3(0)), false, false); }

struct Camera {
  vec3 pos, tgt;  // position, target
  float rol, fcs; // roll, focal length
};
mat3 _mat3(Camera c) {
  vec3 w = normalize(c.pos-c.tgt);
  vec3 u = normalize(cross(w,vec3(sin(c.rol),cos(c.rol),0)));
  return mat3(u,normalize(cross(u,w)),w);
}

struct AABB { vec3 bmin, bmax; };
struct Light { vec3 dir, col; };
vec3 _diff(vec3 n, Light l){ return clamp((dot(n, l.dir)+1.)*.5, 0., 1.)*l.col; }


// Grobal valiables ----------
const float bpm = 126.0;
const Light amb = Light(vec3(0,-1,0), vec3(0.4));
const Light dif = Light(normalize(vec3(0,-1,0)), vec3(1));
float phase;


// Utilities ----------
vec3  _hsv(float h, float s, float v) { return ((clamp(abs(fract(h+vec3(0,2,1)/3.)*6.-3.)-1.,0.,1.)-1.)*s+1.)*v; }
mat3  _smat(vec2 a) { float x=cos(a.y),y=cos(a.x),z=sin(a.y),w=sin(a.x); return mat3(y,w*z,-w*x,0,x,z,w,-y*z,y*x); }
mat3  _pmat(vec3 a) {
  float sx=sin(a.x),sy=sin(a.y),sz=sin(a.z),cx=cos(a.x),cy=cos(a.y),cz=cos(a.z);
  return mat3(cz*cy,sz*cy,-sy,-sz*cx+cz*sy*sx,cz*cx+sz*sy*sx,cy*sx,sz*sx+cz*sy*cx,-cz*sx+sz*sy*cx,cy*cx);
}
float _checker(vec2 uv, vec2 csize) { return mod(floor(uv.x/csize.x)+floor(uv.y/csize.y),2.); }
float _checker3(vec3 uvt, vec3 csize) { return mod(floor(uvt.x/csize.x)+floor(uvt.y/csize.y)+floor(uvt.z/csize.z),2.); }
float len2(vec3 v) { return dot(v,v); }
float smin(float a, float b, float k) { return -log(exp(-k*a)+exp(-k*b))/k; }
float smax(float a, float b, float k) { return log(exp(k*a)+exp(k*b))/k; }
float vmin(vec3 v) { return min(v.x, min(v.y, v.z)); }
float vmax(vec3 v) { return max(v.x, max(v.y, v.z)); }
vec2  cycl(float t, vec2 f, vec2 r) { return vec2(cos(t*f.x)*r.x+cos(t*f.y)*r.y,sin(t*f.x)*r.x+sin(t*f.y)*r.y); }
vec3  fresnel(vec3 f0, float dp) { return f0+(1.-f0)*pow(1.-abs(dp),5.); }
float rr2rl(float rr) { float v=(rr-1.)/(rr+1.); return v*v; }
float rand(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }

// Intersect functions ----------
vec2 hitAABB(Ray r, AABB b) { 
  vec3 t1=(b.bmin-r.org)/r.dir, t2=(b.bmax-r.org)/r.dir;
  return vec2(vmax(min(t1, t2)), vmin(max(t1, t2)));
}

float ifBox(Ray r, vec3 b) {
  vec2 v = hitAABB(r, AABB(-b,b));
  return (0.<=v.y&&v.x<=v.y)?v.x:NO_HIT_DIST;
}

// Distance functions ----------
float dfPln(vec3 p, vec3 n, float d) { return dot(p,n) + d; }
float dfBox(vec3 p, vec3 b, float r) { return r-length(max(abs(p)-b,0.));}

const float vtx[11] = float[11](1.10, 1.39, 1.75, 2.32, 2.68, 2.08, 1.44, 1.12, 0.98, 0.95, 1.00);
const float vcnt = 5.0;
float dfFreeDSBC(in vec3 p, in float r, in float t) {
  p = vec3(p.x, abs(p.y)-t, abs(p.z*2.0));
  float at = atan(p.z,p.x)/(PI/vcnt);
  float a = floor(at+.5)*(PI/vcnt), c = cos(a), s = sin(a);
  int   i = int(at*2.);
  float v0 = vtx[i]   + textureLod(iChannel0, vec2(1.05-float(i)  /12., 0), 0.).x*2.;
  float v1 = vtx[i+1] + textureLod(iChannel0, vec2(1.05-float(i+1)/12., 0), 0.).x*2.;
  float rf = mix(v0,v1,fract(at*2.)) * r;
  vec3  q = vec3((c*p.x+s*p.z)/rf, p.y/r, abs(-c*p.z+s*p.x)/rf);
  float fcBezel = dot(q, vec3(.544639035, .8386705679, 0))           - .544639035;
  float fcUGird = dot(q, vec3(.636291199, .7609957358, .1265661887)) - .636291199;
  float fcStar  = dot(q, vec3(.332894535, .9328278154, .1378894313)) - .448447409;
  float fcTable =   q.y - .2727511892 - .05;
  float fcCulet = - q.y - .8692867378 * .96;
  float fcGirdl = length(q.xz) - .975;
  return max(fcGirdl, max(fcCulet, max(fcTable, max(fcBezel, max(fcStar, fcUGird)))));
}

float sdChain(vec3 p, float r, float l, float w){
  float a=(l+r-w)*2., x0=mod(p.x, a*2.), x1=mod(p.x+a, a*2.);
  return min(length(vec2(length(vec2(max(abs(x0-a)-l,0.),p.z))-r,p.y))-w,
             length(vec2(length(vec2(max(abs(x1-a)-l,0.),p.y))-r,p.z))-w);
}


// Domain Operations ----------
vec3 doXZ(vec3 p, vec2 r){
  vec2 hr = r*.5;
  return vec3(mod(p.x+hr.x, r.x)-hr.x, p.y, mod(p.z+hr.y, r.y)-hr.y);
}


// Deforimng function ----------
vec3 foXZCircle(vec3 p, float l, float r){
  return vec3(atan(p.z,p.x)*l/P2,p.y,length(p.xz)-r);
}


// Easing Functions ----------


// Ray calcuatoins ----------
Ray rayScreen(in vec2 p, in Camera c) {
  return ray(c.pos, normalize(_mat3(c) * vec3(p.xy, -c.fcs)));
}

Ray rayReflect(in Hit h, in vec3 rl) {
  return ray(h.pos + h.nml*.01, reflect(h.ray.dir, h.nml), h.ray.col*rl, h.ray.rr, h.ray.sgn);
}

Ray rayRefract(in Hit h, in float rr) {
  vec3 r = refract(h.ray.dir, h.nml, h.ray.rr/rr);
  if (len2(r)<.001) return rayReflect(h, vec3(1));
  return ray(h.pos - h.nml*.01, r, h.ray.col*h.srf.tc, rr, -h.ray.sgn);
}


// Sphere tracing ----------.
Surface map(in vec3 p){
  float t = exp(cos(phase/2.0)*5.0)*0.001;
  float isGlass = sign(p.x) * 0.5 + 0.5; // glass=1, mirror=0
  vec3 rr = vec3(isGlass*1.8),
       rl = vec3(rr2rl(rr.x)),
       col = vec3(0),
       tc = vec3(1.0,0.2,0.2)*isGlass;
  return near(
    Surface(dfFreeDSBC(vec3(abs(p.x)-2.7,p.y-2.0,p.z)*_pmat(vec3(phase/32.0,0.0,0.5*PI)), 1.6, 0.1), col, tc, rl, rr),
    Surface(sdChain(foXZCircle((p-vec3(0,2,0))*_pmat(vec3(0,sin(phase/32.0)*P2,phase/32.0)),30.6,6.0), 0.35, 0.1+t, 0.1+t), 
            vec3(0), vec3(1.0), rl, rr)
  );
}
//doXZ(p, vec2(1.5*8.-0.4*16.))*mat

// Lighting ----------
vec4 _cs(in vec3 pos, in vec3 dir) {
  vec4 col;
  float len = SS_MIN_MARCHING;
  for (int i=SS_MAX_TRACE_STEP; i!=0; --i) {
    Surface s = map(pos + dir*len);
    col = vec4(s.tc, min(col.a, SS_SHARPNESS*s.d/len));
    len += max(s.d, SS_MIN_MARCHING);
    if (s.d<TRACE_PRECISION || len>SS_MAX_TRACE_DIST) break;
  }
  col.a = clamp(col.a, 0., 1.);
  col.rgb = pow((1.-col.a), CS_SHARPNESS) * col.rgb * CS_STRENGTH;
  return col;
}

vec4 _gi(in vec3 pos, in vec3 nml) {
  vec4 col = vec4(0);
  for (int i=GI_TRACE_STEP; i!=0; --i) {
    float hr = .01 + float(i) * GI_LENGTH / 4.;
    Surface s = map(nml * hr + pos);
    col += vec4(s.kd, 1.) * (hr - s.d);
  }
  col.rgb *= GI_STRENGTH / GI_LENGTH;
  col.a = clamp(1.-col.a * AO_STRENGTH / GI_LENGTH, 0., 1.);
  return col;
}

vec3 _back(in Ray ray) {
  ray.len = ifBox(ray, vec3(22));
  vec3 p = _pos(ray);
  vec3 ap = abs(p);
  float c = len2(fract(p*0.5)-0.5)-0.25;
  float ct = 0.7 + p.y/33.0;
  return (fract(c*cos(phase/16.)*16.0)*0.2+ct)*vec3(1.0-(ap.x+ap.y+ap.z-vmin(ap))/44.2);
}

vec3 lighting(in Hit h) {
  if (h.ray.len > MAX_TRACE_DIST) return _back(h.ray);
  vec4 fgi = _gi(h.pos, h.nml);    // Fake Global Illumination
  vec4 fcs = _cs(h.pos, dif.dir);  // Fake Caustic Shadow
  //   lin = ([Ambient]        + [Diffuse]        * [SS]  + [CS])    * [AO]  + [GI]
  vec3 lin = (_diff(h.nml, amb) + _diff(h.nml, dif) * fcs.w + fcs.rgb) * fgi.w + fgi.rgb;
  return  h.srf.kd * lin;
}


// Ray tracing ----------
vec3 _calcNormal(in vec3 p){
  vec3 v=vec3(.001,0,map(p).d);
  return normalize(vec3(map(p+v.xyy).d-v.z,map(p+v.yxy).d-v.z,map(p+v.yyx).d-v.z));
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
  vec3 p = _pos(r);
  float interior = .5-r.sgn*.5;
  s.rr = mix(s.rr, vec3(1), interior);
  s.tc = max(s.tc, interior);
  return Hit(p, _calcNormal(p)*r.sgn, r, s, (len2(s.tc)>.001), (len2(s.rl)>.001));
}

Hit trace(in Ray r) {
  return sphereTrace(r);
}


// Rendering ----------
vec3 _difColor(inout Hit h) {
  if (len2(h.srf.kd) < .001) return vec3(0);
  vec3 col = lighting(h) * h.ray.col;
  h.ray.col *= 1. - h.srf.kd;
  return col;
}

Ray _nextRay(Hit h) {
  if (h.isTransparent) return rayRefract(h, h.srf.rr.x);
  return rayReflect(h, fresnel(h.srf.rl, dot(h.ray.dir, h.nml)));
}

vec4 render(in Ray r){
  vec3 col = vec3(0), rl, c;
  Hit h0, h1;
  float l0;

  // first trace
  h0 = trace(r);
  l0 = h0.ray.len;

  // first diffusion surface
  col += _difColor(h0);
  if (!h0.isReflect) return vec4(col, l0);

  // first reflection
  rl = fresnel(h0.srf.rl, dot(h0.ray.dir, h0.nml));
  h1 = trace(rayReflect(h0, rl));
  col += _difColor(h1);
  h0.ray.col *= 1. - rl;
  if (!h0.isTransparent) h0 = h1;
 
  // repeating trace
  for (int i=RAY_TRACE_COUNT; i!=0; --i) {
    if (!h0.isReflect) return vec4(col, l0);
    h0 = trace(_nextRay(h0));
    col += _difColor(h0);
  }

  // cheap trick
  c = h0.ray.col;
  if (len2(c) >= .25) col += _back(h0.ray) * c * c;

  return vec4(col, l0);
}

vec4 gamma(in vec4 i) {
  return vec4(pow(i.xyz, GAMMA), i.w);
}


// Entry point ----------
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  phase = iTime * bpm / 60. * P2;

  vec2   m = vec2(cos(phase/128.), sin(phase/128.))*15.;
  Camera c = Camera(vec3(m.x,sin(phase/32.)*10.,m.y), vec3(0,2,0), 0., 1.73205081);
  //Camera c = Camera(vec3(0,10,1),vec3(0,1,0), 0., 1.73205081);
  Ray    r = rayScreen((fragCoord.xy * 2. - iResolution.xy) / iResolution.x, c);

  vec4 res = render(r);
  res.w = min(abs(res.w - length(c.pos)+15.)/100., 1.);

  fragColor = gamma(res);
}
