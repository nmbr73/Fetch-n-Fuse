
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)
#define E(U) _tex2DVecN(iChannel4,(U).x/R.x,(U).y/R.y,15)

// How many stars
#define N 5.0f

__DEVICE__ float G (float w, float s) {
    return 0.15915494309f*_expf(-0.5f*w*w/s/s)/(s*s);
}
__DEVICE__ float building(float2 U, float2 R) {
    float2 v = _floor(U*N/(R*to_float2(0.64f,1)));
    U = mod_f2f2(U*N,R*to_float2(0.64f,1));
    float n = _floor(5.0f+8.0f*fract(0.521f*v.x+4123.341f*v.y));
    float2 u = U-to_float2(0.3f,0.5f)*R;
    if (length(u)<0.05f*R.y) return 1.0f;
    if (_sinf(n*_atan2f(u.y,u.x))>2.1f*length(u)/R.y) return 1.0f;

    return 0.0f;
}
__DEVICE__ bool cell (float2 u) {
    return u.x>=0.0f&&u.y>=0.0f&&u.x<1.0f&&u.y<1.0f;
}
__DEVICE__ float _12(float2 U, float2 R) {
    U = _floor(U);
    return U.x+U.y*R.x;
}
__DEVICE__ float2 _21(float i,float2 R) {
    return 0.5f+to_float2(mod_f(i,R.x),_floor(i/R.x));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3

__KERNEL__ void SeaStarsFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    U+=0.5f;
    float2 R=iResolution;float4 M=iMouse;float T=iTime;int I=iFrame;
    
    if (building(U,R)==0.0f) {Q = to_float4_s(0);  SetFragmentShaderComputedColor(Q); return;}

    Q = A(U);
    float4 d = D(U);
    float4 c = C(swi2(Q,x,y));
    float2 f = to_float2(0,-0.2f/R.y);
    
    for (float _x = -4.0f; _x <= 4.0f; _x +=1.0f) 
    for (float _y = -4.0f; _y <= 4.0f; _y +=1.0f)
    if (_x!=0.0f||_y!=0.0f) {
        float2 u = to_float2(_x,_y);
        float4 a = A(U+u);
        float4 c = C(swi2(Q,x,y)+u-0.015f);
        float2 r = swi2(a,x,y)-swi2(Q,x,y);
        float l = length(r);
        float L = length(u);
        f -= 0.01f*c.w*u/L;
        if ((l-L)<4.0f)  
          f += 1e-1*r/l*(l-L)/L;
    }
    
    swi2S(Q,z,w, _mix(swi2(Q,z,w),swi2(c,x,y),0.1f));
    //swi2(Q,z,w) += f;
    Q.z+=f.x;
    Q.w+=f.y;
    swi2S(Q,x,y, swi2(Q,x,y) + swi2(Q,z,w)*1.0f/_sqrtf(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w))));
    //if (length(swi2(Q,z,w))>2.0f) swi2(Q,z,w) = 2.0f*normalize(swi2(Q,z,w));
    
    if (Q.y<1.0f)     { Q.z*=0.0f; Q.w*=0.0f; } // swi2(Q,z,w) *= 0.0f;
    if (Q.x<1.0f)     { Q.z*=0.0f; Q.w*=0.0f; } //swi2(Q,z,w) *= 0.0f;
    if (R.y-Q.y<1.0f) { Q.z*=0.0f; Q.w*=0.0f; } //swi2(Q,z,w) *= 0.0f;
    if (R.x-Q.x<1.0f) { Q.z*=0.0f; Q.w*=0.0f; } //swi2(Q,z,w) *= 0.0f;

    if (M.z>0.0f)     swi2S(Q,z,w, swi2(Q,z,w) - 1e-2*(swi2(M,x,y)-swi2(Q,x,y))/(1.0f+length((swi2(M,x,y)-swi2(Q,x,y)))));

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
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void SeaStarsFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    U+=0.5f;
    float2 R=iResolution;float4 M=iMouse;float T=iTime;int I=iFrame;
    //Q = to_float4_s(-1);
    
    float _Q[4] = {-1.0f,-1.0f,-1.0f,-1.0f}; 

    int i = 0;
    for (int _x=-2;_x<=2;_x++)
    for (int _y=-2;_y<=2;_y++) {
       float4 b = B(U+to_float2(_x,_y));
       float _b[4] = {b.x,b.y,b.z,b.w};
       
       for (int k = 0; k < 4; k++) if (_b[k]>0.0f) {
           float2 u = _21(_b[k],R);
           float4 a = A(u);
           if (cell(swi2(a,x,y)-U))
               _Q[i] = _b[k], i+=1;
           if (i>3) break;
       }
       if (i>3) break;
    }
    
    
    float4 d = D(U);
    float4 a = A(_21(d.x,R));
    //if (cell(swi2(a,x,y)-U)&&i<4&&d.x!=Q.x&&d.x!=Q.y&&d.x!=Q.z&&d.x!=Q.w) _Q[i]= d.x;
    if (cell(swi2(a,x,y)-U)&&i<4&&d.x!=_Q[0]&&d.x!=_Q[1]&&d.x!=_Q[2]&&d.x!=_Q[3]) _Q[i]= d.x;

    Q = to_float4(_Q[0],_Q[1],_Q[2],_Q[3]);
    
    if (I<1) Q = to_float4(_12(U,R),0,0,0);
    
    SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3

