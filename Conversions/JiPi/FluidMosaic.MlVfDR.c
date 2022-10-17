
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


//Fluid Algorithm  https://www.shadertoy.com/view/MtdBDB
//float2 R;float N;
__DEVICE__ float4 T ( float2 U, float2 R, __TEXTURE2D__ iChannel0) {return texture(iChannel0,U/R);}
__DEVICE__ float X (float2 U0, float2 U, float2 U1, inout float4 *Q, in float2 r, float2 R, float N, __TEXTURE2D__ iChannel0) {
    float2 V = U + r, u = swi2(T(V,R,iChannel0),x,y),
         V0 = V - u,
         V1 = V + u;
         
    float P = T (V0,R,iChannel0).z, rr = length(r);
    swi2S(*Q,x,y, swi2(*Q,x,y) - r*(P-(*Q).z)/rr/N);
    return (0.5f*(length(V0-U0)-length(V1-U1))+P)/N;
}
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}


__KERNEL__ void FluidMosaicFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
   U+=0.5f;
   
   float2 U0 = U - swi2(T(U,R,iChannel0),x,y),
          U1 = U + swi2(T(U,R,iChannel0),x,y);
   float P = 0.0f; Q = T(U0,R,iChannel0);
   float N = 4.0f;
   P += X (U0,U,U1,&Q, to_float2( 1, 0),R,N,iChannel0 );
   P += X (U0,U,U1,&Q, to_float2( 0,-1),R,N,iChannel0 );
   P += X (U0,U,U1,&Q, to_float2(-1, 0),R,N,iChannel0 );
   P += X (U0,U,U1,&Q, to_float2( 0, 1),R,N,iChannel0 );
   Q.z = P;
   if (iFrame < 1)                                             Q = to_float4_s(0);
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) Q.x=0.0f,Q.y=0.0f;//swi2(Q,x,y) *= 0.0f;

   if (length(U-to_float2(0.1f,0.5f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(0.5f,-0.3f));
   if (length(U-to_float2(0.7f,0.3f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(-0.6f,0.3f));
   if (length(U-to_float2(0.2f,0.2f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(0.4f,0.6f));
   if (length(U-to_float2(0.7f,0.5f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(-0.1f,-0.3f));
   if (length(U-to_float2(0.5f,0.6f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(0,-0.7f));
    
   float4 mo = texture(iChannel2,to_float2_s(0));
   float l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   if (mo.z > 0.0f && l < 10.0f)  
        swi3S(Q,x,y,z, swi3(Q,x,y,z) + to_float3_aw((10.0f-l)*(swi2(mo,x,y)-swi2(mo,z,w))/R.y,(10.0f-l)*(length(swi2(mo,x,y)-swi2(mo,z,w))/R.y)*0.02f));
 
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


// Voronoi based particle tracking

//float2 R;float N;
//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__DEVICE__ float4 P ( float2 U, float2 R, __TEXTURE2D__ iChannel1 ) {return texture(iChannel1,U/R);}
__DEVICE__ void swap (float2 U, inout float4 *Q, float2 u, float2 R, __TEXTURE2D__ iChannel1) {
    float4 p = P(U+u,R,iChannel1);
    float dl = length(U-swi2(*Q,x,y)) - length(U-swi2(p,x,y));
    float e = 0.1f;
    // allows for probabistic reproduction
    *Q = _mix(*Q,p,0.5f+0.5f*sign_f(_floor(1e5*dl)));//the value next to dl adjusts the proabability of reproduction
    
    //uncomment and comment the line above to make it not self healing 
    *Q = _mix(*Q,p,dl>0.?1.:0.);
}

//__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
//    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
//}


__KERNEL__ void FluidMosaicFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
   U+=0.5f;
   
   // go back through the fluid and test the neighbors
   //  for the closes particles
   U = U-swi2(T(U,R,iChannel0),x,y);
   Q = P(U,R,iChannel1);
   swap(U,&Q,to_float2(1,0),R,iChannel1);
   swap(U,&Q,to_float2(0,1),R,iChannel1);
   swap(U,&Q,to_float2(0,-1),R,iChannel1);
   swap(U,&Q,to_float2(-1,0),R,iChannel1);
 

   // add color from the jets in the fluid
   if ((length(swi2(Q,x,y)-to_float2(0.1f,0.5f)*R) < 0.02f*R.y))
        Q.z=1.0f,Q.w=1.0f;//swi2S(Q,z,w) = to_float2(1,1);
    if ((length(swi2(Q,x,y)-to_float2(0.7f,0.3f)*R) < 0.02f*R.y))
        Q.z=3.0f,Q.w=3.0f;//swi2S(Q,z,w) = to_float2(3,3);
    if ((length(swi2(Q,x,y)-to_float2(0.2f,0.2f)*R) < 0.02f*R.y))
        Q.z=6.0f,Q.w=5.0f;//swi2S(Q,z,w) = to_float2(6,5);
   if (length(swi2(Q,x,y)-to_float2(0.7f,0.5f)*R) < 0.02f*R.y)
        Q.z=2.0f,Q.w=7.0f;//swi2S(Q,z,w) = to_float2(2,7);
   if (length(swi2(Q,x,y)-to_float2(0.5f,0.6f)*R) < 0.02f*R.y) 
        Q.z=5.0f,Q.w=4.0f;//swi2S(Q,z,w) = to_float2(5,4);
   float4 mo = texture(iChannel2,to_float2_s(0));
   if (mo.z > 0.0f && ln(U,swi2(mo,x,y),swi2(mo,z,w)) < 10.0f) Q = to_float4(U.x,U.y,1,3.0f*_sinf(0.4f*iTime));
 
   // advect this particle with the fluid
   swi2S(Q,x,y, swi2(Q,x,y) + swi2(T(swi2(Q,x,y),R,iChannel0),x,y));
   if (iFrame < 1)  Q = to_float4(_floor(U.x/10.0f+0.5f)*10.0f,_floor(U.y/10.0f+0.5f)*10.0f,0.2f,-0.1f);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0


// keep track of mouse
__KERNEL__ void FluidMosaicFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float4 p = texture(iChannel0,fragCoord/iResolution);
    if (iMouse.z>0.0f) {
        if (p.z>0.0f) fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else fragColor = to_float4_f2f2(-iResolution,-iResolution);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2


//Fluid Algorithm  https://www.shadertoy.com/view/MtdBDB
//float2 R;float N;
#ifdef XXXXXXXXX
__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__DEVICE__ float X (float2 U0, float2 U, float2 U1, inout float4 Q, in float2 r) {
    float2 V = U + r, u = T(V).xy,
         V0 = V - u,
         V1 = V + u;
    float P = T (V0).z, rr = length(r);
    swi2(Q,x,y) -= r*(P-Q.z)/rr/N;
    return (0.5f*(length(V0-U0)-length(V1-U1))+P)/N;
}
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
#endif

__KERNEL__ void FluidMosaicFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
   U+=0.5f;

   float2 U0 = U - swi2(T(U,R,iChannel0),x,y),
         U1 = U + swi2(T(U,R,iChannel0),x,y);
   float P = 0.0f; Q = T(U0,R,iChannel0);
   float N = 4.0f;
   P += X (U0,U,U1,&Q, to_float2( 1, 0),R,N,iChannel0 );
   P += X (U0,U,U1,&Q, to_float2( 0,-1),R,N,iChannel0 );
   P += X (U0,U,U1,&Q, to_float2(-1, 0),R,N,iChannel0 );
   P += X (U0,U,U1,&Q, to_float2( 0, 1),R,N,iChannel0 );
   Q.z = P;
   if (iFrame < 1) Q = to_float4_s(0);
    if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f)  Q.x=0.f,Q.y=0.0f;//swi2(Q,x,y) *= 0.0f;
   
   if (length(U-to_float2(0.1f,0.5f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(0.5f,-0.3f));
   if (length(U-to_float2(0.7f,0.3f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(-0.6f,0.3f));
   if (length(U-to_float2(0.2f,0.2f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(0.4f,0.6f));
   if (length(U-to_float2(0.7f,0.5f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(-0.1f,-0.3f));
   if (length(U-to_float2(0.5f,0.6f)*R) < 0.03f*R.y) 
        swi2S(Q,x,y, swi2(Q,x,y)*0.9f+0.1f*to_float2(0,-0.7f));
    
   float4 mo = texture(iChannel2,to_float2_s(0));
   float l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   if (mo.z > 0.0f && l < 10.0f) 
        swi3S(Q,x,y,z, swi3(Q,x,y,z) + to_float3_aw((10.0f-l)*(swi2(mo,x,y)-swi2(mo,z,w))/R.y,(10.0f-l)*(length(swi2(mo,x,y)-swi2(mo,z,w))/R.y)*0.02f));
 
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'https://soundcloud.com/symphonicsamples/cornfield-chase-hans-zimmer-midi-mockup' to iChannel2


// concept for voronoi tracking from user stb
//Render particles
//float2 R;
//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
//__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}


__KERNEL__ void FluidMosaicFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  U+=0.5f;

  C = P(U,R,iChannel1);
  float2 
        n = swi2(P(U+to_float2(0,1),R,iChannel1),x,y),
        e = swi2(P(U+to_float2(1,0),R,iChannel1),x,y),
        s = swi2(P(U-to_float2(0,1),R,iChannel1),x,y),
        w = swi2(P(U-to_float2(1,0),R,iChannel1),x,y);
   float d = (length(n-swi2(C,x,y))-1.0f+
        length(e-swi2(C,x,y))-1.0f+
        length(s-swi2(C,x,y))-1.0f+
        length(w-swi2(C,x,y))-1.0f);
        
   float m1 = 2.0f*texture(iChannel2,to_float2(_fabs(0.3f*C.w),0.0f)).x,
         m2 = 1.5f*texture(iChannel2,to_float2(_fabs(0.3f*C.z),0.0f)).x;
         
         m1=0.0f;
         m2=0.0f;
         
   //float p = smoothstep(2.5f,2.0f,length(swi2(C,x,y)-U));
   C = to_float4_s(0.5f)-0.5f*sin_f4(0.2f*(1.0f+m1)*C.z*to_float4_s(1)+0.4f*(3.0f+m2)*C.w*to_float4(1,3,5,4));

   C *= 1.0f-clamp(0.1f*d,0.0f,1.0f);

  SetFragmentShaderComputedColor(C);
}