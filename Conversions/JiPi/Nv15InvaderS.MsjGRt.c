
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel0


#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Created by sebastien durand - 01/2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// ------------------------------------------------------------------------------
// Deep Space 
// Star Nest by Kali
// https://www.shadertoy.com/view/4dfGDM
// ------------------------------------------------------------------------------
// Metal effect
// Exterminate! by Antonalog
// https://www.shadertoy.com/view/ldX3RX
// Blue Spiral by donfabio
// ------------------------------------------------------------------------------
// Spiral effect
// BLue Spiral by donfabio
// https://www.shadertoy.com/view/lds3WB

//#define ANTIALIASING
// Anti-Aliasing Level
#define AA 3


#define NB_ITER 64
#define PI 3.14159265359f
#define TAO 6.28318531f
#define MAX_DIST 4000.0f
#define PRECISION 0.0001f

#define PLANET 200.0f
#define SHIP_GLOB 103.0f
#define SHIP_HUBLOT 104.0f
#define SHIP_TOP 500.0f
#define SHIP_BOTTOM 501.0f
#define SHIP_SIDE 502.0f
#define SHIP_ARM 505.0f
#define FLAG 300.0f

#define COS cos
#define SIN sin
#define ATAN _atan2f



//__DEVICE__ const float2 Ve = swi2(V01,y,x)*0.001f;




__DEVICE__ float C1, S1, C2, S2;//, time; 

//__DEVICE__ int AnimStep = 0;
//__DEVICE__ bool withPlanet = true;

// 0.1% error - enough for animations
__DEVICE__ float sin_(in float _x) {
  _x = mod_f(PI+_x,2.0f*PI) - PI;
  float s = _x*(1.27323954f - 0.4052847345f*_fabs(_x));
  return s*(0.776f + 0.224f*_fabs(s));
}

// 0.1% error - enough for animations
__DEVICE__ float cos_(in float _x) {
  return sin_(_x+PI*0.5f);
}
/*
__DEVICE__ float atan2_(float y, float x) {
  float t0, t1, t2, t3, t4;
  t3 = _fabs(x);
  t1 = _fabs(y);
  t0 = _fmaxf(t3, t1);
  t1 = _fminf(t3, t1);
  t3 = float(1) / t0;
  t3 = t1 * t3;
  t4 = t3 * t3;
  t0 =         - 0.013480470f;
  t0 = t0 * t4 + 0.057477314f;
  t0 = t0 * t4 - 0.121239071f;
  t0 = t0 * t4 + 0.195635925f;
  t0 = t0 * t4 - 0.332994597f;
  t0 = t0 * t4 + 0.999995630f;
  t3 = t0 * t3;
  t3 = (_fabs(y) > _fabs(x)) ? 1.570796327f - t3 : t3;
  t3 = (x < 0.0f) ?  3.141592654f - t3 : t3;
  return (y < 0.0f) ? -t3 : t3;;
}
*/

// k : [0..1]
__DEVICE__ float steps(in float _x, in float k) {
  float fr = fract(_x);
  return _floor(_x)+(fr<k?0.0f:(fr-k)/(1.0f-k));
}

__DEVICE__ float pyramid(in float _x) {
  float fr = fract(_x*0.5f+1.0f/16.0f);
  return clamp(4.0f*_fminf(fr,1.0f-fr)-1.0f,-0.75f,0.75f);
}



#define PHI 1.61803398874989484820459 // Φ = Golden Ratio 

__DEVICE__ float Noise(in float2 xy)
{
    float seed = 120293.09922f;
    return fract(_tanf(distance_f2(xy*PHI, xy)*seed)*xy.x);
}




__DEVICE__ float Noise_f3(in float3 _x, __TEXTURE2D__ iChannel0, float seed) {
  float3 p = _floor(_x);
  float3 f = fract_f3(_x);
  f = f*f*(3.0f-2.0f*f);
  
  float2 uv = (swi2(p,x,y)+to_float2(37.0f,17.0f)*p.z) + swi2(f,x,y) * seed;
  //float2 rg = swi2(texture( iChannel0, (uv+ 0.5f + Noise(swi2(_x,x,y)))),y,x);
  float2 rg = swi2(texture( iChannel0, (uv+ 0.5f)),y,x);
  return _mix( rg.x, rg.y, f.z );
}




