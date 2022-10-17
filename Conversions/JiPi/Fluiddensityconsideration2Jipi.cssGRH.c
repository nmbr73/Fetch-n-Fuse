
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define r 12.
#define v to_float2(-0.1f,0)

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0
// Connect Buffer A 'Cubemap: Forest_0' to iChannel1
// Connect Buffer A 'Texture: Blending' to iChannel2

__DEVICE__ float4 T(float2 U, float2 R, __TEXTURE2D__ iChannel0) {
    U -= swi2(A(U),x,y);
    return A(U);
}
__KERNEL__ void Fluiddensityconsideration2JipiFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{

    CONNECT_CHECKBOX0(Reset, 0);  
    CONNECT_SLIDER0(Size1, 0.0f, 50.0f, 20.0f);
    CONNECT_POINT0(Pos1, 0.0f, 0.0f);
    CONNECT_POINT1(Direction1, -0.1f, 0.0f);
    CONNECT_SLIDER1(Strength1, -1.0f, 5.0f, 1.05f);
    
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    
    U+=0.5f;
    Q = T(U,R,iChannel0);
    float4  n = T(U+to_float2(0,1),R,iChannel0),
            e = T(U+to_float2(1,0),R,iChannel0),
            s = T(U-to_float2(0,1),R,iChannel0),
            w = T(U-to_float2(1,0),R,iChannel0),
            m = 0.25f*(n+e+s+w);
    float div = 0.25f*(n.y+e.x-s.y-w.x);
    swi2S(Q,x,y, swi2(Q,x,y) - 0.25f/_fmaxf(1.0f,Q.w)*to_float2(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w));
    Q.z = m.z-div;
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)    Q = to_float4(0,0.3f,Q.z,1.05f);
    //if (_fabs(U.x-0.2f*R.x)<20.0f&&_fabs(U.y-0.5f*R.y)<20.0f) Q = to_float4(v.x,v.y,Q.z,1.05f);
    if (_fabs(U.x-(0.2f+Pos1.x)*R.x)<Size1&&_fabs(U.y-(0.5f+Pos1.y)*R.y)<Size1) Q = to_float4(Direction1.x,Direction1.y,Q.z,Strength1);
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    if (iFrame < 1||U.x<2.0f||R.x-U.x<2.0f || Reset)        Q = to_float4(0.3f,0,1,1);
    if (U.y<2.0f||R.y-U.y<2.0f)                             Q = to_float4(0,0,Q.z,1);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void Fluiddensityconsideration2JipiFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{

    CONNECT_CHECKBOX0(Reset, 0);  
    CONNECT_SLIDER0(Size1, 0.0f, 50.0f, 20.0f);
    CONNECT_POINT0(Pos1, 0.0f, 0.0f);
    CONNECT_POINT1(Direction1, -0.1f, 0.0f);
    CONNECT_SLIDER1(Strength1, -1.0f, 5.0f, 1.05f);

    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

      
    U+=0.5f;
    Q = T(U,R,iChannel0);
    float4  n = T(U+to_float2(0,1),R,iChannel0),
            e = T(U+to_float2(1,0),R,iChannel0),
            s = T(U-to_float2(0,1),R,iChannel0),
            w = T(U-to_float2(1,0),R,iChannel0),
            m = 0.25f*(n+e+s+w);
    float div = 0.25f*(n.y+e.x-s.y-w.x);
    swi2S(Q,x,y, swi2(Q,x,y) - 0.25f/_fmaxf(1.0f,Q.w)*to_float2(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w));
    Q.z = m.z-div;
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)    Q = to_float4(0,0.3f,Q.z,1.05f);
    //if (_fabs(U.x-0.2f*R.x)<20.0f&&_fabs(U.y-0.5f*R.y)<20.0f) Q = to_float4(v.x,v.y,Q.z,1.05f);
    if (_fabs(U.x-(0.2f+Pos1.x)*R.x)<Size1&&_fabs(U.y-(0.5f+Pos1.y)*R.y)<Size1) Q = to_float4(Direction1.x,Direction1.y,Q.z,Strength1);
    
    if (iFrame < 1||U.x<2.0f||R.x-U.x<2.0f||Reset)          Q = to_float4(0.3f,0,1,1);
    if (U.y<2.0f||R.y-U.y<2.0f)                             Q = to_float4(0,0,Q.z,1);

//if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0

