
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

// Orientation
//float2 R;
__DEVICE__ float2 hash(float2 p) // Dave H
{
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));

}
__DEVICE__ float3 rot (float3 p, float3 d) {
    float t = length(d);
    if (t==0.0f) return p;
    d = normalize(d);
    float3 q = p-d*dot(d,p);
    return p+(q)*(_cosf(t)-1.0f) + cross(d,q)*_sinf(t);
}
__DEVICE__ float3 O (float2 U, float2 R, __TEXTURE2D__ iChannel0) {return swi3(texture(iChannel0,U/R),x,y,z);}
__DEVICE__ float3 W (float2 U, float2 R, __TEXTURE2D__ iChannel1) {return swi3(texture(iChannel1,U/R),z,y,z);}


__KERNEL__ void LiquidCrystalFuse__Buffer_A(float4 C, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   CONNECT_CHECKBOX0(Reset, 0);
   
   U+=0.5f;
   
   swi3S(C,x,y,z, rot(O(U,R,iChannel0),W(U,R,iChannel1)));
   
   if (length(swi3(C,x,y,z))==0.0f) C.x=0.0f,C.y=0.0f,C.z=1.0f;//swi3(C,x,y,z) = to_float3(0,0,1);
   float3 ne = 0.125f*(
        O(U+to_float2(0, 1),R,iChannel0)+
        O(U+to_float2(0,-1),R,iChannel0)+
        O(U+to_float2( 1,0),R,iChannel0)+
        O(U+to_float2(-1,0),R,iChannel0)+
        O(U+to_float2(1, 1),R,iChannel0)+
        O(U+to_float2(1,-1),R,iChannel0)+
        O(U+to_float2( 1,1),R,iChannel0)+
        O(U+to_float2(-1,1),R,iChannel0)+
        O(U+to_float2(-1, 1),R,iChannel0)+
        O(U+to_float2(-1,-1),R,iChannel0)+
        O(U+to_float2( 1,-1),R,iChannel0)+
        O(U+to_float2(-1,-1),R,iChannel0)
        );
float AAAAAAAAAAAAAAA;        
   swi3S(C,x,y,z, _mix(swi3(C,x,y,z),ne,0.01f));
   swi3S(C,x,y,z, normalize(swi3(C,x,y,z))*0.05f);
 
   if (iFrame < 1 || Reset) {
     C = to_float4(0,0,1,0);
   if (length(U-0.5f*R)<2.0f) C.x=-1.0f,C.y=0.001f,C.z=0.0f;//swi3(C,x,y,z) = to_float3(-1,0.001f,0);
 }
 
  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


//Angular momentum
#ifdef xxxx
__DEVICE__float2 hash(float2 p) // Dave H
{
    float3 p3 = fract(to_float3(swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}
#endif

__DEVICE__ float3 F (float3 A, float3 B) {
    A = A-B;
  return A/dot(A,A)/length(A);
}
__DEVICE__ float3 T (float3 a, float3 b, float3 A, float3 B) {
  float3
        ap = a+A,
        an = a-A,
        bp = b+B,
        bn = b-B,
        Fp = F(ap,bp)-F(ap,bn),
        Fn = F(an,bn)-F(an,bp);
    return cross(A,Fp)-cross(A,Fn);
}
//__DEVICE__ float3 O (float2 U) {return texture(iChannel0,U/R).xyz;}
//__DEVICE__ float3 W (float2 U) {return texture(iChannel1,U/R).zyz;}
__DEVICE__ float3 P (float2 U, float iTime) {
    if (fract(0.5f*U.y)<0.5f) U.x -= 0.5f;
    U.x *= 1.732050808f;
    float2 h = hash(_floor(U))*2.0f-1.0f;
    float t = 2.0f*iTime+h.x*1000.0f;
    return to_float3_aw(U+0.5f*h+0.5f*to_float2(_sinf(t),-_cosf(t)),0);
}

__DEVICE__ float3 S (float2 U, float2 R, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
  float3 
        t = to_float3_s(0),
        o = O(U,R,iChannel0);
    for (int x = -2; x <= 2; x++)
    for (int y = -2; y <= 2; y++)
    {if (x==0&&y==0) continue;
      t += T(P(U, iTime),P(U+2.481f*to_float2(x,y),iTime),o,O(U+1.1f*to_float2(x,y),R,iChannel0));
    }
  return W(U,R,iChannel1) + t;
}
__KERNEL__ void LiquidCrystalFuse__Buffer_B(float4 C, float2 U, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   CONNECT_CHECKBOX0(Reset, 0);
   
   U+=0.5f;
float BBBBBBBBBBBBBBBBBBBBB;   
   swi3S(C,x,y,z, S(U,R,iTime,iChannel0,iChannel1));
   float3 ne = 0.25f*(
        W(U+to_float2(0, 1),R,iChannel1)+
        W(U+to_float2(0,-1),R,iChannel1)+
        W(U+to_float2( 1,0),R,iChannel1)+
        W(U+to_float2(-1,0),R,iChannel1)
    );
   swi3S(C,x,y,z, _mix(swi3(C,x,y,z),ne,0.5f));
   if (length(swi3(C,x,y,z))>0.2f)  swi3S(C,x,y,z, normalize(swi3(C,x,y,z))*0.2f);
   if (iFrame < 1 || Reset) C = to_float4(0,0,0,0);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define H 1.73205080757f
#ifdef xxxx
float2 hash(float2 p) // Dave H
{
  float3 p3 = fract(to_float3(swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));

}
#endif

__DEVICE__ float intersect (float2 coes) {
    float det = coes.x*coes.x-4.0f*coes.y;
    if (det < 0.0f) return 1e4;
    return 0.5f*(-coes.x-_sqrtf(det));
}
__DEVICE__ float sphere (float3 p, float3 d, float3 c, float r) {
  c = p-c;
float zzzzzzzzzzzzzzzzzzzz;  
    return intersect(to_float2(2.0f*dot(c,d),dot(c,c)-r*r));
}
__DEVICE__ float ellipse (float3 p, float3 d, float3 a, float3 b, float r) {
  a = p-a;b = p-b;
    float 
        rr = r*r,
        ad = dot(a,d),
        bd = dot(b,d),
        aa = dot(a,a),
        bb = dot(b,b);
    return intersect(to_float2(
      ad*aa-ad*bb+bd*bb-bd*aa-rr*(ad+bd),
        -aa*bb+0.25f*(aa*aa+bb*bb+rr*rr)+0.5f*(aa*bb-rr*(aa+bb))
    )/(ad*ad+bd*bd-rr-2.0f*ad*bd));
}
__DEVICE__ float3 norEllipse (float3 p, float3 a, float3 b) {
    return normalize(normalize(p-a)+normalize(p-b));
}
__DEVICE__ float plane (float3 p, float3 d) {
  return  dot(-p,to_float3(0,0,1))/dot(d,to_float3(0,0,1));
}
//__DEVICE__ float3 O (float2 U) {return normalize(texture(iChannel0,U/R).xyz);}
//__DEVICE__ float3 W (float2 U) {return texture(iChannel1,U/R).zyz;}

__DEVICE__ float D (float3 p, float3 d, float2 v, float e, inout float4 *color, float2 R, float iTime, __TEXTURE2D__ iChannel0) {
    
    float2 U = _floor(swi2(p,x,y)+0.5f+v);
    if (fract(0.5f*U.y)<0.5f) U.x += 0.5f;
    float3 o = O(U,R,iChannel0);
    float2 h = hash(U)*2.0f-1.0f;
    float t = 2.0f*iTime+h.x*100.0f;
    float3 q = to_float3_aw(U+0.5f*h+0.3f*to_float2(_sinf(t),-_cosf(t)),_sinf(t)),
         a = q + o,
         b = q - o;
    float f = ellipse(p,d,a,b,2.1f);
    if (f < e) {
      e = f;
        p += d*e;
        float3 n = norEllipse(p,a,b);
        float m = dot(n,o);
        *color = dot(n,to_float3(0,0,-1))*to_float4_aw(swi3(o,x,z,y)*0.5f+0.5f,1);
    }
    return e;
}
__DEVICE__ float4 X (float3 p, float3 d, float2 R, float iTime, __TEXTURE2D__ iChannel0) {
  p += d*plane(p, d);
  float4 color = to_float4_s(0);
    float e = 1e3;
    for (int x = -1; x<=1; x++)
    for (int y = -1; y<=1; y++)
        e = D (p, d, to_float2(x,y), e, &color,R, iTime, iChannel0);
    return color;
}

__KERNEL__ void LiquidCrystalFuse(float4 C, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
   U+=0.5f;
   float2 fragCoord = U;
   
   U = 2.0f*(U-0.5f*R)/R.y;
   float3 p = to_float3_aw(swi2(iMouse,x,y)+19.0f*U,-1),
         d = normalize(to_float3_aw(U*0.1f,1));
   C = X(p,d,R,iTime,iChannel0);
   float3 o = O(fragCoord,R,iChannel0);
   if (iMouse.z <1.0f) swi3S(C,x,y,z, (swi3(o,x,z,y)*0.5f+0.5f));

  SetFragmentShaderComputedColor(C);
}