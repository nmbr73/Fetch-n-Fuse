
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


vec2 R; float4 M; float T; int I;
#define Main void mainImage(out float4 Q, float2 U){UNIS
#define UNIS R=iResolution;M=iMouse;T=iTime;I=iFrame;
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

__DEVICE__ float G (float w, float s) {
    return 0.15915494309f*_expf(-0.5f*w*w/s/s)/(s*s);
}
__DEVICE__ float building(float2 U) {
    
    if (U.x<0.5f*R.x) return 1.0f;
    return 0.0f;

}
__DEVICE__ bool cell (float2 u) {
    return u.x>=0.&&u.y>=0.&&u.x<1.&&u.y<1.0f;
}
__DEVICE__ float _12(float2 U) {
    U = _floor(U);
    return U.x+U.y*R.x;
}
__DEVICE__ float2 _21(float i) {
    return 0.5f+to_float2(mod_f(i,R.x),_floor(i/R.x));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


Main
    
    if (building(U)==0.0f) {Q = to_float4(0); return;}

    Q = A(U);
    float4 d = D(U);
    float4 c = C(swi2(Q,x,y));
    float2 f = to_float2(0,-0.2f/R.y);
    
    for (float x = -3.0f; x <= 3.0f; x ++) 
    for (float y = -3.0f; y <= 3.0f; y ++)
    if (x!=0.||y!=0.0f) {
        float4 c = C(swi2(Q,x,y)+to_float2(x,y));
        f -= 0.01f*c.w*(c.w-1.0f)*to_float2(x,y)/(x*x+y*y);
    }
    
    if (length(f)>0.1f) f = 0.1f*normalize(f);
    swi2(Q,z,w) = _mix(swi2(Q,z,w),swi2(c,x,y),0.5f);
    swi2(Q,z,w) += f;
    swi2(Q,x,y) += 0.5f*f+swi2(Q,z,w)*inversesqrt(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w)));
    
    if (Q.y<1.0f) swi2(Q,z,w) *= 0.0f;
    if (Q.x<1.0f) swi2(Q,z,w) *= 0.0f;
    if (R.y-Q.y<1.0f) swi2(Q,z,w) *= 0.0f;
    if (R.x-Q.x<1.0f) swi2(Q,z,w) *= 0.0f;

    if (M.z>0.0f) swi2(Q,z,w) -= 1e-2*(swi2(M,x,y)-swi2(Q,x,y))/(1.0f+length((swi2(M,x,y)-swi2(Q,x,y))));

    if(I<1) {
    
        Q = to_float4(U,0,0);
        // if (length(U-to_float2_s(0.9f)*R)<0.02f*R.x) swi2(Q,z,w) = to_float2(-2.5f,-1.5f);
    }
    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3



Main
    Q = to_float4_aw(-1);
    int i = 0;
    for (int x=-2;x<=2;x++)
    for (int y=-2;y<=2;y++) {
       float4 b = B(U+to_float2(x,y));
       for (int k = 0; k < 4; k++) if (b[k]>0.0f) {
           float2 u = _21(b[k]);
           float4 a = A(u);
           if (cell(swi2(a,x,y)-U))
               Q[i++] = b[k];
           if (i>3) break;
       }
       if (i>3) break;
    }
    float4 d = D(U);
    float4 a = A(_21(d.x));
    if (cell(swi2(a,x,y)-U)&&i<4
        &&d.x!=Q.x&&d.x!=Q.y&&d.x!=Q.z&&d.x!=Q.w
    ) Q[i]= d.x;
    
    if (I<1) Q = to_float4(_12(U),0,0,0);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


Main
    Q = to_float4_aw(0);
    float w = 0.0f;
    for (float x = -3.0f; x<=3.0f; x++)
    for (float y = -3.0f; y<=3.0f; y++)
    {
        
        float4 b = B(U+to_float2(x,y));
        for (int k = 0; k < 4; k++) {
            if (b[k]>0.0f) {
                float2 u = _21(b[k]);
                float4 a = A(u);
                float2 v = swi2(a,x,y)-U;
                float e = G(length(v),0.8f);
                w += e;
                swi3(Q,x,y,z) += to_float3(swi2(a,z,w),u.x)*e;
                Q.w += G(length(v),1.2f);
            } else break;
        }
    }
    if (w>0.0f) swi2(Q,x,y) /= w;
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


void XY (float2 U, inout float4 Q, float4 q) {
    if (length(U-A(_21(q.x)).xy)<length(U-A(_21(Q.x)).xy)) Q.x = q.x;
}
__DEVICE__ void ZW (float2 U, inout float4 Q, float4 q) {
    if (length(U-A(_21(q.y)).xy)<length(U-A(_21(Q.y)).xy)) Q.y = q.y;
}
Main
    Q = B(U);
    for (int x=-1;x<=1;x++)
    for (int y=-1;y<=1;y++) {
        XY(U,Q,B(U+to_float2(x,y)));
    }
    
    if (I%12==0) 
        Q.y = _12(U);
    else
    {
        float k = _exp2f(float(11-(I%12)));
        ZW(U,Q,B(U+to_float2(0,k)));
        ZW(U,Q,B(U+to_float2(k,0)));
        ZW(U,Q,B(U-to_float2(0,k)));
        ZW(U,Q,B(U-to_float2(k,0)));
    }
    XY(U,Q,swi4(Q,y,x,z,w));
    
    if (I<1) Q = to_float4(_12(U));
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

Main
    float4 c = C(U);
    Q = 1.0f-_sinf(c.w-4.0f*c.z/R.x+to_float4(1,2,3,4));
    Q *= c.w;
    
    if(U.x<3.||U.y<6.||R.x-U.x<3.||R.y-U.y<3.0f) Q *= 0.0f;
    
    //Q = D(U)/R.x/R.x;
}