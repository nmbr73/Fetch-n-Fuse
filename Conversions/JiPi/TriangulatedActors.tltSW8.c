
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(make_float2(to_int2_cfloat(U))+0.5)/R)
#define B(U) texture(iChannel1,(make_float2(to_int2_cfloat(U))+0.5)/R)
#define C(U) texture(iChannel2,(make_float2(to_int2_cfloat(U))+0.5)/R)

#define D(U) texture(iChannel3,(U)/R)


#define Init if (iFrame < 1) 
#define init  (iFrame < 1) 
#define Border if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)
__DEVICE__ float sg (float2 p, float2 a, float2 b) {
  float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  return (length(p-a-(b-a)*i));
}
__DEVICE__ float mp (float2 p, float2 a, float2 b) {
  float2 m = 0.5f*(a+b); // midpoint
  if (length(a-b)<1e-3) return 1e3; // ignore self
  return _fabs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


// Information Storage
__KERNEL__ void TriangulatedActorsFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = A(U);
    if (iMouse.z>0.0f) {
        float2 r = (swi2(iMouse,x,y)-swi2(Q,x,y));
        swi2S(Q,z,w, swi2(Q,z,w) - r/dot(r,r));
    }
     if (iFrame > 10) 
        swi2S(Q,z,w, swi2(Q,z,w) - swi2(D(swi2(Q,x,y)),z,w));
    //Q.w -= 3e-3;
    if (length(swi2(Q,z,w))>1.0f) swi2S(Q,z,w, normalize(swi2(Q,z,w)));
    Q.x+=Q.z; Q.y+=Q.w; // swi2(Q,x,y) += swi2(Q,z,w);
    

    if (Q.x<5.0f)     {Q.x = 5.0f; Q.z *= -1.0f;}
    if (R.x-Q.x<5.0f) {Q.x = R.x-5.0f; Q.z *= -1.0f;}
    if (Q.y<5.0f)     {Q.y = 5.0f; Q.w *= -1.0f;}
    if (R.y-Q.y<5.0f) {Q.y = R.y-5.0f; Q.w *= -1.0f;Q.z*=0.0f;}
    
    if (init)
    {
      U = _floor(U/15.0f+0.5f)*15.0f;
      Q = to_float4(clamp(U.x,3.0f,R.x-3.0f),clamp(U.y,3.0f,R.y-3.0f),0,0);
    }
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


// Nearest Individual
__DEVICE__ void XB (inout float4 *Q, inout float4 *a, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    float4 b = B(U+r);
    float4 n = A(swi2(b,x,y));
    float ln = length(swi2(n,x,y)-U), la = length(swi2(*a,x,y)-U);
    if (ln<la) {
      swi2S(*Q,x,y, swi2(b,x,y));
      swi2S(*a,x,y, swi2(n,x,y));
    }
}
__DEVICE__ void XrB (inout float4 *Q, inout float4 *a, float2 U, float r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
     XB(Q,a,U,to_float2(r,0),R,iChannel0,iChannel1);
     XB(Q,a,U,to_float2(0,r),R,iChannel0,iChannel1);
     XB(Q,a,U,to_float2(0,-r),R,iChannel0,iChannel1);
     XB(Q,a,U,to_float2(-r,0),R,iChannel0,iChannel1);
}


__KERNEL__ void TriangulatedActorsFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Q = B(U);
    float4 a = A(swi2(Q,x,y));
    XrB(&Q,&a,U,1.0f,R,iChannel0,iChannel1);
    XrB(&Q,&a,U,2.0f,R,iChannel0,iChannel1);
    XrB(&Q,&a,U,3.0f,R,iChannel0,iChannel1);
    XrB(&Q,&a,U,4.0f,R,iChannel0,iChannel1);
    Init {
      swi2S(Q,x,y, swi2(A(U),x,y));
      swi2S(Q,z,w, to_float2_s(0.0f));
    }
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


// Nearest Normalized Bisector
__DEVICE__ void XC (inout float4 *Q, inout float4 *a, inout float4 *b, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2) {
    float4 Qn = C(U+r), qn = B(U+r), na = A(swi2(qn,x,y)), nb = A(swi2(Qn,x,y));
     float l = mp(U,swi2(*a,x,y),swi2(*b,x,y));
    
    if (mp(U,swi2(*a,x,y),swi2(na,x,y))<l) {
      *Q = qn;
      *b = na;
    }
    if (mp(U,swi2(*a,x,y),swi2(nb,x,y))<l) {
      *Q = Qn;
      *b = nb;
    }
}
__DEVICE__ void XrC (inout float4 *Q, inout float4 *a, inout float4 *b, float2 U, float r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2) {
     XC(Q,a,b,U,to_float2(r,0),R,iChannel0,iChannel1,iChannel2);
     XC(Q,a,b,U,to_float2(0,r),R,iChannel0,iChannel1,iChannel2);
     XC(Q,a,b,U,to_float2(0,-r),R,iChannel0,iChannel1,iChannel2);
     XC(Q,a,b,U,to_float2(-r,0),R,iChannel0,iChannel1,iChannel2);
}

__KERNEL__ void TriangulatedActorsFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = C(U);
    float4 q = B(U),
         a = A(swi2(q,x,y)),
         b = A(swi2(Q,x,y));
    XrC (&Q,&a,&b,U,1.0f,R,iChannel0,iChannel1,iChannel2);
    XrC (&Q,&a,&b,U,2.0f,R,iChannel0,iChannel1,iChannel2);
    XrC (&Q,&a,&b,U,3.0f,R,iChannel0,iChannel1,iChannel2);
    XrC (&Q,&a,&b,U,4.0f,R,iChannel0,iChannel1,iChannel2);
    
    float2 r = swi2(b,x,y) - swi2(a,x,y);
    if (length(r)>0.0f&&length(swi2(b,x,y))>0.0f) 
        swi2S(Q,z,w, -r/dot(r,r)+10.0f*r/dot(r,r)/length(r) 
        - 0.1f*_fabs(dot(swi2(b,z,w)-swi2(a,z,w),r))*(swi2(b,z,w)-swi2(a,z,w))*_expf(-0.05f*dot(r,r)));
    
    Init {
      Q = A(U);
      Q.z=0.0f;Q.w=0.0f;//swi2(Q,z,w) = to_float2(0);
    }
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2


// Sum the forces
__KERNEL__ void TriangulatedActorsFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = to_float4_s(0);
    for (int x = -3; x <= 3; x++)
    for (int y = -3; y <= 3; y++) {
      float2 r = to_float2(x,y);
      Q += C(U+r)*_expf(-0.5f*dot(r,r));
    }
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void TriangulatedActorsFuse(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    U+=0.5f;
    
    float4 bb = B(U), cc = C(U), dd = D(U),
          a = A(swi2(bb,x,y)), b = A(swi2(cc,x,y));
    float l = length(U-swi2(a,x,y)),
          j = smoothstep(5.0f,4.0f,l);
    Q = to_float4_s(j)*_fmaxf(to_float4_s(0.6f)+0.4f*sin_f4(0.3f+(0.1f*cc.x/R.x+cc.y+1.0f/(1.0f+10.0f*length(swi2(dd,z,w)))*to_float4(1,2,3,4))),to_float4_s(0.0f));
    
  SetFragmentShaderComputedColor(Q);    
}