
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//Texture lookups :
//#define A(U) texelFetch(iChannel0,to_int2(U),0)
#define A(U) texture(iChannel0, (make_float2(to_int2_cfloat(U))+0.5f)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

// Distance to line
__DEVICE__ float2 ln (float2 p, float2 a, float2 b) {
  float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  return to_float2(length(p-a-(b-a)*i),i);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


__DEVICE__ float4 X (in float4 Q, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0 ) {
    float4 n = A(U+r);
    if (ln(U,swi2(n,x,y),swi2(n,z,w)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q = n;
    else if (ln(U,swi2(Q,x,y),swi2(n,z,w)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q.z=n.z, Q.w=n.w;//swi2(Q,z,w) = swi2(n,z,w);
    else if (ln(U,swi2(Q,x,y),swi2(n,x,y)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q.z=n.x, Q.w=n.y;//swi2(Q,z,w) = swi2(n,x,y);
    return Q;
}
__KERNEL__ void DynamicalGraphFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(TexStart, 0);
    CONNECT_SLIDER1(Level, -1.0f, 100.0f, 30.0f);
    CONNECT_POINT0(ClampStart, 0.0f, 0.0f );
    U+=0.5f;
   
    Q = A(U);
    Q=X(Q,U,to_float2(1,0),R,iChannel0);
    Q=X(Q,U,to_float2(0,1),R,iChannel0);
    Q=X(Q,U,to_float2(0,-1),R,iChannel0);
    Q=X(Q,U,to_float2(-1,0),R,iChannel0);
    Q=X(Q,U,to_float2(2,0),R,iChannel0);
    Q=X(Q,U,to_float2(0,2),R,iChannel0);
    Q=X(Q,U,to_float2(0,-2),R,iChannel0);
    Q=X(Q,U,to_float2(-2,0),R,iChannel0);
    
    swi2S(Q,x,y, swi2(Q,x,y) + swi2(B(swi2(Q,x,y)),x,y));
    swi2S(Q,z,w, swi2(Q,z,w) + swi2(B(swi2(Q,z,w)),x,y));
    
    if (Q.x< 1.0f) Q.x=Q.z, Q.y=Q.w;//swi2(Q,x,y) = swi2(Q,z,w);
    if (Q.z< 1.0f) Q.z=Q.x, Q.w=Q.y;//swi2(Q,z,w) = swi2(Q,x,y);
    
    if (iMouse.z>0.0f) {
      float4 n = swi4(iMouse,x,y,x,y);
      if (ln(U,swi2(n,x,y),swi2(n,z,w)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q = n;
      if (ln(U,swi2(Q,x,y),swi2(n,x,y)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q.z=n.x, Q.w=n.y;//swi2(Q,z,w) = swi2(n,x,y);
    }
   
    if (iFrame < 1 || Reset) {
      if(TexStart)
      {
        Q = to_float4_s(0.0f);
        float tex = texture(iChannel2, U/R).w;
        if (tex>0.0f)
          //Q = clamp(swi4(_floor(U/30.0f+0.5f),x,y,x,y)*30.0f,0.2f*swi4(R,x,y,x,y),0.8f*swi4(R,x,y,x,y));
          Q = clamp(swi4(_floor(U/Level+0.5f),x,y,x,y)*Level,(0.2f-ClampStart.x)*swi4(R,x,y,x,y),(0.8f-ClampStart.y)*swi4(R,x,y,x,y));  
      }
      else
        Q = clamp(swi4(_floor(U/Level+0.5f),x,y,x,y)*Level,(0.2f-ClampStart.x)*swi4(R,x,y,x,y),(0.8f-ClampStart.y)*swi4(R,x,y,x,y));  
    }
    if (length(swi2(Q,x,y)-swi2(Q,z,w)) > 100.0f) Q.z=Q.x, Q.w=Q.y;//swi2(Q,z,w) = swi2(Q,x,y);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel1) {
  U -= swi2(B(U),x,y);
  return B(U);
}
__KERNEL__ void DynamicalGraphFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    U+=0.5f;
    Q = T(U,R,iChannel1);
    float4 
        n = T(U+to_float2(0,1),R,iChannel1),
        e = T(U+to_float2(1,0),R,iChannel1),
        s = T(U-to_float2(0,1),R,iChannel1),
        w = T(U-to_float2(1,0),R,iChannel1),
        m = 0.25f*(n+e+s+w);
    float div = 0.25f*(e.x-w.x+n.y-s.y);
    float2 grad = 0.25f*to_float2(e.z-w.z,n.z-s.z);
    //swi2(Q,x,y) -= grad;
    Q.x-=grad.x, Q.y-=grad.y;
    Q.z  = m.z-div;
    swi2S(Q,x,y, _mix(swi2(Q,x,y),to_float2(0.7f,0),_expf(-0.3f*length(U-to_float2(0.3f,0.4f)*R))));
    swi2S(Q,x,y, _mix(swi2(Q,x,y),to_float2(-0.7f,0),_expf(-0.3f*length(U-to_float2(0.7f,0.6f)*R))));
    if (length(U-to_float2(0.7f,0.6f)*R) < 10.0f)         Q.x=-0.7f,Q.y=0.0f;//swi2(Q,x,y) = to_float2(-0.7f,0);
    if (iFrame < 1 || Reset)                              Q = to_float4_s(0);
    if (U.x < 1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1

#ifdef xxx
void X (inout float4 Q, float2 U, float2 r) {
    float4 n = A(U+r);
    if (ln(U,swi2(n,x,y),swi2(n,z,w)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q = n;
    else if (ln(U,swi2(Q,x,y),swi2(n,z,w)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) swi2(Q,z,w) = swi2(n,z,w);
    else if (ln(U,swi2(Q,x,y),swi2(n,x,y)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) swi2(Q,z,w) = swi2(n,x,y);
}
#endif
__KERNEL__ void DynamicalGraphFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(TexStart, 0);
    CONNECT_SLIDER1(Level, -1.0f, 100.0f, 30.0f);
    CONNECT_POINT0(ClampStart, 0.0f, 0.0f );
    
    U+=0.5f;
    Q = A(U);
    Q=X(Q,U,to_float2(1,0),R,iChannel0);
    Q=X(Q,U,to_float2(0,1),R,iChannel0);
    Q=X(Q,U,to_float2(0,-1),R,iChannel0);
    Q=X(Q,U,to_float2(-1,0),R,iChannel0);
    Q=X(Q,U,to_float2(2,0),R,iChannel0);
    Q=X(Q,U,to_float2(0,2),R,iChannel0);
    Q=X(Q,U,to_float2(0,-2),R,iChannel0);
    Q=X(Q,U,to_float2(-2,0),R,iChannel0);
    
    swi2S(Q,x,y, swi2(Q,x,y) + swi2(B(swi2(Q,x,y)),x,y));
    swi2S(Q,z,w, swi2(Q,z,w) + swi2(B(swi2(Q,z,w)),x,y));
    
    if (Q.x< 1.0f) Q.x=Q.z, Q.y=Q.w; //swi2(Q,x,y) = swi2(Q,z,w);
    if (Q.z< 1.0f) Q.z=Q.x, Q.w=Q.y; //swi2(Q,z,w) = swi2(Q,x,y);
    
    if (iMouse.z>0.0f) {
        float4 n = swi4(iMouse,x,y,x,y);
        if (ln(U,swi2(n,x,y),swi2(n,z,w)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q = n;
        if (ln(U,swi2(Q,x,y),swi2(n,x,y)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q.z=n.x, Q.w=n.y;//swi2(Q,z,w) = swi2(n,x,y);
    }
   
    if (iFrame < 1 || Reset) {
      if(TexStart)
      {
        Q = to_float4_s(0.0f);
        float tex = texture(iChannel2, U/R).w;
        if (tex>0.0f)
          //Q = clamp(swi4(_floor(U/30.0f+0.5f),x,y,x,y)*30.0f,0.2f*swi4(R,x,y,x,y),0.8f*swi4(R,x,y,x,y));
          Q = clamp(swi4(_floor(U/Level+0.5f),x,y,x,y)*Level,(0.2f-ClampStart.x)*swi4(R,x,y,x,y),(0.8f-ClampStart.y)*swi4(R,x,y,x,y));  
      }
      else
        Q = clamp(swi4(_floor(U/Level+0.5f),x,y,x,y)*Level,(0.2f-ClampStart.x)*swi4(R,x,y,x,y),(0.8f-ClampStart.y)*swi4(R,x,y,x,y));  
    }
    if (length(swi2(Q,x,y)-swi2(Q,z,w)) > 100.0f) Q.z=Q.x, Q.w=Q.y; //swi2(Q,z,w) = swi2(Q,x,y);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1


//vec4 T (float2 U) {
//  U -= B(U).xy;
//    return B(U);
//}
__KERNEL__ void DynamicalGraphFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    U+=0.5f;
    Q = T(U,R,iChannel1);
    float4 
        n = T(U+to_float2(0,1),R,iChannel1),
        e = T(U+to_float2(1,0),R,iChannel1),
        s = T(U-to_float2(0,1),R,iChannel1),
        w = T(U-to_float2(1,0),R,iChannel1),
        m = 0.25f*(n+e+s+w);
    float div = 0.25f*(e.x-w.x+n.y-s.y);
    float2 grad = 0.25f*to_float2(e.z-w.z,n.z-s.z);
    //swi2(Q,x,y) -= grad;
    Q.x-=grad.x, Q.y-=grad.y;
    Q.z  = m.z-div;
    swi2S(Q,x,y, _mix(swi2(Q,x,y),to_float2(0.7f,0),_expf(-0.3f*length(U-to_float2(0.3f,0.4f)*R))));
    swi2S(Q,x,y, _mix(swi2(Q,x,y),to_float2(-0.7f,0),_expf(-0.3f*length(U-to_float2(0.7f,0.6f)*R))));
    if (length(U-to_float2(0.7f,0.6f)*R) < 10.0f)         Q.x=-0.7f,Q.y=0.0f;//swi2(Q,x,y) = to_float2(-0.7f,0);
    if (iFrame < 1 || Reset)                              Q = to_float4_s(0);
    if (U.x < 1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void DynamicalGraphFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER0(BUz, -1.0f, 10.0f, 3.0f);
  
    U+=0.5f;
    Q = to_float4_s(0);
    for (int x = -2; x <= 2; x++)
        for (int y = -2; y <= 2; y++) {
        float4 a = A(U+to_float2(x,y));
        float2 l = ln(U,swi2(a,x,y),swi2(a,z,w));
        float3 r = to_float3(length(U-swi2(a,x,y)),length(U-swi2(a,z,w)),length(swi2(a,x,y)-swi2(a,z,w)));
        Q += 0.5f*_expf(-0.04f*r.z)*(_expf(-l.x)-0.25f*_expf(-r.x)-0.25f*_expf(-r.y));
     }
   Q *= 0.5f+0.5f*cos_f4(B(U).z*BUz+to_float4(1,2,3,4));

   Q+=Color-0.5f;
   Q.w = Color.w;

  SetFragmentShaderComputedColor(Q);
}