/*
__DEVICE__ float Noise( in float2 x, __TEXTURE2D__ iChannel0 ) {
    float2 p = _floor(_x);
    float2 f = fract_f2(_x);
  float2 uv = swi2(p,x,y) + swi2(f,x,y)*swi2(f,x,y)*(3.0f-2.0f*swi2(f,x,y));
  return texture( iChannel0, (uv+118.4f)/256.0f).x;
}
*/
__DEVICE__ float smin( float a, float b, float k ) {
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix(b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ float Kaleido(inout float2 *v, in float nb){
  float id=_floor(0.5f+ATAN((*v).x,-(*v).y)*nb/TAO);
  float a = id*TAO/nb;
  float ca = COS(a), sa = SIN(a);
  *v = mul_f2_mat2(*v, to_mat2(ca,sa,-sa,ca));
  return id;
}

__DEVICE__ float2 Kaleido2(inout float3 *p, in float nb1, in float nb2, in float d) {
  float2 par = swi2(*p,y,x);
  float id1 = Kaleido(&par, nb1);
  (*p).y=par.x;(*p).x=par.y;
  par = swi2(*p,x,z);
  float id2 = Kaleido(&par, nb2*2.0f);
  (*p).x=par.x;(*p).z=par.y;
  (*p).z+=d;  
  return to_float2(id1,id2);
}

__DEVICE__ float2 minObj(float2 o1, float2 o2) {
  return o1.x<o2.x?o1:o2;
}

__DEVICE__ float2 sminObj(float2 o1, float2 o2, float k) {
  float d = smin(o1.x, o2.x, k);
  return to_float2(d, o1.x<o2.x?o1.y:o2.y);
}

__DEVICE__ float2 maxObj(float2 o1, float2 o2) {
  return o1.x>o2.x?o1:o2;
}


  
// ------------------------------------------------------------------------------
// Spiral texture
// Blue Spiral by donfabio
// https://www.shadertoy.com/view/lds3WB
__DEVICE__ float textureSpiral(float2 uv,float time) {
  float angle = ATAN(uv.y, uv.x),
  shear = length(uv),
  blur = 0.5f;
  return smoothstep(-blur, blur, cos_(8.0f * angle + 200.0f * time - 12.0f * shear));
}


__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r1, float r2) {
    float3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h ) - _mix(r1,r2,h);
}

__DEVICE__ float textureInvader(float2 uv) {
  float y = 7.0f-_floor((uv.y)*16.0f+4.0f);
  if(y < 0.0f || y > 7.0f) return 0.0f;
  float x = _floor((_fabs(uv.x))*16.0f);
  //  if(x < 0.0f || x > 14.0f) return 0.0f;
  float v=(y>6.5f)? 6.0f:(y>5.5f)? 40.0f:(y>4.5f)? 47.0f:(y>3.5f)? 63.0f:
          (y>2.5f)? 27.0f:(y>1.5f)? 15.0f:(y>0.5f)? 4.0f: 8.0f;
  return _floor(mod_f(v/_powf(2.0f,x), 2.0f)) == 0.0f ? 0.0f: 1.0f;
}

__DEVICE__ float4 DEFlag(float3 p) {
  float3 ba = to_float3(1.5f,0,0);
  float h = clamp( dot(p,ba)/dot(ba,ba), 0.0f, 1.0f );
  float2 d = to_float2(length( p - ba*h ) - 0.02f, 1.0f);
  p.y -= 0.4f;
  p.x -= 1.2f;
  float box = length(_fmaxf(abs_f3(p)-to_float3(0.3f,0.4f,0.005f),to_float3_s(0.0f)));
  d = _fminf(to_float2(box, FLAG), d);
  return to_float4(d.x, FLAG, p.y, p.x);
}


