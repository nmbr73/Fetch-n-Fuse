
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float2 mirror (float2 u) {
    if (u.x>1.0f) u.x = 1.0f-fract(u.x);
    if (u.x<0.0f) u.x = fract(u.x);
    if (u.y>1.0f) u.x = 1.0f-fract(u.y);
    if (u.y<0.0f) u.x = fract(u.y);
  return u;
}
#define A(U) texture(iChannel0,mirror((U)/R))
#define B(U) texture(iChannel1,mirror((U)/R))
#define C(U) texture(iChannel2,mirror((U)/R))
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void InverseDiffusionFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    U+=0.5f;
 
    Q = A(U);
    float4 
        n = B(U+to_float2(0,1)),
        e = B(U+to_float2(1,0)),
        s = B(U-to_float2(0,1)),
        w = B(U-to_float2(1,0)),
        m = 0.25f*(n+e+s+w);
    
    float d = 0.25f*(n.y-s.y+e.x-w.x);
    float c = 0.25f*(n.x-s.x-e.y+w.y);
    
    Q.z = m.z*0.999f - _mix(d,c,length(U-0.5f*R)/R.y);
    Q.w = d;
    if (iFrame < 1) Q = to_float4_s(_sinf(U.x)*_cosf(U.y));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void InverseDiffusionFuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
 
    Q = A(U);
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        m = 0.25f*(n+e+s+w);
    //swi2(Q,x,y) = 0.25f*to_float2(e.z-w.z,n.z-s.z);
    Q.x = 0.25f*(e.z-w.z);
    Q.y = 0.25f*(n.z-s.z);
    
       
    if (length(swi2(Q,x,y))>0.0f) swi2S(Q,x,y, _mix(swi2(Q,x,y),normalize(swi2(Q,x,y)),0.2f));
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void InverseDiffusionFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    U+=0.5f;

    Q = A(U);
    float4 
        n = B(U+to_float2(0,1)),
        e = B(U+to_float2(1,0)),
        s = B(U-to_float2(0,1)),
        w = B(U-to_float2(1,0)),
        m = 0.25f*(n+e+s+w);
    
    float d = 0.25f*(n.y-s.y+e.x-w.x);
    float c = 0.25f*(n.x-s.x-e.y+w.y);
    
    Q.z = m.z*0.999f - _mix(d,c,0.2f);
    
    if (iFrame < 1) Q = to_float4_s(_sinf(U.x)*_cosf(U.y));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void InverseDiffusionFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse)
{
    U+=0.5f;

    float2 c = 0.5f*R;
    if (iMouse.z>0.0f) c = swi2(iMouse,x,y);
    
    U -= c;
    U *= 0.99f;
    U += c;
    Q = A(U);
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        m = 0.25f*(n+e+s+w);
    //swi2(Q,x,y) = 0.25f*to_float2(e.z-w.z,n.z-s.z);
    Q.x = 0.25f*(e.z-w.z);
    Q.y = 0.25f*(n.z-s.z);
    
    if (length(swi2(Q,x,y))>0.0f) swi2S(Q,x,y, _mix(swi2(Q,x,y),normalize(swi2(Q,x,y)),0.2f));
 

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__DEVICE__ mat2 r (float a) {
  float2 e = to_float2(_cosf(a),_sinf(a));
  return to_mat2(e.x,-e.y,e.y,e.x);
}
__KERNEL__ void InverseDiffusionFuse(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse)
{
    U+=0.5f;

    float3 
        p = to_float3_aw(0.5f*R,0.62f*R.y),
        d = normalize(to_float3_aw((U-0.5f*R)/R.y,-1)),
        o = to_float3(0.5f,0.1f,0.5f)*swi3(R,x,y,y);
    if (iMouse.z>0.0f) o.x=iMouse.x,o.y=iMouse.y;//swi2(o,x,y) = swi2(iMouse,x,y);
    mat2 m = r(0.44f);
    p.y -= 0.19f*R.y;
    swi2S(d,y,z, mul_f2_mat2(swi2(d,y,z),  m));
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z),  m));
    for (int i = 0; i<30; i++){ 
      p += 0.2f*d*(p.z-4.0f*A(swi2(p,x,y)).z);
    }
    d = normalize(o-p);
    float z = A(swi2(p,x,y)).z;
    float3 n = normalize(to_float3_aw(swi2(B(swi2(p,x,y)),x,y),-0.25f));
    float3 q = d;
    p += 0.1f*d;
    for (int i = 0; i<30; i++){ 
      p += 0.5f*d*_fminf(p.z-4.0f*A(swi2(p,x,y)).z,length(p-o)-1.0f);
    }
    Q = (_expf(-length(p-o)+1.0f))*(cos_f4(-0.1f*iTime+0.1f*z+0.5f*to_float4(1,2,3,4)))*0.5f*(dot(reflect(n,d),q)-dot(n,d));
    Q *= Q;

  SetFragmentShaderComputedColor(Q);
}