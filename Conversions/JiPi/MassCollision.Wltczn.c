
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//#define A(U) texelFetch(iChannel0,to_int2(U),0)
//#define B(U) texelFetch(iChannel1,to_int2(U),0)
//#define C(U) texelFetch(iChannel2,to_int2(U),0)
//#define D(U) texelFetch(iChannel3,to_int2(U),0)

#define A(U) texture(iChannel0,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define B(U) texture(iChannel1,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define C(U) texture(iChannel2,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define D(U) texture(iChannel3,(make_float2(to_int2_cfloat(U))+0.5f)/R)

//#define Main void mainImage(out float4 Q, in float2 U)
__DEVICE__ bool cell (float2 u) {
    return u.x>=0.0f&&u.y>=0.0f&&u.x<1.0f&&u.y<1.0f;
}

#define pack(u) (_floor(u.x)+R.x*_floor(u.y))
#define unpack(i) to_float2(mod_f(_floor(i),R.x),_floor(floor(i)/R.x))

#define touch(u) ((length(u)-2.0f)/(length(u)+2.0f))


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
          swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          //Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.x+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,z,w, _mix(swi2(Q,z,w),  Par, Blend));
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


__KERNEL__ void MassCollisionFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;

    Q = A(U);
    float4 q = B(U);
    float4 c = C(swi2(Q,x,y)); //???
    float n = 0.0f;
    float2 f = to_float2_s(0);
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++)
    {
        float2 u = to_float2(_x,_y);
        float4 c = C(swi2(Q,x,y)+u);
        float _c[4] = {c.x,c.y,c.z,c.w};
        for (int k = 0; k < 4; k++){
            float index = _c[k];
            if (index > 0.0f) {
                float4 a = A(unpack(index));
                float2 u = swi2(a,x,y)-swi2(Q,x,y);
                if (length(u)>0.0f) {
                    float m = touch(u);
                    u = normalize(u);
                    f += 0.25f*m*u;
                    n ++;
                }
            }
        }
    }
    Q.w -= 0.3f/R.y*sign_f(U.y-0.5f*R.y);
    Q.z -= 0.3f/R.x*sign_f(U.x-0.5f*R.x);
    if (n>0.0f)      Q.z+=0.5f*f.x,Q.w+=0.5f*f.y;//swi2(Q,z,w) += 0.5f*f;
    Q.x+=Q.z+f.x, Q.y+=Q.w+f.y; //swi2(Q,x,y) += swi2(Q,z,w)+f;
    
    if (length(swi2(Q,z,w))>1.0f)  swi2S(Q,z,w, normalize(swi2(Q,z,w)));
    
    if (Q.x < 2.0f)     {Q.x = 2.0f; Q.z*=-1.0f;}
    if (R.x-Q.x < 2.0f) {Q.x = R.x-2.0f; Q.z*=-1.0f;}
    if (Q.y < 2.0f)     {Q.y = 2.0f; Q.w*=-1.0f;}
    if (R.y-Q.y < 2.0f) {Q.y = R.y-2.0f; Q.w*=-1.0f;}
    
    
    
    if (Blend1>0.0) Q = Blending(iChannel3, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    
    if (iFrame < 1 || Reset) {
        Q = to_float4(U.x,U.y,0,0);
    }
    
    if (iMouse.z>0.0f&&length(swi2(Q,x,y)-swi2(iMouse,x,y))<20.0f)
        Q.z = 1.0f;
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void MassCollisionFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;

    float4 c = C(U);
    float2 v = unpack(c.x);
    Q = (c.x>0.0f?1.0f:0.0f)*(to_float4_s(0.5f)+0.5f*sin_f4(2.0f+2.0f*sign_f(v.x-0.5f*R.x)+2.0f*sign_f(v.y-0.5f*R.y)*to_float4(1,2,3,4)));
    
    if(Textur)
      Q = (c.x>0.0f?1.0f:0.0f) * D(v);// (to_float4_s(0.5f)+0.5f*sin_f4(2.0f+2.0f*sign_f(v.x-0.5f*R.x)+2.0f*sign_f(v.y-0.5f*R.y)*D(U)));
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3

__KERNEL__ void MassCollisionFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Q = to_float4_s(0);
    int i = 0;
    float _Q[4] = {0.0f,0.0f,0.0f,0.0f};
    
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++)
    {
        float2 u = to_float2(_x,_y);
        float4 c = C(U+u);
        float _c[4] = {c.x,c.y,c.z,c.w};
        for (int k = 0; k < 4; k++){
            float index = _c[k];
            if (index>0.0f&&i<4) {
                float4 a = A(unpack(index));
                if (cell(swi2(a,x,y)-_floor(U)))
                    _Q[i++] = index;
            }
        }
    }
    
    Q = to_float4(_Q[0],_Q[1],_Q[2],_Q[3]);
    
    if (i==0) {
        float4 d = D(U);
        float4 a = A(swi2(d,x,y));
        if (cell(swi2(a,x,y)-_floor(U)))
            Q.x = pack(swi2(d,x,y));
    }
    if (iFrame < 1 || Reset) Q = to_float4(pack(U),0,0,0);
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


