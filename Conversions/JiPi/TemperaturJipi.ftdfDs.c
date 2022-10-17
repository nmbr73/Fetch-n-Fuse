
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
//#define Main void mainImage(out float4 Q, in float2 U)
#define box for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
#define r2 0.70710678118f

__DEVICE__ float4 test_isnan( float4 dQin)
{
  if (isnan(dQin.x))  dQin.x = 0.0001f;
  if (isnan(dQin.y))  dQin.y = 0.0001f;
  if (isnan(dQin.z))  dQin.z = 0.0001f;
  if (isnan(dQin.w))  dQin.w = 0.0001f;
  
  return dQin;  
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

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



// Forces
__KERNEL__ void TemperaturJipiFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    CONNECT_CHECKBOX2(HotCold, 1);  
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    CONNECT_POINT3(LeftRight, 0.0f, 0.0f);
    CONNECT_POINT4(UpDown, 0.0f, 0.0f);
    
    CONNECT_SLIDER5(Qy, -0.1f, 0.1f, 0.0f);
    CONNECT_SLIDER6(Qx, -0.1f, 0.1f, 0.0f);
    
    U+=0.5f;

    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        float f = 0.03f*a.w*(a.w*(a.w-0.9f)+0.4f*a.z);
        swi2S(dQ,x,y, swi2(dQ,x,y) - f*u);
        dQ.z  += a.w*f*dot(swi2(Q,x,y)-swi2(a,x,y),u);
    }
    Q += dQ;
    Q.y -= 5e-5 + Qy; //Schwerkraft
    
    Q.x += Qx;
    
    if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    
    float2 M = 1.5f*R;
    if (iMouse.z>0.0f)            M = swi2(iMouse,x,y);
    if(length(U-M)<0.02f*R.y)     {float2 tmp = -0.5f*normalize(swi2(iMouse,x,y)-0.5f*R);Q = to_float4(tmp.x,tmp.y,0.1f,1.0f);};
    if (length(swi2(Q,x,y))>0.5f) swi2S(Q,x,y, 0.5f*normalize(swi2(Q,x,y)));
    
    // Change boundary/initials:
    if (HotCold) {
    // Left-Right hot cold
        if (iFrame < 1 || Reset)  Q = to_float4(0,0,U.x/R.x,0.3f);
        if (U.x    <18.0f)        Q.z=_mix(Q.z,0.0f+LeftRight.x,0.01f);
        if (R.x-U.x<18.0f||(R.x-U.x<R.x*0.3f&&U.y<10.0f)) Q.z=_mix(Q.z,10.0f+LeftRight.y,0.01f);
    } else {
    // Up down hot cold
        if (iFrame < 1 || Reset)  Q = to_float4(0,0,5.0f*step(U.y+30.0f*_sinf(10.0f*U.x/R.x*6.2f),0.5f*R.y),1.0f);
        if (U.y    <18.0f)        Q.z=_mix(Q.z,5.0f+UpDown.x,0.01f);
        if (R.y-U.y<18.0f)        Q.z=_mix(Q.z,0.0f+UpDown.y,0.01f);
    }
  
    if (U.x < 2.0f||R.x-U.x<2.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    if (U.y < 2.0f||R.y-U.y<2.0f) Q.x*=0.0f,Q.y*=0.0f,Q.z*=0.0f;//swi3(Q,x,y,z) *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Advect
__KERNEL__ void TemperaturJipiFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{
    U+=0.5f;
float BBBBBBBBBBBBBBB;    
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
        float2 u = to_float2(x,y);
        float4 q = A(U+u);
        float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
        float ab = dot(u,b-a);
        float i = dot(u,(0.5f*u-a))/ab;
        float j = 0.5f+0.5f*_fmaxf(1.0f-2.5f*Q.w*q.w,0.0f);
        float k = 0.5f+0.5f*_fmaxf(1.0f-2.5f*Q.w*q.w,0.0f);
        float wa = 0.25f*Q.w*_fminf(i,j)/j;
        float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
        swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + (swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb));
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f)  dQ.x/=dQ.w,dQ.y/=dQ.w,dQ.z/=dQ.w; //dQ.xyz/=dQ.w;
    
    dQ = test_isnan(dQ);
    
    Q = dQ;
    
    if (U.x < 2.0f||R.x-U.x<2.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    if (U.y < 2.0f||R.y-U.y<2.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// Advect
__KERNEL__ void TemperaturJipiFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{
    U+=0.5f;
float CCCCCCCCCCCCC;    
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
        float2 u = to_float2(x,y);
        float4 q = A(U+u);
        float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
        float ab = dot(u,b-a);
        float i = dot(u,(0.5f*u-a))/ab;
        float j = 0.5f;
        float k = 0.5f;
        float wa = 0.25f*Q.w*_fminf(i,j)/j;
        float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
        swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + (swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb));
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f)  dQ.x/=dQ.w,dQ.y/=dQ.w,dQ.z/=dQ.w; //dQ.xyz/=dQ.w;
    
    dQ = test_isnan(dQ);
    
    Q = dQ;
        
    if (U.x < 2.0f||R.x-U.x<2.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    if (U.y < 2.0f||R.y-U.y<2.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// Advect
__KERNEL__ void TemperaturJipiFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{
    U+=0.5f;
float DDDDDDDDDDDDDDDDDDD;    
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
        float2 u = to_float2(x,y);
        float4 q = A(U+u);
        float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
        float ab = dot(u,b-a);
        float i = dot(u,(0.5f*u-a))/ab;
        float j = 0.5f;
        float k = 0.5f;
        float wa = 0.25f*Q.w*_fminf(i,j)/j;
        float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
        swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + (swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb));
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f) dQ.x/=dQ.w,dQ.y/=dQ.w,dQ.z/=dQ.w; //dQ.xyz/=dQ.w;
    
    dQ = test_isnan(dQ);
   
    
    Q = dQ;
        
    if (U.x < 2.0f||R.x-U.x<2.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    if (U.y < 2.0f||R.y-U.y<2.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Fork of "Transport Dynamics II" by wyatt. https://shadertoy.com/view/sdl3RN
// 2021-03-18 22:39:28

// Display
__KERNEL__ void TemperaturJipiFuse(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(ColorON, 0);
    CONNECT_COLOR0(Color1, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER0(Level0, -1.0f, 5.0f, 1.5f);
    CONNECT_SLIDER1(Level1, -1.0f, 2.0f, 0.5f);
    U+=0.5f;
    float4 f = A(U);
    Q = (to_float4_s(0.6f)-Level1*(sin_f4(Level0*f.z+to_float4(1,2,3,4))))*f.w;

    if(ColorON)
      Q+=Color1-0.5f, Q.w=Color1.w;

  SetFragmentShaderComputedColor(Q);
}