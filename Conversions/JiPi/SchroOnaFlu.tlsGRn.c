
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel2
// Connect Buffer A 'Previsualization: Buffer C' to iChannel1


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)




__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U)
{
 
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(U.x/R.x,U.y/R.y)+MulOff.y)*MulOff.x,Blend));
          //Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));
          //Q.x = _mix(Q.x,1.0f,_expf(-0.1f*U.x)*Blend);
          swi2S(Q,x,y, _mix(swi2(Q,x,y),(uv+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          //Q.x = _mix(Q.x,(_expf(-0.1f*U.x)+MulOff.y)*MulOff.x,Blend);
          swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          swi2S(Q,x,y, _mix( swi2(Q,x,y), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  Par, Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
   return Q;
}


__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 TA (float2 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2) {
    U -= 0.5f*swi2(C(U),x,y);
    U -= 0.5f*swi2(C(U),x,y);
    return A(U);
}
    
    
__KERNEL__ void SchroOnaFluFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Clear, 0); 
    
    //Blending
    CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = TA(U,R,iChannel0,iChannel2);
    if (iFrame < 1 || Reset) Q = to_float4_s(0);
    #define _e  to_float2 (1,0)
    float dQ = ((TA(U+swi2(_e,x,y),R,iChannel0,iChannel2)
                +TA(U-swi2(_e,x,y),R,iChannel0,iChannel2)
                +TA(U+swi2(_e,y,x),R,iChannel0,iChannel2)
                +TA(U-swi2(_e,y,x),R,iChannel0,iChannel2)).x/4.0f-Q.x);
    
    //Schr Eq :
    Q.y += dQ-Q.x;
    Q.x +=    Q.y;
     /*
    Q.y += dQ;
    Q.x += Q.y;
   */
    //Wave Eq ^
    
    float4 mo = texture(iChannel1,to_float2_s(0));
    float l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
    if (iMouse.z>1.0f)                                     Q.x = _mix(Q.x,1.0f,_expf(-0.1f*l));
    if ((iFrame < 1 || Reset) && length(0.5f*R-U) < 10.0f) Q.x = 10.0f*_sinf(0.5f*U.x);
    
    if (Blend1>0.0) Q = Blending(iChannel3, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U);
    
    if (Clear) Q = to_float4_s(0);
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel2
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1


// Fluid

__DEVICE__ float4 TB (float2 U, float2 R, __TEXTURE2D__ iChannel0) {
  // sample things where they were, not where they are
  U -= 0.5f*swi2(A(U),x,y);
  U -= 0.5f*swi2(A(U),x,y);
  return A(U);
}

__KERNEL__ void SchroOnaFluFuse__Buffer_B(float4 C, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
   CONNECT_CHECKBOX0(Reset, 0); 
   CONNECT_CHECKBOX1(Clear, 0); 
   
   //Blending
   CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
   CONNECT_SLIDER2(Blend1Off, -10.0f, 10.0f, 0.0f);
   CONNECT_SLIDER3(Blend1Mul, -10.0f, 10.0f, 1.0f);
   CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
   CONNECT_POINT0(Par1, 0.0f, 0.0f);
  
   U+=0.5f;

   C = TB(U,R,iChannel0);
   #define _o 1.0f
  

   // neighborhood
   float4 n = TB(U+to_float2(0.0f,_o),R,iChannel0);
   float4 e = TB(U+to_float2(_o,0.0f),R,iChannel0);
   float4 s = TB(U-to_float2(0.0f,_o),R,iChannel0);
   float4 w = TB(U-to_float2(_o,0.0f),R,iChannel0);
   // xy : velocity, z : pressure, w : spin
   C.x -= 0.25f*(e.z-w.z+C.w*(n.w-s.w));
   C.y -= 0.25f*(n.z-s.z+C.w*(e.w-w.w));
   C.z  = 0.25f*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z));
   C.w  = 0.25f*((n.x-s.x+e.y-w.y)-C.w);
   
  
   // boundary conditions
   if( U.x < 2.0f || U.y < 2.0f || R.x-U.x < 2.0f || R.y-U.y < 2.0f)
        C.x*=0.0f,C.y*=0.0f;//swi2(C,x,y) *= 0.0f;
   float4 mo = texture(iChannel1,to_float2_s(0));
   float l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   if ((iFrame < 1 || Reset) && length(0.5f*R-U) < 10.0f)     C.x = _sinf(0.5f*U.x);
   if (iMouse.z>0.0f)                              swi2S(C,x,y, _mix(swi2(C,x,y),10.0f*(swi2(mo,x,y)-swi2(mo,z,w))/R.y,_expf(-0.5f*l)));

   //if (Blend1>0.0) C = Blending(iChannel3, U/R, C, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U);
  


   if (Clear) C = to_float4_s(0);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer D' to iChannel0


// mouse

__KERNEL__ void SchroOnaFluFuse__Buffer_C(float4 C, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
      U+=0.5f;

      float4 M = iMouse;
      float4 p = A(U);
      if (M.z>0.0f) {
        if (p.z>0.0f)  C = to_float4_f2f2(swi2(M,x,y),swi2(p,x,y));
        else           C = to_float4_f2f2(swi2(M,x,y),swi2(M,x,y));
      }
      else             C = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SchroOnaFluFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);  
    
    U+=0.5f;
    if(Invers)
      Q = to_float4_s(1.0f)-sin_f4(to_float4(1,2,3,4)*length(A(U)));
    else      
      Q = sin_f4(to_float4(1,2,3,4)*length(A(U)));

    Q.w=Alpha;

  SetFragmentShaderComputedColor(Q);
}