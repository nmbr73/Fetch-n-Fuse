
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define A(COORD) texture(iChannel0,(COORD)/iResolution)

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


__KERNEL__ void LocalInteractionFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    U-=0.5f*iResolution;
    U *= 0.997f;
    float a = 0.003f*_sinf(0.5f*iTime-2.0f*length(U-0.5f*iResolution)/iResolution.y);
    U = mul_f2_mat2(U , to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)));
    U+=0.5f*iResolution;
    Q  =  A(U);
    // Neighborhood :
    float4 pX  =  A(U + to_float2(1,0));
    float4 pY  =  A(U + to_float2(0,1));
    float4 nX  =  A(U - to_float2(1,0));
    float4 nY  =  A(U - to_float2(0,1));
    float4 m = 0.25f*(pX+nX+pY+nY);
    float b = _mix(1.0f,_fabs(Q.z),0.8f);
    swi3S(Q,x,y,z, swi3(Q,x,y,z) + (1.0f-b)*(0.25f*to_float3(pX.z-nX.z,pY.z-nY.z,-pX.x+nX.x-pY.y+nY.y)- swi3(Q,x,y,z)));

    Q = _mix(Q,m,b);
    
    if (length(swi2(Q,x,y))>0.0f)  swi2S(Q,x,y, normalize(swi2(Q,x,y)));
    
    if (Blend1>0.0) Q = Blending(iChannel1, U/iResolution, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, iResolution);
    
    
    if(iFrame < 1 || Reset) Q = sin_f4(0.01f*length(U-0.5f*iResolution)*to_float4(1,2,3,4));
    
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<0.1f*iResolution.y) Q *= 0.0f;
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define A(COORD) texture(iChannel0,(COORD)/iResolution)
__DEVICE__ float ln (float3 p, float3 a, float3 b) {return length(p-a-(b-a)*_fminf(dot(p-a,b-a),0.0f)/dot(b-a,b-a));}

__KERNEL__ void LocalInteractionFuse(float4 Q, float2 U, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER0(Level, -10.0f, 10.0f, -3.0f);
  
    U+=0.5f;
    Q  =  A(U);
    float4 pX  =  A(U + to_float2(1,0));
    float4 pY  =  A(U + to_float2(0,1));
    float4 nX  =  A(U - to_float2(1,0));
    float4 nY  =  A(U - to_float2(0,1));
    float3 n = normalize(to_float3(pX.z-nX.z,pY.z-nY.z,1));
    float3 r = reflect(n,to_float3(0,0,-1));
    Q = (0.5f+0.5f*sin_f4(iTime+_atan2f(Q.x,Q.y)*to_float4(3,2,1,4)));
    float d = ln(to_float3(0.4f,0.4f,6)*swi3(iResolution,x,y,y),
                 to_float3_aw(U,0),to_float3_aw(U,0)+r)/iResolution.y;
    Q *= _expf(-d*d)*0.5f+0.5f*_expf(Level*d*d);

    Q+=Color-0.5f;
    Q.w=Color.w;

  SetFragmentShaderComputedColor(Q);
}