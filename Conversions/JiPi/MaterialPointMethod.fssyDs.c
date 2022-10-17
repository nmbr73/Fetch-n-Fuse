
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define R iResolution
#define M iMouse
#define T iTime
#define I iFrame

//vec2 R; float4 M; float T; int I;
//#define Main void mainImage(out float4 Q, float2 U){UNIS
#define UNIS float2 R=iResolution;float4 M=iMouse;float T=iTime;int I=iFrame;

#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define pi 3.14159265359f

__DEVICE__ float building(float2 U, float2 R) {
    
    if (length(U-to_float2(0.8f,0.5f)*R)<0.15f*R.x) return 1.0f;
    
    if (length(U-to_float2(0.2f,0.5f)*R)<0.15f*R.x) return 1.0f;
    
    return 0.0f;

}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void MaterialPointMethodFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;
    
    if (building(U,R)==0.0f) {Q = to_float4_s(0);  SetFragmentShaderComputedColor(Q); return;}

    Q = A(U);
    float4 d = D(U);
    float4 c = C(swi2(Q,x,y));
    float2 f = to_float2(0,-1.0f/R.y);
    
    for (float _x = -3.0f; _x <= 3.0f; _x ++) 
    for (float _y = -3.0f; _y <= 3.0f; _y ++)
    if (_x!=0.0f || _y!=0.0f) {
        float4 c = C(swi2(Q,x,y)+to_float2(_x,_y));
        f -= 0.08f*c.w*(c.w-1.0f)*to_float2(_x,_y)/(_x*_x+_y*_y);
    }
    
    if (length(f)>1.0f) f = normalize(f);
    swi2S(Q,z,w, _mix(swi2(Q,z,w),swi2(c,x,y),0.5f));
    //swi2(Q,z,w) += f;
    Q.z += f.x;
    Q.w += f.y;
    swi2S(Q,x,y, swi2(Q,x,y) + 0.5f*f+swi2(Q,z,w) * 1.0f/_sqrtf(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w))));
    
    if (Q.y<1.0f)      Q.y=1.0f, Q.w *= -1.0f;
    if (Q.x<1.0f)      Q.x=1.0f, Q.z *= -1.0f;
    if (R.y-Q.y<1.0f)  Q.y=R.y-1.0f, Q.w *= -1.0f;
    if (R.x-Q.x<1.0f)  Q.x=R.x-1.0f, Q.z *= -1.0f;

    if (M.z>0.0f)      swi2S(Q,z,w, swi2(Q,z,w) - 3e-2*(swi2(M,x,y)-swi2(Q,x,y))/(1.0f+length((swi2(M,x,y)-swi2(Q,x,y)))));

    if(I<1) {
    
        Q = to_float4(U.x,U.y,0,0);
        // if (length(U-to_float2_s(0.9f)*R)<0.02f*R.x) swi2(Q,z,w) = to_float2(-2.5f,-1.5f);
    }

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__DEVICE__ void XY (float2 U, inout float4 *Q, float4 q, float2 R, __TEXTURE__ iChannel0) {
    if (length(U-swi2(A(swi2(q,x,y)),x,y))<length(U-swi2(A(swi2(*Q,x,y)),x,y))) (*Q).x = q.x, (*Q).y = q.y; // swi2(Q,x,y) = swi2(q,x,y);
}
__DEVICE__ void ZW (float2 U, inout float4 *Q, float4 q, float2 R, __TEXTURE__ iChannel0) {
    if (length(U-swi2(A(swi2(q,z,w)),x,y))<length(U-swi2(A(swi2(*Q,z,w)),x,y))) (*Q).z = q.z, (*Q).w = q.w; // swi2(Q,z,w) = swi2(q,z,w);
}

__KERNEL__ void MaterialPointMethodFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;
    Q = B(U);
    for (int _x=-1;_x<=1;_x++)
    for (int _y=-1;_y<=1;_y++) {
        XY(U,&Q,B(U+to_float2(_x,_y)),R,iChannel0);
    }
    
    if (I%12==0) 
        //swi2(Q,z,w) = U;
        Q.z = U.x, Q.w = U.y;
    else
    {
        float k = _exp2f((float)(11-(I%12)));
        ZW(U,&Q,B(U+to_float2(0,k)),R,iChannel0);
        ZW(U,&Q,B(U+to_float2(k,0)),R,iChannel0);
        ZW(U,&Q,B(U-to_float2(0,k)),R,iChannel0);
        ZW(U,&Q,B(U-to_float2(k,0)),R,iChannel0);
    }
    XY(U,&Q,swi4(Q,z,w,x,y),R,iChannel0);
    
    if (I<1) Q = to_float4_f2f2(U,U);

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3

__KERNEL__ void MaterialPointMethodFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;
    Q = to_float4_s(0);
    float4 w = to_float4_s(0);
    for (float _x = -3.0f; _x<=3.0f; _x++)
    for (float _y = -3.0f; _y<=3.0f; _y++)
    {
        
        float4 b = B(U+to_float2(_x,_y));
        float4 a = A(swi2(b,x,y));
        float4 d = D(swi2(b,x,y));
        float2 v = swi2(a,x,y)-U;
        float4 e = 1.0f/(1.0f+to_float4(8,8,0,4)*(_x*_x+_y*_y));
        float2 u = abs_f2(U+to_float2(_x,_y)-swi2(a,x,y));
        if (u.x>0.5f||u.y>0.5f) continue;
        
        w += e;
        Q += to_float4(a.z,a.w,d.w,1)*e;
    }
    if (w.x>0.0f) Q.x /= w.x, Q.y/=w.x;//swi2(Q,x,y) /= w.x;
    if (w.y>0.0f) Q.z /= w.y;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void MaterialPointMethodFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
   U+=0.5f;
   float4 a = A(U);
   Q = D(U);
   if (I<1)   Q = 0.5f+sin_f4(2.0f*3.1f*U.x/R.x+to_float4(1,2,3,4)), Q.w=0.0f;
   
  SetFragmentShaderComputedColor(Q);   
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void MaterialPointMethodFuse(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;
    
    float4 b = B(U);
    float4 a = A(swi2(b,x,y));
    float4 d = D(swi2(b,x,y));
    float4 c = C(U);
    Q = 2.0f*d*_expf(-1.0f*length(U-swi2(a,x,y)));
    Q -= 0.2f*swi4(c,w,w,w,w);
    
  SetFragmentShaderComputedColor(Q);    
}