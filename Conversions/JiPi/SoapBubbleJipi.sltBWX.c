
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel0
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel1
// Connect Image 'Texture: RGBA Noise Medium' to iChannel2


#define PI       3.14159265359f
#define SPHERE     to_float4 (0.0f, 0.0f, 0.0f, 2.0f)
#define FOV     60.0f

#define RI_AIR    1.000293f
#define RI_SPH    1.55f

#define ETA     (RI_AIR/RI_SPH)
#define R      -0.02f

#define FR_BIAS   0.0f
#define FR_SCALE  1.0f
#define FR_POWER  0.7f

#define FR0      to_float3 (0.0f, 1.0f, 0.7f)

#define PI 3.14159265359f

__DEVICE__ float noise (float2 co, __TEXTURE2D__ iChannel2) {
  return length (_tex2DVecN (iChannel2,co.x,co.y,15));
}

__DEVICE__ mat2 rotate (float fi) {
  float cfi = _cosf (fi);
  float sfi = _sinf (fi);
  return to_mat2 (-sfi, cfi, cfi, sfi);
}

__DEVICE__ float3 hsv2rgb (float3 c) {
    float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
    return c.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), c.y);
}

__DEVICE__ float fbm ( float2 uv, __TEXTURE2D__ iChannel2) {
  return (
    +noise (uv*2.0f,iChannel2)/2.0
    +noise (uv*4.0f,iChannel2)/4.0
    +noise (uv*8.0f,iChannel2)/8.0
    +noise (uv*16.0f,iChannel2)/16.0
    +noise (uv*32.0f,iChannel2)/32.0
  );
}

__DEVICE__ float4 compute (float2 uv, float iTime, __TEXTURE2D__ iChannel2) {  
  uv = mul_f2_mat2(uv,rotate (PI * 0.5f * fbm (uv/256.0f,iChannel2) * length (uv) + iTime));
  uv = (iTime+uv)/196.0f;
  float3 col = to_float3 (fbm (uv,iChannel2)*PI*2.0f, 1.0f, 1.0f);  
  return to_float4_aw (hsv2rgb (col),1.0f) ;
}
        
__DEVICE__ mat3 rotate_x (float fi) {
  float cfi = _cosf (fi);
  float sfi = _sinf (fi);
  return to_mat3 (
                  1.0f, 0.0f, 0.0f,
                  0.0f, cfi, -sfi,
                  0.0f, sfi, cfi);
}

__DEVICE__ mat3 rotate_y (float fi) {
  float cfi = _cosf (fi);
  float sfi = _sinf (fi);
  return to_mat3 (
                  cfi, 0.0f, sfi,
                  0.0f, 1.0f, 0.0f,
                  -sfi, 0.0f, cfi);
}

__DEVICE__ mat3 rotate_z (float fi) {
  float cfi = _cosf (fi);
  float sfi = _sinf (fi);
  return to_mat3 (
                  cfi, -sfi, 0.0f,
                  sfi, cfi, 0.0f,
                  0.0f, 0.0f, 1.0f);
}

__DEVICE__ float4 noise4v (float2 p, __TEXTURE2D__ iChannel2) {
  return _tex2DVecN (iChannel2,p.x,p.y,15);
}

__DEVICE__ float4 fbm4v (float2 p, __TEXTURE2D__ iChannel2) {
  float4 f = to_float4_s (0.0f);
  f += 0.5000f * noise4v (p,iChannel2); p *= 2.01f;
  f += 0.2500f * noise4v (p,iChannel2); p *= 2.02f;
  f += 0.1250f * noise4v (p,iChannel2); p *= 2.03f;
  f += 0.0625f * noise4v (p,iChannel2); p *= 2.04f;
  f /= 0.9375f;
  return f;
}

__DEVICE__ float4 fbm3d4v (float3 p, float s, float iTime, __TEXTURE2D__ iChannel2) {  
  return 
    compute (swi2(p,x,y), iTime/s,iChannel2) * _fabs (p.z) +
    compute (swi2(p,x,z), iTime/s,iChannel2) * _fabs (p.y) +
    compute (swi2(p,y,z), iTime/s,iChannel2) * _fabs (p.x);
}

