
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
//#define A(U) texture(iChannel0,(U)/R)
#define A(U) _tex2DVecN(iChannel0, (U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1, (U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2, (U).x/R.x,(U).y/R.y,15)
#define M(U) (0.25f*(A((U)+to_float2(0,1))+A((U)-to_float2(0,1))+A((U)+to_float2(1,0))+A((U)-to_float2(1,0))) )
#define G (swi2(m,z,w)-swi2(Q,z,w))-0.6f*swi2(Q,z,w)*(1e-4*U.y+1.0f+_expf(-0.5f*dot(swi2(Q,z,w),swi2(Q,z,w))))+0.003f*(swi2(m,x,y)-swi2(Q,x,y))
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void MichaelSAwesomeWaveSystemFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    //CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);
    CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);
  
    CONNECT_SLIDER1(StOffsetY, -400.0f, 400.0f, 0.0f);
  
    U+=0.5f;

    Q = A(U);
    float4 m = M(U);
    
    //swi2(Q,x,y) += G;
    Q.x+=(G).x;
    Q.y+=(G).y;
    //swi2(Q,z,w) += 0.5f*swi2(Q,x,y);
    Q.z+=0.5f*Q.x;
    Q.w+=0.5f*Q.y;


    if (Blend1>0.0f)
    {
      float4 tex = C(U);
      if (tex.w != 0.0f)    
      {
        //tex = tex*2.0 - 1.0f;
        if ((int)Modus & 2)  Q.x = _mix(Q.x,tex.x,Blend1),Q.y = _mix(Q.y,-tex.x,Blend1);
        if ((int)Modus & 4)  Q.y = _mix(Q.y,tex.y,Blend1);
        if ((int)Modus & 8)  Q.z = _mix(Q.z,tex.z,Blend1);
        if ((int)Modus & 16) Q.w = _mix(Q.w,tex.x,Blend1);
        if ((int)Modus & 32) Q.z = _mix(Q.z,1.0f,Blend1),Q.w = _mix(Q.w,-1.0f,Blend1);//Q = to_float4(0.0f,0.0f,1.0f,-1.0f);
      }  
    } 
    
    
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)    Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);
    if (iFrame < 1) Q = to_float4_s(0);
    //if (length(U-0.5f*R)<30.0f&&iFrame<10)                  Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);
    if (length(U+to_float2(0.0f,StOffsetY)-0.5f*R)<30.0f&&iFrame<10)  Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void MichaelSAwesomeWaveSystemFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    //CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);
    CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);  
  
    CONNECT_SLIDER1(StOffsetY, -400.0f, 400.0f, 0.0f);
  
    U+=0.5f;

    Q = A(U);
    float4 m = M(U);
    
    //swi2(Q,x,y) += G;
    Q.x+=(G).x;
    Q.y+=(G).y;
    //swi2(Q,z,w) += 0.5f*swi2(Q,x,y);
    Q.z+=0.5f*Q.x;
    Q.w+=0.5f*Q.y;
    
    if (Blend1>0.0f)
    {
      float4 tex = C(U);
      if (tex.w != 0.0f)    
      {
        //tex = tex*2.0 - 1.0f;
        if ((int)Modus & 2)  Q.x = _mix(Q.x,tex.x,Blend1),Q.y = _mix(Q.y,-tex.x,Blend1);
        if ((int)Modus & 4)  Q.y = _mix(Q.y,tex.y,Blend1);
        if ((int)Modus & 8)  Q.z = _mix(Q.z,tex.z,Blend1);
        if ((int)Modus & 16) Q.w = _mix(Q.w,tex.x,Blend1);
        if ((int)Modus & 32) Q.z = _mix(Q.z,1.0f,Blend1),Q.w = _mix(Q.w,-1.0f,Blend1);//Q = to_float4(0.0f,0.0f,1.0f,-1.0f);
      }  
    } 
   
    
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)    Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);
    if (iFrame < 1) Q = to_float4_s(0);
    
    if (length(U+to_float2(0.0f,StOffsetY)-0.5f*R)<30.0f&&iFrame<10)  Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void MichaelSAwesomeWaveSystemFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    //CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);
    CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);  
    CONNECT_SLIDER1(StOffsetY, -400.0f, 400.0f, 0.0f);
  
    U+=0.5f;
    
    Q = A(U);
    float4 m = M(U);
    
    //swi2(Q,x,y) += G;
    Q.x+=(G).x;
    Q.y+=(G).y;
    //swi2(Q,z,w) += 0.5f*swi2(Q,x,y);
    Q.z+=0.5f*Q.x;
    Q.w+=0.5f*Q.y;
    
    
    if (Blend1>0.0f)
    {
      float4 tex = C(U);
      if (tex.w != 0.0f)    
      {
        //tex = tex*2.0 - 1.0f;
        if ((int)Modus & 2)  Q.x = _mix(Q.x,tex.x,Blend1),Q.y = _mix(Q.y,-tex.x,Blend1);
        if ((int)Modus & 4)  Q.y = _mix(Q.y,tex.y,Blend1);
        if ((int)Modus & 8)  Q.z = _mix(Q.z,tex.z,Blend1);
        if ((int)Modus & 16) Q.w = _mix(Q.w,tex.x,Blend1);
        if ((int)Modus & 32) Q.z = _mix(Q.z,1.0f,Blend1),Q.w = _mix(Q.w,-1.0f,Blend1);//Q = to_float4(0.0f,0.0f,1.0f,-1.0f);
      }  
    }     
    
    
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)    Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);
    if (iFrame < 1) Q = to_float4_s(0);
    //if (length(U-0.5f*R)<30.0f&&iFrame<10)                  Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);
    if (length(U+to_float2(0.0f,StOffsetY)-0.5f*R)<30.0f&&iFrame<10)  Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void MichaelSAwesomeWaveSystemFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    //CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);
    CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);
    CONNECT_SLIDER1(StOffsetY, -400.0f, 400.0f, 0.0f);
    
    U+=0.5f;
    
    Q = A(U);
    float4 m = M(U);
    
    //swi2(Q,x,y) += G;
    Q.x+=(G).x;
    Q.y+=(G).y;
    //swi2(Q,z,w) += 0.5f*swi2(Q,x,y);
    Q.z+=0.5f*Q.x;
    Q.w+=0.5f*Q.y;
    
    if (Blend1>0.0f)
    {
      float4 tex = C(U);
      if (tex.w != 0.0f)    
      {
        //tex = tex*2.0 - 1.0f;
        if ((int)Modus & 2)  Q.x = _mix(Q.x,tex.x,Blend1),Q.y = _mix(Q.y,-tex.x,Blend1);
        if ((int)Modus & 4)  Q.y = _mix(Q.y,tex.y,Blend1);
        if ((int)Modus & 8)  Q.z = _mix(Q.z,tex.z,Blend1);
        if ((int)Modus & 16) Q.w = _mix(Q.w,tex.x,Blend1);
        if ((int)Modus & 32) Q.z = _mix(Q.z,1.0f,Blend1),Q.w = _mix(Q.w,-1.0f,Blend1);//Q = to_float4(0.0f,0.0f,1.0f,-1.0f);
      }  
    }     
    
    
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)    Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);
    if (iFrame < 1) Q = to_float4_s(0);
    //if (length(U-0.5f*R)<30.0f&&iFrame<10)                  Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);
    if (length(U+to_float2(0.0f,StOffsetY)-0.5f*R)<30.0f&&iFrame<10)  Q.z=1.0f,Q.w=-1.0f;//swi2(Q,z,w) = to_float2(1,-1);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest Blurred_0' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2