__DEVICE__ float DEAlienArm(float3 p0) {
  float3 p = p0;
  p.x = -p.x;
  float d = MAX_DIST;
  float dy, dx = _fabs(C1);
  dx = clamp(dx,0.0f,0.8f);
  dy = 0.5f*_sqrtf(1.0f-dx*dx);
  p.x-=dx;
  float _x = dx;
  p = abs_f3(p);
  d = _fminf(d, sdCapsule(p, to_float3(_x-dx,0,-dy), to_float3(_x, 0,dy),0.01f,0.01f));
  d = _fminf(d, sdCapsule(p, to_float3(_x,0.04f,-dy), to_float3(_x-dx,0.04f,dy),0.01f,0.01f));
  d = _fminf(d, length(swi2(p,x,z)+to_float2(_x-dx,-dy))-0.05f);
  d = _fminf(d, length(swi2(p,x,z)+to_float2(-_x,-dy))-0.05f);
  _x+=dx;
  d = _fminf(d, sdCapsule(p, to_float3(_x-dx,0,-dy), to_float3(_x-dx*0.5f, 0,0),0.01f,0.01f));
  d = _fminf(d, sdCapsule(p, to_float3(_x-dx*0.5f,0.04f,0.04f), to_float3(_x-dx,0.04f,dy),0.01f,0.01f));
  d = _fmaxf(p.y-0.06f,_fmaxf(-p.y+0.005f,d));
  return d;
}


__DEVICE__ float4 DEAlien(float3 p0, float time, int AnimStep) {
  float3 p=p0;
  float2 d = minObj(to_float2(length(p+to_float3(-0.125f,0,0))-0.75f, SHIP_GLOB),
          to_float2(length(p+to_float3(1.6f,0,0))-2.0f, SHIP_TOP));
  //swi2(p,y,z) = -_fabs(swi2(p,y,z));
  p.y = -_fabs(p.y);
  p.z = -_fabs(p.z);
  
  //swi2(p,y,z)+= 0.7f;
  p.y+= 0.7f;
  p.z+= 0.7f;
  
  d = minObj(to_float2(length(p)-0.24f, SHIP_GLOB),d);
  d = maxObj(to_float2(length(p0-to_float3(1.6f,0,0))-2.0f,SHIP_BOTTOM),d);
  p.x+= 0.1f;
  //swi2(p,y,z) = -_fabs(swi2(p,y,z));
  p.y = -_fabs(p.y);
  p.z = -_fabs(p.z);

  p.y+= 0.3f;
  p.z+= 0.3f;
  d = maxObj(to_float2(length(p0)-1.15f, SHIP_SIDE), sminObj(to_float2(length(p)-0.2f,SHIP_BOTTOM), d, 0.1f));

  float r=0.0f;
  if (AnimStep >= 6) {
    r = 0.0f; //0.05f*(1.0f-clamp(0.0f,1.0f,time-7.25f));
  } else if (AnimStep >= 3) { // ouverture du panneau
    r = 0.05f*clamp(0.0f,1.0f,time-5.25f);
  }
  if (r>0.0f) {
    float dd = length(_fmaxf(abs_f3(p0+to_float3(0.35f,0,0))-to_float3(r,r,9.0f*r),to_float3_s(0.0f)))-r;
    d = maxObj(to_float2(-dd,SHIP_SIDE), d); 
  }
  if (AnimStep == 4 || AnimStep == 5) {
    p0.x += 0.2f;
    d = minObj(to_float2(DEAlienArm(p0), SHIP_ARM), d);
  }
  
  return to_float4(d.x,d.y, p0.y,p0.z);
}

__DEVICE__ float DECrater(float3 p) {
  float d = MAX_DIST;
  float2 id = Kaleido2(&p, 9.0f,6.0f,2.0f);
  float _noise = Noise(id*10.0f);
  if (_noise<0.6f && _fabs(id.y)>0.0f&&_fabs(id.y)<6.0f-1.0f) {  
    d = sdCapsule(p, to_float3(0,0,-0.15f), to_float3(0,0,0.1f),0.1f+_noise*0.2f,0.1f+_noise*0.5f);
    d = _fmaxf(-(length(p-to_float3(0,0,-0.25f))-(0.1f+_noise*0.5f)),d);
    d = _fmaxf(-(length(p-to_float3(0,0,-0.05f))-(0.1f+_noise*0.15f)),d);
    d*=0.8f;
  }
  return d;
}

