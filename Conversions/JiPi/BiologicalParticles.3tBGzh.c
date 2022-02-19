
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define A(U) _tex2DVecN(iChannel0, (U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1, (U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2, (U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3, (U).x/R.x,(U).y/R.y,15)
#define E(U) _tex2DVecN(iChannel4, (U).x/R.x,(U).y/R.y,15)


#define S to_float4(2,4,6,8)
#define M 0.1f*to_float4(4,3,2,1)
#define O 0.5f/S/S
#define I 12.0f

__DEVICE__ float4 hash (float p) // Dave (Hash)kins
{
  float4 p4 = fract_f4(to_float4_s(p) * to_float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
    p4 += dot(p4, swi4(p4,w,z,x,y)+19.19f);
    return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x))*2.0f-1.0f;
    
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


// FLUID DYNAMICS
// FORCE on FLUID = (PARTICLE)*(GRADIENT OF BUFFER D)

__DEVICE__ float4 T (float2 U, float2 iResolution, __TEXTURE2D__ iChannel0 ) {return A(U-swi2(A(U),x,y));}

__KERNEL__ void BiologicalParticlesFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
  //CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);
  CONNECT_BUTTON0(Modus, 1, Icks, Yps, Zet, Weh, Erase);
    
    U += 0.5f;
     
    Q = T(U,iResolution,iChannel0);
    float4 // neighborhood
        n = T(U+to_float2(0,1),iResolution,iChannel0),
        e = T(U+to_float2(1,0),iResolution,iChannel0),
        s = T(U-to_float2(0,1),iResolution,iChannel0),
        w = T(U-to_float2(1,0),iResolution,iChannel0);
   // FLUID DYNAMICS
   Q.x -= (0.25f*(e.z-w.z-Q.w*(n.w-s.w)));
   Q.y -= (0.25f*(n.z-s.z-Q.w*(e.w-w.w)));
   Q.z += (0.25f*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z))-Q.z);
   Q.w += (0.25f*(s.x-n.x+w.y-e.y)-Q.w);
   // COMPUTE HORMONE FEILD
   n = D(U+to_float2(0,1));
   e = D(U+to_float2(1,0));
   s = D(U-to_float2(0,1));
   w = D(U-to_float2(1,0));
   // THIS PARTICLE
   float4 b = B(U);
   // COMPUTE HORMONE SIGNATURE
   float4 h = hash(b.w);
   // SUM HORMONE FORCE
   float2 v = to_float2_s(0);
   v += h.x*to_float2(e.x-w.x,n.x-s.x);
   v += h.y*to_float2(e.y-w.y,n.y-s.y);
   v += h.z*to_float2(e.z-w.z,n.z-s.z);
   v += h.w*to_float2(e.w-w.w,n.w-s.w);
   // APPLY HORMONE FORCE TO THIS PARTICLE
   float2 Qxy = swi2(Q,x,y) + v*smoothstep(1.0f,0.0f,length(U-swi2(b,x,y)));
   Q.x=Qxy.x;Q.y=Qxy.y;
   
   // BOUNDARY CONDITIONS
   if (fract_f(0.1f*iTime)<0.2f)   { 
      float2 Qxy = swi2(Q,x,y) + 0.03f*smoothstep(20.0f,0.0f,length(U-0.5f*R+0.3f*R*swi2(hash(_floor(0.1f*iTime)),x,y)))*to_float2(_cosf(_floor(0.1f*iTime)),_sinf(_floor(0.1f*iTime)));
      Q.x=Qxy.x;Q.y=Qxy.y;
   }
   
   if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f||iFrame<1)
       //swi3(Q,x,y,w) = to_float3(0);
       Q.x = 0.0f, Q.y=0.0f, Q.w = 0.0f;


  if (Blend1>0.0f)
  {
    float4 tex = E(U);
    if (tex.w != 0.0f)    
    {
      tex = tex*2.0 - 1.0f;
      if ((int)Modus & 2) Q.x = _mix(Q.x,tex.x,Blend1);
      if ((int)Modus & 4) Q.y = _mix(Q.y,tex.y,Blend1);
      if ((int)Modus & 8) Q.z = _mix(Q.z,tex.z,Blend1);
      if ((int)Modus & 16) Q.w = _mix(Q.w,tex.x,Blend1);
      if ((int)Modus & 32) Q = to_float4(0.0f,0.0f,1.0f,1.0f);
    }  
  } 


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


