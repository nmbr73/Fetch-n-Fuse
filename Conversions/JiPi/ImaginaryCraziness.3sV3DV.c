
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)

#define G m-Q*(2.0f+_expf(-0.5f*dot(swi2(Q,x,y),swi2(Q,x,y))))
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void ImaginaryCrazinessFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

    
    U+=0.5f;
    Q = A(U);
    float4 m = 0.25f*(A(U+to_float2(0,1))+A(U-to_float2(0,1))+A(U+to_float2(1,0))+A(U-to_float2(1,0)));

    Q.y -= 0.3f*(G).x+0.2f*(m.y-Q.y)+0.2f*(m.x-Q.x);
    
    if (iFrame < 1 || Reset) 
      swi2S(Q,x,y, normalize(_expf(-0.01f*dot(U-0.5f*R,U-0.5f*R))*to_float2(_sinf(10.0f*iTime+0.1f*U.x),_cosf(10.0f*iTime+0.1f*U.x))+to_float2(1,0)));
        
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<30.0f) 
      swi2S(Q,x,y, normalize(_expf(-0.01f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y)))*to_float2(_sinf(10.0f*iTime+0.1f*U.x),_cosf(10.0f*iTime+0.1f*U.x))+to_float2(1,0)));
         
   
        //Blending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = D(U);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+Blend1Off)*Blend1Mul,Blend1));

        if ((int)Modus&4)
          swi2S(Q,x,y, _mix( swi2(Q,x,y), normalize(_expf(-0.01f*dot(U-0.5f*R,U-0.5f*R))*to_float2(_sinf(10.0f*iTime+0.1f*U.x),_cosf(10.0f*iTime+0.1f*U.x))+to_float2(1,0)), Blend1));
        
        if ((int)Modus&8)
          swi2S(Q,x,y, _mix( swi2(Q,x,y), Par1, Blend1));

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix( swi2(Q,x,y), to_float2_s(0.0f), Blend1));

      }
      else
        if ((int)Modus&32) //Special
          swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+Blend1Off)*Blend1Mul,Blend1));
        
    }
         
         
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void ImaginaryCrazinessFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{   
    CONNECT_CHECKBOX0(Reset, 0);
  
    U+=0.5f;
    Q = A(U);
    float4 m = 0.25f*(A(U+to_float2(0,1))+A(U-to_float2(0,1))+A(U+to_float2(1,0))+A(U-to_float2(1,0)));

    Q.x += 0.3f*(G).y+0.2f*(m.y-Q.y)+0.2f*(m.x-Q.x);
  
                                      
    if (iFrame < 1 || Reset) 
        swi2S(Q,x,y,normalize(_expf(-0.01f*dot(U-0.5f*R,U-0.5f*R))*to_float2(_sinf(10.0f*iTime+0.1f*U.x),_cosf(10.0f*iTime+0.1f*U.x))+to_float2(1,0)));
        
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<30.0f) 
        swi2S(Q,x,y, normalize(_expf(-0.01f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y)))*to_float2(_sinf(10.0f*iTime+0.1f*U.x),_cosf(10.0f*iTime+0.1f*U.x))+to_float2(1,0)));
         
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel2
// Connect Image 'Cubemap: Uffizi Gallery Blurred_0' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void ImaginaryCrazinessFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel1, sampler2D iChannel2)
{
    U+=0.5f;
    float4 a = A(U),
        h = A(U+2.0f),
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    float3 no = normalize(to_float3(length(swi2(e,x,y))-length(swi2(w,x,y)),length(swi2(n,x,y))-length(swi2(s,x,y)),1));
    float4 tx = decube_f3(iChannel1,-1.0f*reflect(-no,to_float3(0,0,1)));
    tx*=0.5f+0.5f*tx;
    Q = (0.8f+0.2f*dot(no,normalize(to_float3_s(1))))*(0.7f+0.1f*tx)* atan_f4(0.3f*length(swi2(a,x,y))*to_float4(1,2,3,4)/3.0f, to_float4_s(1.0f));

    Q = _mix((0.1f+0.5f*texture(iChannel2,(U-swi2(no,x,y))/R))-0.05f*length(swi2(h,x,y)),Q,0.1f+0.9f*Q);

  SetFragmentShaderComputedColor(Q);
}