__DEVICE__ bool intersectSphere(in float3 ro, in float3 rd, in float3 c, in float r) {
  ro -= c;
  float b = dot(rd,ro), d = b*b - dot(ro,ro) + r*r;
  return (d>0.0f && -_sqrtf(d)-b > 0.0f);
}

// float4 : distance / id (object) / uv (texture) 
__DEVICE__ float4 DE(float3 p0,__TEXTURE2D__ iChannel0, float time, int AnimStep, bool withPlanet, float seed) {
  
  const mat2 Rot1 = to_mat2(0.54030230586f, 0.8414709848f, -0.8414709848f, 0.54030230586f);
  
  float scalePlanet = 10.0f,
          scaleFlag = 2.0f,
         scaleAlien = 0.5f;
  float4 res = to_float4_s(1000);  
  float3 p = p0;
  float d,d1,dx;

//    if (withPlanet) {
  p = p0;
  p.x+=2.0f;
  p*=scalePlanet;
  swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , Rot1));
  swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , to_mat2(C2,S2,-S2,C2)));
  if (withPlanet) {    
      d1 = DECrater(p);
    // Much better but cannot be render with the full scene in my computer
    //  swi2(p,x,z) *= Rot1;
    //  swi2(p,x,y) *= Rot1;
    //  float d2 = DECrater(p);
    d = smin(length(p)-2.0f,d1,0.15f); //smin(d2, d1,0.2f),0.15f);

    d += 0.1f*Noise_f3((p)*2.0f, iChannel0, seed);
    d += 0.005f*Noise_f3((p)*20.0f,iChannel0, seed);
    
    res = to_float4(d/=scalePlanet,PLANET, length(p), p.z);
  }
    
  if (AnimStep >= 4) {
    dx = _fabs(C1);
    dx = clamp(dx,0.1f,0.8f);
    
    if (AnimStep == 4) {
      p = p0;
      p.x += (2.5f*dx/scaleAlien) - 2.1f;
    } else {
      p /= scalePlanet;
      //p.x-=1.0f;
    }
    p = p*scaleFlag;
    float4 dFlag = DEFlag(p);
    dFlag.x /= scaleFlag;
    res = (dFlag.x<res.x) ? dFlag: res;
  }
  
  if (AnimStep > 1 && AnimStep < 7) {

    p = p0;
    if (AnimStep < 3) {
      p.x -= 3.2f-steps(10.0f*(0.038f+time-5.25f),0.75f);
      p.z -= 2.0f* /*floor*/(5.0f*pyramid(10.0f*(0.038f+time-5.25f)));
    } else if(AnimStep>5) {
      p.x -= 3.2f-steps(10.0f*(0.038f+6.75f-time),0.75f);
      p.z -= 2.0f* /*floor*/(5.0f*pyramid(10.0f*(0.038f+6.75f-time)));
    } else {
      p.x-=3.2f;
    }
    p*=scaleAlien;
    float4 dAlien = DEAlien(p,time, AnimStep);
    dAlien.x/=scaleAlien;
    res = (dAlien.x<res.x) ? dAlien: res;
  }
  return res;
}

__DEVICE__ float3 N(in float3 p,__TEXTURE2D__ iChannel0, float time, int AnimStep, bool withPlanet, float seed) {
    const float2 V01 = to_float2(0,1);
    const float2 Ve = swi2(V01,y,x)*0.001f;
    float2 e = to_float2(Ve.x, -Ve.x); 
    return normalize(swi3(e,x,y,y) * DE(p + swi3(e,x,y,y),iChannel0,time,AnimStep, withPlanet,seed).x + 
                     swi3(e,y,y,x) * DE(p + swi3(e,y,y,x),iChannel0,time,AnimStep, withPlanet,seed).x + 
                     swi3(e,y,x,y) * DE(p + swi3(e,y,x,y),iChannel0,time,AnimStep, withPlanet,seed).x + 
                     swi3(e,x,x,x) * DE(p + swi3(e,x,x,x),iChannel0,time,AnimStep, withPlanet,seed).x);
}

