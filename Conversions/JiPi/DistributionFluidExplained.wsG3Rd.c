
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define T(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
__DEVICE__ float2 h(float2 p) // Dave H
{
    float3 p3 = fract_f3(swi3(p,x,y,x) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));

}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


// FLUID EVOLUTION
// Velocity
__DEVICE__ float2 v (float4 b) {
  return to_float2(b.x-b.y,b.z-b.w);
}
// Pressure
__DEVICE__ float p (float4 b) {
  return 0.25f*(b.x+b.y+b.z+b.w);
}
// TRANSLATE COORD BY Velocity THEN LOOKUP STATE
__DEVICE__ float4 A(float2 U, float2 R, __TEXTURE2D__ iChannel0) {
    U-=0.5f*v(T(U));
    U-=0.5f*v(T(U));
    return T(U);
}
__KERNEL__ void DistributionFluidExplainedFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(ParXY, 0.0f, 0.0f);
    CONNECT_POINT0(ParZW, 0.0f, 0.0f);
  
    U+=0.5f;

    // THIS PIXEL
    Q = A(U,R,iChannel0);
    float pq = p(Q);
    // NEIGHBORHOOD
    float4 
        n = A(U+to_float2(0,1),R,iChannel0),
        e = A(U+to_float2(1,0),R,iChannel0),
        s = A(U-to_float2(0,1),R,iChannel0),
        w = A(U-to_float2(1,0),R,iChannel0);
    // GRADIENT of PRESSURE
    float px = 0.25f*(p(e)-p(w));
    float py = 0.25f*(p(n)-p(s)); 
    // boundary Energy exchange in :   
    Q += to_float4_s(0.25f*(n.w + e.y + s.z + w.x)
          // boundary Energy exchange out :
          -pq)
          // dV/dt = dP/dx,  dEnergy In dTime = dEnergy in dSpace
          -1.0f*to_float4(px,-px,py,-py);
    
    

    //Blending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = B(U);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          Q = _mix(Q,(tex+Blend1Off)*Blend1Mul,Blend1);

        if ((int)Modus&4)
          swi2S(Q,x,y, _mix( swi2(Q,x,y), swi2(tex,x,y) , Blend1));
        
        if ((int)Modus&8)
          Q = _mix( Q, to_float4_f2f2(ParXY,ParZW), Blend1);

        if ((int)Modus&16) 
          Q = _mix( Q, to_float4_s(0.2f), Blend1);

      }
      else
        if ((int)Modus&32) //Special
          swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+Blend1Off)*Blend1Mul,Blend1));
        
    }
    
    
    // boundary conditions
    if (iFrame < 1 || Reset) Q = to_float4_s(0.2f);
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f) {
      Q = pq+0.1f*to_float4(_sinf(iTime),-_sinf(iTime),_cosf(iTime),-_cosf(iTime));;
    }
    else if (length(U-to_float2(0.1f,0.5f)*R)<15.0f) {
      swi2S(Q,x,y, swi2(Q,x,y) + 0.01f*to_float2(1,-1)/(1.0f+pq));
    }
    if(U.x<3.0f||R.x-U.x<3.0f||U.y<3.0f||R.y-U.y<3.0f)    Q = to_float4_s(p(Q));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0

