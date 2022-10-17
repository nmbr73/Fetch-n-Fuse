
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


__DEVICE__ float hash(float2 p)
{ // Dave H
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
  return fract((p3.x + p3.y) * p3.z);
}
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 T ( float2 U, float2 R, __TEXTURE2D__ iChannel0 ) {return texture(iChannel0,U/R);}
__DEVICE__ float X (float2 U0, float2 U, float2 U1, inout float4 *Q, in float2 r, float2 R, float N, __TEXTURE2D__ iChannel0) {
    float2 V = U + r;
    float4 t = T(V, R, iChannel0);
    float2 V0 = V - swi2(t,x,y),
         V1 = V + swi2(t,x,y);
    float P = t.z, rr = length(r);
    swi2S(*Q,x,y, swi2(*Q,x,y) - r*(P-(*Q).z)/rr/N);
    return (0.5f*(length(V0-U0)-length(V1-U1))+P)/N;
}

__KERNEL__ void FixedVelocityFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{

 CONNECT_CHECKBOX0(Reset, 0);
  
 U+=0.5f;
   
 float N;
  
 //R = iResolution;
 float2 U0 = U - swi2(T(U, R, iChannel0),x,y),
        U1 = U + swi2(T(U, R, iChannel0),x,y);
 float  P  = 0.0f; Q = T(U0,R,iChannel0);
 if (length(swi2(Q,x,y))==0.0f || iFrame < 1 || Reset) {
       float h = 6.3f*hash(U);
       Q = to_float4(0.4f*_cosf(h), 0.4f*_sinf(h),0,0);
          
 } else {
   N = 4.0f;;
   P += X (U0,U,U1,&Q, to_float2( 1, 0),R,N,iChannel0);
   P += X (U0,U,U1,&Q, to_float2( 0,-1),R,N,iChannel0);
   P += X (U0,U,U1,&Q, to_float2(-1, 0),R,N,iChannel0);
   P += X (U0,U,U1,&Q, to_float2( 0, 1),R,N,iChannel0);
   Q.z = P;
   swi2S(Q,x,y, _mix(swi2(Q,x,y),0.4f*normalize(swi2(Q,x,y)),0.01f));
   float4 mo = texture(iChannel2,to_float2_s(0)+0.0f);
   float l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   if (mo.z > 0.0f && l < 10.0f) Q.z -= 0.1f*(10.0f-l);
 }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


// keep track of mouse
__KERNEL__ void FixedVelocityFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    float4 p = texture(iChannel0,fragCoord/iResolution);
    if (iMouse.z>0.0f) {
      if (p.z>0.0f) fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else          fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else fragColor = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


// Voronoi based particle tracking

//float2 R;float N;
//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__DEVICE__ float4 P ( float2 U, float2 R, __TEXTURE2D__ iChannel1 ) {return texture(iChannel1,U/R);}
__DEVICE__ void swap (float2 U, inout float4 *Q, float2 u, float2 R, __TEXTURE2D__ iChannel1) {
    float4 p = P(U+u, R, iChannel1);
    float dl = length(U-swi2(*Q,x,y)) - length(U-swi2(p,x,y));
    float e = 0.1f;
  
    // allows for probabistic reproduction
    *Q = _mix(*Q,p,0.5f+0.5f*sign_f(_floor(1e3*dl+0.5f)));
}
__KERNEL__ void FixedVelocityFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  
   CONNECT_CHECKBOX0(Reset, 0);
  
   U+=0.5f;
   
   float N;
   //R = iResolution;
   U = U-2.0f*swi2(T(U,R,iChannel0),x,y);
   U = U-2.0f*swi2(T(U,R,iChannel0),x,y);
   Q = P(U,R,iChannel1);
   swap(U,&Q,to_float2(1,0),R, iChannel1);
   swap(U,&Q,to_float2(0,1),R, iChannel1);
   swap(U,&Q,to_float2(0,-1),R, iChannel1);
   swap(U,&Q,to_float2(-1,0),R, iChannel1);
   swi2S(Q,x,y, swi2(Q,x,y) + 2.0f*swi2(T(swi2(Q,x,y),R,iChannel0),x,y));
   swi2S(Q,x,y, swi2(Q,x,y) + 2.0f*swi2(T(swi2(Q,x,y),R,iChannel0),x,y));
   if (Q.z == 0.0f) Q = to_float4_f2f2(_floor(U/10.0f)*10.0f,U);
   
   if (Reset) Q = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel1
// Connect Buffer D 'Previsualization: Buffer D' to iChannel2


//Render particles
//float2 R;
//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
//__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}
__DEVICE__ float4 D ( float2 U, float2 R, __TEXTURE2D__ iChannel2 ) {return texture(iChannel2,U/R);}
__KERNEL__ void FixedVelocityFuse__Buffer_D(float4 C, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  CONNECT_CHECKBOX0(Reset, 0);
  
  U+=0.5f;
  
  //R = iResolution;
   C = P(U,R, iChannel1);
   C = to_float4_aw(to_float3_s(smoothstep(1.5f,0.5f,length(swi2(C,x,y)-U))),1);
   C = C+to_float4(0.995f,0.98f,0.95f,1.0f)*(D(U,R, iChannel2));
   if(iFrame < 1 || Reset) C = to_float4_s(0);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


//vec2 R;
//__DEVICE__ float4 T (float2 U) {return texture(iChannel0,U/R);}
__DEVICE__ float4 DI (float2 U,float2 R,__TEXTURE2D__ iChannel1) {return texture(iChannel1,U/R);}
__KERNEL__ void FixedVelocityFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
   R = iResolution;
   float4 i = DI(U,R,iChannel1),
          t = T(U,R,iChannel0);
   float2 d = to_float2(
                        DI(U+to_float2(1,0),R,iChannel1).x-DI(U-to_float2(1,0),R,iChannel1).x,
                        DI(U+to_float2(0,1),R,iChannel1).x-DI(U-to_float2(0,1),R,iChannel1).x
                       );
   C = abs_f4(sin_f4(0.2f*sqrt_f4(i)*to_float4(1.0f,1.3f,1.5f,4)));
float IIIIIIIIIIIIIIIIII;
  SetFragmentShaderComputedColor(C);
}