__DEVICE__ float softshadow(in float3 ro, in float3 rd, in float mint, in float maxt, in float k,__TEXTURE2D__ iChannel0, float time, int AnimStep, bool withPlanet,float seed) {
  float res = 1.0f, h, t = mint;
    for( int i=0; i<20; i++ ) {
      h = DE( ro + rd*t,iChannel0,time,AnimStep,withPlanet,seed).x;
      res = _fminf( res, k*h/t );
      if( res<0.0001f ) break;
      t += 0.02f;
    }
    

    return clamp(res, 0.0f, 1.0f);
}

// ------------------------------------------------------------------------------
// Deep Space 
// Star Nest by Kali
// https://www.shadertoy.com/view/4dfGDM

#define iterations 17
#define formuparam 0.53f
#define volsteps 10
#define stepsize 0.1f
#define tile   0.850f
#define brightness 0.0015f
#define darkmatter 1.500f
#define distfading 0.530f
#define saturation 0.650f

__DEVICE__ float4 space(float3 rd)
{
  float3 dir=rd;
  float3 from=to_float3(1.0f,0.5f,0.5f);

  //volumetric rendering
  float s=0.1f,fade=1.0f;
  float3 v=to_float3_s(0.0f);
  for (int r=0; r<volsteps; r++) {
    float3 p=from+s*dir*0.5f;
    p = abs_f3(to_float3_s(tile)-mod_f3(p,(tile*2.0f))); // tiling fold
    float pa,a=pa=0.0f;
    for (int i=0; i<iterations; i++) { 
      p=abs_f3(p)/dot(p,p)-formuparam; // the magic formula
      a+=_fabs(length(p)-pa); // absolute sum of average change
      pa=length(p);
    }
    float dm=_fmaxf(0.0f,darkmatter-a*a*0.001f); //dark matter
    a*=a*a; // add contrast
    if (r>6) fade*=1.0f-dm; // dark matter, don't render near
    v+=fade;
    v+=to_float3(s,s*s,s*s*s*s)*a*brightness*fade; // coloring based on distance
    fade*=distfading; // distance fading
    s+=stepsize;
  }
  v=_mix(to_float3_s(length(v)),v,saturation); //color adjust
  return to_float4_aw(v*0.01f,1.0f);  
}

// ------------------------------------------------------------------------------
// Metal effect
// Exterminate! by Antonalog
// https://www.shadertoy.com/view/ldX3RX
#define one_pi  0.31830988618f


/*
__DEVICE__ float3 rho_d = to_float3(0.147708f, 0.0806975f, 0.033172f);
__DEVICE__ float3 rho_s = to_float3(0.160592f, 0.217282f, 0.236425f);
__DEVICE__ float3 alpha = to_float3(0.122506f, 0.108069f, 0.12187f);
__DEVICE__ float3 ppp = to_float3(0.795078f, 0.637578f, 0.936117f);
__DEVICE__ float3 F_0 = to_float3(9.16095e-12, 1.81225e-12, 0.0024589f);
__DEVICE__ float3 F_1 = to_float3(-0.596835f, -0.331147f, -0.140729f);
__DEVICE__ float3 K_ap = to_float3(5.98176f, 7.35539f, 5.29722f);
__DEVICE__ float3 sh_lambda = to_float3(2.64832f, 3.04253f, 2.3013f);
__DEVICE__ float3 sh_c = to_float3(9.3111e-08, 8.80143e-08, 9.65288e-08);
__DEVICE__ float3 sh_k = to_float3(24.3593f, 24.4037f, 25.3623f);
__DEVICE__ float3 sh_theta0 = to_float3(-0.284195f, -0.277297f, -0.245352f);
*/
__DEVICE__ float3 rho_d;
__DEVICE__ float3 rho_s;
__DEVICE__ float3 alpha;
__DEVICE__ float3 ppp;
__DEVICE__ float3 F_0;
__DEVICE__ float3 F_1;
__DEVICE__ float3 K_ap;
__DEVICE__ float3 sh_lambda;
__DEVICE__ float3 sh_c;
__DEVICE__ float3 sh_k;
__DEVICE__ float3 sh_theta0;