//#define R iResolution
//#define A(U) texture(iChannel0,(U)/R)
__DEVICE__ float f (float4 a) {return _sqrtf(dot(a,a));}
__KERNEL__ void MichaelSAwesomeWaveSystemFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Special, 0);
    CONNECT_COLOR0(Color, 0.8f, 0.2f, 1.0f, 1.0f);
    CONNECT_COLOR1(ColorBKG, 0.0f, 0.8f, 0.8f, 1.0f);

    CONNECT_SLIDER2(BKGThres, 0.0f, 1.0f, 0.0f);
    //float4 Color = to_float4(0.8f, 0.2f, 1.0f, 1.0f);
   
    U+=0.5f;
    
    float4 a = A(U);
    float
        n = f(A(U+to_float2(0,1))),
        e = f(A(U+to_float2(1,0))),
        s = f(A(U-to_float2(0,1))),
        w = f(A(U-to_float2(1,0)));
    float3 no = normalize(to_float3(e-w,n-s,10));

    float
        n2 = f(C(U+to_float2(0,1))),
        e2 = f(C(U+to_float2(1,0))),
        s2 = f(C(U-to_float2(0,1))),
        w2 = f(C(U-to_float2(1,0)));
    float3 no2 = normalize(to_float3(e2-w2,n2-s2,10));
        
    float4 col1 = Color*_fabs(_atan2f(0.1f*f(a),1.0f))*(0.5f+0.5f*decube_f3(iChannel1,-no));                
    float4 col2 = Color*_fabs(_atan2f(0.1f*f(a),1.0f))*(0.5f+0.5f*decube_f3(iChannel1,no));        
    
    //Original
    //Q = to_float4(0.8f,0.2f,1,1)*_fabs(_atan2f(0.1f*f(a),1.0f))*(0.5f+0.5f*decube_f3(iChannel1,-no));
    Q = Color*_fabs(_atan2f(0.1f*f(a),1.0f))*(0.5f+0.5f*decube_f3(iChannel1,-no));

    if ( Q.x<BKGThres&&Q.y<BKGThres&&Q.z<BKGThres) Q=ColorBKG;

    if(Special)
      Q=sqrt_f4(col1);

    Q.w = Color.w;

  SetFragmentShaderComputedColor(Q);
}