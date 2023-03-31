
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Audio' to iChannel0


/** 
    License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License
    
    AudioFlight  v2 - music Boris Brejcha - Gravity
    4/14/22 @byt3_m3chanic
    
    Path shader based around @Shane's stuff - he has a ton of amazing ones.
    https://www.shadertoy.com/view/MlXSWX
    
    Music EQ based around @blackle's domain rep tricks
    
    Lots of fo

*/
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R           iResolution
#define T           iTime
#define M           iMouse

#define PI2         6.28318530718f
#define PI          3.14159265358f

#define MINDIST     0.0001f
#define MAXDIST     125.0f

#define r2(a) to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))


__DEVICE__ float _powcf(float x, float y) {
    float ret = _powf(x,y);
    if (isnan(ret)) {
        ret = 0.0001f;
    }
    return ret;
}



__DEVICE__ float hash21(float2 p){  return fract(_sinf(dot(p, to_float2(27.609f, 57.583f)))*43758.5453f); }
__DEVICE__ float sampleFreq(float freq, __TEXTURE2D__ iChannel0) {
    return texture(iChannel0, to_float2(freq, 0.1f)).x;
}





//http://mercury.sexy/hg_sdf/
__DEVICE__ float pMod(inout float *p, float size) {
  float c = _floor((*p + size*0.5f)/size);
  *p = mod_f(*p + size*0.5f, size) - size*0.5f;
  //*p = fmodf(*p + size*0.5f, size) - size*0.5f;
  return c;
}
__DEVICE__ float2 _pMod(inout float2 *p, float size) {
  float2 c = _floor((*p + size*0.5f)/size);
  *p = mod_f2(*p + size*0.5f, size) - size*0.5f;
  return c;
}
__DEVICE__ float3 pMod_f3(inout float3 *p, float size) {
  float3 c = _floor((*p + size*0.5f)/size);
  *p = mod_f3(*p + size*0.5f, size) - size*0.5f;
  //*p = fmodf(*p + size*0.5f, to_float3_s(size)) - size*0.5f;
  return c;
}
__DEVICE__ float2 pModPolar(in float2 p, float repetitions) {
    float angle = 2.0f*PI/repetitions;
    float a = _atan2f((p).y, (p).x) + angle/2.0f,
          r = length(p),
          c = _floor(a/angle);
    a = mod_f(a,angle) - angle/2.0f;
    p = to_float2(_cosf(a), _sinf(a))*r;
    //if (_fabs(c) >= (repetitions/2.0f)) c = _fabs(c);
    return p;
}
__DEVICE__ float vmax(float2 v) {  return _fmaxf(v.x, v.y);            }
__DEVICE__ float vmax_f3(float3 v) {  return _fmaxf(max(v.x, v.y), v.z);        }
__DEVICE__ float fBox(float3 p, float3 b) {
  float3 d = abs_f3(p) - b;
  return length(_fmaxf(d, to_float3_s(0))) + vmax_f3(_fminf(d, to_float3_s(0)));
}
__DEVICE__ float fBox2(float2 p, float2 b) {
  float2 d = abs_f2(p) - b;
  return length(_fmaxf(d, to_float2_s(0))) + vmax(_fminf(d, to_float2_s(0)));
}
//@iq
__DEVICE__ float sdCap( float3 p, float h, float r ){
  p.y -= clamp( p.y, 0.0f, h );
  return length( p ) - r;
}
// @Shane - https://www.shadertoy.com/view/MlXSWX
__DEVICE__ float2 path(in float z){ 
    float2 p1 =to_float2(2.35f*_sinf(z * 0.125f)+2.38f*_cosf(z * 0.25f), 3.5f*_cosf(z * 0.0945f));
    float2 p2 =to_float2(3.2f*_sinf(z * 0.19f), 4.31f*_sinf(z * 0.125f)-2.38f*_cosf(z * 0.115f));
    return (p1 - p2)*0.3f;
}

