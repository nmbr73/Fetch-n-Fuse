
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)


#define Me Q = A(U);
#define Them float4 M = 0.25f*(A(U+to_float2(0,1))+A(U+to_float2(1,0))+A(U-to_float2(0,1))+A(U-to_float2(1,0)));

#define F (0.5f*(M-Q-Q*(dot(swi2(Q,x,y),swi2(Q,x,y))-length(swi2(Q,x,y)))-0.01f*Q.z))

#define Mouse if (iMouse.z>0.0f && length(U-swi2(iMouse,x,y))<10.0f) { Q.x=_sinf(U.x+U.y); Q.y=_cosf(U.x+U.y); }


#define First if (iFrame < 1) {Q = to_float4(1,0,0,0); if (length(U-0.5f*R)<30.0f) { Q.x=_sinf(U.x); Q.y=_cosf(U.x);}} //swi2(Q,x,y) = to_float2(_sinf(U.x),_cosf(U.x));}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void SurprisingPhenomenonFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    U+0.5f;
float AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA;
    Me
    Them
       
    Q.x += (F).y;
        
    Mouse 
    First


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SurprisingPhenomenonFuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{
    U+0.5f;
float BBBBBBBBBBBBBBBBBB;    
    Me
    Them
        
    Q.y -= (F).x;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__DEVICE__ float angle (float2 a, float2 b) {
  return _atan2f(a.x*b.y-a.y*b.x,dot(swi2(a,x,y),swi2(b,x,y)));
}
__KERNEL__ void SurprisingPhenomenonFuse__Buffer_C(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
    Me
    float4
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    
    Q.x = angle(swi2(e,x,y),swi2(w,x,y));
    Q.y = angle(swi2(n,x,y),swi2(s,x,y));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel1


__KERNEL__ void SurprisingPhenomenonFuse__Buffer_D(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
    Me
    Them
    float4
        n = B(U+to_float2(0,1)),
        e = B(U+to_float2(1,0)),
        s = B(U-to_float2(0,1)),
        w = B(U-to_float2(1,0));
   Q.z = _mix(Q.z,M.z,0.5f)+0.25f*(n.y+e.x-s.y-w.x);
  
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SurprisingPhenomenonFuse(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f; 
    
    Me

  SetFragmentShaderComputedColor(Q);
}