__DEVICE__ void initShipColor() {     

  rho_d = to_float3(0.0657916f, 0.0595705f, 0.0581288f);
  rho_s = to_float3(1.55275f, 2.00145f, 1.93045f);
  alpha = to_float3(0.0149977f, 0.0201665f, 0.0225062f);
  ppp = to_float3(0.382631f, 0.35975f, 0.361657f);
  F_0 = to_float3(4.93242e-13, 1.00098e-14, 0.0103259f);
  F_1 = to_float3(-0.0401315f, -0.0395054f, -0.0312454f);
  K_ap = to_float3(50.1263f, 38.8508f, 34.9978f);
  sh_lambda = to_float3(3.41873f, 3.77545f, 3.78138f);
  sh_c = to_float3(6.09709e-08, 1.02036e-07, 1.01016e-07);
  sh_k = to_float3(46.6236f, 40.8229f, 39.1812f);
  sh_theta0 = to_float3(0.183797f, 0.139103f, 0.117092f);
}
      
__DEVICE__ float3 Fresnel(float3 F0, float3 F1, float V_H)
{
  return F0 - V_H * F1  + (1.0f - F0)*_powf(1.0f - V_H, 5.0f);
}

__DEVICE__ float3 D(float3 _alpha, float3 _p, float cos_h, float3 _K)
{
  float cos2 = cos_h*cos_h;
  float tan2 = (1.0f-cos2)/cos2;
  float3 ax = _alpha + tan2/_alpha;
  
  ax = _fmaxf(ax,to_float3_s(0.0f)); //bug?
  
  return one_pi * _K * exp_f3(-ax)/(pow_f3(ax,_p) * cos2 * cos2);
  // return to_float3( 0.0f / (cos2 * cos2));
}

__DEVICE__ float3 G1(float theta) {
  theta = clamp(theta,-1.0f,1.0f); //bug?
  return 1.0f + sh_lambda * (1.0f - exp_f3(sh_c * pow_f3(_fmaxf(_acosf(theta) - sh_theta0, to_float3_s(0.0f)), sh_k)));
}

__DEVICE__ float3 shade(float inLight, float n_h, float n_l, float n_v, float v_h)
{
  
    return  one_pi*inLight*(n_l*rho_d+rho_s*D(alpha,ppp,n_h,K_ap)*G1(n_l)*G1(n_v)*Fresnel(F_0,F_1,v_h));
}

__DEVICE__ float3 brdf(float3 lv, float3 ev, float3 n, float lightIntensity)
{
  float3 halfVector = normalize(lv + ev);
      
  float v_h = dot(ev, halfVector);
  float n_h = dot(n, halfVector);
  float n_l = dot(n, lv); 
  float inLight = 1.0f;
  if (n_l < 0.0f) inLight = 0.0f;
  float n_v = dot(n, ev); 
  
  float3 sh = shade(inLight, n_h, n_l, n_v, v_h);
  sh = clamp( sh, 0.0f, 1.0f); //bug?
  float3 retColor = lightIntensity * sh;
  
  return retColor;
}

// -------------------------------------------------------------------- 

  
__DEVICE__ float3 findColor(float obj, float2 uv, float3 n, float time) {
  
  const float3 
              COLOR_GLOBE1 = to_float3(0.1f,0.1f,0.1f),
              COLOR_GLOBE2 = to_float3(0.1f,2.0f,2.0f),
              COLOR_HUBLOT = to_float3(0.2f,0.2f,0.2f),
              COLOR_SIDE = to_float3(0.0f,9.0f,9.0f);
  
  if (obj == FLAG) {
// FLAG
    float c = textureInvader(uv);
    return to_float3(1.0f,c, c);
  } else if (obj == PLANET) {
// PLANET
    return _mix(to_float3(0.7f,0.3f,0),to_float3(1,0,0), clamp(1.1f-5.0f*(uv.x-1.8f),0.1f,0.9f));
  } else if (obj == SHIP_SIDE) {
    float spi = textureSpiral(uv, time);
    return _mix(COLOR_SIDE, 0.4f*COLOR_SIDE, spi);
  } else {
    float3 c, sp = swi3(space(n),x,y,z);
    if (obj == SHIP_GLOB || obj == SHIP_HUBLOT) {
      c = _mix(COLOR_GLOBE1, COLOR_GLOBE2, 0.5f+0.5f*C2);
      return _mix(c, sp, 0.8f);
    } else if (obj == SHIP_ARM) {
      return _mix(to_float3_s(1), sp, 0.2f);
    } else {      
      float spi = textureSpiral(uv,time);
      const float3 lightblue = 0.25f*to_float3(0.5f, 0.7f, 0.9f);
      c = _mix(lightblue,lightblue*0.4f, spi);
      return _mix(c, sp, 0.4f);
    }
  }
}