__DEVICE__ float2 fragtail(float3 pos, float scale, float3 cxz) {
    float ss=1.15f;
    float r = 1e5;
    
    for (int i = 0;i<2;i++) {
        pos=abs_f3(pos);
        if ( pos.x- pos.y<0.0f) swi2S(pos,y,x, swi2(pos,x,y));
        if ( pos.x- pos.z<0.0f) swi2S(pos,z,x, swi2(pos,x,z));
        if ( pos.y- pos.z<0.0f) swi2S(pos,z,y, swi2(pos,y,z));
        
        pos.x=scale * pos.x-cxz.x*(scale-1.0f);
        pos.y=scale * pos.y-cxz.y*(scale-1.0f);
        pos.z=scale * pos.z;
        
        if (pos.z>0.5f*cxz.z*(scale-1.0f)) pos.z-=cxz.z*(scale-1.0f);

        r = fBox2(swi2(pos,x,y),to_float2(5,1.5f+0.25f*_sinf(pos.x*5.0f)))-0.0015f;
        ss*=1.0f/scale;
    }

    return to_float2(r*ss,1.0f);
}

//@blackle domain rep https://www.shadertoy.com/view/Wl3fD2 
__DEVICE__ float2 edge(float2 p) {
    float2 p2 = abs_f2(p);
    if (p2.x > p2.y) return to_float2((p.x < 0.0f) ? -1.0f : 1.0f, 0.0f);
    else             return to_float2(0.0f, (p.y < 0.0f) ? -1.0f : 1.0f);
}

//#define mod_f fmodf

// scene map
__DEVICE__ float2 map (in float3 p, float sg, __TEXTURE2D__ iChannel0, float scale, float3 cxz, float travelSpeed, mat2 r4, mat2 r5, out float3 *g_hp, out float *ga, out float *iqd, float tm, float time, out float *beams, out float *flight, out float *glow, out float *objglow, float ths, float offWobble) {
  
    float2 res = to_float2(100.0f,-1.0f);
    float msize = 7.25f;
    
    // set path(s) vector(s)
    float2 tun = swi2(p,x,y) - path(p.z);
    float3 q = to_float3_aw(tun,p.z);
    float3 o = to_float3_aw(tun+to_float2(0.0f,0.0f),p.z+travelSpeed+4.25f);
   
    float3 s = q;

    swi2S(o,z,x, mul_f2_mat2(swi2(o,z,x),r5));
    swi2S(o,y,z, mul_f2_mat2(swi2(o,y,z),r4));
    o = abs_f3(o)-(offWobble*0.25f);
    float obj = fBox(o,to_float3_s(0.15f*offWobble))-0.015f;
    if(obj<res.x ) {
        res = to_float2(obj,11.0f);
        *g_hp=o;
    }
 
    // mods and vectors
    float pid = _floor((q.z+(msize/2.0f))/msize);
    float trackmod = mod_f(pid,18.0f);
    float deg = trackmod<12.0f ? trackmod<6.0f ? 4.0f : 6.0f : 10.0f;
    swi2S(q,x,y, pModPolar(swi2(q,x,y),deg));
    swi2S(s,x,y, pModPolar(swi2(s,x,y),deg*0.5f));    
    
    float3 r =s;
    float3 fs=s-to_float3(2.85f,0,0);
    r = to_float3(_fabs(r.x),_fabs(r.y),r.z);

    // audio bards
    fs.z*=2.0f;
    float2 center = _floor(swi2(fs,x,z)) + 0.5f;
    float2 neighbour = center + edge(swi2(fs,x,z) - center);

    float chs = _floor(center.y);
    float bmod = mod_f(chs,16.0f);

    float height = (sampleFreq(bmod*0.0465f, iChannel0));
    height=smoothstep(0.001f,1.0f,height);
    
    *ga=height;
    
    float tmp = s.z;
    //float ids = pMod(&(s.z),msize);
    float ids = pMod(&tmp,msize);
    s.z = tmp;
    
    float3 qid = pMod_f3(&q,msize);
    float ld = mod_f(ids,6.0f);
    float lq = mod_f(ids,2.0f);    

    *iqd=qid.x;

    float zprs= mod_f(chs, tm <8.0f? tm <4.0f? tm <4.0f? 2.0f: 2.0f: 5.0f: _floor(height*1.45f));

    float d4a = length(swi2(r,x,y)-to_float2(2.5f,1.75f))-0.1f;
    float d4 =  length(swi2(r,x,y)-to_float2(2.5f,1.75f))-0.04f+0.027f+0.027f*_sinf(r.z-time*4.5f);
    if(d4<res.x ) {
        res = to_float2(d4,12.0f);
        *g_hp = p;
    }
   
    // fractal
    float2 d1 = fragtail(q,scale,cxz);
    d1.x = _fmaxf(d1.x,-d4a);
 
    s.z=_fabs(s.z);
    float blt = sdCap(s-to_float3(2.45f,-0.58f,2.725f),1.16f ,0.015f);
    if(lq<2.0f) d1.x = _fminf(blt,d1.x);
    if(d1.x<res.x) {
        res = swi2(d1,x,y);
        *g_hp = p;
    }
    
    float me =   fBox(fs-to_float3(0,0,center.y),   to_float3(0.05f,0.150f+height,0.25f));
    float next = fBox(fs-to_float3(0,0,neighbour.y),to_float3(0.05f,0.001f+height,0.25f));
    float dlt = _fminf(me, next);
    if(dlt<res.x) {
        //float mid= zprs<4.? zprs<3.? zprs<2.? 3.0f : 4.0f : 4.0f  : 3.0f;
        res = to_float2(dlt,4.0f); //tm <8.0f ? mid : 4.0f);
        *g_hp = p;
    }

    if(sg==1.0f)            *beams += 0.0001f/(0.000003f+d4*d4);
    if(sg==1.0f&&lq<1.0f)   *flight += 0.00025f/(0.0000001f+blt*blt);
    if(sg==1.0f&&zprs<0.1f) *glow += 0.00015f/(0.000002f+dlt*dlt);
    if(sg==1.0f&&tm<ths)    *objglow += 0.0005f/(0.0005f+obj*obj);
        
    return res;
}

