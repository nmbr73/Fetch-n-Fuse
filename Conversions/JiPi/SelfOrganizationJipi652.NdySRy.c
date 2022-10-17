
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define A(U) texture(iChannel0, (U)/R)
#define B(U) texture(iChannel1, (U)/R)
#define C(U) texture(iChannel2, (U)/R)
#define D(U) texture(iChannel3, (U)/R)

//controls :

//#define all 1.0f
//#define M   0.2f
//#define PN  0.5f
//#define RGB 0.5f

//#define loss 0.03f



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
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
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
// Connect Buffer A 'Texture: London' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void SelfOrganizationJipi652Fuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel3)
{ 
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Textur, 0);
    CONNECT_SLIDER0(TexMul, -10.0f, 20.0f, 5.0f);
    
        //Blending
    CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    
    U+=0.5f;
    Q = A(U);
    Q += B(U);
    //if (iFrame < 1) Q = 5.0f*_sinf(0.01f*length(U-0.5f*R)*to_float4(1,2,3,4));
    if (iFrame < 1 || Reset) 
    {
      if(Textur)
        Q = texture(iChannel3,U/iResolution)*TexMul - (TexMul/2.0f);//*5.0f - 2.5f;//
      else
        Q = 5.0f*sin_f4(0.01f*length(U-0.5f*R)*to_float4(1,2,3,4));
    }
    
    if (Blend1>0.0) Q = Blending(iChannel3, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    if(iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<24.0f||(length(U-0.5f*R)<2.0f&&iFrame<2)) Q = to_float4_s(0);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void SelfOrganizationJipi652Fuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX2(VarianteRGB, 0);
    CONNECT_SLIDER3(all,  -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(M,    -10.0f, 10.0f, 0.2f);
    CONNECT_SLIDER5(PN,   -10.0f, 10.0f, 0.5f);
    CONNECT_SLIDER6(RGB,  -10.0f, 10.0f, 0.5f);
    CONNECT_SLIDER7(loss, -0.5f, 0.5f, 0.03f);
    
    U+=0.5f;
    Q = B(U);
    float4 a = A(U);
    float4 c = C(U);
    float4 dm = 1.0f/4.0f*(
        A(U+to_float2(1,0))+
        A(U+to_float2(0,1))+
        A(U-to_float2(1,0))+
        A(U-to_float2(0,1))
    )-a;
    float4 dmb = 1.0f/4.0f*(
        B(U+to_float2(1,0))+
        B(U+to_float2(0,1))+
        B(U-to_float2(1,0))+
        B(U-to_float2(0,1))
    )-Q;
    Q += dm + loss*dmb;
    float mag = _sqrtf(a.w*a.w+dot(swi2(c,x,y),swi2(c,x,y))+dot(swi3(a,x,y,z),swi3(a,x,y,z)));
    if ((length(swi3(a,x,y,z))>0.0f)&&VarianteRGB) swi3S(Q,x,y,z, swi3(Q,x,y,z) + RGB*3.0f/6.0f*swi3(a,x,y,z)/length(swi3(a,x,y,z)));
    
    if (_fabs(a.w)>0.0f) Q.w+= M*1.0f/6.0f*a.w/_fabs(a.w);
    if (mag > 0.0f) Q -= all*a/mag;
    if(iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<24.0f||(length(U-0.5f*R)<2.0f&&iFrame<2)) Q = to_float4_s(0);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q *= 0.0f;

    if (iFrame < 1 || Reset) 
    {
      Q = to_float4_s(0.0f);
    }


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Texture: London' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void SelfOrganizationJipi652Fuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Textur, 0);
    CONNECT_SLIDER0(TexMul, -10.0f, 20.0f, 5.0f);
    U+=0.5f;
    Q = C(U);
    Q += D(U);
    //if (iFrame < 1) Q = 5.0f*_sinf(0.01f*length(U-0.5f*R)*to_float4(4,5,3,4));
    if (iFrame < 1 || Reset) 
    {
      if(Textur)
        Q = texture(iChannel0,U/iResolution)*TexMul - (TexMul/2.0f);//*5.0f - 2.5f;//
      else
        Q = 5.0f*sin_f4(0.01f*length(U-0.5f*R)*to_float4(1,2,3,4));
    }
    
    if(iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<24.0f||(length(U-0.5f*R)<2.0f&&iFrame<2)) Q = to_float4_s(0);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q *= 0.0f;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void SelfOrganizationJipi652Fuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX3(VariantePN, 0);
    
    CONNECT_SLIDER3(all,  -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(M,    -10.0f, 10.0f, 0.2f);
    CONNECT_SLIDER5(PN,   -10.0f, 10.0f, 0.5f);
    CONNECT_SLIDER6(RGB,  -10.0f, 10.0f, 0.5f);
    CONNECT_SLIDER7(loss, -0.5f, 0.5f, 0.03f);
    
    U+=0.5f;
    Q = D(U);
    float4 a = A(U);
    float4 c = C(U);
    float4 dm = 1.0f/4.0f*(
        C(U+to_float2(1,0))+
        C(U+to_float2(0,1))+
        C(U-to_float2(1,0))+
        C(U-to_float2(0,1))
    )-c;
    float4 dmd = 1.0f/4.0f*(
        D(U+to_float2(1,0))+
        D(U+to_float2(0,1))+
        D(U-to_float2(1,0))+
        D(U-to_float2(0,1))
    )-Q;
    Q += dm + loss*dmd;
    float mag = _sqrtf(a.w*a.w+dot(swi2(c,x,y),swi2(c,x,y))+dot(swi3(a,x,y,z),swi3(a,x,y,z)));
    if ((length(swi2(c,x,y))>0.0f) && VariantePN) swi2S(Q,x,y, swi2(Q,x,y) + 2.0f/6.0f*PN*swi2(c,x,y)/length(swi2(c,x,y)));
    if (mag > 0.0f) swi2S(Q,x,y, swi2(Q,x,y) - all*swi2(c,x,y)/mag);
    if(iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<24.0f||(length(U-0.5f*R)<2.0f&&iFrame<2)) Q = to_float4_s(0);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q *= 0.0f;

    if (iFrame < 1 || Reset) 
    {
      Q = to_float4_s(0.0f);
    }


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


__DEVICE__ float T(float2 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3) {
    float4 a = A(U);
    float4 b = B(U);
    float4 c = C(U);
    float4 d = D(U);
    return dot(a,a)+dot(b,b)+dot(swi3(c,x,y,z),swi3(c,x,y,z))+dot(swi3(d,x,y,z),swi3(d,x,y,z));
}
__KERNEL__ void SelfOrganizationJipi652Fuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_COLOR0(Color, 0.0f, 0.0f, 1.0f, 1.0f);

    U+=0.5f;
    float4 a = A(U);
    float4 b = B(U);
    float4 c = C(U);
    float4 d = D(U);
    float
        n = T(U+to_float2(0,1),R,iChannel0,iChannel1,iChannel2,iChannel3),
        e = T(U+to_float2(1,0),R,iChannel0,iChannel1,iChannel2,iChannel3),
        s = T(U-to_float2(0,1),R,iChannel0,iChannel1,iChannel2,iChannel3),
        w = T(U-to_float2(1,0),R,iChannel0,iChannel1,iChannel2,iChannel3);
    float3 g = normalize(to_float3(e-w,n-s,1));
    c = 0.1f*to_float4(c.x,0.5f*c.x+0.5f*c.y,c.y,1);
    d = 0.1f*to_float4(d.x,0.5f*d.x+0.5f*d.y,d.y,1);
    Q = 0.8f*(sqrt_f4(c*c+10.0f*d*d+0.01f*(a*a+5.0f*b*b)));

    swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z),normalize(swi3(Q,x,y,z)),_fminf(1.0f,length(swi3(Q,x,y,z)))));
    //Q *= dot(reflect(g,to_float3(0,0,1)),normalize(to_float3_s(1)))*0.3f+0.7f;
    Q *= dot(reflect(g,swi3(Color,x,y,z)),normalize(to_float3_s(1)))*0.3f+0.7f;

    Q.w = Color.w;
  SetFragmentShaderComputedColor(Q);
}