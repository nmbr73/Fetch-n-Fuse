
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
//#define Main void mainImage(out float4 Q, in float2 U)
#define box for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
#define r2 0.70710678118f


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 tmp)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          //Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          swi2S(Q,x,y, _mix( swi2(Q,x,y), (swi2(tex,x,y)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.x+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));
          //Q = _mix(Q,to_float4(tmp.x,tmp.y,-1.0f,1.0f),Blend);
          Q = _mix(Q,to_float4(Par.x,Par.y,-1.0f,1.0f),Blend);

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix(swi2(Q,x,y),  Par, Blend));
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


// Forces
__KERNEL__ void AquaVistaJipi559Fuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;

    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        float f = 0.07f*a.w*((a.w-1.0f)+0.3f);
        //swi2(dQ,x,y) -= f*u;
        dQ.x -= f*u.x;
        dQ.y -= f*u.y;
    }
    Q += dQ;
    if (_fabs(U.y-0.6f*R.y)<0.3f*R.y) {
        swi2S(Q,x,y, swi2(Q,x,y) - swi2(Q,x,y)*0.1f*_expf(-16.0f*Q.w*Q.w));
    }
    Q.y -= 3e-4;
    float2 tmp = 0.1f*normalize(swi2(iMouse,x,y)-0.5f*R);
    if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, tmp);
    
    
    float2 M = 1.5f*R;
    if (iMouse.z>0.0f)                         M = swi2(iMouse,x,y);
    if(length(U-M)<0.02f*R.y)                  {float2 tmp = 0.1f*normalize(swi2(iMouse,x,y)-0.5f*R); Q = to_float4(tmp.x,tmp.y,-1,1.0f);}
    if (length(swi2(Q,x,y))>0.5f)              swi2S(Q,x,y, 0.5f*normalize(swi2(Q,x,y)));
    if (iFrame < 1 || Reset)                   Q = to_float4(0,0,0,0.12f);
    if (R.y-U.y<2.0f)                          swi2S(Q,z,w, _mix(swi2(Q,z,w),to_float2(-8.0f*step(U.x,0.5f*R.x)*U.x/R.x,1.5f-U.x/R.x*1.0f),0.005f));
    if (U.x < 1.0f)                            Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    if (U.y < 1.0f || R.y-U.y<1.0f)            Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    if (R.x-U.x<1.0f) Q.w *= 0.0f;
    if (_fabs(R.x*0.5f-U.x)<1.0f && U.y<0.15f*R.y) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Advect
__KERNEL__ void AquaVistaJipi559Fuse__Buffer_B(float4 Q, float2 U,float2 iResolution)
{
    U+=0.5f;

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
       float j = 0.5f+0.5f*_fmaxf(1.0f-3.0f*Q.w*q.w,0.0f);
       float k = 0.5f+0.5f*_fmaxf(1.0f-3.0f*Q.w*q.w,0.0f);
       float wa = 0.25f*Q.w*_fminf(i,j)/j;
       float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
       swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb);
       dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f)  dQ.x/=dQ.w,dQ.y/=dQ.w,dQ.z/=dQ.w;//  dQ.xyz/=dQ.w;
    Q = dQ;
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// Advect
__KERNEL__ void AquaVistaJipi559Fuse__Buffer_C(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;

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
       swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb);
       dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f)  dQ.x/=dQ.w,dQ.y/=dQ.w,dQ.z/=dQ.w;//  dQ.xyz/=dQ.w;
    Q = dQ;
    

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// Advect
__KERNEL__ void AquaVistaJipi559Fuse__Buffer_D(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;

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
       swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb);
       dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f)  dQ.x/=dQ.w,dQ.y/=dQ.w,dQ.z/=dQ.w;//  dQ.xyz/=dQ.w;
    Q = dQ;
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Fork of "Temperatures" by wyatt. https://shadertoy.com/view/fsf3zS
// 2021-03-22 22:23:14

// Fork of "Transport Dynamics II" by wyatt. https://shadertoy.com/view/sdl3RN
// 2021-03-18 22:39:28

// Display
__KERNEL__ void AquaVistaJipi559Fuse(float4 Q, float2 U, float2 iResolution)
{
    CONNECT_CHECKBOX1(Invers, 0);  
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
    U+=0.5f;

    float4 f = A(U);
    Q = to_float4_s(1.0f)-(to_float4_s(0.5f)-0.5f*(sin_f4(3.5f-0.3f*(f.z)+to_float4(1,2,3,4))))*f.w;

    //if (Invers) Q = ((to_float4_s(0.5f)-0.5f*(sin_f4(3.5f-0.3f*(f.z)+to_float4(1,2,3,4))))*f.w)+(Color-0.5f);
    if (Invers) Q = ((to_float4_s(0.5f)-0.5f*(sin_f4(3.5f-0.3f*(f.z)+to_float4(1,2,3,4))))+Color-0.5f) * f.w;
    
    Q.w=Color.w;

  SetFragmentShaderComputedColor(Q);
}