__KERNEL__ void SeaStarsFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    U+=0.5f;
    float2 R=iResolution;float4 M=iMouse;float T=iTime;int I=iFrame;
    Q = to_float4_s(0);
    float w = 0.0f;
    
    for (float _x = -3.0f; _x<=3.0f; _x+=1.0f)
    for (float _y = -3.0f; _y<=3.0f; _y+=1.0f)
    {
        
        float4 b = B(U+to_float2(_x,_y));
        float _b[4] = {b.x,b.y,b.z,b.w};
        for (int k = 0; k < 4; k++) {
            if (_b[k]>0.0f) {
                float2 u = _21(_b[k],R);
                float4 a = A(u);
                float2 v = swi2(a,x,y)-U;
                float e = G(length(v),2.0f);
                w += e;
                swi3S(Q,x,y,z, swi3(Q,x,y,z) + to_float3_aw(swi2(a,z,w),u.x)*e);
                Q.w += G(length(v),1.2f);
            } else break;
        }
    }
    if (w>0.0f) {Q.x/=w; Q.y/=w;} // swi2(Q,x,y) /= w;

//Q=B(U);

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


__DEVICE__ void XY (float2 U, inout float4 *Q, float4 q, float2 R, __TEXTURE2D__ iChannel0) {
    if (length(U-swi2(A(_21(q.x,R)),x,y))<length(U-swi2(A(_21((*Q).x,R)),x,y))) (*Q).x = q.x;
}
__DEVICE__ void ZW (float2 U, inout float4 *Q, float4 q,float2 R, __TEXTURE2D__ iChannel0) {
    if (length(U-swi2(A(_21(q.y,R)),x,y))<length(U-swi2(A(_21((*Q).y,R)),x,y))) (*Q).y = q.y;
}

__KERNEL__ void SeaStarsFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    U+=0.5f;
    float2 R=iResolution;float4 M=iMouse;float T=iTime;int I=iFrame;
    Q = B(U);
    for (int _x=-1;_x<=1;_x++)
    for (int _y=-1;_y<=1;_y++) {
        XY(U,&Q,B(U+to_float2(_x,_y)),R,iChannel0);
    }
    
    if (I%12==0) 
        Q.y = _12(U,R);
    else
    {
        float k = _exp2f((float)(11-(I%12)));
        ZW(U,&Q,B(U+to_float2(0,k)),R,iChannel0);
        ZW(U,&Q,B(U+to_float2(k,0)),R,iChannel0);
        ZW(U,&Q,B(U-to_float2(0,k)),R,iChannel0);
        ZW(U,&Q,B(U-to_float2(k,0)),R,iChannel0);
    }
    XY(U,&Q,swi4(Q,y,x,z,w),R,iChannel0);
    
    if (I<1) Q = to_float4_s(_12(U,R));
    
  SetFragmentShaderComputedColor(Q);        
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "MPM dambreak" by wyatt. https://shadertoy.com/view/NdSczm
// 2022-01-30 22:14:48

// Fork of "Material Point Method" by wyatt. https://shadertoy.com/view/fssyDs
// 2022-01-26 19:42:56

__KERNEL__ void SeaStarsFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    U+=0.5f;
    float2 R=iResolution;float4 M=iMouse;float T=iTime;int I=iFrame;
    float4 c = C(U);
    Q = to_float4_s(1.4f)-1.3f*sin_f4(to_float4_s(c.w)-4.0f*c.z/R.x+to_float4(1,2,3,4));
    Q = _mix(0.15f*to_float4(0.5f,0.6f,1,1),Q,c.w);
 
    if(U.x<3.0f||U.y<6.0f||R.x-U.x<3.0f||R.y-U.y<3.0f) Q *= 0.0f;
    
    //Q = D(U)/R.x/R.x;
    
    SetFragmentShaderComputedColor(Q);    
}