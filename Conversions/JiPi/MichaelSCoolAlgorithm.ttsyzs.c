
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//float4 M;
//float T;
//int I;
#define A(U) texture(cha,(U)/R)
#define B(U) texture(chb,(U)/R)
//#define Main void mainImage (out float4 Q, in float2 U)
__DEVICE__ float signe (float x) {return _atan2f(100.0f*x, 1.0f);}

__DEVICE__ void prog (float2 U, out float4 *a, out float4 *b, float2 R, __TEXTURE2D__ cha, __TEXTURE2D__ chb) {
  
    *a = to_float4_s(0); *b = to_float4_s(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        float2 u = to_float2(x,y);
        float4 aa = A(U+u), bb = B(U+u);
        swi2S(aa,x,y, swi2(aa,x,y) + swi2(aa,z,w));
        #define q clamp(bb.x,1.0f,1.1f)
        float4 w = clamp(to_float4_f2f2(swi2(aa,x,y)-0.5f*q,swi2(aa,x,y)+0.5f*q), swi4(U,x,y,x,y) - to_float4_s(0.5f), swi4(U,x,y,x,y) + to_float4_s(0.5f));
        float m = (w.w-w.y)*(w.z-w.x)/(q*q);
        swi2S(aa,x,y, 0.5f*(swi2(w,x,y)+swi2(w,z,w)));
        *a += aa*bb.x*m;
        (*b).x += bb.x*m;
        swi3S(*b,y,z,w, swi3(*b,y,z,w) + swi3(bb,y,z,w)*bb.x*m);
    }
    if ((*b).x>0.0f) {
        *a/=(*b).x;
        //b.yzw/=b.x;
        (*b).y/=(*b).x,(*b).z/=(*b).x,(*b).w/=(*b).x;
    }
}
__DEVICE__ void prog2 (float2 U, out float4 *a, out float4 *b, float2 R, float T, int I, float4 M, bool Reset, bool TexOn, float2 BlendMulOff, __TEXTURE2D__ cha, __TEXTURE2D__ chb, __TEXTURE2D__ chc) {
  
    *a = A(U); *b = B(U);
    float2 f = to_float2_s(0); float m = 0.0f;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    
    if (_fabs(x)!=_fabs(y))
    {
        float2 u = to_float2(x,y);
        float4 aa = A(U+u), bb = B(U+u);
        f += 0.01f*(bb.x*(0.05f-0.01f*bb.x))*u;
        m += bb.x;
    }
    if (m>0.0f) (*a).z+=f.x, (*a).w+=f.y; //swi2(a,z,w) += f;
    
    
    // Boundaries:
    (*a).w -= 0.1f/R.y*signe((*b).x);
    if ((*a).x<10.0f) {(*a).z -= -0.1f;(*b).z*=0.9f;} if (R.x-(*a).x<10.0f) {(*a).z -= 0.1f;(*b).z*=0.9f;}if ((*a).y<10.0f) {(*a).w -= -0.05f;(*b).z*=0.9f;}if (R.y-(*a).y<10.0f) {(*a).w -= 0.1f;(*b).z*=0.9f;}
    if (I<1||Reset||U.x<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f||R.x-U.x<1.0f) {
        if(TexOn)
        {
          float4 Tex = texture(chc,U/R);
          *b = to_float4_s(0);
          if(Tex.w)
          {        
            *a = to_float4(U.x,U.y,0,0);
            //*b = to_float4_s(0);
            (*b).x = (Tex.x+BlendMulOff.y)*BlendMulOff.x;//15.0f;
            (*b).w = (Tex.x+BlendMulOff.y)*BlendMulOff.x;//1.0f;
          }
        }
        else
        {
          *a = to_float4(U.x,U.y,0,0);
          *b = to_float4_s(0);
          if (length(U-0.5f*R) < 0.4f*R.y) (*b).x = 15.0f;
          if (U.x<0.5f*R.x)                (*b).w = 1.0f;
        }
    }
    if (M.z>0.0f && length(U-swi2(M,x,y)) < 20.0f) {
        (*b).x = 2.0f;
        //swi2(a,x,y) = U;
        (*a).x=U.x,(*a).y=U.y;
        swi2S(*a,z,w, 0.6f*to_float2(_cosf(0.1f*T),_sinf(0.1f*T)));
        (*b).w = 2.0f;
    }
}


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
          Q = _mix(Q, to_float4(U.x,U.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend);
          //swi2S(Q,x,y, _mix(swi2(Q,x,y), to_float2(U.x,U.y), Blend));
          

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4_aw((swi3(tex,x,y,z)+MulOff.y)*MulOff.x,(tex.x+MulOff.y)*MulOff.x),Blend);
        
        
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
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1
// Connect Buffer A 'Texture: Blending' to iChannel2

__KERNEL__ void MichaelSCoolAlgorithmFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    U+=0.5f;
   
    float4 a, b;   
    prog (U,&a,&b,R,iChannel0,iChannel1);
    
    Q = a;
    
    //if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer C' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void MichaelSCoolAlgorithmFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    U+=0.5f;
    
    float4 a, b;
    prog (U,&a,&b,R,iChannel0,iChannel1);
    
    Q = b;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Texture: Blending' to iChannel2


__KERNEL__ void MichaelSCoolAlgorithmFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(TexOn, 0);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    U+=0.5f;    
float CCCCCCCCCCCCCCCCCCC;    
    float4 a, b;
    prog2 (U,&a,&b,R,iTime,iFrame,iMouse,Reset,TexOn, to_float2(Blend1Mul,Blend1Off),iChannel0,iChannel1,iChannel2);
    
    Q = a;

    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Texture: Blending' to iChannel2


__KERNEL__ void MichaelSCoolAlgorithmFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(TexOn, 0);
    
    //Blending
    CONNECT_SLIDER5(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend2Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(Blend2Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus2, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT3(Par2, 0.0f, 0.0f);
  
    U+=0.5f; 
    
    float4 a, b;    
    prog2 (U,&a,&b,R,iTime,iFrame,iMouse,Reset,TexOn, to_float2(Blend2Mul,Blend2Off),iChannel0,iChannel1,iChannel2);
       
    Q = b;
    
    if (Blend2>0.0) Q = Blending(iChannel2, U/R, Q, Blend2, Par2, to_float2(Blend2Mul,Blend2Off), Modus2, U, R);

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1

__KERNEL__ void MichaelSCoolAlgorithmFuse(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
{
  
    U+=0.5f;    

    Q = texture(iChannel1,U/R);
    Q.x = 0.4f*_logf(1.0f+Q.x);
    Q = Q.x*(to_float4_s(0.6f)+0.4f*cos_f4(1.2f*Q.w*to_float4(1,2,3,4)));

  SetFragmentShaderComputedColor(Q);    
}