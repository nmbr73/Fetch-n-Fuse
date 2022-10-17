
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

//#define VORTEX_SHEDDING_MODE
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0


__DEVICE__ float4 T (float2 U,float2 R, __TEXTURE2D__ iChannel0) {
  // sample things where they were, not where they are
  // half step backwards through time twice
  U -= 0.5f*swi2(A(U),x,y);
  U -= 0.5f*swi2(A(U),x,y);
  return A(U);
}
__KERNEL__ void NimitzSVorticityConfinementFuse__Buffer_A(float4 C, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);  

   U+=0.5f;
    // me and my neighborhoood
    // anytime there is a "0.25*__"
    // this is because we are using a grid
    // if we had hexagonal tiling you would see "1/6*__"
   float4 me = T(U,R,iChannel0),
           n = T(U+to_float2(0,1),R,iChannel0), // north  up
           e = T(U+to_float2(1,0),R,iChannel0), // east   left
           s = T(U-to_float2(0,1),R,iChannel0), // south  down
           w = T(U-to_float2(1,0),R,iChannel0), // west   right
          mu = 0.25f*(n+e+s+w); // average
   C = me;
   C.x -= 0.25f*(e.z-w.z); // change in pressure from left to right
   C.y -= 0.25f*(n.z-s.z); // change in pressure from top to bottom
   // divergence plus pressure exchange :
   C.z = mu.z // average pressure of neighborhood
        +0.25f*(s.y-n.y+w.x-e.x); // how much is the neighborhood pushing on me

/////////////////////
// adapted from : https://www.shadertoy.com/view/4tGfDW     
#define SPIN_PERMITIVITY 0.1f    
/**/C.w = 
    _mix(mu.w,C.w,SPIN_PERMITIVITY) // neighbors trade spin
    +       SPIN_PERMITIVITY*(// the curl puts spin into the cell
        (e.y - w.y - n.x + s.x) - C.w // difference between the curl and the spin
    );
/**/swi2S(C,x,y, swi2(C,x,y) + 
    _fabs(C.w)* // the spin of the cell 
    0.25f*to_float2(
        n.w-s.w,  // the baseball shooter force from top to bottom - so it shoots from left to right
        w.w-e.w)); // the baseball shooter force from the left to the right - so it shoots up or down
