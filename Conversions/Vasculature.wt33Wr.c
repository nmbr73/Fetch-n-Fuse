
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
__DEVICE__ mat2 r (float a) {
    float2 e = to_float2(_cosf(a),_sinf(a));
  return to_mat2(e.x,-e.y,e.y,e.x);
}
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
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void VasculatureFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
    
    Q = A(U);
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        m = 0.25f*(n+e+s+w);
    
    float d = 0.25f*(n.y-s.y+e.x-w.x);
    float c = 0.25f*(n.x-s.x-e.y+w.y);
    float2 g = 0.25f*to_float2(e.z-w.z,n.z-s.z);
    Q.z = _mix(m.z,d,0.4f);
    
    swi2S(Q,x,y, _mix(swi2(Q,x,y),swi2(m,x,y),0.3f));
    //swi2(Q,x,y) -= g;
    Q.x -= g.x;
    Q.y -= g.y;

        
    if (length(swi2(Q,x,y))>0.0f)  swi2S(Q,x,y, _mix(swi2(Q,x,y),normalize(swi2(Q,x,y)),1.0f));
    
    if (iFrame < 1 || Reset) Q = to_float4(_sinf(0.1f*U.y),_cosf(0.1f*U.x),0,0);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void VasculatureFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse)
{
    U+=0.5f;
    
    float2 p = 0.5f*R+10.0f*to_float2(_cosf(iTime),_sinf(iTime));
    if (iMouse.z>0.0f) p = swi2(iMouse,x,y);
    
    U -= p;
    U = mul_f2_mat2(U*(1.0f-5e-3f*(1.0f-length(U)/R.x)),r(-0.001f));
    U += p;
    Q = A(U);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void VasculatureFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);    
    U+=0.5f;
    
    Q = A(U);
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        m = 0.25f*(n+e+s+w);
    
    float d = 0.25f*(n.y-s.y+e.x-w.x);
    float c = 0.25f*(n.x-s.x-e.y+w.y);
    float2 g = 0.25f*to_float2(e.z-w.z,n.z-s.z);
    Q.z = _mix(m.z,d,0.4f);

    swi2S(Q,x,y, _mix(swi2(Q,x,y),swi2(m,x,y),0.3f));
    //swi2(Q,x,y) -= g;
    Q.x -= g.x;
    Q.y -= g.y;

    if (length(swi2(Q,x,y))>0.0f)    swi2S(Q,x,y, _mix(swi2(Q,x,y),normalize(swi2(Q,x,y)),1.0f));
    
    if (iFrame < 1 || Reset) Q = to_float4(_sinf(0.1f*U.y),_cosf(0.1f*U.x),0,0);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void VasculatureFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse)
{
    U+=0.5f;
    
    float2 p = 0.5f*R+10.0f*to_float2(_cosf(iTime),_sinf(iTime));
    if (iMouse.z>0.0f) p = swi2(iMouse,x,y);
    
    U -= p;
    U = mul_f2_mat2(U*(1.0f-5e-3f*(1.0f-length(U)/R.x)),r(-0.001f));
    U += p;
    Q = A(U);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void VasculatureFuse(float4 Q, float2 U, float2 iResolution)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);  
    
    U+=0.5f;
    
    float4 a = A(U);
    Q = 4.0f*to_float4(-a.z,0.1f*_fabs(a.z),1.5f*a.z,1);
    
    if (Invers) Q = to_float4_s(1.0f) - Q;
    if (ApplyColor)
    {
      Q = Q + (Color-0.5f);
      Q.w = Color.w;
    }

  SetFragmentShaderComputedColor(Q);
}