
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//vec2 R;
#define R iResolution

__DEVICE__ float4 T ( float2 U, float2 R, __TEXTURE2D__ iChannel0 ) {return texture(iChannel0,U/R);}
__DEVICE__ float4 P ( float2 U, float2 R, __TEXTURE2D__ iChannel1 ) {return texture(iChannel1,U/R);}
// This is my normal fluid interaction with a minor variation
__DEVICE__ float X (float2 U, float2 u, inout float4 *Q, in float2 r, float2 R, __TEXTURE2D__ iChannel0) {
    float2 V = U + r, v = swi2(T(V,R,iChannel0),x,y);
    float4 t = T (V-v,R,iChannel0);
    // the "w" channel contributes to the force interaction
    
    swi2S(*Q,x,y, swi2(*Q,x,y) - 0.25f*r*(t.z-(*Q).z+t.w-(*Q).w)); 
    return (0.5f*(length(r-v+u)-length(r+v-u))+t.z);
}

__KERNEL__ void ThermalParticlesFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   U+=0.5f;
    
float AAAAAAAAAAAAAAAAA;
   float2 u = swi2(T(U,R,iChannel0),x,y), e = to_float2(1,0);
   Q = T(U-u,R,iChannel0);
   float4 p = P(U-u,R,iChannel1);
   // The particles sit in a damped fluid
   Q.z = 0.25f*(X(U,u,&Q,e,R,iChannel0)+X(U,u,&Q,-1.0f*e,R,iChannel0)+X(U,u,&Q,swi2(e,y,x),R,iChannel0)+X(U,u,&Q,-1.0f*swi2(e,y,x),R,iChannel0));
  // the particles interact with the space so i just draw a circle where they are
   Q.w =  sign_f(p.x)*smoothstep(2.0f,0.0f,length(U-abs_f2(swi2(p,x,y))));
   // the pressure is damped so that it doesn't go too crazy
   Q.z *= 0.96f;
   // make a little pressure and density where the mouse is
   if (iMouse.z > 0.0f) Q.w = _mix(Q.w,3.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1) Q = to_float4_s(0);
   if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.0f) Q.x=0.f,Q.y=0.0f;//swi2(Q,x,y) = to_float2(0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
//__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}
// This is my normal fluid interaction with a minor variation
//__DEVICE__ float X (float2 U, float2 u, inout float4 Q, in float2 r) {
//    float2 V = U + r, v = T(V).xy;
//    float4 t = T (V-v);
    // the "w" channel contributes to the force interaction
//    swi2(Q,x,y) -= 0.25f*r*(t.z-Q.z+t.w-Q.w); 
//    return (0.5f*(length(r-v+u)-length(r+v-u))+t.z);
//}

__KERNEL__ void ThermalParticlesFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   U+=0.5f; 
  
float BBBBBBBBBBBBBBBB;
   float2 u = swi2(T(U,R,iChannel0),x,y), e = to_float2(1,0);
   Q = T(U-u,R,iChannel0);
   float4 p = P(U-u,R,iChannel1);
   // The particles sit in a damped fluid
   Q.z = 0.25f*(X(U,u,&Q,e,R,iChannel0)+X(U,u,&Q,-1.0f*e,R,iChannel0)+X(U,u,&Q,swi2(e,y,x),R,iChannel0)+X(U,u,&Q,-1.0f*swi2(e,y,x),R,iChannel0));
  // the particles interact with the space so i just draw a circle where they are
   Q.w =  sign_f(p.x)*smoothstep(2.0f,0.0f,length(U-abs_f2(swi2(p,x,y))));
   // the pressure is damped so that it doesn't go too crazy
   Q.z *= 0.96f;
   // make a little pressure and density where the mouse is
   if (iMouse.z > 0.0f) Q.w = _mix(Q.w,3.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1) Q = to_float4_s(0);
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) Q.x=0.0f,Q.y=0.0f; //swi2(Q,x,y) = to_float2(0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel1


#ifdef XXX
__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}
// This is my normal fluid interaction with a minor variation
__DEVICE__ float X (float2 U, float2 u, inout float4 Q, in float2 r) {
    float2 V = U + r, v = T(V).xy;
    float4 t = T (V-v);
    // the "w" channel contributes to the force interaction
    swi2(Q,x,y) -= 0.25f*r*(t.z-Q.z+t.w-Q.w); 
    return (0.5f*(length(r-v+u)-length(r+v-u))+t.z);
}
#endif

__KERNEL__ void ThermalParticlesFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   U+=0.5f;
   
