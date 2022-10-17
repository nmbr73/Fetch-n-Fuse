
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



//
//
// Stranger Tides   
//
// I like the kinda abstract and fluid looks that go along with when distributing and
// animating voronoi´s to surfaces. I have always wanted to get get it working in one of
// my 50 cents of shadertoy´ing. This mood is not perfect at all, but with surprising 
// ease to get this started...
//
//
// This shader shall exist in its/this form on shadertoy.com only 
// You shall not use this shader in any commercial or non-commercial product, website or project. 
// This shader is not for sale nor can´t be minted as NFT.
//
//
// Related examples
//
//
// IQ´s 2nd order voronoi algorithm:
// https://www.shadertoy.com/view/MsXGzM      
//
//
//


#define PI 3.14159265359
#define MAX_ITER_INTERNAL 0
#define MAX_DIST 10.0
#define MIN_DIST 0.01
#define FOV 1.05
#define FK(k) floatBitsToInt(_cosf(k))^floatBitsToInt(k)



struct ray {
  float4 c;
  float3 p;
  float3 d;
  float3 n;
  float t;
  int i;
};




//////////////////////////////////////////////////////////////////////////////////////
// x,y,z rotation(s)
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ mat3 rotx(float a) {
  float c=_cosf(a);
  float s=_sinf(a);
  return to_mat3(1.0f,0.0f,0.0f,0.0f,c,-s,0.0f,s,c);
}
__DEVICE__ mat3 roty(float a) {
  float c=_cosf(a);
  float s=_sinf(a);
  return to_mat3(c,0.0f,s,0.0f,1.0f,0.0f,-s,0.0f,c);
}
__DEVICE__ mat3 rotz(float a) {
  float c=_cosf(a);
  float s=_sinf(a);
  return to_mat3(c,-s,0.0f,s,c,0.0f,0.0f,0.0f,1.0f);
}
__DEVICE__ mat3 rot(float3 z,float a) {
  float c=_cosf(a);
  float s=_sinf(a);
  float b=1.0f-c;
  return to_mat3( b*z.x*z.x+c,b*z.x*z.y-z.z*s,b*z.z*z.x+z.y*s,
                  b*z.x*z.y+z.z*s,b*z.y*z.y+c,b*z.y*z.z-z.x*s,
                  b*z.z*z.x-z.y*s,b*z.y*z.z+z.x*s,b*z.z*z.z+c);
}
__DEVICE__ mat2 rot2d(float a) {
  float c=_cosf(a);
  float s=_sinf(a);
  return to_mat2(c,-s, s, c);
}




//////////////////////////////////////////////////////////////////////////////////////
// smooth minimum(s) -> https://iquilezles.org/www/articles/smin/smin.htm
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ float smin(float a,float b,float k) {
  float h=clamp(0.5f+0.5f*(b-a)/k,0.0f,1.0f);
    return _mix(b,a,h)-k*h*(1.0f-h);
}

//Smooth min by IQ
__DEVICE__ float smin( float a, float b ) {
  float k = 0.95f;
  float h = clamp( 0.5f + 0.5f*(b-a)/k, 0.0f, 1.0f );
  return _mix( b, a, h ) - k*h*(1.0f-h);
}


//////////////////////////////////////////////////////////////////////////////////////
// randomness and hash´ing like a pro
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ void srand(float2 p, float *s) {
  *s=_sinf(dot(p,to_float2(423.62431f,321.54323f)));
}

__DEVICE__ float rand(float *s) {
  *s=fract(*s*32322.65432f+0.12333f);
  return _fabs(fract(*s));
}

__DEVICE__ float hash( in float3 p ) {
    return fract(_sinf(p.x*15.32758341f+p.y*39.786792357f+p.z*59.4583127f+7.5312f) * 43758.236237153f)-0.5f;
}


//#define FK(k) floatBitsToInt(_cosf(k))^floatBitsToInt(k)

// floatBitsToUint() und  uintBitsToFloat()
union Zahl
 {
   float  _Float; //32bit float
   int   _Int;  //32bit unsigend integer
 };


__DEVICE__ float hash(float2 k) {
  
  Zahl z;
  //int _x = FK(k.x);
  z._Float = k.x;
  int _x = z._Int;
  
  
  //int _y = FK(k.y);
  z._Float = k.y;
  int _y = z._Int;
  
  return (float)((_x*_x-_y)*(_y*_y+_x)-_x)/3.14e9;
}

__DEVICE__ float hash3(float3 k) {
  float h1 = hash(swi2(k,x,y));
  return hash(to_float2(h1, k.z));
}


__DEVICE__ float3 hash33(float3 k) {
  float h1 = hash3(k);
  float h2 = hash3(k*h1);
  float h3 = hash3(k*h2);
  return to_float3(h1, h2, h3);
}