__DEVICE__ float2 marcher(float3 ro, float3 rd, int maxstep, float sg, __TEXTURE2D__ iChannel0, float scale, float3 cxz, float travelSpeed, mat2 r4, mat2 r5, out float3 *g_hp, out float *ga, out float *iqd, float tm, float time, out float *beams, out float *flight, out float *glow, out float *objglow, float ths, float offWobble){
    float d =  0.0f,
          m = -1.0f;
        for(int i=0;i<maxstep;i++){
            float3 p = ro + rd * d;
            float2 t = map(p,sg,iChannel0,scale,cxz,travelSpeed,r4,r5,g_hp,ga,iqd,tm,time,beams,flight,glow,objglow,ths,offWobble);
            if(_fabs(t.x)<d*MINDIST||d>MAXDIST)break;
            d += i<42? t.x*0.35f : t.x;
            m  = t.y;
        }
    return to_float2(d,m);
}

__DEVICE__ float3 normal(float3 p, float t, __TEXTURE2D__ iChannel0, float scale, float3 cxz, float travelSpeed, mat2 r4, mat2 r5, out float3 *g_hp, out float *ga, out float *iqd, float tm, float time, out float *beams, out float *flight, out float *glow, out float *objglow, float ths, float offWobble) {
    float e = MINDIST*t;
    float2 h = to_float2(1,-1)*0.5773f;
    return normalize( 
                    swi3(h,x,y,y)*map( p + swi3(h,x,y,y)*e,0.0f,iChannel0,scale,cxz,travelSpeed,r4,r5,g_hp,ga,iqd,tm,time,beams,flight,glow,objglow,ths,offWobble).x + 
                    swi3(h,y,y,x)*map( p + swi3(h,y,y,x)*e,0.0f,iChannel0,scale,cxz,travelSpeed,r4,r5,g_hp,ga,iqd,tm,time,beams,flight,glow,objglow,ths,offWobble).x + 
                    swi3(h,y,x,y)*map( p + swi3(h,y,x,y)*e,0.0f,iChannel0,scale,cxz,travelSpeed,r4,r5,g_hp,ga,iqd,tm,time,beams,flight,glow,objglow,ths,offWobble).x + 
                    swi3(h,x,x,x)*map( p + swi3(h,x,x,x)*e,0.0f,iChannel0,scale,cxz,travelSpeed,r4,r5,g_hp,ga,iqd,tm,time,beams,flight,glow,objglow,ths,offWobble).x );
}

