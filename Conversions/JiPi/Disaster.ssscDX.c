
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

// Info
// In building() Einen Wurfball definieren und dann in A und B Zeitpunkt festlegen für die Beschleunigung in X und Y Richtung 


//vec2 R; float4 M; float T; int I;

#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)


__DEVICE__ float building(float2 U, float2 R, __TEXTURE__ iChannel2) {
    
    if (U.y<10.0f)                     return 0.0f;

#define TEXT
    #ifdef TEXT
    float ratio = 1.0f;//R.x/R.y;
    
    if (length(U-to_float2_s(0.9f)*R)<0.02f*R.x) return 1.0f; // Wurfball oben rechts (0.9)
    
    if (length(U-to_float2(0.1f, 0.9f)*R)<0.02f*R.x) return 1.0f; // Zweiter Wurfball oben links ( 0.1,0.9 )
      
    if (length(U-to_float2(0.5f, 0.95f)*R)<0.03f*R.x) return 1.0f; // Dritter Wurfball Test außerhalb Sicht
    
    
    float tex = _tex2DVecN(iChannel2, U.x/R.x*ratio,U.y/R.y,15).w;
    //if (tex > 0.90f) return 1.0f;
       
    if (tex == 0.0f) return 0.0f;
    return (1.0f);             
    #endif
    
    //#define ORG
    #ifdef ORG
    if (length(U-to_float2_s(0.9f)*R)<0.02f*R.x) return 1.0f;
    float r = U.x/R.x*30.0f-15.0f;
    float x = round(U.x/R.x*30.0f)-15.0f;
    
    float y = _expf(-x*x/40.0f)*(1.5f+_sinf(10.0f*x));
    if (_fabs(r-x)>0.3f+0.5f*_sinf(x)) return 0.0f;
    if (U.y/R.y < 0.4f*y)              return 1.0f;
    else                               return 0.0f;
    #endif
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void DisasterFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    U+=0.5f;
    float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;

//    Q = to_float4(building(U,R,iChannel2),0.0f,0.0f,1.0f);
//    SetFragmentShaderComputedColor(Q);
//    return;
    
    if (building(U,R,iChannel2) == 0.0f) {Q = to_float4_s(0); SetFragmentShaderComputedColor(Q); return;}

    Q = A(U);

    float2 f = to_float2(0,-4e-4);
    
    for (float x = -5.0f; x <= 5.0f; x +=1.0f) 
    for (float y = -5.0f; y <= 5.0f; y +=1.0f) {
        float4 b = B(swi2(Q,x,y)+to_float2(x,y));
        float4 a = A(swi2(b,x,y));
        float2 u = abs_f2(swi2(Q,x,y)+to_float2(x,y)-swi2(a,x,y));
        if (u.x>0.5f||u.y>0.5f)continue;
        float2 r = swi2(a,x,y)-swi2(Q,x,y);
        float l = length(r);
        if (l<1.0f||l>6.0f) continue;
        float L = length(U-swi2(b,x,y));
        if ((l-L)<0.1f*L||L>5.0f) 
            f += 3e-1*r*(l-L)/l/L/l;
    }
    
    if (length(f)>1.0f) f = normalize(f);
    
    //swi2(Q,z,w) += f;
    Q.z+=f.x;
    Q.w+=f.y;
    swi2S(Q,x,y, swi2(Q,x,y) + 0.5f*f+swi2(Q,z,w)*1.0f/_sqrtf(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w))));
    
    if (Q.y<30.0f) Q.z *= 0.8f, Q.w *= 0.8f;
    
    if (Q.y<10.0f) Q.y = 10.0f, Q.z *= 0.0f, Q.w *= 0.0f; 

    if (M.z>0.0f) swi2S(Q,z,w, swi2(Q,z,w) + 3e-2*(swi2(M,x,y)-swi2(Q,x,y))/(1.0f+length((swi2(M,x,y)-swi2(Q,x,y)))));

    if(I<1) {
    
        Q = to_float4(U.x,U.y,0,0);
        if (length(U-to_float2_s(0.9f)*R)<0.02f*R.x)     Q.z = -2.5f, Q.w = -1.5f; //swi2(Q,z,w) = to_float2(-2.5f,-1.5f);
        if (length(U-to_float2(0.1f, 0.9f)*R)<0.02f*R.x) Q.z = 2.5f, Q.w = -1.5f;
    }
    if (I == 300)
      if (length(U-to_float2(0.5f, 0.95f)*R)<0.03f*R.x) Q.z = 0.0f, Q.w = -2.5f;
    
  //if (iFrame == 0) Q=to_float4_s(0.0f);  //!
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__DEVICE__ void XY (float2 U, inout float4 *Q, float4 q, float2 R, __TEXTURE__ iChannel0) {
    if (length(U-swi2(A(swi2(q,x,y)),x,y))<length(U-swi2(A(swi2(*Q,x,y)),x,y))) (*Q).x = q.x, (*Q).y = q.y; // swi2(Q,x,y) = swi2(q,x,y);
}
__DEVICE__ void ZW (float2 U, inout float4 *Q, float4 q, float2 R, __TEXTURE__ iChannel0) {
    if (length(U-swi2(A(swi2(q,z,w)),x,y))<length(U-swi2(A(swi2(*Q,z,w)),x,y))) (*Q).z = q.z, (*Q).w = q.w; // swi2(Q,z,w) = swi2(q,z,w);
}

