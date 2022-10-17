
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define PATTERN_TIME (5.485714f / 4.0f)

__DEVICE__ float3 hash33_(float3 p){     
    float n = _sinf(dot(p, to_float3(7, 157, 113)));    
    return fract_f3(to_float3(2097152, 262144, 32768)*n); 
}
#define M0 1597334673U
#define M1 3812015801U
#define M2 make_uint2(M0, M1)
#define M3 make_uint3(M0, M1, 2798796413U)

__DEVICE__ float hash11( float q )
{
    uvec2 n = (uint)(q) * M2;
    return (float)((n.x ^ n.y) * M0) * (1.0f/(float)(0xffffffffU));
}

__DEVICE__ float  hash12(float2 p) { uvec2 q = make_uint2(to_int2_cfloat(p)) * M2; uint n = (q.x ^ q.y) * M0; return (float)(n) * (1.0f/(float)(0xffffffffU)); }
__DEVICE__ float3 hash33(float3 p) { uvec3 q = make_uint3(to_int3_cfloat(p)) * M3; q = (q.x ^ q.y ^ q.z)*M3; return to_float3(q) * (1.0f/(float)(0xffffffffU)); }

__DEVICE__ float voronoi(float3 p){
  float3 b, r, g = _floor(p);
  p = fract_f3(p); 
  float d = 1.0f;      
    for(int j = -1; j <= 1; j++) {
      for(int i = -1; i <= 1; i++) {        
        b = to_float3(i, j, -1); r = b - p + hash33(g+b); d = _fminf(d, dot(r,r));        
        b.z = 0.0f; r = b - p + hash33(g+b); d = _fminf(d, dot(r,r)); 
            b.z = 1.0f; r = b - p + hash33(g+b); d = _fminf(d, dot(r,r));          
      }
  }  
  return d;
}

__DEVICE__ float noiseLayers(in float3 p) {
    float3 t = to_float3(0.0f, 0.0f, p.z);    
    float tot = 0.0f, sum = 0.0f, amp = 1.0f;
    for (int i = 0; i < 2; i++) { tot += voronoi(p + t) * amp; p *= 2.0f; t *= 1.5f; sum += amp; amp *= 0.5f; }    
    return tot / sum;
}

__DEVICE__ float noiseF( in float2 p )
{
    float2 i = _floor( p ), f = fract_f2( p ), u = f*f*f*(3.0f-2.0f*f);

    return _mix( _mix( hash12( i + to_float2(0.0f,0.0f) ), 
                       hash12( i + to_float2(1.0f,0.0f) ), u.x),
                 _mix( hash12( i + to_float2(0.0f,1.0f) ), 
                       hash12( i + to_float2(1.0f,1.0f) ), u.x), u.y);
}