__DEVICE__ float sphere_intersect (in float3 o, in float3 d, in float4 c, out float *t0, out float *t1) {
  float3 oc = o - swi3(c,x,y,z);
  float A = dot (d, d);
  float B = 2.0f * dot (oc, d);
  float C = dot (oc, oc) - c.w;
  float D = B*B - 4.0f*A*C;
  float q = (-B - _sqrtf (D) * sign_f (B))/2.0f;
  float _t0 = q/A;
  float _t1 = C/q;
  *t0 = _fminf (_t0, _t1);
  *t1 = _fmaxf (_t0, _t1);
  return step (0.0f, D);
}

__DEVICE__ float fresnel_step (float3 I, float3 N, float3 f) {
  return clamp (f.x + f.y * _powf (1.0f + dot (I, N), f.z), 0.0f, 1.0f);
}

__DEVICE__ float2 to_spherical_normalized (float3 pt) {
  float r = length (pt);
  return to_float2 (_acosf (pt.z / r)/PI, _atan2f (pt.y, pt.x)/PI + 0.5f); 
}

__DEVICE__ float3 spherical (float3 cart) {
  float r = length (cart);
  float i = (_acosf (cart.z/r)/(PI/2.0f) - 0.5f)*2.0f;
  float a = _atan2f (cart.y, cart.x)/PI;
  return to_float3 (r, a, i);
}

__KERNEL__ void SoapBubbleJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

CONNECT_CHECKBOX0(FBM3D4F, 0);
CONNECT_POINT0(ViewXY, 0.0f, 0.0f );
CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);

  float2 uv = (2.0f*fragCoord - iResolution)/_fminf (iResolution.x, iResolution.y) * _tanf (radians (FOV)/2.0f);
  float2 mo = PI * swi2(iMouse,x,y) / iResolution;
  
  float3 up = to_float3 (0.0f, 1.0f, 0.0f) + to_float3_aw(ViewXY,ViewZ);       // up 
  float3 fw = mul_f3_mat3(to_float3 (0.0f, 0.0f, 1.0f) , rotate_y (mo.x * PI));      // forward
         fw = mul_f3_mat3(fw , rotate_x (mo.y * PI));         // forward
//         fw = mul_f3_mat3(fw , rotate_z (ViewZ * PI));      // forward
             
  float3 lf = cross (up, fw);                    // left
  
  float3 ro = -fw * 5.0f;                        // ray origin
  float3 rd = normalize (uv.x * lf + uv.y * up + fw) ;     // ray direction
  float3 rn = rd;
  float3 dr = swi3(fbm4v (uv/64.0f + _sinf (iTime/128.0f),iChannel2),x,y,z) - 0.5f;
  //rd = normalize (rd + dr/32.0f);
  
  float4 sp = SPHERE + to_float4 (0.0f, 1.0f, 0.0f, 0.0f)*_sinf (iTime);
    
  
  float t0 = 0.0f, t1 = 0.0f;          // sphere intersection points
  
  float d = sphere_intersect (ro, rd, sp, &t0, &t1);         // initial intersection
      
  float4 color = _tex2DVecN (iChannel0,rn.x,rn.y,15);
  
  if (d > 0.0f) {
    float3 pt0 = ro + rd*t0;
    float3 pt1 = ro + rd*t1;
    float3 pn0 = normalize (pt0 - swi3(sp,x,y,z));  
    float3 pn1 = normalize (swi3(sp,x,y,z) - pt1);
    
    
    float3 r0 = reflect (rd, pn0);      
    float3 r1 = reflect (rd, pn1);
    
    float4 s0,s1;
    
    if(FBM3D4F)       
    {
      s0 = fbm3d4v (normalize (pn0), 8.0f,iTime,iChannel2);
      s1 = fbm3d4v (normalize (pn1), 8.0f,iTime,iChannel2);
    }
    else
    {
      s0 = compute (swi2(spherical (swi3(pn0,z,x,y)),y,z)/2.0f, iTime/8.0f,iChannel2);
      s1 = compute (swi2(spherical (swi3(pn1,z,x,y)),y,z)/2.0f, iTime/8.0f,iChannel2);
    } 
    float4 c0 = _tex2DVecN (iChannel1,r0.x,r0.y,15);
    float4 c1 = _tex2DVecN (iChannel1,r1.x,r1.y,15);    
    
    color = _mix (color,c1 + c1*s1, fresnel_step (rd, pn1, FR0));
    color = _mix (color,c0 + c0*s0, fresnel_step (rd, pn0, FR0));
    
  }
  
  fragColor = color;

  SetFragmentShaderComputedColor(fragColor);
}