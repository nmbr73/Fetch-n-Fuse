
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3



//float2 R;
//__DEVICE__ float4 I (float2 U) {return texture(iChannel3,U/R);}
//__DEVICE__ float4 f (float2 U) {return texture(iChannel0,U/R);}


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution

#define IA(U) texture(iChannel3,(U)/R)
#define f(U) texture(iChannel0,(U)/R)
#define EA(U) texture(iChannel1,(U)/R)
//#define M(U) texture(iChannel2,(U)/R)

// Fluid
__DEVICE__ float4 F (float2 U, float2 R, __TEXTURE2D__ iChannel0) {
    // advection step :
    //  count backwards through spacetime
    //  what was going on two half times ago?
    U-=0.5f*swi2(f(U),x,y);// where I am - half speed = where I was half a time ago
    U-=0.5f*swi2(f(U),x,y);// where I was half time ago - half speed = where I was last
    return f(U);   // now what ever was I doing one time ago?
}
//__DEVICE__ float4 E (float2 U) {return texture(iChannel1,U/R);}
//__DEVICE__ float4 M (float2 U) {return texture(iChannel2,U/R);}
__DEVICE__ float4 X (float2 U, in float4 C, float2 r, float2 R, __TEXTURE2D__ iChannel0) {
  float4 n = F(U+r,R,iChannel0); // neighbor
    float2 rp = to_float2(-r.y,r.x); // perpiduclar to r
    float2 _r =  r *(n.z-C.z)  //  pressure gradent
               + rp*(n.w*C.w)  // + spin product
               + swi2(C,x,y);  //  + advected velocity 
    
    return to_float4( _r.x,_r.y,
           //====================//   = equals velocity dxy/dt
           dot(r ,swi2(n,x,y)-swi2(C,x,y))+n.z,    // pressure = radial change in velocity + pressure between cells
           dot(rp,swi2(n,x,y)-swi2(C,x,y))-(n.w)); // spin     = circular change in velocity + spin between cells
    
}
__KERNEL__ void WeirdScienceIvFuse__Buffer_A(float4 C, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

   U+=0.5f;
   C = F(U,R,iChannel0);
   float r2 = _sqrtf(2.0f)*0.5f; // without renormalization, the neighborhood needs to be equidistant to the cell
   // calculate the sum of all neighbor interactions
   C = X(U,C,to_float2( 1, 0),R,iChannel0) + 
       X(U,C,to_float2( 0, 1),R,iChannel0)+
       X(U,C,to_float2(-1, 0),R,iChannel0)+
       X(U,C,to_float2( 0,-1),R,iChannel0)+
       X(U,C,to_float2( r2, r2),R,iChannel0)+
       X(U,C,to_float2(-r2, r2),R,iChannel0)+
       X(U,C,to_float2(-r2,-r2),R,iChannel0)+
       X(U,C,to_float2( r2,-r2),R,iChannel0);
   C /= 8.0f; // divide by the neighborhood size

   swi2S(C,x,y, swi2(C,x,y) + swi2(EA(U),x,y)*(1.0f-IA(U).x));
   if (iFrame < 1)                                      C = to_float4_s(0);
   if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)  C.x*=0.0f,C.y*=0.0f;//swi2(C,x,y) *= 0.0f;


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


// ELECTRIC FIELD

//float2 R;
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
//__DEVICE__ float2 v (float2 U) {return texture(iChannel0, U/R).xy;}
#define v(U) swi2(texture(iChannel0,(U)/R),x,y)

__DEVICE__ float3 EB (float3 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    swi2S(U,x,y, swi2(U,x,y) - 0.5f*v(swi2(U,x,y)));
    swi2S(U,x,y, swi2(U,x,y) - 0.5f*v(swi2(U,x,y)));
  return swi3(texture(iChannel1,swi2(U,x,y)/R),x,y,z);
}
__DEVICE__ float3 MB (float3 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2) {
    swi2S(U,x,y, swi2(U,x,y) - 0.5f*v(swi2(U,x,y)));
    swi2S(U,x,y, swi2(U,x,y) - 0.5f*v(swi2(U,x,y)));
  return swi3(texture(iChannel2,swi2(U,x,y)/R),x,y,z);
}
//__DEVICE__ float4 IB (float2 U) {return texture(iChannel3,U/R);}
#define IB(U) texture(iChannel3,(U)/R)

__DEVICE__ float3 XB (float3 U, float3 _R, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2) {
  return cross(_R,MB(U+_R,R,iChannel0,iChannel2));
}

