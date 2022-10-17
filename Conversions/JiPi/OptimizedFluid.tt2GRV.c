
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0, (U)/R)
#define C(U) texture(iChannel2, (U)/R)
#define D(U) texture(iChannel3, (U)/R)

#define N 3.0f
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
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
          
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),to_float2(0,0.7f), 0.3f*smoothstep(1.0f,-1.0f,length(U-JET)-6.0f)));
          

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

// Calculate forces and pressure
__KERNEL__ void OptimizedFluidFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    U+=0.5f;

    Q = A(U);
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    swi2S(Q,x,y, swi2(Q,x,y) - (
        // pressure force
        0.25f*to_float2(e.z-w.z,n.z-s.z)+
        // magnus force
        0.25f*Q.w*to_float2(n.w-s.w,e.w-w.w)));
    // divergence
    Q.z  = 0.25f*(s.y-n.y+w.x-e.x+n.z+e.z+s.z+w.z);
    // curl
    Q.w = _mix(Q.w,0.25f*(s.x-n.x+w.y-e.y),0.5f);
    
    
    if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);    
    
    
    //Boundary conditions
    float2 JET = to_float2(0.5f+0.25f*_sinf((float)(iFrame)/500.0f),0.1f)*R;
    if (iMouse.z>0.0f) JET = swi2(iMouse,x,y);
    swi2S(Q,x,y, _mix(swi2(Q,x,y),to_float2(0,0.7f),0.3f*smoothstep(1.0f,-1.0f,length(U-JET)-6.0f)));
    if (iFrame<1 || Reset) Q = to_float4_s(0);
    if (U.x<4.0f||U.y<4.0f||R.x-U.x<4.0f||R.y-U.y<4.0f) Q.x*=0.0f,Q.y*=0.0f;Q.w*=0.0f;//  swi3(Q,x,y,w)*=0.0f;



  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Advect along velocity and curl feild
__KERNEL__ void OptimizedFluidFuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{
  
    U+=0.5f;  

    for (float i = 0.0f; i< N;i+=1.0f) {
        Q = A(U);
        float co = _cosf(Q.w/N), si = _sinf(Q.w/N);
        U -= mul_f2_mat2(swi2(Q,x,y) , to_mat2(co,-si,si,co))/N;
    }
    Q = A(U);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel2


// Advect along velocity and curl feild
__KERNEL__ void OptimizedFluidFuse__Buffer_C(float4 Q, float2 U, float2 iResolution)
{

    U+=0.5f;

    for (float i = 0.0f; i< N;i+=1.0f) {
        Q = A(U);
        float co = _cosf(Q.w/N), si = _sinf(Q.w/N);
        U -= mul_f2_mat2(swi2(Q,x,y) , to_mat2(co,-si,si,co))/N;
    }
    Q = C(U);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Texture: Blending' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Texture: Blending2' to iChannel3

// draw color and acount for divergence
__KERNEL__ void OptimizedFluidFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
        //Blending
    CONNECT_SLIDER5(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend2Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(Blend2Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus2, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT3(Par2, 0.0f, 0.0f);
    
    U+=0.5f;

    Q = C(U);
    // neighborhood
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
      nc = C(U+to_float2(0,1)),
        ec = C(U+to_float2(1,0)),
        sc = C(U-to_float2(0,1)),
        wc = C(U-to_float2(1,0));
    Q = _fmaxf(Q,to_float4_s(0.0f));
    // divergence 
    Q += 0.25f*(s.y*sc-n.y*nc+w.x*wc-e.x*ec);
    
    
    if (Blend2>0.0) Q = Blending(iChannel3, U/R, Q, Blend2, Par2, to_float2(Blend2Mul,Blend2Off), Modus2, U, R);    
    
    
    // boundary conditions
    float2 JET = to_float2(0.5f+0.25f*_sinf(float(iFrame)/500.0f),0.1f)*R;
    if (iMouse.z>0.0f) JET = swi2(iMouse,x,y);
    Q = _mix(Q,0.5f+0.5f*cos_f4((float)(iFrame)/900.0f*(1.5f+0.5f*to_float4(1,2,3,4))),smoothstep(1.0f,-1.0f,length(U-JET)-5.0f));
    if (iFrame<1 || Reset) Q = to_float4_s(0);
    if (U.x<4.0f||U.y<4.0f||R.x-U.x<4.0f||R.y-U.y<4.0f) Q*=0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


/*
  Improvements:
    account for curl when advecting
    separate advection step removes extra sampling steps
    account for divergence of color
    acount for the magnus force

*/

__DEVICE__ float ln (float3 p, float3 a, float3 b) {return length(p-a-(b-a)*_fminf(dot(p-a,b-a),0.0f)/dot(b-a,b-a));}
__KERNEL__ void OptimizedFluidFuse(float4 Q, float2 U, float2 iResolution)
{

    U+=0.5f;

    float 
        n = length(C(U+to_float2(0,1))),
        e = length(C(U+to_float2(1,0))),
        s = length(C(U-to_float2(0,1))),
        w = length(C(U-to_float2(1,0)));
    float4 a = A(U);
    Q = C(U);
    Q += 0.1f*(0.25f*to_float4_s(n+e+s+w)-Q);
    float3 no = normalize(to_float3(e-w,n-s,0.3f+0.2f*Q.w));
    float light = ln(to_float3(2,2,2),to_float3_aw(U/R.y,0),to_float3_aw(U/R.y,0)+reflect(normalize(to_float3_aw((U-0.5f*R)/R.y,1)),no));
    Q *= 0.8f*exp_f4(-0.5f*to_float4(1,2,3,4)*light)+0.8f*exp_f4(-0.06f*to_float4(1,2,3,4)*light);
    Q = atan_f4(3.0f*Q,to_float4_s(1.0f))*0.666f;
   
    U = 0.5f*R-abs_f2(U-0.5f*R);
    Q += (to_float4_s(0.5f)-0.5f*cos_f4(6.0f*a.z*(1.0f+0.5f*to_float4(1,2,3,4))))*smoothstep(1.0f,-1.0f,_fminf(U.x,U.y)-6.0f);
    
  SetFragmentShaderComputedColor(Q);
}