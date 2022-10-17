
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
//#define Main void mainImage(out float4 Q, in float2 U)
#define box for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
#define dt 0.2f
#define _a 0.05f
#define _b 0.05f

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


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1
// Connect Buffer A 'Texture: Blending' to iChannel2


__KERNEL__ void CombustRebuildFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

    
    U+=0.5f;

    Q = A(U);
    float4 b = B(U);
    Q.w = b.w;
    float4 T = Q;
    box if(_fabs(x)!=_fabs(y))
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        float f = dt*0.25f*(a.w*(a.w-1.0f+a.z));
        swi2S(Q,x,y, swi2(Q,x,y) - f*swi2(u,x,y));
        Q.z  += 0.5f*0.25f*(a.z-T.z)-Q.w*f*dot(swi2(a,x,y)-swi2(T,x,y)+swi2(u,x,y),swi2(u,x,y)); 
    }
    float M = dt*Q.z*Q.w*b.x*b.y;
    Q.z += 10.0f*M;
    Q.y -= 0.1f/R.y;
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<50.0f)
        Q.x = 0.5f;
    if (iFrame < 1 || Reset)       Q = to_float4(0,0,0.1f,0.1f);
    if (U.x < 4.0f||R.x-U.x<4.0f)  Q.x*=0.0f,Q.y*=0.0f,Q.z*=0.0f;//swi3(Q,x,y,z) *= 0.0f;
    if (U.y < 4.0f||R.y-U.y<4.0f)  Q.x*=0.0f,Q.y*=0.0f,Q.z*=0.0f;//swi3(Q,x,y,z) *= 0.0f;
    Q = clamp(Q,to_float4(-0.5f,-0.5f,0,0),to_float4(0.5f,0.5f,2,Q.w));
    if (length(swi2(Q,x,y))>0.5f)  swi2S(Q,x,y, 0.5f*normalize(swi2(Q,x,y)));
    

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void CombustRebuildFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    U+=0.5f;
    
    Q = A(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
       float2 u = to_float2(x,y);
       float4 q = A(U+u);
       float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5f*u-a))/ab;
       float j = 0.5f+0.5f*_fmaxf(1.0f-5.0f*Q.w*q.w,0.0f);
       float k = 0.5f+0.5f*_fmaxf(1.0f-5.0f*Q.w*q.w,0.0f);
       float wa = 0.25f*Q.w*_fminf(i,j)/j;
       float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
       swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Q,x,y,z)*wa+swi3(q,x,y,z)*wb);
       dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f) dQ.x/=dQ.w, dQ.y/=dQ.w, dQ.z/=dQ.w; //  dQ.xyz/=dQ.w;
    Q = dQ;
    
    if (U.x < 4.0f||R.x-U.x<4.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    if (U.y < 4.0f||R.y-U.y<4.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void CombustRebuildFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    U+=0.5f;
    
    Q = A(U);
    float4 Qb = B(U);
    float4 dQ = to_float4_s(0);
    box if(_fabs(x)!=_fabs(y))
    {
       float2 u = to_float2(x,y);
       float4 q = A(U+u),
             qb = B(U+u);
       float2 a = swi2(Q,x,y),
             b = swi2(q,x,y)+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5f*u-a))/ab;
       float j = 0.5f;//+0.2f*_fmaxf(1.0f-5.0f*Q.w*q.w,0.0f);
       float k = 0.5f;//+0.2f*_fmaxf(1.0f-5.0f*Q.w*q.w,0.0f);
       float wa = 0.25f*Q.w*_fminf(i,j)/j;
       float wb = 0.25f*q.w*_fmaxf(k+i-1.0f,0.0f)/k;
       swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + swi3(Qb,x,y,z)*wa+swi3(qb,x,y,z)*wb);
       dQ.w += wa+wb;
        
    }
    if (dQ.w>0.0f) dQ.x/=dQ.w, dQ.y/=dQ.w, dQ.z/=dQ.w; //dQ.xyz/=dQ.w;
    Q = dQ;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Texture: Blending' to iChannel2


// Reaction
__KERNEL__ void CombustRebuildFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
            //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    U+=0.5f;
    
    Q = A(U);
    float4 b = B(U);
    Q.w = b.w;
    
    float M = dt*b.z*b.w*Q.x*Q.y;
    Q.x += -_a*M;
    Q.y += -_b*M;
    Q.z += M;
    Q.w += (1.0f-_a-_b)*M;
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    if (iFrame<1 || Reset) {
        float l = length(U-to_float2(0.5f,0.1f)*R);
        Q = to_float4(step(l,0.1f*R.y),
                      1,
                      0,0.5f);
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1


__KERNEL__ void CombustRebuildFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    U+=0.5f;
    
    float4 f = A(U), c = B(U);
    Q = c*_fminf(f.w,1.3f);
    Q = ((to_float4_s(0.5f)-0.5f*cos_f4(5.0f*c.x+c.y*to_float4(1.0f,2,3,4))))*atan_f4(swi4(f,w,w,w,w), to_float4_s(1.0f));
    Q += f.z*f.w*to_float4(1,0.5f,0,1);

  SetFragmentShaderComputedColor(Q);
}