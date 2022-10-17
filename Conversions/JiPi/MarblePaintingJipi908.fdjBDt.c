
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

//#define Main void mainImage(out float4 Q, in float2 U)

#define box for(int _x=-1;_x<=1;_x++)for(int _y=-1;_y<=1;_y++)
#define r2 0.70710678118f
__DEVICE__ float hash(float2 p) // Dave H
{
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}
__DEVICE__ float Paper (float2 U) { //https://www.shadertoy.com/view/NsfXWs
    float h = 0.005f*(_sinf(0.6f*U.x+0.1f*U.y)+_sinf(0.7f*U.y-0.1f*U.x));
    for (float _x = -1.0f; _x<=1.0f;_x++)
    for (float _y = -1.0f; _y<=1.0f;_y++){
        h += 0.15f*0.125f*hash(U+to_float2(_x,_y));
    }
    return h;
}

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float iTime)
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
          //Q.w = _mix(Q.w,(tex.x+MulOff.y)*MulOff.x,Blend);
          Q = _mix(Q,0.5f+0.5f*sin_f4(to_float4(1,2,3,4)+iTime),Blend);

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix( swi2(Q,x,y), to_float2_s(0.2f), Blend));

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
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


// Forces
__KERNEL__ void MarblePaintingJipi908Fuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
  
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
  
    U+=0.5f;

    Q = A(U);
    float4 m = C(U);
    float d = dot(m,to_float4(-1,1,-1,1));
    box if(_x!=0 || _y!=0)
    {
        float2 u = to_float2(_x,_y);
        float4 a = A(U+u);
        float4 c = C(U+u);
        float p = 0.1f*Paper(U+u);
        float f = length(m-c)*d;
        swi2S(Q,x,y, swi2(Q,x,y) - 0.05f*a.w*(p+a.w-Q.w*f-1.0f)*u/length(u));
    }
    Q.w *= 1.0f-1e-4;
    //Q.y -= 1e-4*Q.w;
    swi2S(Q,x,y, swi2(Q,x,y) * (1.0f-_expf(-10.0f*Q.w)));
    if (iMouse.z>0.0f && length(U-swi2(iMouse,x,y))<20.0f)
        Q.w = 1.0f;
      
    //if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, iTime);  

//#ifdef XXX
    if (Blend1 > 0.0f)
    {
      float4 tex = texture(iChannel1,U/R);
      if (tex.w > 0.0f)
      {  
        Q.w = _mix(Q.w,(tex.x+Blend1Off)*Blend1Mul,Blend1);
      }
    }
//#endif      
      
    if (length(swi2(Q,x,y))>0.5f)      swi2S(Q,x,y, 0.5f*normalize(swi2(Q,x,y)));
    if (iFrame < 3 || Reset)           Q = to_float4(0,0,0.0f,0.05f);
    if (U.x < 4.0f || R.x-U.x<4.0f)    Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    if (U.y < 4.0f || R.y-U.y<4.0f)    Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Advect
__KERNEL__ void MarblePaintingJipi908Fuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
  
    U+=0.5f;

    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(_x)!=_fabs(_y))
    {
       float2 u = to_float2(_x,_y);
       float4 q = A(U+u);
       float2 a = swi2(Q,x,y),
              b = swi2(q,x,y)+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5f*u-a))/ab;
       float j = 0.5f+0.5f*_fmaxf(1.0f-Q.w*q.w,0.0f);
       float k = 0.5f+0.5f*_fmaxf(1.0f-Q.w*q.w,0.0f);
       float wa = 0.25f*Q.w*_fminf(i,j)/j;
       float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
       swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb);
       dQ.w += wa+wb;
    }
    if (dQ.w>0.0f)  dQ.x/=dQ.w, dQ.y/=dQ.w, dQ.z/=dQ.w;
    Q = dQ;
    
    if (U.x < 4.0f || R.x-U.x<4.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    if (U.y < 4.0f || R.y-U.y<4.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


// Advect
__KERNEL__ void MarblePaintingJipi908Fuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = A(U);
    float4 Qb = B(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(_x)!=_fabs(_y))
    {
        float2 u = to_float2(_x,_y);
        float4 q = A(U+u),
              qb = B(U+u);
        float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5f*u-a))/ab;
       float j = 0.5f;
       float k = 0.5f;
       float wa = 0.25f*Q.w*_fminf(i,j)/j;
       float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
       swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Qb,x,y,z)*wa+swi3(qb,x,y,z)*wb);
       dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f)   dQ.x/=dQ.w, dQ.y/=dQ.w, dQ.z/=dQ.w;//dQ.xyz/=dQ.w;
    Q = dQ;
    if (iMouse.z>0.0f && length(U-swi2(iMouse,x,y))<20.0f)
        Q = 0.5f+0.5f*sin_f4(to_float4(1,2,3,4)+iTime);
      
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, iTime);        
      
    if (iFrame<1 || Reset)
        Q = 0.5f+0.5f*sin_f4(to_float4(1,2,3,4)*4.0f*length(U-0.5f*R)/R.x);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void MarblePaintingJipi908Fuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;

    Q = D(U);
    float4 a = A(U);
    float4 c = C(U)*a.w;
    
    Q = _mix(Q,c,0.001f*a.w*a.w);
    
    if (iFrame<1 || Reset) Q = to_float4_s(0.0f);
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "Colors!" by wyatt. https://shadertoy.com/view/7dX3z7
// 2021-04-29 06:26:31

// Fork of "Transport Dynamics II" by wyatt. https://shadertoy.com/view/sdl3RN
// 2021-03-14 01:41:52

// Display

__KERNEL__ void MarblePaintingJipi908Fuse(float4 Q, float2 U, float2 iResolution)
{
    //U+=0.5f;
    float h = Paper(U);
    Q = 1.5f*D(U)+0.8f*C(U)*A(U).w;
    Q = to_float4_s(0.8f+h)-Q;

  SetFragmentShaderComputedColor(Q);
}