#define N 12
__DEVICE__ void X (inout float4 *Q, inout float *r, float2 U, float4 b, float2 R, __TEXTURE2D__ iChannel0) {
  
    float4 a = A(swi2(b,x,y));
    
    if (length(swi2(a,x,y)-U)<*r) {
      *r = length(swi2(a,x,y)-U);
      //swi2(Q,x,y) = swi2(b,x,y);
      (*Q).x = b.x;
      (*Q).y = b.y;
     
    }

}

__DEVICE__ void Z (inout float4 *Q, inout float *r, float2 U, float4 b, float2 R, __TEXTURE2D__ iChannel0) {
  
    float4 a = A(swi2(b,z,w));
    
    if (length(swi2(a,x,y)-U)<*r) {
      *r = length(swi2(a,x,y)-U);
      //swi2(Q,z,w) = swi2(b,z,w);
      (*Q).z = b.z;
      (*Q).w = b.w;
    }

}

__KERNEL__ void MassCollisionFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Q = B(U);
    float r = length(swi2(A(swi2(Q,x,y)),x,y)-U);
    X(&Q,&r,U,B(U+to_float2(0,1)),R,iChannel0);
    X(&Q,&r,U,B(U+to_float2(1,0)),R,iChannel0);
    X(&Q,&r,U,B(U-to_float2(0,1)),R,iChannel0);
    X(&Q,&r,U,B(U-to_float2(1,0)),R,iChannel0);
    X(&Q,&r,U,B(U+to_float2(0,2)),R,iChannel0);
    X(&Q,&r,U,B(U+to_float2(2,0)),R,iChannel0);
    X(&Q,&r,U,B(U-to_float2(0,2)),R,iChannel0);
    X(&Q,&r,U,B(U-to_float2(2,0)),R,iChannel0);
    X(&Q,&r,U,swi4(Q,z,w,z,w),R,iChannel0);
    r = length(swi2(A(swi2(Q,z,w)),x,y)-U);
    if (iFrame%N==0) swi2(Q,z,w) = U;
    else {
      float k = _exp2f((float)(N-1-(iFrame%N)));
      Z(&Q,&r,U,B(U+to_float2(0,k)),R,iChannel0);
      Z(&Q,&r,U,B(U+to_float2(k,0)),R,iChannel0);
      Z(&Q,&r,U,B(U-to_float2(0,k)),R,iChannel0);
      Z(&Q,&r,U,B(U-to_float2(k,0)),R,iChannel0);
    }
    
  
    if (iFrame < 1 || Reset) Q = to_float4(U.x,U.y,U.x,U.y);
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2

__KERNEL__ void MassCollisionFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Q = B(U);
    
  SetFragmentShaderComputedColor(Q);
}