//iq of hsv2rgb
__DEVICE__ float3 hsv2rgb( in float3 c ) {
    float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );
    return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

__KERNEL__ void AudioFlightv2JipiFuse(float4 O, float2 F, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_COLOR0(CXZ, 3.15f, 4.75f, 3.0f, 1.0f);
    
    CONNECT_COLOR1(Color1, 0.212f, 0.671f, 0.576f, 1.0f);
    CONNECT_COLOR2(Color2, 0.757f, 0.686f, 0.341f, 1.0f);
    CONNECT_COLOR3(Color3, 0.882f, 0.459f, 0.867f, 1.0f);
    
    CONNECT_SLIDER0(scale, -1.0f, 10.0f, 3.0f);
    CONNECT_SLIDER1(ths, -1.0f, 20.0f, 13.25f);
    
    CONNECT_SLIDER2(TM1, -1.0f, 2.0f, 0.3f);
    CONNECT_SLIDER3(TM2, -1.0f, 30.0f, 18.0f);
    CONNECT_SLIDER4(TravelSpeed, -1.0f, 20.0f, 5.0f);
    
    CONNECT_SLIDER5(OffWobble1, -1.0f, 5.0f, 1.5f);
    CONNECT_SLIDER6(OffWobble2, -1.0f, 5.0f, 1.15f);
    CONNECT_SLIDER7(OffWobble3, -1.0f, 1.0f, 0.1f);
    
    CONNECT_SLIDER8(BEAMS, -1.0f, 2.0f, 0.65f);
    CONNECT_SLIDER9(FLIGHT, -1.0f, 2.0f, 0.75f);
    CONNECT_SLIDER10(GLOW, -1.0f, 2.0f, 0.7f);
    CONNECT_SLIDER11(OBJGLOW, -1.0f, 2.0f, 0.65f);
    
    CONNECT_SLIDER12(Crop, -0.21f, 0.11f, 0.0f);


    // globals
    //float time,tm,travelSpeed;

    // globals and stuff
    float glow=0.0f,iqd=0.0f,flight=0.0f,beams=0.0f,gcolor=0.0f,objglow=0.0f,boxsize=0.0f;
    float ga=0.0f,sa=0.0f,slp=0.0f;
    float3 g_hp=to_float3_s(0.0f),s_hp=to_float3_s(0.0f);
    //mat2 r4,r5;

    //const float3 cxz = to_float3(3.15f,4.75f,3.0f);
    float3 cxz = swi3(CXZ,x,y,z);//to_float3(3.15f,4.75f,3.0f);
    //const float scale = 3.0f;

    //float ths= 13.25f;
    
    float2 t = to_float2_s(0.0f);

    // precal
    float time = iTime;
    float tm = mod_f(time*0.3f, 18.0f);
    //float travelSpeed = (time * 5.0f);
    float travelSpeed = (time * TravelSpeed);
    
    //float offWobble = 1.5f+1.15f*_sinf(tm+time*0.1f);
    float offWobble = OffWobble1+OffWobble2*_sinf(tm+time*OffWobble3);
    
    mat2 r4 =r2(time);
    mat2 r5 =r2(time);
    
    // pixel screen coordinates
    float2 uv = (swi2(F,x,y) - swi2(R,x,y)*0.5f)/_fmaxf(R.x,R.y);
    float3 C = to_float3_s(0.0f),
          FC = to_float3_s(0.03f);

    float crop = clamp((-0.05f)+(T*0.05f),0.0f,0.18f) + Crop;
    if(uv.y<crop&&uv.y>-crop){
      float3 lp = to_float3(0.0f,0.0f,0.0f-travelSpeed);
      float3 ro = to_float3(0.0f,0,0.15f);

      // mouse
      float x = (M.x==0.0f & M.y==0.0f) || M.z<0.0f ? 0.0f: (M.y/R.y*1.0f-0.5f)*PI;
      float y = (M.x==0.0f & M.y==0.0f) || M.z<0.0f ? 0.0f:-(M.x/R.x*2.0f-1.0f)*PI;

      swi2S(ro,z,y, mul_f2_mat2(swi2(ro,z,y) , r2(x)));
      
      ro +=lp; 

      swi2S(lp,x,y, swi2(lp,x,y) + path(lp.z));
      swi2S(ro,x,y, swi2(ro,x,y) + path(ro.z));

      // set camera
      float3 f=normalize(lp-ro),
           r=normalize(cross(to_float3(0,1,0),f)),
           u=normalize(cross(f,r)),
           c=ro+f*0.183f,
           i=c+uv.x*r+uv.y*u,
           rd=i-ro;

      // center tracking
      swi2S(rd,x,y, mul_mat2_f2(r2( (0.2f*_sinf(time*0.3f))-path(lp.z).x/ 24.0f ) , swi2(rd,x,y)));
      swi2S(rd,x,z, mul_mat2_f2(r2( y-path(lp.z+1.0f).y/ 14.0f ) , swi2(rd,x,z)));

      // march
            t = marcher(ro,rd, 164,1.0f,iChannel0,scale,cxz,travelSpeed,r4,r5,&g_hp,&ga,&iqd,tm,time,&beams,&flight,&glow,&objglow,ths,offWobble);
      float d = t.x,
            m = t.y;
      s_hp=g_hp;
      
      // if visible 
      if(d<MAXDIST)
      {
          float3 p = ro+rd*d;
  
          float3 n = normal(p,d,iChannel0,scale,cxz,travelSpeed,r4,r5,&g_hp,&ga,&iqd,tm,time,&beams,&flight,&glow,&objglow,ths,offWobble);
          float3 lpos = to_float3(0,0,0.25f)-p;
          lpos +=lp;
          swi2S(lpos,x,y, swi2(lpos,x,y) + path(lpos.z));
          float3 l = normalize(lpos);
          
          float diff = clamp(dot(n,l),0.01f,1.0f);

          float spec = _powcf(_fmaxf(dot(reflect(l,n),rd),0.01f),24.0f);

          float3 h = m==11.0f ? to_float3_s(0.005f): to_float3_s(1.0f);
          if(m==3.0f||m==4.0f) h = to_float3_s(0.012f);
          if(tm>ths) {
              C = (h * diff + spec);
          } else {
              if(m==3.0f||m==4.0f) C = (hsv2rgb(to_float3(s_hp.z*0.01f,0.8f,0.6f))  * diff);
          }
          
      } 
      
      if(tm>ths) {
          if(mod_f(T,0.1f)<0.05f) FC = to_float3_s(0.8f);
      }else{

        C += _fabs(glow*GLOW)*hsv2rgb(to_float3(s_hp.z*0.01f,0.8f,0.6f));
        C += _fabs(objglow*OBJGLOW)*to_float3(1,1,1);
      }
    C = _mix( C, FC, 1.0f-_expf(-0.000075f*t.x*t.x*t.x));
    C += _fabs(beams*BEAMS)*hsv2rgb(to_float3(s_hp.z*0.025f,0.8f,0.6f));
    C += _fabs(flight*FLIGHT)*to_float3(0.5f,1,0.2f);
    }
    
    
    float px = 1.0f/R.x;

            
    float d1 = fBox2(uv+to_float2(-0.485f,0.2675f),to_float2_s(0.005f))-0.002f;
    d1=smoothstep(px,-px,d1);
    //C=_mix(C,to_float3(0.212f,0.671f,0.576f),d1);
    C=_mix(C,swi3(Color1,x,y,z),d1);
  
    d1 = fBox2(uv+to_float2(-0.465f,0.2675f),to_float2_s(0.005f))-0.002f;
    d1=smoothstep(px,-px,d1);
    //C=_mix(C,to_float3(0.757f,0.686f,0.341f),d1);
    C=_mix(C,swi3(Color2,x,y,z),d1);
  
    d1 = fBox2(uv+to_float2(-0.445f,0.2675f),to_float2_s(0.005f))-0.002f;
    d1=smoothstep(px,-px,d1);
    //C=_mix(C,to_float3(0.882f,0.459f,0.867f),d1);
    C=_mix(C,swi3(Color3,x,y,z),d1);
    
    C = pow_f3(C, to_float3_s(0.4545f));
    O = to_float4_aw(C,Color1.w);

  SetFragmentShaderComputedColor(O);
}
