
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//Texture lookups :
//#define A(U) texelFetch(iChannel0,to_int2(U),0)
#define A(U) texture(iChannel0, (make_float2(to_int2_cfloat(U))+0.5f)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

//#define Main void mainImage( out float4 Q, in float2 U )
#define N 7.0f
#define For for (float i = -(N); i<=(N); i+=1.0f)
#define S 2.0f
#define Gaussian(i) 0.3989422804f/S*_expf(-0.5f*(i)*(i)/S/S)
#define W 25.0f

// Distance to line
__DEVICE__ float2 ln (float2 p, float2 a, float2 b) {
  float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  return to_float2(length(p-a-(b-a)*i),i);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


__DEVICE__ float4 X (in float4 Q, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0) {
    float4 n = A(U+r);
    float pppppppppppppppppppppppp;
    if      (ln(U,swi2(n,x,y),swi2(n,z,w)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q = n;
    else if (ln(U,swi2(Q,x,y),swi2(n,z,w)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) swi2S(Q,z,w, swi2(n,z,w));
    
    return Q;
}
__DEVICE__ float4 Xr (in float4 Q, float2 U, float r, float2 R, __TEXTURE2D__ iChannel0) {
     Q=X(Q,U,to_float2(r,0),R,iChannel0);
     Q=X(Q,U,to_float2(0,r),R,iChannel0);
     Q=X(Q,U,to_float2(0,-r),R,iChannel0);
     Q=X(Q,U,to_float2(-r,0),R,iChannel0);
     float zzzzzzzzzzzzzzzzzz;
     return Q;
}
__KERNEL__ void CommunicatorsFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{

  U+=0.5f;

  Q = to_float4_s(0);
  Q=Xr(Q,U,1.0f,R,iChannel0); 
  Q=Xr(Q,U,2.0f,R,iChannel0); 
  Q=Xr(Q,U,3.0f,R,iChannel0); 
    
    swi2S(Q,x,y, swi2(Q,x,y) + swi2(B(swi2(Q,x,y)),x,y));
    swi2S(Q,z,w, swi2(Q,z,w) + swi2(B(swi2(Q,z,w)),x,y));
    
    if (length(U-swi2(Q,z,w))<length(U-swi2(Q,x,y))) {
        float2 u = swi2(Q,x,y);
        swi2S(Q,x,y, swi2(Q,z,w));
        swi2S(Q,z,w, u);
    }
float AAAAAAAAAAAAAAAAA;    
    if (Q.x< 1.0f) swi2S(Q,x,y, swi2(Q,z,w));
    if (Q.z< 1.0f) swi2S(Q,z,w, swi2(Q,x,y));
    
    if (iMouse.z>0.0f) {
        float4 n = swi4(iMouse,x,y,x,y);
        if (ln(U,swi2(n,x,y),swi2(n,z,w)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) Q = n;
        if (ln(U,swi2(Q,x,y),swi2(n,x,y)).x<ln(U,swi2(Q,x,y),swi2(Q,z,w)).x) swi2S(Q,z,w, swi2(n,x,y));
    }
   
    if (iFrame < 1) {
      if (length(U-0.5f*R)<55.0f)
      Q = swi4(_floor(U/10.0f+0.5f),x,y,x,y)*10.0f;
    }
    if (length(swi2(Q,x,y)-swi2(Q,z,w)) > W) swi2S(Q,z,w, swi2(Q,x,y));
    
    if (iFrame%40==0) if (length(U-0.5f*R+7.0f*_sinf((float)(iFrame)))<length(U-swi2(Q,x,y))) swi2S(Q,x,y, 0.5f*R); 

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void CommunicatorsFuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{

  U+=0.5f;
float BBBBBBBBBBBBBBBBBB;
  float4 a = A(U), b = B(U);
  float2 u = swi2(a,x,y)-swi2(a,z,w);
  float r = length(u);
  float2 l = ln(U,swi2(a,x,y),swi2(a,z,w));
  
  float2 tmp=   250.0f*u/r/r*smoothstep(3.0f,1.0f,l.x)*_expf(-5.0f*l.y);
  
  if (r>1.0f) Q.x=tmp.x,Q.y=tmp.y;//swi2S(Q,x,y,  250.0f*u/r/r*smoothstep(3.0f,1.0f,l.x)*_expf(-5.0f*l.y));
  else        Q.x=0.0f,Q.y=0.0f;//swi2S(Q,x,y, to_float2_s(0));
    
  swi2S(Q,z,w, to_float2_s(1));
  Q *= smoothstep(1.0f,2.0f,length(U-swi2(a,x,y)));
  Q *= smoothstep(0.0f,1.0f,ln(U,swi2(a,x,y),swi2(a,z,w)).x);
  swi2S(Q,z,w, _mix(swi2(Q,z,w),swi2(b,z,w),0.7f));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void CommunicatorsFuse__Buffer_C(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
float CCCCCCCCCCCCCCCCCCCC;  
    Q = to_float4_s(0);
    For Q += Gaussian(i) * A(U+to_float2(0,i));
    
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void CommunicatorsFuse__Buffer_D(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
    
    float DDDDDDDDDDDDDDDDDDDD;
  
    Q = to_float4_s(0);
    For Q += Gaussian(i) * A(U+to_float2(i,0));
    
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel2


__KERNEL__ void CommunicatorsFuse(float4 Q, float2 U, float2 iResolution)
{

  U+=0.5f;

  Q = B(U).z * swi4(C(U+2.0f),z,z,z,z);

  SetFragmentShaderComputedColor(Q);
}