
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)
#define E(U) _tex2DVecN(iChannel4,(U).x/R.x,(U).y/R.y,15)

#define pi 3.14159265359

__DEVICE__ float building(float2 U,float2 R, __TEXTURE2D__ iChannel4) {
    
    
    float tex = E(U).w;
    if (tex == 0.0f) return 0.0f;
    return 1.0f;
    
    #ifdef ORG
    if (U.y<0.9f*R.y&&_fabs(U.x-0.3f*R.x)<25.0f)  // Left Rectangle
        return 1.0f;
    
    if (U.x>0.5f*R.x&&U.y<0.8f*R.y                // Right Waterarea 
        &&mod_f(U.x+U.y,6.0f)<3.0f)
        return 1.0f;
    return 0.0f;
    #endif

}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void WaterHammerFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3, sampler2D iChannel4)
{
  
    U+=0.5f;
    float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;
    
    if (building(U,R, iChannel4)==0.0f) {Q = to_float4_s(0);  SetFragmentShaderComputedColor(Q); return;}

    Q = A(U);
    float4 d = D(U);
    float4 c = C(swi2(Q,x,y));
    float2 f = to_float2(0,-0.5f/R.y);
    
    for (float _x = -3.0f; _x <= 3.0f; _x +=1.0f) 
    for (float _y = -3.0f; _y <= 3.0f; _y +=1.0f)
    if (_x!=0.0f||_y!=0.0f) {
        float4 c = C(swi2(Q,x,y)+to_float2(_x,_y));
        float4 b = B(swi2(Q,x,y)+to_float2(_x,_y));
        float4 a = A(swi2(b,x,y));
        
        if (U.x<0.6f*R.x) {
            float2 u = abs_f2(swi2(Q,x,y)+to_float2(_x,_y)-swi2(a,x,y)); // Kraft auf der linken Seite
            if (u.x>0.5f||u.y>0.5f)continue;
            float2 r = swi2(a,x,y)-swi2(Q,x,y);
            float l = length(r);
            if (l<1.0f||l>6.0f) continue;
            float L = length(U-swi2(b,x,y));
            if ((l-L)<0.08f*L) 
                f += 3e-1*r*(l-L)/l/L/l;
        }
        else {
            f -= 0.005f*c.w*(c.w-2.0f)*to_float2(_x,_y)/(_x*_x+_y*_y); // Org: 0.05 Kraft auf der rechten Seite
        } 
    }

    //swi2(Q,z,w) = swi2(c,x,y);
    Q.z=c.x;Q.w=c.y;
    //swi2(Q,z,w) += f;
    Q.z+=f.x;
    Q.w+=f.y;
    swi2S(Q,x,y, swi2(Q,x,y) + 0.5f*f+swi2(Q,z,w)*1.0f/_sqrtf(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w))));

    
    if (Q.y<1.0f)     Q.y=1.0f, Q.w *= -1.0f;
    if (Q.x<1.0f)     Q.x=1.0f, Q.z *= -1.0f;
    if (R.y-Q.y<1.0f) Q.y=R.y-1.0f, Q.w *= -1.0f;
    if (R.x-Q.x<1.0f) Q.x=R.x-1.0f, Q.z *= -1.0f;

    if (M.z>0.0f&&U.x>0.5f*R.x) swi2S(Q,z,w, swi2(Q,z,w) - 3e-2*(swi2(M,x,y)-swi2(Q,x,y))/(1.0f+length((swi2(M,x,y)-swi2(Q,x,y)))));

    if(I<1||(U.x<0.5f*R.x&&U.y<2.0f)) {
        Q = to_float4(U.x,U.y,0,0);
        if (U.x>0.5f*R.x) Q.w = -0.2f; //Anfangsbeschleunigung -> rechte Bildschirmseite
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

__KERNEL__ void WaterHammerFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3, sampler2D iChannel4)
{
  
    U+=0.5f;
    float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;
    Q = B(U);
    for (int x=-1;x<=1;x++)
    for (int y=-1;y<=1;y++) {
        XY(U,&Q,B(U+to_float2(x,y)),R,iChannel0);
    }
    
    if (I%12==0) 
        //swi2(Q,z,w) = U;
        Q.z=U.x,Q.w=U.y;
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


__KERNEL__ void WaterHammerFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3, sampler2D iChannel4)
{
  
    U+=0.5f;
    float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;
    Q = to_float4_s(0);
    float4 w = to_float4_s(0);
    for (float _x = -3.0f; _x<=3.0f; _x+=1.0f)
    for (float _y = -3.0f; _y<=3.0f; _y+=1.0f)
    {
        
        float4 b = B(U+to_float2(_x,_y));
        float4 a = A(swi2(b,x,y));
        float4 d = D(swi2(b,x,y));
        float2 v = swi2(a,x,y)-U;
        float4 e = 1.0f/(1.0f+to_float4_s(1)*(_x*_x+_y*_y));
        float2 u = abs_f2(U+to_float2(_x,_y)-swi2(a,x,y));
        if (u.x>0.5f||u.y>0.5f) continue;
        
        w += e;
        Q += to_float4(a.z,a.w,d.w,1)*e;
    }
    if (w.x>0.0f) Q.x /= w.x, Q.y /= w.x; //swi2(Q,x,y) /= w.x;
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


__KERNEL__ void WaterHammerFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3, sampler2D iChannel4)
{
  
   U+=0.5f;
   float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;
   float4 a = A(U);
   Q = D(U);
   //if (I<1) Q = 0.5f+0.5f*sin_f4(1.8f*3.1f*U.x/R.x+to_float4(1,2,3,4)),Q.w=0.0f;
   if (I<1) Q = E(U);
   
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
// 2022-01-23 20:24:27

__KERNEL__ void WaterHammerFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3, sampler2D iChannel4)
{
  
    U+=0.5f;
    float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;
    
    float4 b = B(U);
    float4 a = A(swi2(b,x,y));
    float4 d = D(swi2(b,x,y));
    float4 c = C(U);
    //Q = _mix(to_float4_s(0.8f),d,smoothstep(2.0f,1.0f,length(U-swi2(a,x,y))));
    Q = _mix(to_float4_s(0.0f),d,smoothstep(2.0f,1.0f,length(U-swi2(a,x,y))));

  SetFragmentShaderComputedColor(Q);         
}