__KERNEL__ void WeirdScienceIvFuse__Buffer_B(float4 c, float2 u, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
   u+=0.5f;
 
   float3 U = to_float3_aw(u,0);
   # define l (1.0f/_sqrtf (2.0f))
   float3 mu = 0.25f*(EB(U+to_float3(1,0,0),R,iChannel0,iChannel1)
                     +EB(U-to_float3(1,0,0),R,iChannel0,iChannel1)
                     +EB(U+to_float3(0,1,0),R,iChannel0,iChannel1)
                     +EB(U-to_float3(0,1,0),R,iChannel0,iChannel1));
   float3 C = _mix(EB(U,R,iChannel0,iChannel1),mu,0.1f)
           + (-1.0f * MB(U,R,iChannel0,iChannel2) + 
      ( XB(U, to_float3( 1, 0,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3(-1, 0,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3( 0, 1,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3( 0,-1,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3( l, l,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3( l,-l,0),R,iChannel0,iChannel2) +
        XB(U, to_float3(-l,-l,0),R,iChannel0,iChannel2) +
        XB(U, to_float3(-l, l,0),R,iChannel0,iChannel2) ) / 8.0f);
   
   swi2S(C,x,y, swi2(C,x,y) + swi2(IB(swi2(U,x,y)),z,w));

   if (iFrame < 1)   C = to_float3_s(0);
   c = to_float4_aw(C,0);

  SetFragmentShaderComputedColor(c);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


// MAGNETIC FIELD
//float2 R;
//__DEVICE__ float4 I (float2 U) {return texture(iChannel3,U/R);}
//__DEVICE__ float2 v (float2 U) {return texture(iChannel0,U/R).xy;}
__DEVICE__ float3 EC (float3 U, float2 R, __TEXTURE2D__ iChannel1) {
  return swi3(texture(iChannel1,swi2(U,x,y)/R),x,y,z);
}

#ifdef XXX
__DEVICE__ float3 M (float3 U) {
    swi2(U,x,y) -= 0.5f*v(swi2(U,x,y));
    swi2(U,x,y) -= 0.5f*v(swi2(U,x,y));
  return swi3(texture(iChannel2,swi2(U,x,y)/R),x,y,z);
}


__DEVICE__ float3 X (float3 U, float3 R) {
  return cross(R,E(U+R));
}
#endif

__KERNEL__ void WeirdScienceIvFuse__Buffer_C(float4 c, float2 u, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
   u+=0.5f;
 
   float3 U = to_float3_aw(u,0);
float CCCCCCCCCCCCCCCCCCCCC;        
   # define l (1.0f/_sqrtf(2.0f))
   float3 mu = 0.25f*(MB(U+to_float3(1,0,0),R,iChannel0,iChannel2)
                     +MB(U-to_float3(1,0,0),R,iChannel0,iChannel2)
                     +MB(U+to_float3(0,1,0),R,iChannel0,iChannel2)
                     +MB(U-to_float3(0,1,0),R,iChannel0,iChannel2));
   float3 C = _mix(MB(U,R,iChannel0,iChannel2),mu,0.1f)
              + (  EC(U,R,iChannel1) + 
      ( XB(U, to_float3( 1, 0,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3(-1, 0,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3( 0, 1,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3( 0,-1,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3( l, l,0),R,iChannel0,iChannel2) + 
        XB(U, to_float3( l,-l,0),R,iChannel0,iChannel2) +
        XB(U, to_float3(-l,-l,0),R,iChannel0,iChannel2) +
        XB(U, to_float3(-l, l,0),R,iChannel0,iChannel2) ) / 8.0f);        
     
        
   if (iFrame < 1) C = to_float3_s(0);
   c = to_float4_aw(C,0);


  SetFragmentShaderComputedColor(c);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// ion particles
//float2 R;
//__DEVICE__ float2 v (float2 U) {return texture(iChannel0,U/R).xy;}
//__DEVICE__ float4 i (float2 U) {return texture(iChannel3,U/R);}
#define i(U) texture(iChannel3,(U)/R)

__DEVICE__ float4 I (float2 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel3) {
    U-=0.5f*swi2(v(U),x,y);
    U-=0.5f*swi2(v(U),x,y);
    return i(U);
}
//__DEVICE__ float4 E (float2 U) {return texture(iChannel1,U/R);}
#define ED(U) texture(iChannel1,(U)/R)

__KERNEL__ void WeirdScienceIvFuse__Buffer_D(float4 C, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{

   U+=0.5f;
   C = I(U,R,iChannel0,iChannel3);
   float n = I(U+to_float2(0,1),R,iChannel0,iChannel3).x,
         e = I(U+to_float2(1,0),R,iChannel0,iChannel3).x,
         s = I(U-to_float2(0,1),R,iChannel0,iChannel3).x,
         w = I(U-to_float2(1,0),R,iChannel0,iChannel3).x,
         mu = 0.25f*(n+e+s+w);
   
   if (iMouse.z>0.0f)   C.x = _mix(C.x,1.0f,smoothstep(10.0f,0.0f,length(U-swi2(iMouse,x,y))));
   if (C.x < 0.5f)      C.x -= 0.01f*C.x*C.x;
   else if (C.x > 0.6f) C.x += 0.01f*(1.0f-C.x)*(1.0f-C.x);
float DDDDDDDDDDDDDDDDDD;    
   swi2S(C,z,w, -1.0f*to_float2(n-s,-e+w)-to_float2(e-w,n-s));
    
   if (iFrame < 1) {
       C = to_float4_s(0);
       C.x = smoothstep(0.15f*R.y,0.15f*R.y-3.0f,length(U-0.5f*R));
   }

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


//vec2 R;
//__DEVICE__ float4 F (float2 U) {return texture(iChannel0,U/R);}
#define FI(U) texture(iChannel0,(U)/R)
//__DEVICE__ float4 E (float2 U) {return texture(iChannel2,U/R);}
#define EI(U) texture(iChannel2,(U)/R)
//__DEVICE__ float4 I (float2 U) {return texture(iChannel3,U/R);}
#define II(U) texture(iChannel3,(U)/R)


__KERNEL__ void WeirdScienceIvFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
   U+=0.5f;
   float4 i = II(U);
   float4 f = FI(U);
   float4 e = EI(U);
   C = 0.5f+0.5f*sin_f4(1.0f*length(e)+10.0f*to_float4(1,1.3f,1.5f,4)*f.z);

   float3 n = normalize(to_float3_aw(swi2(f,x,y),0.1f));
   C *= 0.5f+0.5f*decube_f3(iChannel1,n);
   float IIIIIIIIIIIIIIIIIIII;
   C *= to_float4_s(1.0f)-swi4(i,x,x,x,x)+2.0f*length(swi2(i,z,w));

  SetFragmentShaderComputedColor(C);
}