__DEVICE__ float3 Render(in float3 p, in float3 rd, in float t, in float obj, in float2 uv, float time, __TEXTURE2D__ iChannel0, int AnimStep, bool withPlanet, float seed) {
  
  float lightIntensity = 1.0f;
  
  float3 L = normalize(to_float3(10.25f,0.33f,-0.7f));
  //return swi3(V01,x,x,y)*(dot(N(p,iChannel0),L));  

  float3 nor = N(p,iChannel0,time,AnimStep, withPlanet,seed);
  float3 col = findColor(obj, uv, reflect(rd,nor),time);  
  float3 sunLight = L;
  float  amb = clamp(0.5f+0.5f*nor.y, 0.0f, 1.0f),
            dif = clamp(dot( nor, sunLight ), 0.0f, 1.0f),
            bac = clamp(dot( nor, normalize(to_float3(-sunLight.x,0.0f,-sunLight.z))), 0.0f, 1.0f)*clamp( 1.0f-p.y,0.0f,1.0f);

  float sh = softshadow( p, sunLight, 0.02f, 100.0f, 7.0f,iChannel0,time,AnimStep,withPlanet,seed); 
  
  if (obj != PLANET && obj != FLAG) {
    if (obj != SHIP_ARM) {
      initShipColor();
    }
    float gamma = 2.2f;
    lightIntensity *= 10.0f*(5.0f+0.5f*sh);
    col *= (brdf(sunLight, -rd, nor, lightIntensity) + 0.4f*brdf(-sunLight, -rd, nor, lightIntensity));
    return sqrt_f3(col);
  } else {
      dif *= sh; 

      float3 _brdf = 
                  0.2f*(amb*to_float3(0.10f,0.11f,0.13f) + bac*0.15f) +
                  1.2f*dif*to_float3(1.0f,0.9f,0.7f);

      float 
          pp = (obj == PLANET) ? 0.0f : clamp(dot(reflect(rd,nor), sunLight),0.0f,1.0f),
          spe = 2.0f*sh*_powf(pp,16.0f),  // brillance
          fre = _powf( clamp(1.0f+dot(nor,rd),0.0f,1.0f), 2.0f);
  
      col = col*(0.1f+_brdf + spe) + 0.2f*fre*(0.5f+0.5f*col)*_expf(-0.01f*t*t);
    
    return sqrt_f3(clamp(col,0.0f,1.0f));
  }
}


__DEVICE__ mat3 lookat(in float3 ro, in float3 up){
  float3 fw=normalize(ro);
  float3 rt=normalize(cross(fw,up));
  return to_mat3_f3(rt, cross(rt,fw),fw);
}



__DEVICE__ float3 RD(in float3 ro, in float3 cp, float2 fCoord, float2 iResolution) {
  const float2 V01 = to_float2(0,1);
  return mul_mat3_f3(lookat(cp-ro, swi3(V01,y,x,x)),normalize(to_float3_aw((2.0f*fCoord-iResolution)/iResolution.y, 12.0f)));
}