__DEVICE__ float noiseFF(in float2 uv) {
    uv *= 2.0f;
    
   mat2 m = to_mat2( 1.6f,  1.2f, -1.2f,  1.6f ) * 1.25f;
    
    float f  = 0.5f   *noiseF( uv ); uv = mul_mat2_f2(m,uv);
          f += 0.2500f*noiseF( uv ); uv = mul_mat2_f2(m,uv);
          f += 0.1250f*noiseF( uv ); uv = mul_mat2_f2(m,uv);
          f += 0.0625f*noiseF( uv ); uv = mul_mat2_f2(m,uv);   
    
    return f;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


#define getNormal getNormalHex
#define V float3
#define W float2
#define F float
#define FAR 330.0f
#define INF 1e32
//#define IT iTime
#define mt iChannelTime[1]

#define PI 3.14159265f
#define PHI (1.618033988749895f)

struct Timeline {
    float songTime;
    float rPatternTime;
    float nPatternTime;
    float smoothNPatternTime;
    
    float patternNum;
} timeline;

struct SdfMixer {
    float ifs1;
    float grid;        
    float star;
} sdfMixer;

float 
    Z = 0.0f, 
    J = 1.0f, 
  vol = 0.0f,
  noise = 0.0f;
 

#define H(P) fract(_sinf(dot(P,to_float2(127.1f,311.7f)))*43758.545f)
__DEVICE__ float n(in float3 p) {
    float3  i = _floor(p), 
            f = fract(p),   
            u = f*f*(3.0f-f-f);
    
    float2 ii = swi2(i,x,y) + i.z * 5.0f + 5.0f;
    
    #define II(a,b) H(swi2(i,x,y) + i.z * (to_float2_s(5.0f) + to_float2(a,b))
    
    float   v1 = _mix(_mix(II(Z,Z),II(J,Z),u.x), 
                 _mix(II(Z,J),II(J,J),u.x), u.y);
    
    #define I2(a,b) H(ii + to_float2(a,b))
    return _fmaxf(_mix(v1,_mix(mix(I2(Z,Z),I2(J,Z),u.x), 
                  _mix(I2(Z,J),I2(J,J),u.x), u.y),u.z),Z);
}
#define A w *= 0.5f; s *= 2.0f; r += w * n(s * x);
__DEVICE__ float B(float3 _x) {
    float r = Z, w = J, s = J;
    //A A A A;
    w *= 0.5f; s *= 2.0f; r += w * n(s * _x);
    w *= 0.5f; s *= 2.0f; r += w * n(s * _x);
    w *= 0.5f; s *= 2.0f; r += w * n(s * _x);
    w *= 0.5f; s *= 2.0f; r += w * n(s * _x);
    return r;
}

#define fromRGB(a, b, c) to_float3_aw(F(a), F(b), F(c)) / 255.0f;
    
float3 
    light = to_float3(0.0f ,1.0f, 1.0f),
    lightDir,
    lightColour = normalize(to_float3(0.5f, 0.6f, 0.5f) ); 

__DEVICE__ float3 _saturatef(float3 a) { return clamp(a, 0.0f, 1.0f); }
__DEVICE__ float2 _saturatef(float2 a) { return clamp(a, 0.0f, 1.0f); }
__DEVICE__ float  _saturatef(float  a) { return clamp(a, 0.0f, 1.0f); }

// Repeat only a few times: from indices <start> to <stop> (similar to above, but more flexible)
__DEVICE__ float pModInterval1(inout float *p, float size, float start, float stop) {
  float halfsize = size*0.5f;
  float c = _floor((*p + halfsize)/size);
  *p = mod_f(p+halfsize, size) - halfsize;
  if (c > stop) { //yes, this might not be the best thing numerically.
    *p += size*(c - stop);
    c = stop;
  }
  if (c <start) {
    *p += size*(c - start);
    c = start;
  }
  return c;
}



__DEVICE__ float smin( float a, float b, float k )
{
    float res = _expf( -k*a ) + _expf( -k*b );
    return -_logf( res )/k ;
}

__DEVICE__ float2 pR(float2 p, float a) {
  p = _cosf(a)* p + _sinf(a)*to_float2(p.y, -p.x);
  return p;
}

__DEVICE__ float opU2( float d1, float d2 ) {
    if (d1 < d2) return d1;
    return d2;
}

__DEVICE__ float3 opU2( float3 d1, float3 d2 ) {
    if (d1.x < d2.x) return d1;
    return d2;
}

struct geometry {
    float dist;
    float materialIndex;
    float specular;
    float diffuse;
    float3 color;  
    float3 space;
    float mirror;
    float3 index;
};

__DEVICE__ geometry geoU(geometry g1, geometry g2) {
    if (g1.dist < g2.dist) return g1;
    return g2;
}

__DEVICE__ float3 opS2( float3 d1, float3 d2 ){  
    if (-d2.x > d1.x) return -d2;
    return d1;
}

__DEVICE__ float3 opI2( float3 d1, float3 d2 ) {
   if (d1.x > d2.x) return d1;
    return d2;
}

__DEVICE__ float vmin(float2 v) {
  return _fminf(v.x, v.y);
}


// Maximum/minumum elements of a vector
__DEVICE__ float vmax(float2 v) {
  return _fmaxf(v.x, v.y);
}

__DEVICE__ float vmax(float3 v) {
  return _fmaxf(max(v.x, v.y), v.z);
}

__DEVICE__ float vmax(float4 v) {
  return _fmaxf(max(v.x, v.y), _fmaxf(v.z, v.w));
}

// Sign function that doesn't return 0
__DEVICE__ float sgn(float x) {
  return (x<0.0f)?-1.0f:1.0f;
}

__DEVICE__ float2 sgn(float2 v) {
  return to_float2((v.x<0.0f)?-1.0f:1.0f, (v.y<0.0f)?-1.0f:1.0f);
}


// Repeat space along one axis. Use like this to repeat along the x axis:
// <float cell = pMod1(p.x,5);> - using the return value is optional.
__DEVICE__ float pMod1(inout float *p, float size) {
  float halfsize = size*0.5f;
  float c = _floor((*p + halfsize)/size);
  *p = mod_f(*p + halfsize, size) - halfsize;
  return c;
}


// Repeat in two dimensions
__DEVICE__ float2 pMod2(inout float2 *p, float2 size) {
  float2 c = _floor((*p + size*0.5f)/size);
  *p = mod_f(*p + size*0.5f,size) - size*0.5f;
  return c;
}
// Repeat around the origin by a fixed angle.
// For easier use, num of repetitions is use to specify the angle.
__DEVICE__ float pModPolar(inout float2 *p, float repetitions) {
  float angle = 2.0f*PI/repetitions;
  float a = _atan2f((*p).y, (*p).x) + angle/2.0f;
  float r = length(*p);
  float c = _floor(a/angle);
  a = mod_f(a,angle) - angle/2.0f;
  *p = to_float2(_cosf(a), _sinf(a))*r;
  // For an odd number of repetitions, fix cell index of the cell in -x direction
  // (cell index would be e.g. -5 and 5 in the two halves of the cell):
  if (_fabs(c) >= (repetitions/2.0f)) c = _fabs(c);
  return c;
}

#ifdef ORG
// Mirror at an axis-aligned plane which is at a specified distance <dist> from the origin.
__DEVICE__ float pMirror (inout float *p, float dist) {
  float s = sgn(*p);
  *p = _fabs(*p)-dist;
  return s;
}
#endif
__DEVICE__ float pMirror (float p, float dist) {
  //float s = sgn(p);
  p = _fabs(p)-dist;
  return p;
}



__DEVICE__ float2 pMirrorOctant (inout float2 *p, float2 dist) {
  float2 s = sgn(*p);
  p.x = pMirror(p.x, dist.x);
  p.y = pMirror(p.y, dist.y);
  if (p.y > p.x)
    swi2S(p,x,y, swi2(p,y,x));
 
  return s;
}

__DEVICE__ float2 pModMirror2(inout float2 *p, float2 size) {
  float2 halfsize = size*0.5f;
  float2 c = _floor((*p + halfsize)/size);
  *p = mod_f(*p + halfsize, size) - halfsize;
  *p *= mod_f2(c,2.0f)*2.0f - to_float2_s(1.0f);
  return c;
}

// Box: correct distance to corners
__DEVICE__ float fBox(float3 p, float3 b) {
  float3 d = abs_f3(p) - b;
  return length(_fmaxf(d, to_float3_s(0))) + vmax(_fminf(d, to_float3_s(0)));
}

// Same as above, but in two dimensions (an endless box)
__DEVICE__ float fBox2Cheap(float2 p, float2 b) {
  return vmax(abs_f2(p)-b);
}

__DEVICE__ float fCross(float3 p, float3 size) {
    float obj = fBox(p, size);
    obj = opU2(obj, fBox(p, swi3(size,z,x,y)));
    obj = opU2(obj, fBox(p, swi3(size,y,z,x)));
               
    return obj;
}


__DEVICE__ float fSphere(float3 p, float r) {
  return length(p) - r;
}

__DEVICE__ mat2 rot(float angle)
{
    return to_mat2(_cosf(angle), -_sinf(angle), _sinf(angle), _cosf(angle));
}
//IFS iterations : try 2 or 3
#define NIFS 4
//scale and translate for the IFS in-loop transformation
#define SCALE 1.2f
#define TRANSLATE 4.2f
__DEVICE__ float3 sd2d(float2 p, float o)
{
    p *= 1.7f;
    float time = o * 0.2f;//0.2f*o+0.6f*iTime;
    float s =0.45f, d, d2 = 1.0f, d3= 0.0f;
    p*= s;
    float RADIUS =2.5f;//(1.0f+_sinf(iTime));
    
    int i;
    float3 col;  
    
    p = mul_f2_mat2(p,rot((mod_f(timeline.patternNum, 4.0f) > 1.0f ? -1.0f : 1.0f) * -0.9f * time));// twist

    for ( i = 0; i<NIFS; i++)
    {        
        if (p.x < 0.0f) { p.x = -p.x; col.x++;}
        //p = mul_f2_mat2(p,rot(0.1f*_sinf(time)));
        if (p.y < 0.0f) {p.y = -p.y; col.y++; }
        if (p.x-p.y < 0.0f){ swi2(p,x,y) = swi2(p,y,x); col.z++;}        
        
        p = p * SCALE - TRANSLATE;
        //p = mul_f2_mat2(p , rot(0.1f * iTime));
        d = 0.25f * (length(p) - RADIUS) * _powf(SCALE, (float)(-i)) / s;

        if (d < 1.1f) {
           d2 = (float)(i);
           d3 = 1.0f;
           break;
        }
    }
    
    
    //d = fCross(to_float3(swi2(p,x,y), 1.0f), to_float3_s(1.0f)) - RADIUS * _powf(SCALE, float(-i)) * 2.0f;
    //col/=float(NIFS);
    //vec3 oc = _mix(to_float3(0.7f,col.y,0.2f),to_float3(0.2f,col.x,0.7f), col.z);
    
    return to_float3(d, d2, d3);
}

// Cone with correct distances to tip and base circle. Y is up, 0 is in the middle of the base.
__DEVICE__ float fCone(float3 p, float radius, float height) {
  float2 q = to_float2(length(swi2(p,x,z)), p.y);
  float2 tip = q - to_float2(0, height);
  float2 mantleDir = normalize(to_float2(height, radius));
  float mantle = dot(tip, mantleDir);
  float d = _fmaxf(mantle, -q.y);
  float projected = dot(tip, to_float2(mantleDir.y, -mantleDir.x));
  
  // distance to tip
  if ((q.y > height) && (projected < 0.0f)) {
    d = _fmaxf(d, length(tip));
  }
  
  // distance to base ring
  if ((q.x > radius) && (projected > length(to_float2(height, radius)))) {
    d = _fmaxf(d, length(q - to_float2(radius, 0)));
  }
  return d;
}

__DEVICE__ float3 opTwist(in float3 p, float k)
{
    float c = _cosf(k*p.y);
    float s = _sinf(k*p.y);
    mat2  m = to_mat2(c,-s,s,c);
    float3  q = to_float3_aw(mul_mat2_f2(m,swi2(p,x,z),p.y));
    
    return q;
}

__DEVICE__ float3 discoBall(float3 p, float spikes) {
    
    //p -= ballPos;
    
    //p = opTwist(p, 0.1f);
    //p = opTwist(swi3(p,y,x,z), 0.1f);//_sinf(length(p) * 0.01f));
    
    //pR(swi2(p,x,z), iTime * 0.3f - _fminf(1.0f, iTime) * vol * 1.5f);
    //pR(swi2(p,y,z), iTime * 0.24f - _fminf(1.0f, iTime) * vol * 0.5f);
    
    //dpb = p;
    
    float2 _pxz = swi2(p,x,z);
    float pxz = pModPolar(&_pxz, 7.0f);//_ceil(vol2 * 10.0f));// * hash12(to_float2(_floor(part2Time * 3.55f))) * _fminf(part2Time, 30.0f));
    p.x=_pxz.x;p.z=_pxz.y;
    
    float2 _pyz = swi2(p,x,y);
    float pyz = pModPolar(&_pyz, 7.0f);
    p.y=_pyz.x;p.z=_pyz.y;
    //pR(swi2(p,x,z), length(p) * 0.01f);
    //pR(swi2(p,x,y), length(p) * 0.01f);
    
    
    return to_float3(
        fCone(swi3(p,z,x,y), 4.0f, spikes), 
        8.0f, 16.0f
    );

}

__DEVICE__ geometry map(float3 p,float iTime) {
    geometry box, fl;
    float3 bp = p, bp2 = p;
  //p.x *= _sinf(timeline.smoothNPatternTime + p.x);
    p *= (_sinf(length(bp)) * 0.2f - 1.0f) * (1.0f + timeline.nPatternTime);
    
    //p += _sinf(iTime);
    
    //pR(swi2(bp,y,x), p.y * 0.2f + _sinf(iTime));
    
    //pR(swi2(bp,x,y), PI / 4.0f);
    swi2S(bp,x,y, pR(swi2(bp,x,y), timeline.songTime * 0.02f + n(to_float3(iTime * 0.1f)) * 5.0f));
    //pModPolar(swi2(p,x,y), 4.0f);
    //pModPolar(swi2(p,x,z), 4.0f);
    float3 tb = bp;
    
    float bpx=bp.x,bpy=bp.y,bpz=bp.z;
    float3 reps = to_float3(
        pModInterval1(&bpx, 5.0f, -2.0f, 2.0f),
        pModInterval1(&bpy, 5.0f, -2.0f, 2.0f),
        pModInterval1(&bpz, 5.0f, -2.0f, 2.0f)
    );
    bp.x=bpx;bp.y=bpy;bp.z=bpz;
    
    
    box.dist = fSphere(bp, 1.5f);
    
    bp = tb;

    bpx=bp.x,bpy=bp.y,bpz=bp.z;
    float3 reps2 = to_float3(
        pModInterval1(&bpx, 5.0f, -1.5f, 1.5f),
        pModInterval1(&bpy, 5.0f, -1.5f, 1.5f),
        pModInterval1(&bpz, 5.0f, -1.5f, 1.5f)
    );
    bp.x=bpx;bp.y=bpy;bp.z=bpz;
    
    box.dist = _mix(box.dist, fBox(bp, to_float3_s(1.5f)), mod_f(timeline.patternNum, 2.0f));//fSphere(bp, 1.5f);
 
    box.diffuse = 4.0f;
    box.specular = 5.0f;
    box.mirror = 1.0f;
    box.color = to_float3_s(0.0f);
    
    //pR(swi2(p,x,z), timeline.songTime * 0.1f);
    //p = _mix(p, swi3(p,x,z,y), 0.9f);
    float3 s = sd2d(swi2(p,x,z), p.y) * 0.9f;
    
    float a = smin(s.x, box.dist, _powf(sin(iTime / 2.0f) / 2.0f + 0.5f, 3.0f) + 0.8f);
    
    box.dist = _mix(box.dist, a, sdfMixer.ifs1);    
    
    reps = to_float3_s(s.y);
    
    //box.dist = smin(box.dist, fSphere(p, 5.0f), _fabs(_sinf(iTime * 0.1f)));
    box.index = _ceil(reps + (n(reps + timeline.patternNum + timeline.nPatternTime * 10.0f) - 0.5f) * 3.0f);
    
    
    //box.dist = _mix(box.dist, fBox(p, to_float3_s(10.0f)), _sinf(iTime) / 2.0f + 0.5f);
    //!!!  box.dist = _mix(box.dist, fBox(p, to_float3_s(10.0f)), 1.0f-n(to_float3(p) * 0.5f));
    //pR(swi2(bp2,z,x), p.y * 0.2f + _sinf(iTime));
    
    //swi3(db,y,z,x) = opTwist(swi3(db,y,z,x), _sinf(length(db)) * 0.001f);
    float di = discoBall(bp2, 25.0f * (1.0f-timeline.smoothNPatternTime / 4.0f)).x;
    box.dist = smin(box.dist, di, 0.9f);
    
   // box.dist = _fminf(box.dist, -(length(bp2) -140.0f));
    box.dist = _mix(fSphere(bp2, 15.0f), box.dist, _fminf(timeline.songTime * 0.1f, 1.0f));
    
    p= bp2;
    //swi3(p,z,x,y) = opTwist(swi3(p,z,x,y), 0.03f);
    //float m = pModPolar(swi2(p,x,z), 10.0f);
    //p.x -= 35.0f ;//+ _sinf(p.y * 0.3f+ IT) * 3.0f;;
    float ss = sd2d(-swi2(p,z,x) * 1.4f, p.y).x;
    //p.y += _sinf(m + IT) * 10.0f;
    box.dist = _mix(ss, box.dist, _fminf(1.0f, timeline.songTime*0.1f));
    
    return box;
    box.diffuse = 4.0f;
    box.specular = 5.0f;
    box.mirror = 1.0f;
    box.color = to_float3(0.0f, 0.0f, 0.0f);
    box.index = to_float3_s(1.0f);
    
    bp.x += _sinf(timeline.songTime * 0.04f) * 20.0f;
    swi2S(bp,z,y, pR(swi2(bp,z,y), _sinf(timeline.songTime * 0.03f)));
    
    float2 bpxy = swi2(bp,x,y);
    pModPolar(&bpxy, 5.0f);
    bp.x=bpxy.x; bp.y=bpxy.y;
    
    box.dist = discoBall(bp * 2.0f, 25.0f).x;
    bp = bp2;
    bp.x += _sinf(timeline.songTime * 0.02f) * 20.0f;
    bp.y += _cosf(timeline.songTime * 0.02f) * 20.0f;
    swi2S(bp,x,y, pR(swi2(bp,x,y), _sinf(timeline.songTime * 0.1f)));
    swi2S(bp,z,y, pR(swi2(bp,z,y), _sinf(timeline.songTime * 0.1f)));
    
    box.dist = smin(box.dist, discoBall(bp * 2.0f, 25.0f).x,0.1f);
    
    return box;
}



float t_max = FAR;

float minDist = 1e3;
float glow = 0.0f;
bool firstpass = true;

__DEVICE__ geometry trace(float3 o, float3 d, int maxI, float2 iTime) {
    float omega = 0.3f;
    float t = 0.01f;
    float candidate_error = INF;
    float candidate_t = t;
    float previousRadius = 0.0f;
    float stepLength = 0.0f;
    float pixelRadius = 1.0f / 250.0f;
    float functionSign = map(o,iTime).dist < 0.0f ? -1.0f : +1.0f;
    
    geometry mp;

    for (int i = 0; i < 200; ++i) {
        if (maxI > 0 && i > maxI) break; 
        mp = map(d * t + o,iTime);
        
        if (mp.index.x == 0.0f && firstpass) {
            minDist = _fminf(minDist, mp.dist * 1.0f);
            glow = _powf( 1.0f / minDist, 0.5f);
        } 
        
        float signedRadius = functionSign * mp.dist;
        float radius = _fabs(signedRadius);
        bool sorFail = omega > 1.0f &&
        (radius + previousRadius) < stepLength;
        
        if (sorFail) {
            stepLength -= omega * stepLength;
            omega = 0.1f;
        } else {
            stepLength = signedRadius * omega;
        }
        
        previousRadius = radius;
        
        float error = radius / t;
        
        if (!sorFail && error < candidate_error) {
            candidate_t = t;
            candidate_error = error;
        }
        
        if (!sorFail && error < pixelRadius || t > t_max) break;
        
        t += stepLength;
     }
    
    mp.dist = candidate_t;
    
    if (
        (t > t_max || candidate_error > pixelRadius)
      ) mp.dist = INF;
    
    return mp;
}


__DEVICE__ float softShadow(float3 ro, float3 lp, float k) {
    const int maxIterationsShad = 15;
    float3 rd = (lp - ro); 

    float shade = 1.0f;
    float dist = 2.05f;
    float end = _fmaxf(length(rd), 0.01f);
    float stepDist = end / float(maxIterationsShad);

    rd /= end;
    for (int i = 0; i < maxIterationsShad; i++) {
        float h = map(ro + rd * dist).dist;
        shade = _fminf(shade, k*h/dist);
        dist += _fminf(h, stepDist * 2.0f); 
        if (h < 0.001f || dist > end) break;
    }
    return _fminf(max(shade, 0.0f), 1.0f);
}


//  normal calculation
__DEVICE__ float3 normal(float3 p) {
    float e=0.001f, d = map(p).dist; return normalize(to_float3(map(p+to_float3(e,0,0)).dist-d,map(p+to_float3(0,e,0)).dist-d,map(p+to_float3(0,0,e)).dist-d));
}

__DEVICE__ float getAO(float3 h, float3 n, float d) { return clamp(map(h + n * d).dist / d, 0.3f, 1.0f); }

__DEVICE__ float3 clouds(float3 d, float3 o) {
    float2 u = swi2(d,x,z) / d.y;
   
    return to_float3_aw(
        B(
            to_float3(
                u + to_float2(0.0f, o.z  * 0.05f), 9.
            )
        ) * to_float3(1.0f, 0.5f, 0.0f)

    ) * _fmaxf(0.0f, d.y);   
}

__DEVICE__ float3 Sky(in float3 rd, bool showSun, float3 lightDir, float3 ro)
{
   
    float sunSize = 0.5f;
    float sunAmount = _fmaxf(dot(rd, lightDir), 0.0f);
    float v = _powf(1.0f - _fmaxf(rd.y, 0.0f), 1.1f);
    float3 cl = to_float3_s(1.0f);//fromRGB(0,136,254);
    //cl.z *= _sinf(p.z * 0.3f);
    float3 sky = _mix(cl, to_float3(0.1f, 0.2f, 0.3f), v);
   //vec3 lightColour = to_float3(0.1f, 0.2f, 0.3f);
    
    sky += lightColour * sunAmount * sunAmount * 1.0f + lightColour * _fminf(_powf(sunAmount, 122.0f)* sunSize, 1.2f * sunSize);
    //sky += to_float3(0.0f, 0.0f, 0.0f) * _fmaxf(0.0f,rd.y);
    return clamp(sky, 0.3f, 1.0f) + clouds(rd, ro);// * H(to_float2(IT)) * _floor(B(rd * 12.1f) * 1.0f+ 0.5f) * 1.0f;
}

__DEVICE__ float3 doColor( in float3 sp, in float3 rd, in float3 sn, in float3 lp, geometry obj) {
    float3 sceneCol = obj.color;
    lp = sp + lp;
    float3 ld = lp - sp; // Light direction vector.
    float lDist = _fmaxf(length(ld / 2.0f), 0.01f); // Light to surface distance.
    ld /= lDist; // Normalizing the light vector.

    float atten = 1.0f / (1.0f + lDist * 0.025f + lDist * lDist * 0.5f);

    float diff = _fmaxf(dot(sn, ld), obj.diffuse);
    float spec = _fmaxf(dot(reflect(-ld, sn), -rd), obj.specular);

    
    float3 objCol = obj.color;//getObjectColor(sp, sn, obj);
    return objCol * (diff + 0.15f) * spec * 0.1f;
}


__DEVICE__ float3 applyFog( in float3  rgb,      // original color of the pixel
               in float distance, // camera to point distance
               in float3  rayOri,   // camera position
               in float3  rayDir ) {  // camera to point vector
    
    float c = 0.08f;
    float b = 0.1f;
    //rayOri.y -= 14.0f;
    float fogAmount = c * _expf(-rayOri.y * b) * (1.0f-_expf( -distance*rayDir.y*b ))/rayDir.y;
    
    float3  fogColor  = to_float3(1.0f, 1.0f, 1.0f);//Sky(rayDir, false, normalize(light)) * 1.0f;//
    
    return _mix( rgb, fogColor, fogAmount );
}

__KERNEL__ void QuartzWipRebuildFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float iChannelTime[], sampler2D iChannel0, sampler2D iChannel1)
{


    timeline.songTime     = texture(iChannel0, (make_float2(to_int2(0, 0))+0.5f)/R).x;
    timeline.rPatternTime = texture(iChannel0, (make_float2(to_int2(0, 0))+0.5f)/R).y;
    timeline.nPatternTime = texture(iChannel0, (make_float2(to_int2(0, 0))+0.5f)/R).z;
    timeline.smoothNPatternTime = texture(iChannel0, (make_float2(to_int2(0, 0))+0.5f)/R).w;
    
    timeline.patternNum   = texture(iChannel0, (make_float2(to_int2(1, 0))+0.5f)/R).x;
    
    float3 campos = swi3(texture(iChannel0, (make_float2(to_int2( 5, 0))+0.5f)/R),x,y,z);
    sdfMixer.grid =      texture(iChannel0, (make_float2(to_int2(11, 0))+0.5f)/R).x;
    sdfMixer.star =      texture(iChannel0, (make_float2(to_int2(13, 0))+0.5f)/R).y;
    sdfMixer.ifs1 =      texture(iChannel0, (make_float2(to_int2(15, 0))+0.5f)/R).x;
    
    
    //if (fragCoord.x < 100.0f) {
        //fragColor = to_float4(timeline.rPatternTime);
        //return;
    //}
    float  mat = 0.0f,
        camShY = 0.0f;
    
    //vol = (texture(iChannel0, to_float2(0.92f, 0.15f)).r) * 2.0f;
    
    float2 uv = (2.0f * fragCoord - iResolution) / iResolution.x * 1.5f;
    //if (length(uv) < 0.9f) pR(uv, PI);
    light = to_float3(0.0f, 0.0f, 100.0f);        
    float rr = timeline.songTime + (n(to_float3_s(timeline.songTime * 0.4f)) * 2.0f) - 0.5f;
    float3 vuv = to_float3(0.0f, 1.0f, 0.4f ), // up
            ro = campos;
    float3 vrp =  to_float3(0.0f, 0.0f, 0.0f),//+ ro,
    
      vpn = normalize(vrp - ro),
        u = normalize(cross(vuv, vpn)),
        rd = normalize( vpn + uv.x * u  + uv.y * cross(vpn, u) ),
        hit,
        ord = rd;
       
  
    float3 sceneColor = to_float3_s(1.0f);
    
    geometry tr = trace(ro, rd, 90, iTime);    
    
    //float fog = smoothstep(FAR * FOG, 0.0f, tr.dist) * 1.0f;
    hit = ro + rd * tr.dist;
  
    float odist = tr.dist;
    
    float3 sn = normal(hit);  
    
    // float sh = softShadow(hit, hit + light, 3.0f);
    
    float ao = getAO(hit, sn, 0.6f);
  
    //ao *= _saturatef(getAO(hit + sn * 0.2f, sn, 0.5f));
    //ao *= _saturatef(getAO(hit + sn * 1.0f, sn, 3.0f));
    
    float alpha = 1.0f;
    float3 sky = Sky(rd, true, normalize(light), ro);
    
    if (tr.dist < FAR) { 
        sceneColor = (doColor(hit, rd, sn, light, tr) * 1.0f) * 1.0f;
       // sceneColor *= ao; 
        //sceneColor *= sh;
        sceneColor = _mix(sceneColor, sky, _saturatef(tr.dist * 2.0f / FAR));
        //sceneColor = _mix(sceneColor, lightColour, 0.1f);        
        sceneColor *= 0.9f + to_float3(length(
            _fmaxf(
                to_float2_s(0.0f),
                0.7f * _fmaxf(
                0.0f,
                length(normalize(light.y) * _fmaxf(0.0f, sn.y))
                )
            )
        ));
        
        firstpass = false;
        
        glow *= 1.0f- timeline.nPatternTime;
        
        sceneColor += _powf(glow, 2.0f);
        
        if (glow < 1.0f) {
            if (tr.mirror > 0.0f) {   
                float mirror = tr.mirror;
                float3 refSceneColor = sceneColor;
                rd = reflect(rd, sn);// + _sinf(t));
                //hit += rd * 3.0f;
                //rd += n(rd * 3.0f) * 0.2f;
                //rd = normalize(rd);

                tr = trace(hit, rd, 69, iTime);
                hit = hit + rd * tr.dist;
                
                if (tr.dist < FAR) {
                    sn = normal(hit);
                    refSceneColor = _mix(sceneColor, _fabs(doColor(hit, rd, sn, light, tr)), mirror);                
                } else {
                    sky = _mix(Sky(rd, true, normalize(light), hit), to_float3_s(0.0f), 0.3f);
                    sky = Sky(rd, true, normalize(light), hit);
                    refSceneColor = _mix(refSceneColor, sky, mirror);
                }

                sceneColor = _mix(sceneColor, refSceneColor, mirror);
                
            } else {
                sceneColor = _mix(sceneColor, sky, 1.0f);
            }
        }
      sceneColor *= ao; 
      //sceneColor = _mix(sceneColor, to_float3_aw(sceneColor.x + sceneColor.y + sceneColor.z)/ 3.0f, odist / 10.0f);
    
      alpha = odist / 25.0f;        
    } else {
        alpha = 1.0f;
        sceneColor = sky;        
    }

    sceneColor += _powf(glow, 3.0f) * to_float3(1.0f, 0.5f, 0.0f);
    
    
    float3 bsceneColor = swi3(texture(iChannel1, (fragCoord + to_float2(1.0f, 0.0f)) / iResolution),x,y,z);
    
    sceneColor = _mix(sceneColor, bsceneColor, 0.2f);
    //sceneColor *= timeline.smoothNPatternTime;
    //if (length(uv) < 0.5f) sceneColor = 1.0f- sceneColor;//pR(uv, PI);
    fragColor = to_float4(clamp(sceneColor * (1.0f - length(uv) / 2.0f), 0.0f, 1.0f), 1.0f) * 1.4f;
    //fragColor = _powf(fragColor, 1.0f/to_float4_s(1.2f));
    fragColor.w = alpha;
    
    //swi3(fragColor,x,y,z) = to_float3(timeline.nPatternTime);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1




__DEVICE__ void compute(in int2 coord, out float4 *color, float3 U[2]) {
    float time = U[0].x,
          pt = mod_f(time, PATTERN_TIME),
          pt2 = mod_f(pt - PATTERN_TIME / 4.0f, PATTERN_TIME) / 3.0f,
          npt = pt / PATTERN_TIME,
          pn = _floor(time / PATTERN_TIME); // pattern num
                
    switch (coord.x) {
        // R - songtime, G - pattern time (1->0), B - pattern time(0->1), smootstep patterntime) 
        case 0:
          *color = to_float4(U[1].x, (PATTERN_TIME - pt) / PATTERN_TIME, npt, smoothstep(0.0f, 1.0f, pt / PATTERN_TIME));
            
          break;
        case 1:
            // R - pattern number;
          *color = to_float4(pn, 0.0f, 0.0f, 0.0f);
          break;
        case 5: 
          // RG - cam position 
          float2 x = swi2(texture(iChannel1, to_float2(0.5f, fract(time * 0.2f))),x,y) * 0.4f * npt;
          float2 r = time * 0.95f + x - _sinf(pt) * npt * 6.14f * x;
        
          r += pn * 0.5f;
        
          *color = to_float4(
                _sinf(r.x) * 29.0f, 
                -10.0f, 
                _cosf(r.x) * 26.0f, 
                0.
            );
          
          break;

        case 9:
          *color = to_float4_s(pt);
          break;
        case 11:
          /// sdf mixer GRID,
          *color = to_float4(pt, _sinf(pt), 0.0f, 0.0f);
          break;
        case 13: 
          break;
        
        case 15:
          /// sdf mixer IFS1
          *color = to_float4(_fmaxf(0.0f, 1.0f- pt2));
          break;
    }
}

__KERNEL__ void QuartzWipRebuildFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, int iFrame, float iChannelTime[], sampler2D iChannel1)
{

    float3 U[2] = {to_float3_s(0), to_float3_s(0)};

    U[1].x = (float)(iFrame);
    U[0].x = _fmaxf(22.0f, iTime);
    
    int2 store = to_int2_cfloat(fragCoord);
    float2 uv = fragCoord / swi2(U[0],y,z);   
    
    fragColor = to_float4_s(0.0f);

    float ck = _cosf(iTime * 0.1f) * 0.1f;
    int index = 0;

    if (store.y != 0 || store.x > 15) {        
        if (U[1].x < 10.0f) {
            fragColor.x = noiseLayers(to_float3_aw(fragCoord * 0.03f, 2.0f));
            fragColor.z = noiseFF(fragCoord * 0.01f);
            
            SetFragmentShaderComputedColor(fragColor);
            return;
        } else {
            SetFragmentShaderComputedColor(fragColor);
            return;
          //discard;
        }
    } else {
        compute(store, &fragColor, U);         
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


// Fork of "Quartz - wip - private" by patu. https://shadertoy.com/view/3tjXRW
// 2021-04-02 00:42:21

__DEVICE__ float2 hash( float2 p) { p=to_float2(dot(p,to_float2(127.1f,311.7f)),dot(p,to_float2(269.5f,183.3f))); return fract_f2(sin_f2(p)*18.5453f); }

// return distance, and cell id
__DEVICE__ float2 voronoi( in float2 _x, float iTime  )
{
    float2 n = _floor( _x );
    float2 f = fract_f2( _x );

    float3 m = to_float3_s( 8.0f );
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
      float2  g = to_float2( (float)(i), (float)(j) );
      float2  o = hash( n + g );
      //vec2  r = g - f + o;
      float2  r = g - f + (0.5f+0.5f*_sinf(iTime+6.2831f*o));
      float d = dot( r, r );
      if( d<m.x )
         m = to_float3( d, o.x, o.y );
    }

    return to_float2( _sqrtf(m.x), m.y+m.z );
}



__DEVICE__ float3 dof(__TEXTURE2D__ tex,float2 uv,float rad, float2 res)
{
    mat2 rot = to_mat2(_cosf(2.399f),_sinf(2.399f),-_sinf(2.399f),_cosf(2.399f));
    
    float3 acc=to_float3_s(0);
    float2 pixel=to_float2(0.002f*res.y/res.x,0.002f),angle=to_float2(0,rad);;
    rad=1.0f;
    for (int j=0;j<50;j++)
    {  
      rad += 1.0f/rad;
      angle = mul_f2_mat2(angle, rot);
      float4 col=texture(tex,uv+pixel*(rad-1.0f)*angle);
      acc+=swi3(col,x,y,z);
    }
  return acc/50.0f;
}

//-------------------------------------------------------------------------------------------
__KERNEL__ void QuartzWipRebuildFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 res = iResolution;
    float2 uv = fragCoord / swi2(res,x,y);
    float2 c = voronoi( (14.0f+6.0f*_fabs(_sinf(1.2f*iTime))) * uv / res.x * res.y / 4.0f);
    
    float cell = _fmaxf(0.0f, _floor(_fabs(_sinf(iTime)) - fract(c.y * 0.1f) + 0.1f));
    //uv += texelFetch(iChannel1, to_int2(swi2(gl_FragCoord,x,y)), 0).b * 0.04f;
    
    //float cell = _fmaxf(0.0f, _floor(_fabs(_sinf(iTime + fract(c.y * 0.1f) + 0.9f))));
    float sh = (cell / 50.0f) * texture(iChannel1, to_int2(9, 0)).x;//cell > 0.2f ? c.y * 0.5f : 0.0f;
    uv.x += sh * 3.0f;//* hash(uv).x * 2.2f;
    
    float a   = _fabs(0.7f - _powf(_tex2DVecN(iChannel0,uv.x,uv.y,15).w * 1.2f, 1.4f));
    fragColor = to_float4_aw(dof(iChannel0,uv,a, res),1.0f);
    fragColor = _mix(fragColor, _powf( _fmaxf(
                                       to_float4_s(0.0f), 
                                       1.0f-normalize(fragColor)), to_float4_s(4.0f)), sh * 40.0f);
    //swi3(fragColor,x,z,y) -= cell * 0.1f;
    //swi3(fragColor,x,y,z) -= to_float3(0.2f, 0.1f, 0.0f) * cell * 0.5f + (cell > 0.5f ? _powf(_fabs(cell * -c.x * 2.0f), 10.0f) : 0.0f);
    //fragColor = to_float4(a);
    
   // fragColor = texelFetch(iChannel1, to_int2(0, 0), 0).aaaa;


  SetFragmentShaderComputedColor(fragColor);
}