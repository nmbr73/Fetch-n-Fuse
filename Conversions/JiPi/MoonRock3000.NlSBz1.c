
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define pi 3.14159f

#define thc(a,b) _tanhf(a*_cosf(b))/_tanhf(a)
#define ths(a,b) _tanhf(a*_sinf(b))/_tanhf(a)
#define sabs(x) _sqrtf(x*x+1e-2)
//#define sabs(x, k) _sqrtf(x*x+k)-0.1

#define Rot(a) to_mat2(_cosf(a), -_sinf(a), _sinf(a), _cosf(a))

__DEVICE__ float test(in float2 uv, float res)
{
    uv -= _floor(uv) + 0.5f;

    float k = 10.0f / res;
    float m = 0.25f;
    
    float d = length(uv);
    float s = smoothstep(-k, k, -length(uv) + m);
       
    for (int i = 0; i < 6; i++) {
        uv = abs_f2(uv) - m;
        m *= 0.5f;
        s = _fmaxf(s, smoothstep(-k, k, -length(uv) + m));
    }
    
    return s;
}

__DEVICE__ float sfloor(float a, float b) {
    return _floor(b) + 0.5f + 0.5f * _tanhf(a * (fract(b) - 0.5f)) / _tanhf(0.5f * a);
}

__DEVICE__ float cc(float a, float b) {
    float f = thc(a, b);
    return sign_f(f) * _powf(_fabs(f), 0.25f);
}

__DEVICE__ float cs(float a, float b) {
    float f = ths(a, b);
    return sign_f(f) * _powf(_fabs(f), 0.25f);
}

__DEVICE__ float3 pal(in float t, in float3 a, in float3 b, in float3 c, in float3 d) {
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__DEVICE__ float h21(float2 a) {
    return fract(_sinf(dot(swi2(a,x,y), to_float2(12.9898f, 78.233f))) * 43758.5453123f);
}

__DEVICE__ float mlength_f2(float2 uv) {
    return _fmaxf(_fabs(uv.x), _fabs(uv.y));
}

__DEVICE__ float mlength_f3(float3 uv) {
    return _fmaxf(_fmaxf(_fabs(uv.x), _fabs(uv.y)), _fabs(uv.z));
}

__DEVICE__ float smin(float a, float b) {
    float k = 0.12f;
    float h = clamp(0.5f + 0.5f * (b-a) / k, 0.0f, 1.0f);
    return _mix(b, a, h) - k * h * (1.0f - h);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Stars' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define MAX_STEPS 400
#define MAX_DIST 100.0f
#define SURF_DIST 0.001f

union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };

#define FK(k) floatBitsToInt(k*k/7.0f)^floatBitsToInt(k)
__DEVICE__ float hash(float a, float b) {
	
	union Zahl z1, z2;
    //int x = FK(a), y = FK(b);
	z1._Float = a*a/7.0f;
	z2._Float = a;
	
	int x = z1._Uint^z2._Uint;
	
	z1._Float = b*b/7.0f;
	z2._Float = b;
	
  int y = z1._Uint^z2._Uint;
  return (float)((x*x+y)*(y*y-x)-x)/2.14e9f;
}

__DEVICE__ float3 erot(float3 p, float3 ax, float ro) {
  return _mix(dot(ax, p)*ax, p, _cosf(ro)) + cross(ax,p)*_sinf(ro);
}

__DEVICE__ float3 face(float3 p) {
  float3 a = abs_f3(p);
  return step(swi3(a,y,z,x), swi3(a,x,y,z))*step(swi3(a,z,x,y), swi3(a,x,y,z))*sign_f3(p);
}

__DEVICE__ float sdBox(float3 p, float3 s) {
  p = abs_f3(p)-s;
  return length(_fmaxf(p, to_float3_s(0.0f)))+_fminf(_fmaxf(p.x, _fmaxf(p.y, p.z)), 0.0f);
}

__DEVICE__ float3 getRo(float4 iMouse, float2 iResolution, float iTime) {
    float2 m = swi2(iMouse,x,y)/iResolution;
    float r = 3.0f;
    float t = 0.2f * iTime;
    float3 ro = to_float3(r * _cosf(t), 2, r * _cosf(t) + _sinf(t));
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    return ro;
}

__DEVICE__ float GetDist(float3 p, float2 iResolution, float iTime) {
    float sc = 0.125f;
    
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , Rot(0.5f * length(swi2(p,x,z)) - 0.1f * iTime)));
    float c1 = test(sc * swi2(p,x,y), iResolution.y);
    float c2 = test(sc * swi2(p,y,z), iResolution.y);
    float c3 = test(sc * swi2(p,z,x), iResolution.y);

    p.y += 0.05f * _cosf(_atan2f(p.x, p.z) + iTime);
  
    float r1 = 1.0f;
    float r2 = 1.0f;
    float d1 = length(swi2(p,x,z)) - r1;
    float d2 = length(to_float2(d1,p.y)) - r2;
    //d2 = length(p) - 1.5f;
     d2 += (0.5f + 0.5f * thc(4.0f, iTime/3.0f)) * 0.5f * _fmaxf(max(c1,c2), c3);
    return 0.15f * d2;
}

