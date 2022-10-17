
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

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
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

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
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define Init if (iFrame < 1 || Reset) 
#define Border if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)
#define T(U) A((U)-dt*swi2(A(U),x,y))
#define NeighborhoodT float4 n = T(U+to_float2(0,1)), e = T(U+to_float2(1,0)), s = T(U-to_float2(0,1)), w = T(U-to_float2(1,0)), m = 0.25f*(n+e+s+w);
#define Neighborhood float4 n = A(U+to_float2(0,1)), e = A(U+to_float2(1,0)), s = A(U-to_float2(0,1)), w = A(U-to_float2(1,0)), m = 0.25f*(n+e+s+w);
#define grd 0.25f*to_float2(e.z-w.z,n.z-s.z)
#define div 0.25f*(e.x-w.x+n.y-s.y)
#define dt (iFrame<500?1.0f:0.1f)
#define I 4
//Dave H :
__DEVICE__ float3 hash33(float3 p3)
{
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
  return fract_f3((swi3(p3,x,x,y) + swi3(p3,y,x,x))*swi3(p3,z,y,x));
}
__DEVICE__ float pie (float2 p, float2 a, float2 b) {
  float2 m = 0.5f*(a+b); // midpoint
  if (length(a-b)<1e-3) return 1e3; // ignore self
  return _fabs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1


__KERNEL__ void JfaCausticFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
float AAAAAAAAAAAAAAA;    
    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    U+=0.5f;
  // Fluid:
  if (iFrame%2==0) {
    Q = T(U);
    NeighborhoodT;
    swi2S(Q,x,y, swi2(Q,x,y) - dt*grd);
    Init Q = to_float4_s(0);
    Border Q.x*=00.0f,Q.y*=00.0f;//swi2(Q,x,y) *= 0.0f;
  } else {
    Q = A(U);
    Neighborhood;
    Q.z -= dt*div; 
  }
  float2 mo = 0.5f*R;
  if (iMouse.z>0.0f)   mo = swi2(iMouse,x,y);
  
  float2 tmp =  0.5f*_sinf(dt*2.0f*iTime)*to_float2(_sinf(dt*iTime),0);
  Q = _mix(Q,to_float4(tmp.x,tmp.y, Q.z,1.0f ), _expf(-0.2f*length(U-mo)));
  
  // Blending  
  if (Blend>0.0) Q = Blending(iChannel1, U/R, Q, Blend, Par, to_float2(BlendMul,BlendOff), Modus, U, R);            
            
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// Translate Coordinate by gradient of pressure
// Strength of translation determined by wavelength
__KERNEL__ void JfaCausticFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    U+=0.5f;
float BBBBBBBBBBBBBBBBBBBBBB;        
    if (iFrame%I==0) {
        Neighborhood;
        swi3S(Q,x,y,z, hash33(to_float3_aw(U,iFrame)));
        swi2S(Q,x,y, U+swi2(Q,x,y)*2.0f-1.0f);
        swi2S(Q,x,y, swi2(Q,x,y) + 3e3*grd*(1.0f+Q.z));
    } else Q = B(U);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


// Hierarchical Sort
__DEVICE__ void X (inout float4 *Q, inout float4 *r, float2 U, float2 u, float2 R, int iFrame, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2) {
        float4 b,c;float2 i; float l;
        if (iFrame%I==0) {
            i = U+u; 
            b = B(i);
            l = length(swi2(b,x,y)-U);
            if (l<(*r).x) {
                (*Q).x=i.x;(*Q).y=i.y;//swi2(Q,x,y) = i;
                (*r).x = l;
            } else if (l<(*r).y) {
                (*Q).z=i.x;(*Q).w=i.y;//swi2(Q,z,w) = i;
                (*r).y = l;
            }
        } else {
            c = C(U+u);
            b = B(swi2(c,x,y));
            float4 bb = B(swi2(*Q,x,y));
            i = swi2(c,x,y);
            l = length(swi2(b,x,y)-U);
            if (l<(*r).x) {
                (*Q).x=i.x;(*Q).y=i.y;//swi2(Q,x,y) = i;
                (*r).x = l;
            } else if (l<(*r).y) {
                (*Q).z=i.x;(*Q).w=i.y;//swi2(Q,z,w) = i;
                (*r).y = l;
            }
            b = B(swi2(c,z,w));
            bb = B(swi2(*Q,x,y));
            i = swi2(c,z,w);
            l = length(swi2(b,x,y)-U);
            if (l<(*r).x) {
                (*Q).x=i.x;(*Q).y=i.y;//swi2(Q,x,y) = i;
                (*r).x = l;
            } else if (l<(*r).y) {
                (*Q).z=i.x;(*Q).w=i.y;//swi2(Q,z,w) = i;
                (*r).y = l;
            }
        }
}


__KERNEL__ void JfaCausticFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
float CCCCCCCCCCCCCCCCCCCCCC;    
    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = to_float4_s(1e9);
    float4 r = to_float4_s(1e9);
    if (iFrame%I>0) {
      Q = C(U);
        r = to_float4(
              length(U-swi2(B(swi2(Q,x,y)),x,y)),
              length(U-swi2(B(swi2(Q,z,w)),x,y)),
              0,0 );
    }
    float k = _exp2f((float)(I-(iFrame%I)));
    X(&Q,&r,U,to_float2(0,k),R,iFrame,iChannel1,iChannel2);
    X(&Q,&r,U,to_float2(k,0),R,iFrame,iChannel1,iChannel2);
    X(&Q,&r,U,to_float2(0,-k),R,iFrame,iChannel1,iChannel2);
    X(&Q,&r,U,to_float2(-k,0),R,iFrame,iChannel1,iChannel2);
    
    X(&Q,&r,U,to_float2(k,k),R,iFrame,iChannel1,iChannel2);
    X(&Q,&r,U,to_float2(-k,k),R,iFrame,iChannel1,iChannel2);
    X(&Q,&r,U,to_float2(k,-k),R,iFrame,iChannel1,iChannel2);
    X(&Q,&r,U,to_float2(-k,-k),R,iFrame,iChannel1,iChannel2);
    
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// Draw Photons to the screen
__DEVICE__ float4 P (float2 U, float3 p) {
  return 0.0033f/(1.0f+dot(U-swi2(p,x,y),U-swi2(p,x,y)))*_fmaxf(cos_f4(p.z*6.2f+to_float4(1,2,3,4)),to_float4_s(0.0f));
}

__KERNEL__ void JfaCausticFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
float DDDDDDDDDDDDDDDDDDDDD;    
    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Q = D(U);
    if (iFrame%I==0) Q *= 0.9f;
    for (int x = -3; x <=3; x++) {
        for (int y = -3; y<=3; y++) {
            float2 u = U+to_float2(x,y);
            float4 c = C(u);
            float4 b = B(swi2(c,x,y));
            Q += P(U,swi3(b,x,y,z));
            b = B(swi2(c,z,w));
            Q += P(U,swi3(b,x,y,z));
            b = B(u);
            Q += P(U,swi3(b,x,y,z));
        }
    }
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void JfaCausticFuse(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
float IIIIIIIIIIIIIIIIIIIIIII;    
    //Blending
    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(BlendMul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par, 0.0f, 0.0f);
    
    CONNECT_COLOR0(Color1, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.0f);

    
    U+=0.5f;
//    if (iFrame%I<I-1) {   SetFragmentShaderComputedColor(Q); return;} //discard;
    Q = A(U);
    Q *= sqrt_f4(Q);
    
    Q = to_float4_aw(swi3(Q,x,y,z)+swi3(Color1,x,y,z)-0.5f, Color1.w);

  SetFragmentShaderComputedColor(Q);    
}