float CCCCCCCCCCCCCCCCCC;   
   float2 u = swi2(T(U,R,iChannel0),x,y), e = to_float2(1,0);
   Q = T(U-u,R,iChannel0);
   float4 p = P(U-u,R,iChannel1);
   // The particles sit in a damped fluid
   Q.z = 0.25f*(X(U,u,&Q,e,R,iChannel0)+X(U,u,&Q,-1.0f*e,R,iChannel0)+X(U,u,&Q,swi2(e,y,x),R,iChannel0)+X(U,u,&Q,-1.0f*swi2(e,y,x),R,iChannel0));
  // the particles interact with the space so i just draw a circle where they are
   Q.w =  sign_f(p.x)*smoothstep(2.0f,0.0f,length(U-abs_f2(swi2(p,x,y))));
   // the pressure is damped so that it doesn't go too crazy
   Q.z *= 0.96f;
   // make a little pressure and density where the mouse is
   if (iMouse.z > 0.0f) Q.w = _mix(Q.w,3.0f,_expf(-0.1f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))));
   if (iFrame < 1) Q = to_float4_s(0);
   if (U.x < 1.0f||U.y < 1.0f||R.x-U.x < 1.0f||R.y-U.y < 1.0f) Q.x=0.0f,Q.y=0.0f; //swi2(Q,x,y) = to_float2(0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


// Voronoi based particle tracking

#ifdef XXX
float2 R;float N;
__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}//sample fluid
__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}//sample particles

#endif
__DEVICE__ void swap (float2 U, inout float4 *Q, float2 u, float2 R, __TEXTURE2D__ iChannel1) {
    float4 p = P(U+u,R,iChannel1);
    float dl = length(U-abs_f2(swi2(*Q,x,y))) - length(U-abs_f2(swi2(p,x,y)));
    *Q = _mix(*Q,p,(float)(dl>0.0f));
}
__KERNEL__ void ThermalParticlesFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
   U+=0.5f;
   
float DDDDDDDDDDDDDDDDDD;   
   Q = P(U,R,iChannel1);
   // exchange information with neighbors to find particles
   swap(U,&Q,to_float2(1,0),R,iChannel1);
   swap(U,&Q,to_float2(0,1),R,iChannel1);
   swap(U,&Q,to_float2(0,-1),R,iChannel1);
   swap(U,&Q,to_float2(-1,0),R,iChannel1);
   swap(U,&Q,to_float2(3,0),R,iChannel1);
   swap(U,&Q,to_float2(0,3),R,iChannel1);
   swap(U,&Q,to_float2(0,-3),R,iChannel1);
   swap(U,&Q,to_float2(-3,0),R,iChannel1);
   // find this particle fluid state
   float4 t = T(swi2(Q,x,y),R,iChannel0);
   float2 e = to_float2(1,0);
   // find neighbor fluid states
   float4 
        a = T(swi2(Q,x,y)+swi2(e,x,y),R,iChannel0),
        b = T(swi2(Q,x,y)+swi2(e,y,x),R,iChannel0),
        c = T(swi2(Q,x,y)-swi2(e,x,y),R,iChannel0),
        d = T(swi2(Q,x,y)-swi2(e,y,x),R,iChannel0);
   // accelerate this particle in the direction of the z and w fields
   // z field is force at a distance. the particles displace the fluid and create a pressure field
   // w field is the discritized particle. its just a drawing of where they are
   // but when they get close they transfer energies because they interact with the radial drawing of the particle and push away
   swi2S(Q,z,w, swi2(Q,z,w)*0.95f+to_float2(a.z-c.z,b.z-d.z)-to_float2(a.w-c.w,b.w-d.w));
   // the particle moves in the diredction of its momentum
   swi2S(Q,x,y, swi2(Q,x,y) + 0.1f*swi2(Q,z,w));
      
   if (iFrame < 1) { // I don't know why the init looks so cool in practice, I just wanted a grid
       Q = to_float4_f2f2(_floor(U/10.0f)*10.0f,U);
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


/*
I plan on doing a few iterations of this
the physics is definitely not realistic yet
but it's a good practical start

The idea is that the particles sit in a fluid of smaller particles
their interaction with the fluid determines the force field around them
in this setup, the force is very weak and inaccurate. I plan on improving that.
The particles bounce off each other because of an interaction with the field that is 
the drawing of the particles.

*/


//float2 R;
//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
//__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}

__KERNEL__ void ThermalParticlesFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

     U+=0.5f;
   
     float4 t = T(U,R,iChannel0);
     float4 p = P(U,R,iChannel1);
     swi3S(Q,x,y,z, 0.5f+0.5f*sin_f3(1.5f*(2.0f*t.w+t.z*to_float3(1,2,3))));

 Q.w=1.0f;

  SetFragmentShaderComputedColor(Q);
}