////////////////////// 
    
   
  // boundary conditions
   #ifdef VORTEX_SHEDDING_MODE
   if (length(U-to_float2(0.1f,0.5f)*R)<0.02f*R.x) C = _mix(C,to_float4(0,0,0,1),0.01f);
   if (U.x < 1.0f || iFrame < 1 || Reset)          C = to_float4(0.15f,0,0,0);
   #else
   if (length(U-to_float2(0.1f,0.5f)*R)<0.02f*R.x) C = _mix(C,to_float4(0.7f,0,0,1),0.01f);
   if (length(U-to_float2(0.9f,0.5f)*R)<0.02f*R.x) C = _mix(C,to_float4(-0.7f,0,0,1),0.01f);
   if (length(U-to_float2(0.5f,0.1f)*R)<0.02f*R.x) C = _mix(C,to_float4(0,0.7f,0,1),0.01f);
   if (length(U-to_float2(0.5f,0.9f)*R)<0.02f*R.x) C = _mix(C,to_float4(0,-0.7f,0,1),0.01f);
   if (iFrame < 1) C = to_float4_s(0);
   #endif


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void NimitzSVorticityConfinementFuse__Buffer_B(float4 C, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0);  
  
   U+=0.5f;
   U = U-0.5f*swi2(A(U),x,y);
   U = U-0.5f*swi2(A(U),x,y);
   C = B(U);
   #ifdef VORTEX_SHEDDING_MODE
   if (length(U-to_float2(0.1f,0.5f)*R)<0.02f*R.x) C = to_float4(0,0,0,1.5f);
   if (U.x < 1.0f || iFrame < 1 || Reset)          C = to_float4_s(0);
   #else
   if (length(U-to_float2(0.1f,0.5f)*R)<0.02f*R.x) C.x = 1.0f;
   if (length(U-to_float2(0.9f,0.5f)*R)<0.02f*R.x) C.y = 1.0f;
   if (length(U-to_float2(0.5f,0.1f)*R)<0.02f*R.x) C.z = 1.0f;
   if (length(U-to_float2(0.5f,0.9f)*R)<0.02f*R.x) C.w = 1.0f;
   if (iFrame < 1) C = to_float4_s(0);
   #endif
   if (iFrame < 1) C = to_float4_s(0);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void NimitzSVorticityConfinementFuse__Buffer_C(float4 C, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0)
{
   CONNECT_CHECKBOX0(Reset, 0);     
   
   U+=0.5f;
    // me and my neighborhoood
    // anytime there is a "0.25*__"
    // this is because we are using a grid
    // if we had hexagonal tiling you would see "1/6*__"
   float4 me = T(U,R,iChannel0),
           n = T(U+to_float2(0,1),R,iChannel0), // north  up
           e = T(U+to_float2(1,0),R,iChannel0), // east   left
           s = T(U-to_float2(0,1),R,iChannel0), // south  down
           w = T(U-to_float2(1,0),R,iChannel0), // west   right
          mu = 0.25f*(n+e+s+w); // average
   C = me;
   C.x -= 0.25f*(e.z-w.z); // change in pressure from left to right
   C.y -= 0.25f*(n.z-s.z); // change in pressure from top to bottom
   // divergence plus pressure exchange :
   C.z = mu.z // average pressure of neighborhood
        +0.25f*(s.y-n.y+w.x-e.x); // how much is the neighborhood pushing on me

/////////////////////
// adapted from : https://www.shadertoy.com/view/4tGfDW     
#define SPIN_PERMITIVITY 0.1f    
/**/C.w = 
    _mix(mu.w,C.w,SPIN_PERMITIVITY) // neighbors trade spin
    +       SPIN_PERMITIVITY*(// the curl puts spin into the cell
        (e.y - w.y - n.x + s.x) - C.w // difference between the curl and the spin
    );
/**/swi2S(C,x,y, swi2(C,x,y) + 
    _fabs(C.w)* // the spin of the cell 
    0.25f*to_float2(
        n.w-s.w,  // the baseball shooter force from top to bottom - so it shoots from left to right
        w.w-e.w)); // the baseball shooter force from the left to the right - so it shoots up or down
////////////////////// 
    
   
  // boundary conditions
   #ifdef VORTEX_SHEDDING_MODE
   if (length(U-to_float2(0.1f,0.5f)*R)<0.02f*R.x) C = _mix(C,to_float4(0,0,0,1),0.01f);
   if (U.x < 1.0f || iFrame < 1 || Reset)          C = to_float4(0.15f,0,0,0);
   #else
   if (length(U-to_float2(0.1f,0.5f)*R)<0.02f*R.x) C = _mix(C,to_float4(0.7f,0,0,1),0.01f);
   if (length(U-to_float2(0.9f,0.5f)*R)<0.02f*R.x) C = _mix(C,to_float4(-0.7f,0,0,1),0.01f);
   if (length(U-to_float2(0.5f,0.1f)*R)<0.02f*R.x) C = _mix(C,to_float4(0,0.7f,0,1),0.01f);
   if (length(U-to_float2(0.5f,0.9f)*R)<0.02f*R.x) C = _mix(C,to_float4(0,-0.7f,0,1),0.01f);
   if (iFrame < 1) C = to_float4_s(0);
   #endif

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0



//float2 R;
//__DEVICE__ float4 T ( float2 U ) {return texture(iChannel0,U/R);}
//__DEVICE__ float4 P ( float2 U ) {return texture(iChannel1,U/R);}
__KERNEL__ void NimitzSVorticityConfinementFuse__Buffer_D(float4 C, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0);  
  
   U+=0.5f;
   U = U-0.5f*swi2(A(U),x,y);
   U = U-0.5f*swi2(A(U),x,y);
   C = B(U);
   #ifdef VORTEX_SHEDDING_MODE
   if (length(U-to_float2(0.1f,0.5f)*R)<0.02f*R.x) C = to_float4(0,0,0,1.5f);
   if (U.x < 1.0f || iFrame < 1)                   C = to_float4_s(0);
   #else
   if (length(U-to_float2(0.1f,0.5f)*R)<0.02f*R.x) C.x = 1.0f;
   if (length(U-to_float2(0.9f,0.5f)*R)<0.02f*R.x) C.y = 1.0f;
   if (length(U-to_float2(0.5f,0.1f)*R)<0.02f*R.x) C.z = 1.0f;
   if (length(U-to_float2(0.5f,0.9f)*R)<0.02f*R.x) C.w = 1.0f;
   if (iFrame < 1) C = to_float4_s(0);
   #endif
   if (iFrame < 1 || Reset) C = to_float4_s(0);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void NimitzSVorticityConfinementFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
    U+=0.5f;
    float4 me = A(U),
            a = A(U+to_float2(1,0)),
            b = A(U-to_float2(1,0)),
            c = A(U+to_float2(0,1)),
            d = A(U-to_float2(0,1));
    float3 n = normalize( to_float3(a.z-b.z, c.z-d.z, 0.1f  ));
    float4 p = B(U);
    p = p*p*p;
    C = 1.3f*abs_f4(sin_f4(
        4.0f*swi4(me,z,w,z,w) + (
        p.w*to_float4(1,2,3,4)+
        p.z*to_float4(3,2,1,4)+
        p.y*to_float4(2,3,1,4)+
        p.x*to_float4(3,1,2,4))
    ));
   C *= 0.5f+0.5f*decube_f3(iChannel2,n);
  
   if (Invers) C = to_float4_s(1.0f) - C;
   
   if (ApplyColor)
   {
     C = (C + Color-0.5f) * (C.w-0.5f);
     C.w = Color.w;
   }

  SetFragmentShaderComputedColor(C);
}