__KERNEL__ void DisasterFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    U+=0.5f;
    float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;

    Q = B(U);
    for (int x=-1;x<=1;x++)
    for (int y=-1;y<=1;y++) {
        XY(U,&Q,B(U+to_float2(x,y)),R,iChannel0);
        //XY(U,&Q,to_float4(swi2(Q,x,y)+to_float2(x,y),0,0),R,iChannel0);
        XY(U,&Q,to_float4(Q.x+x,Q.y+y,0,0),R,iChannel0);
    }
    
    if (I%12==0) 
        Q.z = U.x, Q.w = U.y; //swi2(Q,z,w) = U;
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

  //if (iFrame == 0) Q=to_float4_s(0.0f); //!

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void DisasterFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    U+=0.5f;
    float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;
    
    if (building(U,R,iChannel2) == 0.0f) {Q = to_float4_s(0); SetFragmentShaderComputedColor(Q); return;}

    Q = A(U);

    float2 f = to_float2(0,-4e-4);
    
    for (float x = -5.0f; x <= 5.0f; x +=1.0f) 
    for (float y = -5.0f; y <= 5.0f; y +=1.0f) {
        float4 b = B(swi2(Q,x,y)+to_float2(x,y));
        float4 a = A(swi2(b,x,y));
        float2 u = abs_f2(swi2(Q,x,y)+to_float2(x,y)-swi2(a,x,y));
        if (u.x>0.5f||u.y>0.5f)continue;
        float2 r = swi2(a,x,y)-swi2(Q,x,y);
        float l = length(r);
        if (l<1.0f||l>6.0f) continue;
        float L = length(U-swi2(b,x,y));
        if ((l-L)<0.1f*L||L>5.0f) 
            f += 3e-1*r*(l-L)/l/L/l;
    }
    
    if (length(f)>1.0f) f = normalize(f);
    
    //swi2(Q,z,w) += f;
    Q.z+=f.x;
    Q.w+=f.y;
    swi2S(Q,x,y, swi2(Q,x,y) + 0.5f*f+swi2(Q,z,w)*1.0f/_sqrtf(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w))));
    
    if (Q.y<30.0f) Q.z *= 0.8f, Q.w *= 0.8f;
    
    if (Q.y<10.0f) Q.y = 10.0f, Q.z *= 0.0f, Q.w *= 0.0f; 

    if (M.z>0.0f) swi2S(Q,z,w, swi2(Q,z,w) + 3e-2*(swi2(M,x,y)-swi2(Q,x,y))/(1.0f+length((swi2(M,x,y)-swi2(Q,x,y)))));

    if(I<1) {
    
        Q = to_float4(U.x,U.y,0,0);
        if (length(U-to_float2_s(0.9f)*R)<0.02f*R.x) Q.z = -2.5f, Q.w = -1.5f; //swi2(Q,z,w) = to_float2(-2.5f,-1.5f);
        if (length(U-to_float2(0.1f, 0.9f)*R)<0.02f*R.x) Q.z = 2.5f, Q.w = -1.5f;        
    }
    if (I == 300)
      if (length(U-to_float2(0.5f, 0.95f)*R)<0.03f*R.x) Q.z = 0.0f, Q.w = -2.5f;

  //if (iFrame == 0) Q=to_float4_s(0.0f);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0

#ifdef XXX
void XY (float2 U, inout float4 Q, float4 q) {
    if (length(U-A(swi2(q,x,y)).xy)<length(U-A(swi2(Q,x,y)).xy)) swi2(Q,x,y) = swi2(q,x,y);
}
__DEVICE__ void ZW (float2 U, inout float4 Q, float4 q) {
    if (length(U-A(swi2(q,z,w)).xy)<length(U-A(swi2(Q,z,w)).xy)) swi2(Q,z,w) = swi2(q,z,w);
}
#endif

__KERNEL__ void DisasterFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  
    U+=0.5f;
    float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;
    Q = B(U);
    for (int x=-2;x<=2;x++)
    for (int y=-2;y<=2;y++) {
        XY(U,&Q,B(U+to_float2(x,y)),R,iChannel0);
        //XY(U,&Q,to_float4(swi2(Q,x,y)+to_float2(x,y),0,0),R,iChannel0);
        XY(U,&Q,to_float4(Q.x+x,Q.y+y,0,0),R,iChannel0);
    }
    
    if (I%12==0) 
        Q.z = U.x, Q.w = U.y; //swi2(Q,z,w) = U;
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

  //if (iFrame == 0) Q=to_float4_s(0.0f);
  
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void DisasterFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{


    float2 R=iResolution; float4 M=iMouse; float T=iTime; int I=iFrame;
    
    float4 b = B(U);
    float4 a = A(swi2(b,x,y));

    float4 c = C(swi2(b,x,y));
    
    Q = to_float4_s(0)+step(length(U-swi2(a,x,y)),0.75f);

    if (Q.x > 0.5f && c.w > 0.0f) Q = c;


  SetFragmentShaderComputedColor(Q);
}