//////////////////////////////////////////////////////////////////////////////////////
// As in IQ´s rocks
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ float3 voronoi3d(const in float3 _x) {

  float3 p = _floor(_x);
  float3 f = fract_f3(_x);

  float id = 0.0f;
  float2 res = to_float2_s(100.0f);
  for (int k = -1; k <= 1; k++) {
    for (int j = -1; j <= 1; j++) {
      for (int i = -1; i <= 1; i++) {
        float3 b = to_float3((float)(i), (float)(j), (float)(k));
        float3 r = (b) - f + hash(p + b );
         //r.x *= 0.5f + 0.5f*_sinf( iTime );
         //r.z *= 0.5f + 2.5f*_cosf( iTime );
         //r *= rot( r, iTime );


        float d = dot(r, r);

        float cond = _fmaxf(sign_f(res.x - d), 0.0f);
        float nCond = 1.0f - cond;

        float cond2 = nCond * _fmaxf(sign_f(res.y - d), 0.0f);
        float nCond2 = 1.0f - cond2;

        id = (dot(p + b, to_float3(1.0f, 57.0f, 113.0f)) * cond) + (id * nCond);
        res = to_float2(d, res.x) * cond + res * nCond;

        res.y = cond2 * d + nCond2 * res.y;
      }
    }
  }
  return to_float3_aw(sqrt_f2(res), _fabs(id));
}


//////////////////////////////////////////////////////////////////////////////////////
// Voronoi 2d
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ float voronoi2d(float2 p,float t, float iTime) {
  float v=0.0f;
  float s;
  float2 f=_floor(p)+to_float2_s(0.25f);
  for(float i=-3.0f;i<3.0f;i+=1.0f)
        for(float j=-3.0f;j<3.0f;j+=1.0f){
            srand(f+to_float2(i,j),&s);
            float2 o;
            o.x=rand(&s);
            o.y=rand(&s);
            o=mul_f2_mat2(o,rot2d(iTime*(rand(&s)-0.5f)));
            float r=distance_f2(p,f+to_float2(i,j)+o);
            v+=_expf(-16.0f*r);
        }
  return -smin((1.0f/16.0f)*_logf(v),-0.1f,0.1f);
}



//////////////////////////////////////////////////////////////////////////////////////
// Torus
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ float torus(inout ray *r, float iTime) {

    float3 vt = (*r).p + ((voronoi3d((*r).p * 2.6f + iTime * 0.1f).x)*0.5f + 0.5f);
    vt.x -= 0.5f;
    vt.y -= 1.5f;
    vt.z -= 0.60f;
    vt = mul_f3_mat3(vt,rotz( _sinf(iTime * 0.5f) ));
    vt = mul_f3_mat3(vt,rotx( _cosf(iTime * 0.25f) ));
    return length(to_float2(length(swi2(vt,x,y)) -1.74f, vt.z)) -0.55f;  

}



//////////////////////////////////////////////////////////////////////////////////////
// Dome
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ float dome(inout ray *r, float iTime) {
  float v=voronoi2d(swi2((*r).p,x,y),(*r).t,iTime);
  float d=dot((*r).p,to_float3(0.0f,0.0f,0.75f))+0.5f-0.5f*v;
  if(d<0.0f){
    (*r).c=to_float4_s(1.0f);
    (*r).i=MAX_ITER_INTERNAL+1;
  }
  return d;
}



//////////////////////////////////////////////////////////////////////////////////////
// Bubbles
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ float3 sphercoord(float2 p) {
  float l1 = _acosf(p.x);
  float l2 = _acosf(-0.5f)*p.y;
  return to_float3(_cosf(l1), _sinf(l1)*_sinf(l2), _sinf(l1)*_cosf(l2));
}

__DEVICE__ float3 erot(float3 p, float3 ax, float ro) {
  return _mix(dot(p,ax)*ax, p, _cosf(ro)) + _sinf(ro)*cross(p,ax);
}

__DEVICE__ float comp(float3 p, float3 ro, float t) {
  float3 ax = sphercoord(swi2(ro,x,y));
  p.z -= t;
  p = erot(p, ax, ro.z*_acosf(-1.0f));
  float scale = 7.0f + hash(swi2(ro,x,z))*0.25f+0.25f;
  p = (fract_f3(p/scale)-0.65f)*scale;
  return length(p) - 0.2f;
}



//////////////////////////////////////////////////////////////////////////////////////
// distance field
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ float dist(inout ray *r,float iTime) {
  
  float d=MAX_DIST;
  float _dist = d;
  
  for( int i = 0; i < 4; i++ ) {
       float3 _rot = hash33(to_float3((float)(i+1), _cosf((float)(i)), _sinf((float)(i))));
       float d_ = comp((*r).p, _rot, iTime/2.0f*((float)(i+1)));
       _dist = smin(_dist, d_, 0.5f);
  }
   
  d=smin(d,dome(r,iTime));
  d=smin(d,smin(torus(r,iTime),_dist, 0.2f));
    
  return d;

}



