
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)
#define E(U) _tex2DVecN(iChannel4,(U).x/R.x,(U).y/R.y,15)

#define O to_float4(0.01f,0.2f,0.5f,0.01f)
#define I 20
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


__DEVICE__ void swap (inout float4 *Q, float2 U, float2 r,float2 R, __TEXTURE2D__ iChannel0) {
  float4 n = A(U+r);
    //if (length(U-swi2(n,x,y))<length(U-swi2(*Q,x,y))) *Q = n;
    if (length(U-swi2(n,x,y))<length(U-to_float2((*Q).x,(*Q).y))) *Q = n;
} 
__KERNEL__ void CommunicationAndGroupingFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
  
  CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_BUTTON0(Modus, 1, Icks, PunchIn, Zet, Weh, PunchOut);
  
   U+=0.5f; 
   
   Q = A(U);
   swap(&Q,U, to_float2(0,1),R,iChannel0);
   swap(&Q,U, to_float2(1,0),R,iChannel0);
   swap(&Q,U, to_float2(0,-1),R,iChannel0);
   swap(&Q,U, to_float2(-1,0),R,iChannel0);
   swap(&Q,U, to_float2(1,1),R,iChannel0);
   swap(&Q,U, to_float2(1,-1),R,iChannel0);
   swap(&Q,U, to_float2(-1,-1),R,iChannel0);
   swap(&Q,U, to_float2(-1,1),R,iChannel0);
   swap(&Q,U, to_float2(0,2),R,iChannel0);
   swap(&Q,U, to_float2(2,0),R,iChannel0);
   swap(&Q,U, to_float2(0,-2),R,iChannel0);
   swap(&Q,U, to_float2(-2,0),R,iChannel0);
    
    float2 u = _mix(swi2(Q,x,y),U,0.0f);
    float4
        n = D(u+to_float2(0,1)),
        e = D(u+to_float2(1,0)),
        s = D(u-to_float2(0,1)),
        w = D(u-to_float2(1,0));
    //swi2(Q,x,y) -= 0.5f*swi2(Q,z,w);
    Q.x -= 0.5f*Q.z;
    Q.y -= 0.5f*Q.w;

    float2 g = to_float2(e.w-w.w,n.w-s.w);
    //swi2(Q,z,w) = -g;
    Q.z=-g.x;
    Q.w=-g.y;
    
    if (length(swi2(Q,z,w))>1.0f) swi2S(Q,z,w, normalize(swi2(Q,z,w)));
    
    if (iFrame < 1||(iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)){
        float2 u =_floor(U/10.0f+0.5f)*10.0f;
        Q = to_float4(u.x,u.y,0,0);
    }
    
  if (Blend1>0.0f)
  {
    float4 tex = B(U);
    if (tex.w != 0.0f)    
    {
      tex = tex*2.0 - 1.0f;
      if ((int)Modus & 2)  Q.x = _mix(Q.x,tex.x,Blend1);
      //if ((int)Modus & 4) Q = to_float4(0.0,0.0f,-1.0f,-1.0f);
      if ((int)Modus & 8)  Q.z = _mix(Q.z,tex.z,Blend1);
      if ((int)Modus & 16) Q.w = _mix(Q.w,tex.x,Blend1);
      if ((int)Modus & 32) Q   = to_float4(u.x,u.y,1.0f,1.0f);
    }
    else
    {
      if ((int)Modus & 4) Q = to_float4(u.x,u.y,1.0f,1.0f);
    }
    
  } 

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0



__KERNEL__ void CommunicationAndGroupingFuse__Buffer_B(float4 Q, float2 U, float2 iResolution)
{
   U+=0.5f;
   Q = to_float4_s(0);
    
    for (int i = -I; i <= I; i++) {
        float2 u = U+to_float2(i,0);
        float4 a = A(u);
        Q += (exp_f4(-1.0f*O*(float)(i*i)))*smoothstep(1.5f,1.0f,length(u-swi2(a,x,y)));
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0



__KERNEL__ void CommunicationAndGroupingFuse__Buffer_C(float4 Q, float2 U, float2 iResolution)
{
   U+=0.5f;
   Q = to_float4_s(0);

    for (int i = -I; i <= I; i++) {
      Q += (exp_f4(-1.0f*O*(float)(i*i)))*A(U+to_float2(0,i));
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3



__KERNEL__ void CommunicationAndGroupingFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
   U+=0.5f;
   Q = C(U);

   Q.w = 0.1f*Q.x - Q.y;
   
   if (iMouse.z>0.0f) Q.w -= 100.0f*_expf(-0.05f*length(swi2(iMouse,x,y)-U));
   
   if (iFrame > 1)
   Q.w = _mix(Q.w,D(U).w,0.75f);

   //if (iFrame < 1) Q = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


__DEVICE__ float ln (float3 p, float3 a, float3 b) {return length(p-a-(b-a)*_fminf(dot(p-a,b-a),0.0f)/dot(b-a,b-a));}

__KERNEL__ void CommunicationAndGroupingFuse(float4 Q, float2 U, float2 iResolution)
{
  
   CONNECT_SLIDER1(Alpha,0.0f,1.0f,1.0f);
   U+=0.5f;
   float4 
        n = D(U+to_float2(0,1)),
        e = D(U+to_float2(1,0)),
        s = D(U-to_float2(0,1)),
        w = D(U-to_float2(1,0));
    float4 a = A(U);
    Q = C(U);
    Q = to_float4(0.7f,0.8f,0.9f,1);
    float3 no = normalize(to_float3(e.w-w.w,n.w-s.w,2));
    float3 re = reflect(normalize(to_float3_aw((U-0.5f*R)/R.y,1)),no);
    float light = ln(to_float3(2,2,2),to_float3_aw(U/R.y,0),to_float3_aw(U/R.y,0)+re);
    Q *= (_expf(-light)+0.4f*_expf(-0.3f*light))*(0.7f+0.5f*dot(re,normalize(to_float3_aw(U/R.y,0)-to_float3(2,2,2))));

    Q.w=Alpha;
  SetFragmentShaderComputedColor(Q);
}