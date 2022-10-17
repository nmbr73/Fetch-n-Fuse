
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define A(U) texture(iChannel0,(U)/iResolution)
#define B(U) texture(iChannel1,(U)/iResolution)
#define C(U) texture(iChannel2,(U)/iResolution)
#define D(U) texture(iChannel3,(U)/iResolution)

__DEVICE__ float erf(in float x) {
    //x *= std;
    //return sign(x) * _sqrtf(1.0f - _expf(-1.239192f * x * x));
    return sign_f(x) * _sqrtf(1.0f - _exp2f(-1.787776f * x * x)); // likely faster version by @spalmer
}
__DEVICE__ float erfstep (float a, float b, float x) {
    return 0.5f*(erf(b-x)-erf(a-x));
}
__DEVICE__ float G (float w, float s) {
    return 0.15915494309f*_expf(-0.5f*w*w/s/s)/(s*s);
}
__DEVICE__ float building(float2 U, float2 R) {
    
    if (U.x<0.5f*R.x) return 1.0f;
    return 0.0f;

}
__DEVICE__ bool cell (float2 u) {
    return u.x>=0.0f&&u.y>=0.0f&&u.x<1.0f&&u.y<1.0f;
}
__DEVICE__ float _12(float2 U, float2 R) {
    U = _floor(U);
    return U.x+U.y*R.x;
}
__DEVICE__ float2 _21(float i, float2 R) {
    return 0.5f+to_float2(mod_f(i,R.x),_floor(i/R.x));
}



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
          Q = _mix(Q,to_float4_f2f2(U,(swi2(tex,x,y)+MulOff.y)*MulOff.x), Blend);
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
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3
// Connect Buffer A 'Texture: Blending' to iChannel4

__KERNEL__ void MpmDambreakFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Start, 1);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    if (building(U,iResolution)==0.0f && Start ) {Q = to_float4_s(0); SetFragmentShaderComputedColor(Q); return;}

    Q = A(U);
    float4 d = D(U);
    float4 c = C(swi2(Q,x,y));
    float2 f = to_float2(0,-0.2f/iResolution.y);
    
    for (float x = -2.0f; x <= 2.0f; x ++) 
    for (float y = -2.0f; y <= 2.0f; y ++)
    if (x!=0.0f||y!=0.0f) {
        float4 cc = C(swi2(Q,x,y)+to_float2(x,y));
        f -= 0.05f*c.w*cc.w*(cc.w-1.0f)*to_float2(x,y)/(x*x+y*y);
    }
    
    if (length(f)>0.1f) f = 0.1f*normalize(f);
    swi2S(Q,z,w, _mix(swi2(Q,z,w),swi2(c,x,y),0.5f));
    //swi2(Q,z,w) += f;
    Q.z += f.x;
    Q.w += f.y;
    
    swi2S(Q,x,y, swi2(Q,x,y) + 0.5f*f+swi2(Q,z,w)* 1.0f/_sqrtf(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w))));
    
    if (Q.y<1.0f)               Q.z*=0.0f, Q.w*=0.0f;//swi2(Q,z,w) *= 0.0f;
    if (Q.x<1.0f)               Q.z*=0.0f, Q.w*=0.0f;//swi2(Q,z,w) *= 0.0f;
    if (iResolution.y-Q.y<1.0f) Q.z*=0.0f, Q.w*=0.0f;//swi2(Q,z,w) *= 0.0f;
    if (iResolution.x-Q.x<1.0f) Q.z*=0.0f, Q.w*=0.0f;//swi2(Q,z,w) *= 0.0f;

    if (iMouse.z>0.0f)  swi2S(Q,z,w, swi2(Q,z,w) - 1e-2*(swi2(iMouse,x,y)-swi2(Q,x,y))/(1.0f+length((swi2(iMouse,x,y)-swi2(Q,x,y)))));

    if(iFrame<1 || Reset) {
    
        Q = to_float4(U.x,U.y,0,0);
        // if (length(U-to_float2_s(0.9f)*R)<0.02f*R.x) swi2(Q,z,w) = to_float2(-2.5f,-1.5f);
    }
    
    
  if (Blend1>0.0) Q = Blending(iChannel4, U/iResolution, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, iResolution);  

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


union A2F
 {
   float4  F; //32bit float
   float  A[4];  //32bit unsigend integer
 };