__DEVICE__ float4 trace(inout ray *r, float iTime) {
  (*r).c=to_float4_s(1.0f);
  for(int i=0;i<32;i++){
    float d=dist(r,iTime);
    if((*r).i>MAX_ITER_INTERNAL)break;
    (*r).p+=(*r).d*_fmaxf(d,MIN_DIST);
  }
    
  return to_float4_s(2.0f/_expf(_fabs((*r).p.z)));
}




//////////////////////////////////////////////////////////////////////////////////////
// particles ( --> by Andrew Baldwin)
//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ float particles(float3 direction, float2 fragCoord, float2 iResolution, float iTime)
{
  float help = 0.0f;
  const mat3 p = to_mat3(13.323122f,23.5112f,21.71123f,21.1212f,28.7312f,11.9312f,21.8112f,14.7212f,61.3934f);
  float2 uvx = to_float2(direction.x,direction.z)-to_float2(1.0f,iResolution.y/iResolution.x)*fragCoord / iResolution;

  float acc = 0.0f;
  float DEPTH = direction.y*direction.y-0.3f;
  float WIDTH =0.1f;
  float SPEED = 0.09f;
  for (int i=0;i<10;i++) 
  {
    float fi = (float)(i);
    float2 q = uvx*(1.0f+fi*DEPTH);

    q += to_float2(q.y*(WIDTH*mod_f(fi*7.238917f,1.0f)-WIDTH*0.5f),SPEED*iTime/(1.0f+fi*DEPTH*0.03f));
    float3 n = to_float3_aw(_floor(q),31.189f+fi);
    float3 m = _floor(n)*0.00001f + fract_f3(n);
    float3 mp = (31415.9f+m)/fract_f3(mul_mat3_f3(p,m));
    float3 r = fract_f3(mp);
    float2 s = abs_f2(mod_f2(q,1.0f)-0.5f+0.9f*swi2(r,x,y)-0.45f);
    float d = 0.7f*_fmaxf(s.x-s.y,s.x+s.y)+_fmaxf(s.x,s.y)-0.01f;
    float edge = 0.04f;
    acc += smoothstep(edge,-edge,d)*(r.x/1.0f);
    help = acc;
  }
  return help;
}



__KERNEL__ void StrangerTidesJipi742Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

  CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
  
  float t = iTime;
  float r = iResolution.x/iResolution.y;
    
  float2 m=to_float2( (iMouse.x-iResolution.x/2.0f)/iResolution.x*r,
                      (iMouse.y-iResolution.y/2.0f)/iResolution.y);
  float2 s=to_float2( (fragCoord.x-iResolution.x/2.0f)/iResolution.x*r,
                      (fragCoord.y-iResolution.y/2.0f)/iResolution.y);      

  float3 l=to_float3(0.0f,0.0f,0.0f);
  float3 tmp=to_float3(2.0f,3.0f,2.0f);//2.0
  tmp=mul_f3_mat3(tmp,roty((PI*m.y)/4.0f-PI/8.0f));
  tmp=mul_f3_mat3(tmp,rotz(2.0f*PI*m.x));

  float3 e=l+tmp;
  float3 u=to_float3(0.0f,0.0f,1.0f);
  float3 d=normalize(l-e);
  float3 h=normalize(cross(d,u));
  float3 v=normalize(cross(h,d));
  float f=0.75f;
  d=mul_f3_mat3(d,rot(v,FOV*s.x));
  d=mul_f3_mat3(d,rot(h,FOV*s.y));
  ray a={to_float4_s(0.0f),e,d,to_float3_s(0.0f),t,0};

  float4 col = trace(&a,iTime);
  col.x -= 0.98f;
  col.y -= 0.48f;
  col.z -= 0.018f;
    
  float2 position = fragCoord / iResolution;   
  position.y *=-1.0f;

  col *= 0.27f + 5.5f * position.x * position.y * ( 1.0f - position.x ) * ( -1.0f - position.y );
  swi3S(col,x,y,z, swi3(col,x,y,z) + particles(swi3(a.c,x,y,z),fragCoord,iResolution,iTime));
  
  float2 rnd1  = to_float2(12.9898f,78.233f);
  float rnd2 = 43758.5453f;
  float c = fract(_sinf(dot(swi2(s,x,y) ,rnd1)) * rnd2+(iTime*0.5f));
  swi3S(col,x,y,z, (swi3(col,x,y,z)*0.85f)+(swi3(col,x,y,z)*0.12f*to_float3_s(c)));    

  //swi3(col,x,y,z) *= 1.1f;
  col.x *= 1.1f;
  col.y *= 1.1f;
  col.z *= 1.1f;
  

  fragColor = col * col * 2.1f;

  fragColor.w = Alpha;  
  
  SetFragmentShaderComputedColor(fragColor);
}