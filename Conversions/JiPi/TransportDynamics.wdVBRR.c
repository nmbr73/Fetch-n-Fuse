
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1, (U).x/R.x,(U).y/R.y,15)
//#define Main void mainImage(out float4 Q, in float2 U)

__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float ln (float3 p, float3 a, float3 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0


// Apply forces
__DEVICE__ float2 F (float2 u, float2 fragCoord, float2 R, __TEXTURE2D__ iChannel0) {
  float4 a = A(swi2(fragCoord,x,y) + u);
  return 0.125f*_fmaxf(a.w-0.25f,0.0f)*(a.w-0.5f)*u/dot(u,u);
}


__KERNEL__ void TransportDynamicsFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER1(Blend1_Thr, 0.0f, 10.0f, 4.0f);
  CONNECT_SLIDER2(Blend2_Thr, 0.0f, 10.0f, 1.0f);
  
  U+=0.5f;
  Q = A(U);
  for (int _x = -1; _x <= 1; _x++)
  for (int _y = -1; _y <= 1; _y++)
  if (_x!=0||_y!=0)
     swi2S(Q,x,y, swi2(Q,x,y) - F(to_float2(_x,_y),U, R, iChannel0));
   
  Q.y -= 0.2f/R.y*(1.0f+Q.z);
  //if (length(swi2(Q,x,y))>2.0f) swi2(Q,x,y) = 2.0f*normalize(swi2(Q,x,y));
  if (iFrame < 1) {
    Q = to_float4(0,0,0,0);
      if (U.y<0.5f*R.y) Q.w = 1.0f;

      if (length(U-0.5f*R)<0.25f*R.y) swi2S(Q,z,w, to_float2(4.0f,1));
    }
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<20.0f)
        Q = to_float4(1,0,6,1);
      
  //Textureblending
  if (Blend1 > 0.0f)
  {
    float2 tuv = to_float2(U.x,U.y);
        
    //float4 tex = B(U); //Org
    float4 tex = B(tuv);

    if (tex.w > 0.0f)
    {
      Q.z = _mix(Q.z,tex.x*Blend1_Thr,Blend1);       
      //Q.w = 1.0;
      Q.w = _mix(Q.w,tex.x*Blend2_Thr,Blend1);
    }
  }   
  
  if(Reset) Q = to_float4(0,0,0,0);
  
  if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)  Q.x*=0.0f, Q.y*=0.0f;//
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Conservative advect y-direction
__KERNEL__ void TransportDynamicsFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  U+=0.5f;
  Q = to_float4_s(0);
  if (iFrame%2==0)
  for (float i = -1.0f; i <= 1.0f; i+=1.0f)
    {
      float2 u = to_float2(0,i);
      float4 a = 0.5f*A(U+u-to_float2(0,0.5f)),
             b = A(U+u),
             c = 0.5f*A(U+u+to_float2(0,0.5f));
      float w1 = 1.0f+c.y-a.y;
      if (w1>0.0f) {
          float w = clamp(u.y+0.5f+c.y,-0.5f,0.5f)-
                    clamp(u.y-0.5f+a.y,-0.5f,0.5f);
          swi3S(Q,x,y,z, swi3(Q,x,y,z) + b.w*w/w1*swi3(b,x,y,z));
          Q.w += b.w*w/w1;
        }
    } else 
  for (float i = -1.0f; i <= 1.0f; i+=1.0f)
    {
      float2 u = to_float2(i,0);
      float4 a = 0.5f*A(U+u-to_float2(0.5f,0)),
             b = A(U+u),
             c = 0.5f*A(U+u+to_float2(0.5f,0));
      float w1 = 1.0f+c.x-a.x;
      if (w1 > 0.0f) {
          float w = clamp(u.x+0.5f+c.x,-0.5f,0.5f)-
                    clamp(u.x-0.5f+a.x,-0.5f,0.5f);
          swi3S(Q,x,y,z, swi3(Q,x,y,z) + b.w*w/w1*swi3(b,x,y,z));
          Q.w += b.w*w/w1;
        }
    }

    if (Q.w > 0.0f)                                      Q.x/=Q.w,Q.y/=Q.w,Q.z/=Q.w; //swi3(Q,x,y,z) /= Q.w;
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)  Q.x*=0.0f, Q.y*=0.0f;       // swi2(Q,x,y)*=0.0f;
    
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// density-dependent diffusion
__DEVICE__ float4 X (float4 Q, float2 u, float2 fragCoord, float2 R, __TEXTURE2D__ iChannel0) {
  float4 a = A(swi2(fragCoord,x,y) + u);
    float f = Q.w-a.w;
    return _mix(Q,a,clamp(1.0f*f*f,0.0f,1.0f));
}

__KERNEL__ void TransportDynamicsFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    U+=0.5f;
    Q = A(U);
    Q = (
        0.125f*X(Q,to_float2(1,0),U,R,iChannel0)+
        0.125f*X(Q,to_float2(0,1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(-1,0),U,R,iChannel0)+
        0.125f*X(Q,to_float2(0,-1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(1,1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(1,-1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(-1,-1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(-1,1),U,R,iChannel0));
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)
        Q = to_float4(0.5f,0,6,1);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)  Q.x*=0.0f, Q.y*=0.0f;       //swi2(Q,x,y)*=0.0f;

    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void TransportDynamicsFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  CONNECT_SLIDER5(Alpha, 0.0f, 1.0f, 1.0f);
  CONNECT_SLIDER6(Par1, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER7(Par2, -10.0f, 10.0f, 1.0f);
  CONNECT_COLOR0(Color, 1.0f, 2.0f, 3.0f, 4.0f);
  
  U+=0.5f;
  
  float2 tuv = to_float2(U.x,U.y-100.0f);
  
  //float4 a = A(U); // Org
  float4 a = A(tuv);
  //Q = to_float4_s(1.1f)-_atan2f(1.5f*a.w,1.0f)*(0.8f+0.5f*sin_f4(0.6f+a.z+to_float4(1,2,3,4))); //Org
  Q = to_float4_s(1.1f)-_atan2f(1.5f*a.w,1.0f)*(0.8f+0.5f*sin_f4(0.6f+a.z+Color));
    //Q = swi4(a,w,w,w,w);
  Q.w = Alpha;

  SetFragmentShaderComputedColor(Q);    
}