__DEVICE__ float RayMarch(float3 ro, float3 rd, float z, float2 iResolution, float iTime) {
  
    float dO=0.0f;
    float s = sign_f(z);
    for(int i=0; i<MAX_STEPS; i++) {
      float3 p = ro + rd*dO;
        float dS = GetDist(p,iResolution,iTime);
        if (s != sign_f(dS)) { z *= 0.5f; s = sign_f(dS); }
        if(_fabs(dS)<SURF_DIST || dO>MAX_DIST) break;
        dO += dS*z; 
    }
    
    return _fminf(dO, MAX_DIST);
}

__DEVICE__ float3 GetNormal(float3 p, float2 iResolution, float iTime) {
  float d = GetDist(p,iResolution,iTime);
    float2 e = to_float2(0.001f, 0);
    
    float3 n = d - to_float3(
        GetDist(p-swi3(e,x,y,y),iResolution,iTime),
        GetDist(p-swi3(e,y,x,y),iResolution,iTime),
        GetDist(p-swi3(e,y,y,x),iResolution,iTime));
    
    return normalize(n);
}

__DEVICE__ float3 GetRayDir(float2 uv, float3 p, float3 l, float z) {
    float3 f = normalize(l-p),
        r = normalize(cross(to_float3(0,1,0), f)),
        u = cross(f,r),
        c = f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i);
    return d;
}


__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}


__KERNEL__ void MoonRock3000Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel1)
{

  CONNECT_SLIDER1(refmul, -1.0f, 10.0f, 1.0f); 
  CONNECT_SLIDER2(refoff, -1.0f, 10.0f, 0.0f);
	
	CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.55f);


	CONNECT_CHECKBOX0(Fresnel, 0);
	CONNECT_CHECKBOX1(Option1, 0);
	CONNECT_CHECKBOX2(Option2, 0);
	CONNECT_CHECKBOX3(Option3, 0);


    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
  
    float3 ro = getRo(iMouse,iResolution,iTime);
    
    float3 rd = GetRayDir(uv, ro, to_float3_s(0), 1.5f);
    float3 col = to_float3_s(0);
   
    float d = RayMarch(ro, rd, 1.0f,iResolution, iTime);

    float IOR = 1.05f;
    float3 p = ro + rd * d;
    if(d<MAX_DIST) {
        
      float3 n = GetNormal(p,iResolution,iTime);
      float3 r = reflect(rd, n);

      float3 pIn = p - 4.0f * SURF_DIST * n;
      float3 rdIn = _refract_f3(rd, n, 1.0f/IOR, refmul, refoff);
      float dIn = RayMarch(pIn, rdIn, -1.0f, iResolution, iTime);
      
      float3 pExit = pIn + dIn * rdIn;
      float3 nExit = -1.0f*GetNormal(pExit,iResolution,iTime); // *-1.0f; ?

      float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
      col = to_float3_s(dif);
      
      float sc = 0.5f;
      float c1 = test(sc * swi2(p,x,y), iResolution.y);
      float c2 = test(sc * swi2(p,y,z), iResolution.y);
      float c3 = test(sc * swi2(p,z,x), iResolution.y);

      float3 c4 = swi3(texture(iChannel1, sc + sc * swi2(p,x,y)),x,y,z);
      float3 c5 = swi3(texture(iChannel1, sc + sc * swi2(p,y,z)),x,y,z);
      float3 c6 = swi3(texture(iChannel1, sc + sc * swi2(p,z,x)),x,y,z);

      float fres = _powf(clamp(1.0f + dot(rd, n),0.0f,1.0f), 1.0f);
  
      if (Fresnel)
        col += fres;
      
      float3 an = abs_f3(n);
      float3 c = to_float3_s(c1 * n.z + c2 * n.x + c3 * n.y);
      float3 cc = c4 * n.z + c5 * n.x + c6 * n.y;
      
      if(Option1)
        col = c;
      
      col = mix_f3(col * cc, c, cc * fres);
          
      if(Option2)
        col *= n.y;
          
      float3 e = swi3(Color,x,y,z);//to_float3_s(0.5f);
      col += _expf(0.5f-8.0f * _fabs(_cosf(12.0f * _logf(length(swi2(p,x,z))) - iTime)))
             * pal(_fmaxf(_fmaxf(c1,c2),c3) - c1 * c2 * c3, e, e, e, Level0 * to_float3(0,1,2)/3.0f);
          
      if(Option3)
        col = c;
    }
    col = _mix(col, to_float3_s(1), _expf(-10.0f * length(swi2(p,x,z))));
    col = pow_f3(col, to_float3_s(0.4545f));  // gamma correction
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}