__KERNEL__ void MpmDambreakFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    U+=0.5f;
    A2F _Q;
    _Q.F = to_float4_s(-1);

    int i = 0;
    for (int x=-2;x<=2;x++)
    for (int y=-2;y<=2;y++) {
       A2F b; 
       b.F = B(U+to_float2(x,y));
       for (int k = 0; k < 4; k++) if (b.A[k]>0.0f) {
           float2 u = _21(b.A[k], iResolution);
           float4 a = A(u);
           if (cell(swi2(a,x,y)-U))
               _Q.A[i++] = b.A[k];
           if (i>3) break;
       }
       if (i>3) break;
    }
    float4 d = D(U);

    float4 a = A(_21(d.x, iResolution));
    if (cell(swi2(a,x,y)-U) && i<4 && d.x!=_Q.F.x && d.x!=_Q.F.y && d.x!=_Q.F.z && d.x!=_Q.F.w) _Q.A[i]= d.x;
    
    Q = _Q.F;
    
    if (iFrame<1 || Reset) Q = to_float4(_12(U, iResolution),0,0,0);
    
    
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void MpmDambreakFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    U+=0.5f;

    Q = to_float4_s(0);
    float w = 0.0f;
    for (float x = -3.0f; x<=3.0f; x+=1.0f)
    for (float y = -3.0f; y<=3.0f; y+=1.0f)
    {
        
        A2F b; 
        b.F = B(U+to_float2(x,y));
        float s = 1.0f;
        float n = dot(to_float4(b.F.x>0.0f,b.F.y>0.0f,b.F.z>0.0f,b.F.w>0.0f),to_float4_s(1));
        for (int k = 0; k < 4; k++) {
            if (b.A[k]>0.0f) {
                float2 u = _21(b.A[k], iResolution);
                float4 a = A(u);
                float2 v = swi2(a,x,y)-U;
                float e = G(length(v),s);
                w += e;
                swi3S(Q,x,y,z, swi3(Q,x,y,z) + to_float3_aw(swi2(a,z,w),u.x)*e);
                Q.w += e;
            } else break;
        }
    }
    if (w>0.0f) Q.x /= w, Q.y /= w;// swi2(Q,x,y) /= w;
    
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3
// Connect Buffer D 'Texture: Blending' to iChannel4


__DEVICE__ void XY (float2 U, inout float4 *Q, float4 q, float2 iResolution, __TEXTURE2D__ iChannel0) {
    if (length(U-swi2(A(_21(q.x, iResolution)),x,y))<length(U-swi2(A(_21((*Q).x, iResolution)),x,y)))   (*Q).x = q.x;
}
__DEVICE__ void ZW (float2 U, inout float4 *Q, float4 q, float2 iResolution, __TEXTURE2D__ iChannel0) {
    if (length(U-swi2(A(_21(q.y, iResolution)),x,y))<length(U-swi2(A(_21((*Q).y, iResolution)),x,y)))   (*Q).y = q.y;
}



__KERNEL__ void MpmDambreakFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    //Blending
    CONNECT_SLIDER5(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend2Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(Blend2Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus2, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT3(Par2, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = B(U);
    for (int x=-1;x<=1;x++)
    for (int y=-1;y<=1;y++) {
        XY(U,&Q,B(U+to_float2(x,y)), iResolution, iChannel0);
    }
   
    if (iFrame%12==0) 
        Q.y = _12(U, iResolution);
    else
    {
        float k = _exp2f(float(11-(iFrame%12)));
        ZW(U,&Q,B(U+to_float2(0,k)), iResolution, iChannel0);
        ZW(U,&Q,B(U+to_float2(k,0)), iResolution, iChannel0);
        ZW(U,&Q,B(U-to_float2(0,k)), iResolution, iChannel0);
        ZW(U,&Q,B(U-to_float2(k,0)), iResolution, iChannel0);
    }
    XY(U,&Q,swi4(Q,y,x,z,w), iResolution, iChannel0);
    
    if (iFrame<1 || Reset) Q = to_float4_s(_12(U, iResolution));
    
    if (Blend2>0.0) Q = Blending(iChannel4, U/iResolution, Q, Blend2, Par2, to_float2(Blend2Mul,Blend2Off), Modus2, U, iResolution);  
    
    SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "Material Point Method" by wyatt. https://shadertoy.com/view/fssyDs
// 2022-01-26 19:42:56

__KERNEL__ void MpmDambreakFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 0.5f);
        
    U+=0.5f;
    float4 c = C(U);
    Q = to_float4_s(1.0f)-sin_f4(c.w-4.0f*c.z/iResolution.x+to_float4(1,2,3,4)+Color-0.5f);
    Q *= c.w;
    
    if(U.x<3.0f||U.y<6.0f||iResolution.x-U.x<3.0f||iResolution.y-U.y<3.0f) Q *= 0.0f;
    
    //Q = D(U)/iResolution.x/iResolution.x;
    
    SetFragmentShaderComputedColor(Q);
}