__KERNEL__ void Nv15InvaderSFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  CONNECT_CHECKBOX0(Textur, 0);
  CONNECT_CHECKBOX1(Kinostreifen, 1);
  CONNECT_SLIDER0(RotY,-10.0f,10.0f,0.0f);
  CONNECT_SLIDER1(RotX,-10.0f,10.0f,0.0f);
  CONNECT_SLIDER2(RotZ,-10.0f,10.0f,0.0f);
  CONNECT_SLIDER3(seed,-10.0f,10.0f,0.0f);

  float time = 3.0f*TAO+iTime*0.75f;
  
  C1 = COS(time);
  S1 = SIN(time);
  S2 = 2.0f*S1*C1;
  C2 = 1.0f-2.0f*S1*S1;

// Animation  
  time /= TAO;
  int AnimStep = 0;

  float3 cp = (AnimStep<2) ? to_float3(-2,0,0) : to_float3(0,0,0);
  float rCam = (AnimStep<2)?5.0f:45.0f;

  if (time > 7.25f) {
    AnimStep = 7; // apres de la remontee
    rCam = _mix(45.0f,5.0f,clamp(time-7.25f,0.0f,1.0f));
    cp = _mix(to_float3(0,0,0), to_float3(-2,0,0),clamp(time-7.25f,0.0f,1.0f));
  } else if (time>6.75f) {
     AnimStep = 6; // apres de la remontee
    rCam = 45.0f;
//    cp = to_float3(0,0,0);
    cp = _mix(to_float3(1,0,0),to_float3(0,0,0),clamp(time-6.25f,0.0f,1.0f));
  } else if (time>6.5f) {
    AnimStep = 5; // remontee sans drapeau
    rCam = 45.0f;
//    cp = to_float3(0,0,0);
    cp = _mix(to_float3(1,0,0),to_float3(0,0,0),clamp(time-6.25f,0.0f,1.0f));
  } else if (time>6.25f) {
    AnimStep = 4; // pause du drapeau
    rCam = 45.0f;
    cp = _mix(to_float3(1,0,0),to_float3(0,0,0),clamp(time-6.25f,0.0f,1.0f));
  } else if (time>5.25f) {
    AnimStep = 3; // pause
    rCam = _mix(160.0f,45.0f,clamp(time-5.25f,0.0f,1.0f));
    cp = to_float3(1,0,0);
  } else if (time>3.25f) {    
    AnimStep = 2; // arrivee du vaiseau
    rCam = _mix(5.0f,160.0f,clamp(time-3.25f,0.0f,1.0f));
    cp = _mix(to_float3(-2,0,0), to_float3(1,0,0),clamp(time-3.25f,0.0f,1.0f));
  }

  float3 rd, ro = rCam*to_float3(-0.5f+4.0f*iMouse.y/iResolution.y,
                                 -SIN(time*2.12f+iMouse.x/iResolution.x),
                                 -COS(time*2.12f)
                                );
  
  //ro += to_float3(RotX,RotY,RotZ);
  //ro.x = ro.x + RotX;
  
  float3 ctot = to_float3_s(0);
  
#ifdef ANTIALIASING 
  for (int i=0;i<AA;i++) {
    float2 fCoord = fragCoord+0.4f*to_float2(COS(6.28f*(float)(i)/(float)(AA)),SIN(6.28f*(float)(i)/(float)(AA)));  
#else
    float2 fCoord = fragCoord;
#endif
    rd = RD(ro, cp, fCoord, iResolution);
  
  //rd += to_float3(RotX,RotY,RotZ);
  //rd = rd + RotX;
  
        bool withPlanet = intersectSphere(ro, rd, to_float3(-2.0f,0,0), 0.21f);
        
    // Ray marching
    float t=0.0f,d=1.0f,od=1.0f;
    float4 res;
    for(int i=0;i<NB_ITER;i++){
      if(d<PRECISION|| t>MAX_DIST)break;
      t += res.x;
      res=DE(ro+rd*t,iChannel0,time, AnimStep,withPlanet,seed); // *0.95f;
    }

    // Render colors
    if(t<MAX_DIST){// if we hit a surface color it
      ctot += Render(ro + rd*t, rd,t, res.y, swi2(res,z,w),time,iChannel0, AnimStep, withPlanet,seed);
    } else {
      ctot += swi3(space(rd),x,y,z);
    }
#ifdef ANTIALIASING     
    }
  ctot /= (float)(AA);  
#endif     
  fragColor = to_float4_aw(ctot,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}