// SPACIALLY SORT VORONOI PARTICLES
// ALLOW MOVING PARTICLES TO LEAVE A TRAIL OF CLONES
__DEVICE__ void swap (inout float4 *Q, float2 U, float2 r, float2 iResolution, __TEXTURE2D__ iChannel1) {
  float4 n = B(U+r);
    if (length(U-swi2(n,x,y))<length(U-swi2(*Q,x,y))) *Q = n;
}
__KERNEL__ void BiologicalParticlesFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
  
    U += 0.5f;
    
    // FIND NEAREST PARTICLE
    Q = B(U);
    swap(&Q,U,to_float2(1,0),iResolution,iChannel1);
    swap(&Q,U,to_float2(0,1),iResolution,iChannel1);
    swap(&Q,U,to_float2(-1,0),iResolution,iChannel1);
    swap(&Q,U,to_float2(0,-1),iResolution,iChannel1);
    swap(&Q,U,to_float2(1,1),iResolution,iChannel1);
    swap(&Q,U,to_float2(1,-1),iResolution,iChannel1);
    swap(&Q,U,to_float2(-1,1),iResolution,iChannel1);
    swap(&Q,U,to_float2(-1,-1),iResolution,iChannel1);
    // LEAVE A TRIAL OF CLONES AS PARTICLE TRANSLATES
    float2 Qxy = swi2(Q,x,y) + swi2(A(_mix(U,swi2(Q,x,y),0.7f)),x,y);
    Q.x=Qxy.x;Q.y=Qxy.y;
    // BOUNDARY CONDITIONS
    if ((iMouse.z>0.0f && length(swi2(iMouse,x,y)-U) < 30.0f) || iFrame < 1) 
    {  
    Q = to_float4(U.x,U.y,0,0);
      //Q.w = 0.1f*(Q.x+R.x*Q.y+dot(iDate,to_float4_s(1)));
      Q.w = 0.1f*(Q.x+R.x*Q.y+dot(to_float4( 2022.0f,0001.0f,0003.0f,iTime),to_float4_s(1)));
    }
  
  //if (Blend1>0.0f)
  //{
  //  Q = _mix(Q,E(U),Blend1);
  //}
  

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


// BLUR PARTICLES PASS 1
__KERNEL__ void BiologicalParticlesFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel1)
{
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    U += 0.5f;

    Q = to_float4_s(0);
    for (float i = -I; i <= I; i+=1.0f) {
      float2 _x = U+to_float2(i,0);
      float4 b = B(_x);
      Q += hash(b.w)*M*exp_f4(-i*i*O)*smoothstep(1.0f,0.0f,length(swi2(b,x,y)-_x));
    }
    

//Q=B(U); //TEst
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// BLUR PASS 2
__KERNEL__ void BiologicalParticlesFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);

    U += 0.5f;
    Q = 0.5f*D(U);
    for (float i = -I; i <= I; i+=1.0f) {
      float4 c = C(U+to_float2(0,i));
      Q += c*M*exp_f4(-O*i*i);
    }
    if(iFrame<1) Q = to_float4_s(0);

/*
  if (Blend1>0.0f)
  {
    float4 tex = E(U);
    if (tex.w != 0.0f)    
      Q = _mix(Q,tex,Blend1);
  } 
*/

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


/*

  Fluid dynamics controls velocity field.
  Particles translate with the velocity field.
  Particles reproduce as they move. 
  Particles diffuse  4  hormones. 
  Diffusion is mediated by a 2 pass multi-scale gaussian blur.
  Particles experience a force from each hormone.
  The force is proportional to their own hormone signature. 
  Each hormone diffuses with a different radius. 
  Each initial particle has its own hormone signature.
  Then they battle it out! 

*/
__KERNEL__ void BiologicalParticlesFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);

    float4 b = B(U);
    float4 h = (hash(b.w));
    Q = smoothstep(2.0f,0.0f,length(swi2(b,x,y)-U))*(0.5f+2.0f*h);
  //Q = _fabs(D(U));

  if (Blend1>0.0f)
  {
    float4 tex = E(U);
    if (tex.w != 0.0f)    
      Q = _mix(Q,tex,Blend1);
  } 

  SetFragmentShaderComputedColor(Q);
}