
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R   (iResolution)
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// these values are improperly named dt = d/dt and dx = dispersion
#define dt 1.0f
#define dx 0.001f
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


// ELECTRIC FIELD

__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float3 E (float3 U, float2 R, __TEXTURE2D__ iChannel0) {
  return swi3(texture(iChannel0,swi2(U,x,y)/R),x,y,z);
}
__DEVICE__ float3 M (float3 U, float2 R, __TEXTURE2D__ iChannel1) {

  return swi3(texture(iChannel1,swi2(U,x,y)/R),x,y,z);
}

__DEVICE__ float3 X (float3 U,float3 off, float2 R, __TEXTURE2D__ iChannel1) {
  return cross(off,M(U+off,R,iChannel1));
}

__KERNEL__ void MaxwellEquationsTestFuse__Buffer_A(float4 c, float2 u, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
   CONNECT_CHECKBOX0(Reset, 0);
   
   CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
   CONNECT_SLIDER1(BlendX, -10.0f, 10.0f, 1.0f);
   CONNECT_SLIDER2(BlendY, -10.0f, 10.0f, 1.0f);
   CONNECT_SLIDER3(BlendZ, -10.0f, 10.0f, 1.0f);
   CONNECT_CHECKBOX1(SpecialInv, 0);
   CONNECT_SLIDER4(SpecialValue, -10.0f, 10.0f, 1.0f);
   CONNECT_BUTTON0(Modus, 1, X,  Y, Z, Clear, Special);
  
      
   u+=0.5f;
 
   float3 U = to_float3_aw(u,0);
   # define l 1.0f/_sqrtf(2.0f)
   float3 mu = 0.25f*(E(U+to_float3(1,0,0),R,iChannel0)+E(U-to_float3(1,0,0),R,iChannel0)+E(U+to_float3(0,1,0),R,iChannel0)+E(U-to_float3(0,1,0),R,iChannel0));
   float3 C = _mix(E(U,R,iChannel0),mu,dx)
        + dt*(-1.0f*M(U,R,iChannel1) + 
      ( X(U, to_float3( 1, 0,0),R,iChannel1) + 
        X(U, to_float3(-1, 0,0),R,iChannel1) + 
        X(U, to_float3( 0, 1,0),R,iChannel1) + 
        X(U, to_float3( 0,-1,0),R,iChannel1) + 
        X(U, to_float3( l, l,0),R,iChannel1) + 
        X(U, to_float3( l,-l,0),R,iChannel1) +
        X(U, to_float3(-l,-l,0),R,iChannel1) +
        X(U, to_float3(-l, l,0),R,iChannel1) ) / 8.0f);
   
   if (iFrame < 1 || Reset) C = 1e-1*to_float3_s(-1.0f+2.0f*smoothstep(-10.0f,10.0f,U.x-0.5f*R.x));

   float4 mouse = texture(iChannel2,to_float2_s(0.5f));
   if (iMouse.z > 0.0f ) C = _mix(C,normalize(to_float3_aw(swi2(mouse,x,y)-swi2(mouse,z,w),1)),smoothstep(7.0f,0.0f,ln(swi2(U,x,y),swi2(mouse,x,y),swi2(mouse,z,w))));


    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel3,u/R);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          C.x = _mix(C.x,tex.x*BlendX-BlendX/2.0f,Blend1);

        if ((int)Modus&4)
          C.y = _mix(C.y, tex.y*BlendY-BlendY/2.0f, Blend1);

        if ((int)Modus&8)
        {  
          C.z = _mix(C.z, tex.z*BlendZ-BlendZ/2.0f, Blend1);
        }

        if ((int)Modus&16) //Clear
          C = _mix(C,to_float3_s(0.0f),Blend1);

        if ((int)Modus&32) //Special
        {
          C = _mix(C,swi3(tex,x,y,z),Blend1);
        }

      }
    }




   c = to_float4_aw(C,0);

  SetFragmentShaderComputedColor(c);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// MAGNETIC FIELD

__KERNEL__ void MaxwellEquationsTestFuse__Buffer_B(float4 c, float2 u, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   CONNECT_CHECKBOX0(Reset, 0);
   u+=0.5f;
 
   float3 U = to_float3_aw(u,0);
   //# define l 1.0f/_sqrtf(2.0f)
   float3 mu = 0.25f*(M(U+to_float3(1,0,0),R,iChannel1)
                     +M(U-to_float3(1,0,0),R,iChannel1)
                     +M(U+to_float3(0,1,0),R,iChannel1)
                     +M(U-to_float3(0,1,0),R,iChannel1));
 
   float3 C = _mix(M(U,R,iChannel1),mu,dx) + dt*( E(U,R,iChannel0) -
      ( X(U, to_float3( 1, 0,0),R,iChannel0) + 
        X(U, to_float3(-1, 0,0),R,iChannel0) + 
        X(U, to_float3( 0, 1,0),R,iChannel0) + 
        X(U, to_float3( 0,-1,0),R,iChannel0) + 
        X(U, to_float3( l, l,0),R,iChannel0) + 
        X(U, to_float3( l,-l,0),R,iChannel0) +
        X(U, to_float3(-l,-l,0),R,iChannel0) +
        X(U, to_float3(-l, l,0),R,iChannel0) ) / 8.0f);
   if (iFrame < 1 || Reset) C = to_float3_s(0);
   c = to_float4_aw(C,0);

  SetFragmentShaderComputedColor(c);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0


// keep track of mouse
__KERNEL__ void MaxwellEquationsTestFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float4 p = texture(iChannel0,fragCoord/iResolution);
    if (iMouse.z>0.0f) {
        if (p.z>0.0f) fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else            fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else              fragColor = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


// IMAGE

__KERNEL__ void MaxwellEquationsTestFuse(float4 c, float2 u, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
   u+=0.5f;

   float3 U = to_float3_aw(u,0);
   float3 e = E(U,R,iChannel0);
   float3 m = M(U,R,iChannel1);
 
   float a = _atan2f(e.y,e.x);
   float3 cr = cross(e,m);
   float3 C = 0.5f+0.5f*sin_f3(10.0f*cr.x*to_float3(1,2,3));
   c = to_float4_aw(C,1);

  SetFragmentShaderComputedColor(c);
}