__KERNEL__ void DistributionFluidExplainedFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
  
    U+=0.5f;

    // THIS PIXEL
    Q = A(U,R,iChannel0);
    float pq = p(Q);
    // NEIGHBORHOOD
    float4 
        n = A(U+to_float2(0,1),R,iChannel0),
        e = A(U+to_float2(1,0),R,iChannel0),
        s = A(U-to_float2(0,1),R,iChannel0),
        w = A(U-to_float2(1,0),R,iChannel0);
    // GRADIENT of PRESSURE
    float px = 0.25f*(p(e)-p(w));
    float py = 0.25f*(p(n)-p(s)); 
        // boundary Energy exchange in :   
    Q += to_float4_s(0.25f*(n.w + e.y + s.z + w.x)
          // boundary Energy exchange out :
          -pq)
          // dV/dt = dP/dx,  dEnergy In dTime = dEnergy in dSpace
          -to_float4(px,-px,py,-py);
    
    
    // boundary conditions
    if (iFrame < 1 || Reset) Q = to_float4_s(0.2f);
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f) {
      Q = pq+0.1f*to_float4(_sinf(iTime),-_sinf(iTime),_cosf(iTime),-_cosf(iTime));;
    }
    else if (length(U-to_float2(0.1f,0.5f)*R)<15.0f) {
      swi2S(Q,x,y, swi2(Q,x,y) + 0.01f*to_float2(1,-1)/(1.0f+pq));
    }
    if(U.x<3.0f||R.x-U.x<3.0f||U.y<3.0f||R.y-U.y<3.0f)Q = to_float4_s(p(Q));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void DistributionFluidExplainedFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

  
    U+=0.5f;

    // THIS PIXEL
    Q = A(U,R,iChannel0);
    float pq = p(Q);
    // NEIGHBORHOOD
    float4 
        n = A(U+to_float2(0,1),R,iChannel0),
        e = A(U+to_float2(1,0),R,iChannel0),
        s = A(U-to_float2(0,1),R,iChannel0),
        w = A(U-to_float2(1,0),R,iChannel0);
    // GRADIENT of PRESSURE
    float px = 0.25f*(p(e)-p(w));
    float py = 0.25f*(p(n)-p(s)); 
        // boundary Energy exchange in :   
    Q += to_float4_s(0.25f*(n.w + e.y + s.z + w.x)
          // boundary Energy exchange out :
          -pq)
          // dV/dt = dP/dx,  dEnergy In dTime = dEnergy in dSpace
          -to_float4(px,-px,py,-py);
    
    
    // boundary conditions
    if (iFrame < 1 || Reset) Q = to_float4_s(0.2f);
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f) {
      Q = pq+0.1f*to_float4(_sinf(iTime),-_sinf(iTime),_cosf(iTime),-_cosf(iTime));;
    }
    else if (length(U-to_float2(0.1f,0.5f)*R)<15.0f) {
      swi2S(Q,x,y, swi2(Q,x,y) + 0.01f*to_float2(1,-1)/(1.0f+pq));
    }
    if(U.x<3.0f||R.x-U.x<3.0f||U.y<3.0f||R.y-U.y<3.0f)  Q = to_float4_s(p(Q));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void DistributionFluidExplainedFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

  
    U+=0.5f;

    // THIS PIXEL
    Q = A(U,R,iChannel0);
    float pq = p(Q);
    // NEIGHBORHOOD
    float4 
        n = A(U+to_float2(0,1),R,iChannel0),
        e = A(U+to_float2(1,0),R,iChannel0),
        s = A(U-to_float2(0,1),R,iChannel0),
        w = A(U-to_float2(1,0),R,iChannel0);
    // GRADIENT of PRESSURE
    float px = 0.25f*(p(e)-p(w));
    float py = 0.25f*(p(n)-p(s)); 
        // boundary Energy exchange in :   
    Q += to_float4_s(0.25f*(n.w + e.y + s.z + w.x)
          // boundary Energy exchange out :
          -pq)
          // dV/dt = dP/dx,  dEnergy In dTime = dEnergy in dSpace
          -to_float4(px,-px,py,-py);
    
    
    // boundary conditions
    if (iFrame < 1 || Reset) Q = to_float4_s(0.2f);
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f) {
      Q = pq+0.1f*to_float4(_sinf(iTime),-_sinf(iTime),_cosf(iTime),-_cosf(iTime));;
    }
    else if (length(U-to_float2(0.1f,0.5f)*R)<15.0f) {
      swi2S(Q,x,y, swi2(Q,x,y) + 0.01f*to_float2(1,-1)/(1.0f+pq));
    }
    if(U.x<3.0f||R.x-U.x<3.0f||U.y<3.0f||R.y-U.y<3.0f)Q = to_float4_s(p(Q));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


/*
  A,B,C,and D are the same and have have the fluid code

  Each pixel stores +Energy,-Energy in each direction
  
  swi4(Q,x,y,z,w) = ( +x,-x,+y,-y )
  
  At each step, the fluid completely exchanges its state
  with its neighborhood. 

  +x goes to the +x cell, -x goes to the -x cell
  +y goes to the +y cell, -y goes to the -y cell

  Then the fluid accelerates
  
  The gradient of pressure polarizes the Energy

  (+x,-x) += dP/dx * (-,+)
  (+y,-y) += dP/dy * (-,+)

*/


// This part makes the lines on the fluid.

//#define X 3.0f
//#define L 3.0f


__DEVICE__ float ln (float2 p, float2 a, float2 d,float i) {
    float r = clamp(dot(p-a,d)/dot(d,d),0.0f,1.0f);
  return length(p-a-d*r);
}
__KERNEL__ void DistributionFluidExplainedFuse(float4 Q, float2 U, float2 iResolution, float iTime)
{
  
    CONNECT_CHECKBOX0(Reset, 0); 

    CONNECT_SLIDER3(StrengthV, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(X, -10.0f, 10.0f, 3.0f);
    CONNECT_SLIDER5(L, -10.0f, 10.0f, 3.0f);
    CONNECT_POINT1(Par2, 0.0f, 0.0f);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);

  
    U+=0.5f;

    float2 a = 10.0f*v(T(U))*StrengthV;
    Q =  to_float4_s(1);
    for (int _x = -3; _x <= 3; _x++)
    for (int _y = -3; _y <= 3; _y++) {
        float2 V = _floor(U/X+0.5f+to_float2(_x,_y))*X;
        V += X*h(V)-0.5f;
        V += X*sin_f2(10.0f*h(2.0f*V).x+to_float2(0,3.14f/2.0f)+iTime)-0.5f;
        a = v(T(V));
        for (float i = 0.0f; i < L; i++) {
            Q += smoothstep(1.5f,0.25f,ln(U,V,X*swi2(a,x,y)/L,i));
            V += X*swi2(a,x,y)/L;
            a = v(T(V));
        }   Q += smoothstep(1.5f,0.25f,ln(U,V,X*swi2(a,x,y)/L,L));
    }
    Q  *= 0.05f+0.2f*swi4(T(U),x,z,y,w);
    
    if ( T(U).y < 0.0f )
      Q = to_float4_aw(swi3(Q,x,y,z)+(swi3(Color,x,y,z)-0.5f), Color.w);

    Q.w=Color.w;

  SetFragmentShaderComputedColor(Q);
}