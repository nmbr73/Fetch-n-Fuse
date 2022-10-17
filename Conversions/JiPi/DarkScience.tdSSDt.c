
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Preset: Blending' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


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
          //Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
          Q.x = _mix(Q.x, (tex.x+MulOff.y)*MulOff.x, Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          Q.y = _mix(Q.y, (tex.x+MulOff.y)*MulOff.x, Blend);

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


#define R iResolution
__DEVICE__ float4 F (float2 U, float2 R, __TEXTURE2D__ iChannel1) {return texture(iChannel1,U/R);}
__DEVICE__ float4 TA (float2 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    U -= swi2(F(U,R,iChannel1),x,y);
    return texture(iChannel0,U/R);}
    
__KERNEL__ void DarkScienceFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0); 
  
      //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XValue, YValue, InvMass, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
  U+=0.5f;
  
  Q = TA(U,R,iChannel0,iChannel1);
    #define o 1.5
    
    float a = 1.61803398875f*(float(iFrame) + U.x + R.x*U.y),
        si = _sinf(a),
        co = _cosf(a);
    mat2 m = to_mat2(co,-si,si,co);
    
    float4 
        n = TA(U+mul_f2_mat2(to_float2(0,o),m),R,iChannel0,iChannel1),
        e = TA(U+mul_f2_mat2(to_float2(o,0),m),R,iChannel0,iChannel1),
        s = TA(U-mul_f2_mat2(to_float2(0,o),m),R,iChannel0,iChannel1),
        w = TA(U-mul_f2_mat2(to_float2(o,0),m),R,iChannel0,iChannel1),
        mu = 0.25f*(n+e+s+w),
        la = mu - Q;
    
    Q += la*to_float4(1,0.21f,0,0);
    
    float x = Q.x*Q.y*(1.0f-Q.y);
    Q.y += x-0.023f - 0.02f*Q.y;
    Q.x += -x+0.015f*(1.0f-Q.x);
    
    Q = _fmaxf(Q,to_float4_s(0));
   
    if (iFrame < 1 || Reset) {
      Q = to_float4 (1,0,0,0);
    }
    
    if (length (U-0.5f*R) < 10.0f && (iFrame < 3 || Reset)) 
      Q.y = 1.0f;
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel2
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1


// Fluid

__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 A (float2 U,float2 R, __TEXTURE2D__ iChannel2) {return texture(iChannel2,U/R);}
__DEVICE__ float4 t (float2 U,float2 R, __TEXTURE2D__ iChannel0) { // access buffer
  return texture(iChannel0,U/R);
}
__DEVICE__ float4 TB (float2 U,float2 R, __TEXTURE2D__ iChannel0) {
    // sample things where they were, not where they are
  U -= 0.5f*swi2(t(U,R,iChannel0),x,y);
  U -= 0.5f*swi2(t(U,R,iChannel0),x,y);
    return t(U,R,iChannel0);
}
__DEVICE__ float2 _x (float a, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel2) {
  return r * (A(U+r,R,iChannel2).x-a);
}
__KERNEL__ void DarkScienceFuse__Buffer_B(float4 C, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
   U+=0.5f;
 
   C = TB(U,R,iChannel0);
   #define o 1.2
   float4 // neighborhood
        n = TB(U+to_float2(0,o),R,iChannel0),
        e = TB(U+to_float2(o,0),R,iChannel0),
        s = TB(U-to_float2(0,o),R,iChannel0),
        w = TB(U-to_float2(o,0),R,iChannel0);
   // xy : velocity, z : pressure, w : spin
   C.x -= 0.25f*(e.z-w.z+C.w*(n.w-s.w));
   C.y -= 0.25f*(n.z-s.z+C.w*(e.w-w.w));
   C.z  = 0.25f*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z));
   C.w  = 0.25f*((n.x-s.x+e.y-w.y)-(n.w+e.w+s.w+w.w));
   float a = A(U,R,iChannel2).x;
   float an = 1.61803398875f*((float)(iFrame) + U.x + R.x*U.y),
        si = _sinf(an),
        co = _cosf(an);
   mat2 m = to_mat2(co,-si,si,co);
   swi2S(C,z,y, swi2(C,z,y) + to_float2(0.01f,-0.002f)*A(U,R,iChannel2).x);
   C.z *= 0.995f;
   swi2S(C,x,y, swi2(C,x,y) + 0.15f*(
                           _x (a, U, mul_f2_mat2(to_float2(1,0),m),R,iChannel2)+
                           _x (a, U, mul_f2_mat2(to_float2(-1,0),m),R,iChannel2)+
                           _x (a, U, mul_f2_mat2(to_float2(0,1),m),R,iChannel2)+
                           _x (a, U, mul_f2_mat2(to_float2(0,-1),m),R,iChannel2)));
   // boundary conditions
    if( U.x < 2.0f || U.y < 2.0f || R.x-U.x < 2.0f || R.y-U.y < 2.0f)
        C.x*=0.0f,C.y*=0.0f;//swi2(C,x,y) *= 0.0f;
   float4 mo = texture(iChannel1,to_float2_s(0));
   float l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   swi2S(C,x,y, _mix(swi2(C,x,y),10.0f*(swi2(mo,x,y)-swi2(mo,z,w))/R.y,_expf(-0.1f*l)));

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


// mouse

//__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
//    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
//}
//__DEVICE__ float4 TC ( float2 U, float R, __TEXTURE2D__ iChannel0 ) {return texture(iChannel0,U/R);}
__DEVICE__ float4 P ( float2 U, float2 R, __TEXTURE2D__ iChannel1 ) {return texture(iChannel1,U/R);}

__KERNEL__ void DarkScienceFuse__Buffer_C(float4 C, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    U+=0.5f;   
     
    float4 M = iMouse;
    M.x = _fabs(M.x-0.5f*R.x);
    float4 p = P(U,R,iChannel1);
    if (M.z>0.0f) {
      if (p.z>0.0f) C =  to_float4_f2f2(swi2(M,x,y),swi2(p,x,y));
      else          C =  to_float4_f2f2(swi2(M,x,y),swi2(M,x,y));
    }
    else C = to_float4_f2f2(-iResolution,-iResolution);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1



__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel0) {return texture(iChannel0,U/R);}

__KERNEL__ void DarkScienceFuse(float4 Q, float2 U, float iTime, float2 iResolution, sampler2D iChannel0)
{
  
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
  CONNECT_SLIDER0(XShift, -1.0f, 1.0f, 0.5f);
  CONNECT_CHECKBOX1(Invers, 0); 
  
  Color -= 0.5f;
  Color.w += 0.5f;
  
  Color *= 4.0f;
  
  U+=0.5f;
  U.x = _fabs(U.x-XShift*R.x);
  
  Q = to_float4_s(T(U,R,iChannel0).x);  
  //swi2(Q,y,z) = T(U+_sinf(iTime)).xx;
  Q.y = T(U+_sinf(iTime),R,iChannel0).x;
  Q.z = T(U+_sinf(iTime),R,iChannel0).x;

  float3 n = to_float3(
                       T(U+to_float2(1,0),R,iChannel0).x-T(U-to_float2(1,0),R,iChannel0).x,
                       T(U+to_float2(0,1),R,iChannel0).x-T(U-to_float2(0,1),R,iChannel0).x,1);
  Q += 10.0f*(Q.x-0.5f)*length(swi2(n,x,y));
  
  if (Invers) Q = to_float4_s(1.0f)-Q;
  
  Q += (Color*T(U,R,iChannel0).y);
  Q.w = Color.w;

  SetFragmentShaderComputedColor(Q);
}