__KERNEL__ void Fluiddensityconsideration2JipiFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{

    CONNECT_CHECKBOX0(Reset, 0);  
    CONNECT_SLIDER0(Size1, 0.0f, 50.0f, 20.0f);
    CONNECT_POINT0(Pos1, 0.0f, 0.0f);
    CONNECT_POINT1(Direction1, -0.1f, 0.0f);
    CONNECT_SLIDER1(Strength1, -1.0f, 5.0f, 1.05f);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = T(U,R,iChannel0);
    float4  n = T(U+to_float2(0,1),R,iChannel0),
            e = T(U+to_float2(1,0),R,iChannel0),
            s = T(U-to_float2(0,1),R,iChannel0),
            w = T(U-to_float2(1,0),R,iChannel0),
            m = 0.25f*(n+e+s+w);
    float div = 0.25f*(n.y+e.x-s.y-w.x);
    swi2S(Q,x,y, swi2(Q,x,y) - 0.25f/_fmaxf(1.0f,Q.w)*to_float2(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w));
    Q.z = m.z-div;
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)    Q = to_float4(0,0.3f,Q.z,1.05f);
    //if (_fabs(U.x-0.2f*R.x)<20.0f&&_fabs(U.y-0.5f*R.y)<20.0f) Q = to_float4(v.x,v.y,Q.z,1.05f);
    if (_fabs(U.x-(0.2f+Pos1.x)*R.x)<Size1&&_fabs(U.y-(0.5f+Pos1.y)*R.y)<Size1) Q = to_float4(Direction1.x,Direction1.y,Q.z,Strength1);
    
    if (iFrame < 1||U.x<2.0f||R.x-U.x<2.0f||Reset)          Q = to_float4(0.3f,0,1,1);
    if (U.y<2.0f||R.y-U.y<2.0f)                             Q = to_float4(0,0,Q.z,1);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void Fluiddensityconsideration2JipiFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{

    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER0(Size1, 0.0f, 50.0f, 20.0f);
    CONNECT_POINT0(Pos1, 0.0f, 0.0f);
    CONNECT_POINT1(Direction1, -0.1f, 0.0f);
    CONNECT_SLIDER1(Strength1, -1.0f, 5.0f, 1.05f);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
  
    U+=0.5f;
    Q = T(U,R,iChannel0);
    float4  n = T(U+to_float2(0,1),R,iChannel0),
            e = T(U+to_float2(1,0),R,iChannel0),
            s = T(U-to_float2(0,1),R,iChannel0),
            w = T(U-to_float2(1,0),R,iChannel0),
            m = 0.25f*(n+e+s+w);
    float div = 0.25f*(n.y+e.x-s.y-w.x);
    swi2S(Q,x,y, swi2(Q,x,y) - 0.25f/_fmaxf(1.0f,Q.w)*to_float2(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w));
    Q.z = m.z-div;
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)    Q = to_float4(0,0.3f,Q.z,1.05f);
    //if (_fabs(U.x-0.2f*R.x)<20.0f&&_fabs(U.y-0.5f*R.y)<20.0f) Q = to_float4(v.x,v.y,Q.z,1.05f);
    if (_fabs(U.x-(0.2f+Pos1.x)*R.x)<Size1&&_fabs(U.y-(0.5f+Pos1.y)*R.y)<Size1) Q = to_float4(Direction1.x,Direction1.y,Q.z,Strength1);
    
    if (iFrame < 1||U.x<2.0f||R.x-U.x<2.0f||Reset)          Q = to_float4(0.3f,0,1,1);
    if (U.y<2.0f||R.y-U.y<2.0f)                             Q = to_float4(0,0,Q.z,1);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void Fluiddensityconsideration2JipiFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel2)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  
    U+=0.5f;
    float4 a = A(U),
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    float3 no = normalize(to_float3(e.z+e.w-w.z-w.w,n.z+n.w-s.z-s.w,1));

    Q = to_float4_s(0.7f)+0.7f*sin_f4(a+4.0f*a.z+100.0f*a.w+to_float4(1,2,3,4));
    Q *= 0.5f+0.5f*decube_f3(iChannel2,no);
    
    if (Invers) Q = to_float4_s(1.0f) - Q;
    
    Q = (Q + Color-0.5f);// * Q.w;

    Q.w = Color.w;

  SetFragmentShaderComputedColor(Q);
}