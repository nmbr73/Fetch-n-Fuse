
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


//vec2 R; float T; int I; float4 M;
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
//#define Main void mainImage(out float4 Q, in float2 U) {R=iResolution;T=iTime;I=iFrame;M=iMouse;

// mat2 
#define ei(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))

#define N _fminf(5.0f+(float)(I/10),100.0f)


__DEVICE__ float2 size (float i, float2 R) {
  
    float2 s = to_float2(1,2.0f+3.5f*i/100.0f)*0.012f*R.x;
    return s;
}
__DEVICE__ float2 mag (float2 p, float4 a, float4 t) {
    //https://en.wikipedia.org/wiki/Magnetic_moment#Magnetism
    float2 r = mul_f2_mat2((swi2(p,x,y)-swi2(a,x,y)),ei(-t.x));
    float2 m = to_float2(0,1);
    float l = length(r);
    if (l >0.0f) return 1e2*(3.0f*r/l*dot(r/l,m)-m)/(l*l);
    else return to_float2_s(0);
}
#define dt 0.3f
__DEVICE__ float2 vel (float2 p, float4 a, float4 t) {
    float2 w = swi2(p,x,y)-swi2(a,x,y);
    if (length(w)>0.0f) w = to_float2(-w.y,w.x);
    return swi2(a,z,w) + w*t.y;
}
__DEVICE__ float2 MF (float2 p, float4 a, float4 t, float4 b, float4 bt, int I) {
    float2 r = swi2(p,x,y)-swi2(a,x,y);
    float2 m1 = mul_f2_mat2(to_float2(0,300.0f/_sqrtf(1.0f+N)),ei(t.x));
    float2 m2 = mul_f2_mat2(to_float2(0,300.0f/_sqrtf(1.0f+N)),ei(bt.x));
    float l = length(r);
    if (l>0.0f) {
          float2 rh = r/l;
          return (m2*dot(m1,rh)+m1*dot(m2,rh)+rh*dot(m1,m2)-5.0f*rh*dot(m1,rh)*dot(m2,rh)) / ( l*l*l*l );
    } else return to_float2_s(0);
}
__DEVICE__ float map ( float2 p, float4 a, float4 t, float2 b )
{ //iquilezles.org/www/articles/distfunctions/distfunctions.html
  p = mul_f2_mat2((p-swi2(a,x,y)),ei(-t.x));
  float2 q = (abs_f2(p) - b);
  return length(_fmaxf(q,to_float2_s(0.0f))) + _fminf(max(q.x,q.y),0.0f);
}

__DEVICE__ float2 norm (float2 p, float4 a, float4 t, float2 b)
{   
    float2 e = to_float2(1,0);
    return 1.0f/dt*normalize(to_float2(
        map(p+swi2(e,x,y),a,t,b)-map(p-swi2(e,x,y),a,t,b),
        map(p+swi2(e,y,x),a,t,b)-map(p-swi2(e,y,x),a,t,b)
    ));
    
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void MagnetSimFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

    U += 0.5f;
    float2 R=iResolution; float T=iTime; int I=iFrame; float4 M=iMouse;

    if (U.y>4.0f) { Q = A(U); SetFragmentShaderComputedColor(Q); return;}//discard;}

    
    Q = A(to_float2(U.x,0.5f));
    float4 t = A(to_float2(U.x,1.5f));
    float4 oQ = A(to_float2(U.x,2.5f));
    float4 ot = A(to_float2(U.x,3.5f));
    float2 w = size(_floor(U.x),iResolution);
    
    if (U.y<2.0f) {
        float2 f = to_float2_s(0);
        float tor = 0.0f;
        float h = 0.0f;
        for(float j = 0.0f; j < N; j++) if (j!=_floor(U.x)) {
            float4 b = A(to_float2(j,0)+0.5f);
            float4 bt = A(to_float2(j,1)+0.5f);
            float2 bw = size(j,iResolution);
            for (float x = 0.0f; x<=1.0f; x+=1.0f)
            for (float y = 0.0f; y<=1.0f; y+=1.0f)
            {
                { 
                    float2 q = mul_f2_mat2(w*(to_float2(x,y)-0.5f),ei(t.x))+swi2(Q,x,y);
                    // magnetic force
                    {
                        float2 r = q-swi2(b,x,y);
                        float2 im = MF(q,Q,t,b,bt,I);
                        if (length(r)>0.0f) {
                            tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                            f += im*_fabs(dot(normalize(r),normalize(im)));
                        }
                    }
                    // my point in their box
                    float m = map(q,b,bt,0.5f*bw);
                    if (m<0.0f) {
                        float2 r = q-swi2(Q,x,y);
                        float2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        float2 im = norm(q,b,bt,0.5f*bw)+v;
                        tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                        f += im*_fabs(dot(normalize(r),normalize(im)));
                        h += 1.0f;
                    }
                }
                { // their point in my box
                    float2 q = mul_f2_mat2(bw*(to_float2(x,y)-0.5f),ei(bt.x))+swi2(b,x,y);
                    float m = map(q,Q,t,0.5f*w);
                    if (m<0.0f) {
                        float2 r = q-swi2(Q,x,y);
                        float2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        float2 im = norm(q,b,bt,0.5f*bw)+v;
                        tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                        f += im*_fabs(dot(normalize(r),normalize(im)));
                        h += 1.0f;
                    }
                }
            }
        }
        for (float x = 0.0f; x<=1.0f; x+=1.0f)
        for (float y = 0.0f; y<=1.0f; y+=1.0f)
        {
            float2 q = mul_f2_mat2(w*(to_float2(x,y)-0.5f),ei(t.x))+swi2(Q,x,y);
            float4 b = to_float4(0.5f*R.x,0.5f*R.y,0,0);
            float m = -map(q,b,to_float4_s(0),swi2(b,x,y));
            if (m<0.0f) {
                float2 r = q-swi2(Q,x,y);
                float2 v = vel(q,b,to_float4_s(0))-vel(q,Q,t);
                float l = length(r);
                float2 im = -1.0f*norm(q,b,to_float4_s(0),0.5f*swi2(R,x,y))+v;
                tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                f += im*_fabs(dot(normalize(r),normalize(im)));
                h += 1.0f;
            }
        }
        if (h>0.0f) f/=h, tor/=h;
        //swi2(Q,z,w) += dt*(f);
        Q.z += dt*(f.x);
        Q.w += dt*(f.y);
    
        //swi2(Q,x,y) += dt*(0.5f*f+swi2(Q,z,w));
        Q.x += dt*(0.5f*f.x+Q.z);
        Q.y += dt*(0.5f*f.y+Q.w);
        
        
        t.y += dt*(tor);
        t.x += dt*(0.5f*tor+t.y);
        if (M.z>0.0f)                      { float2 Qzw = swi2(Q,z,w) + 1e-3*(swi2(Q,x,y)-swi2(M,x,y))*_expf(-0.001f*length(swi2(Q,x,y)-swi2(M,x,y))); Q.z=Qzw.x;Q.w=Qzw.y;}
        if (length(swi2(Q,z,w))>1.0f/dt)   { float2 Qzw = 1.0f/dt*normalize(swi2(Q,z,w)); Q.z=Qzw.x;Q.w=Qzw.y;}
        if (_fabs(t.y)>dt)                 t.y = dt*sign_f(t.y);
    }
    
    if(iFrame < 1) {
        //float2 size = size(_floor(U.x),iResolution);
        float2 tmp = 0.5f*R+0.1f*R.y*(1.0f+6.0f*U.x/N)*to_float2(_cosf(36.0f*U.x/N),_sinf(36.0f*U.x/N));
        Q = to_float4(tmp.x,tmp.y,0,0);
        t = to_float4(10.0f*U.x/N,0,0,0);
    }
    if (mod_f(U.y,2.0f)>1.0f) Q = t;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void MagnetSimFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

    U += 0.5f;
    
    float2 R=iResolution; float T=iTime; int I=iFrame; float4 M=iMouse;
    
    if (U.y>4.0f) { SetFragmentShaderComputedColor(Q); return;}//discard;}
       
    Q = A(to_float2(U.x,0.5f));
    float4 t = A(to_float2(U.x,1.5f));
    float4 oQ = A(to_float2(U.x,2.5f));
    float4 ot = A(to_float2(U.x,3.5f));
    float2 w = size(_floor(U.x),iResolution);
    
    if (U.y<2.0f) {
        float2 f = to_float2_s(0);
        float tor = 0.0f;
        float h = 0.0f;
        for(float j = 0.0f; j < N; j+=1.0f) 
          if (j!=_floor(U.x)) {
            float4 b = A(to_float2(j,0)+0.5f);
            float4 bt = A(to_float2(j,1)+0.5f);
            float2 bw = size(j,iResolution);
            for (float x = 0.0f; x<=1.0f; x+=1.0f)
            for (float y = 0.0f; y<=1.0f; y+=1.0f)
            {
                { 
                    float2 q = mul_f2_mat2(w*(to_float2(x,y)-0.5f),ei(t.x))+swi2(Q,x,y);
                    // magnetic force
                    {
                        float2 r = q-swi2(b,x,y);
                        float2 im = MF(q,Q,t,b,bt,I);
                        if (length(r)>0.0f) {
                            tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                            f += im*_fabs(dot(normalize(r),normalize(im)));
                        }
                    }
                    // my point in their box
                    float m = map(q,b,bt,0.5f*bw);
                    if (m<0.0f) {
                        float2 r = q-swi2(Q,x,y);
                        float2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        float2 im = norm(q,b,bt,0.5f*bw)+v;
                        tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                        f += im*_fabs(dot(normalize(r),normalize(im)));
                        h += 1.0f;
                    }
                }
                { // their point in my box
                    float2 q = mul_f2_mat2(bw*(to_float2(x,y)-0.5f),ei(bt.x))+swi2(b,x,y);
                    float m = map(q,Q,t,0.5f*w);
                    if (m<0.0f) {
                        float2 r = q-swi2(Q,x,y);
                        float2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        float2 im = norm(q,b,bt,0.5f*bw)+v;
                        tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                        f += im*_fabs(dot(normalize(r),normalize(im)));
                        h += 1.0f;
                    }
                }
            }
        }
        for (float x = 0.0f; x<=1.0f; x+=1.0f)
        for (float y = 0.0f; y<=1.0f; y+=1.0f)
        {
            float2 q = mul_f2_mat2(w*(to_float2(x,y)-0.5f),ei(t.x))+swi2(Q,x,y);
            float4 b = to_float4(0.5f*R.x,0.5f*R.y,0,0);
            float m = -map(q,b,to_float4_s(0),swi2(b,x,y));
            if (m<0.0f) {
                float2 r = q-swi2(Q,x,y);
                float2 v = vel(q,b,to_float4_s(0))-vel(q,Q,t);
                float l = length(r);
                float2 im = -1.0f*norm(q,b,to_float4_s(0),0.5f*swi2(R,x,y))+v;
                tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                f += im*_fabs(dot(normalize(r),normalize(im)));
                h += 1.0f;
            }
        }
        if (h>0.0f) f/=h, tor/=h;
        
        //swi2(Q,z,w) += dt*(f);
        Q.z += dt*(f.x);
        Q.w += dt*(f.y);
        
        //swi2(Q,x,y) += dt*(0.5f*f+swi2(Q,z,w));
        Q.x += dt*(0.5f*f.x+Q.z);
        Q.y += dt*(0.5f*f.y+Q.w);
        
        t.y += dt*(tor);
        t.x += dt*(0.5f*tor+t.y);
        
        if (M.z>0.0f)                      { float2 Qzw = swi2(Q,z,w) + 1e-3*(swi2(Q,x,y)-swi2(M,x,y))*_expf(-0.001f*length(swi2(Q,x,y)-swi2(M,x,y))); Q.z=Qzw.x;Q.w=Qzw.y;}
        if (length(swi2(Q,z,w))>1.0f/dt)   { float2 Qzw = 1.0f/dt*normalize(swi2(Q,z,w)); Q.z=Qzw.x;Q.w=Qzw.y;}
        if (_fabs(t.y)>dt)                 t.y = dt*sign_f(t.y);
    }
    
    if(iFrame < 1) {
        //float2 size = size(_floor(U.x),iResolution);
        
        float2 tmp = 0.5f*R+0.1f*R.y*(1.0f+6.0f*U.x/N)*to_float2(_cosf(36.0f*U.x/N),_sinf(36.0f*U.x/N));
        Q = to_float4(tmp.x,tmp.y,0,0);
        t = to_float4(10.0f*U.x/N,0,0,0);
    }
    if (mod_f(U.y,2.0f)>1.0f) Q = t;



  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void MagnetSimFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

    U += 0.5f;
    
    float2 R=iResolution; float T=iTime; int I=iFrame; float4 M=iMouse;
        
    if (U.y>4.0f) { SetFragmentShaderComputedColor(Q); return;}//discard;}
    
    Q = A(to_float2(U.x,0.5f));
    float4 t = A(to_float2(U.x,1.5f));
    float4 oQ = A(to_float2(U.x,2.5f));
    float4 ot = A(to_float2(U.x,3.5f));
    float2 w = size(_floor(U.x),iResolution);
    
    if (U.y<2.0f) {
        float2 f = to_float2_s(0);
        float tor = 0.0f;
        float h = 0.0f;
        for(float j = 0.0f; j < N; j++) if (j!=_floor(U.x)) {
            float4 b = A(to_float2(j,0)+0.5f);
            float4 bt = A(to_float2(j,1)+0.5f);
            float2 bw = size(j,iResolution);
            for (float x = 0.0f; x<=1.0f; x+=1.0f)
            for (float y = 0.0f; y<=1.0f; y+=1.0f)
            {
                { 
                    float2 q = mul_f2_mat2(w*(to_float2(x,y)-0.5f),ei(t.x))+swi2(Q,x,y);
                    // magnetic force
                    {
                        float2 r = q-swi2(b,x,y);
                        float2 im = MF(q,Q,t,b,bt,I);
                        if (length(r)>0.0f) {
                            tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                            f += im*_fabs(dot(normalize(r),normalize(im)));
                        }
                    }
                    // my point in their box
                    float m = map(q,b,bt,0.5f*bw);
                    if (m<0.0f) {
                        float2 r = q-swi2(Q,x,y);
                        float2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        float2 im = norm(q,b,bt,0.5f*bw)+v;
                        tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                        f += im*_fabs(dot(normalize(r),normalize(im)));
                        h += 1.0f;
                    }
                }
                { // their point in my box
                    float2 q = mul_f2_mat2(bw*(to_float2(x,y)-0.5f),ei(bt.x))+swi2(b,x,y);
                    float m = map(q,Q,t,0.5f*w);
                    if (m<0.0f) {
                        float2 r = q-swi2(Q,x,y);
                        float2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        float2 im = norm(q,b,bt,0.5f*bw)+v;
                        tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                        f += im*_fabs(dot(normalize(r),normalize(im)));
                        h += 1.0f;
                    }
                }
            }
        }
        for (float x = 0.0f; x<=1.0f; x+=1.0f)
        for (float y = 0.0f; y<=1.0f; y+=1.0f)
        {
            float2 q = mul_f2_mat2(w*(to_float2(x,y)-0.5f),ei(t.x))+swi2(Q,x,y);
            float4 b = to_float4(0.5f*R.x,0.5f*R.y,0,0);
            float m = -map(q,b,to_float4_s(0),swi2(b,x,y));
            if (m<0.0f) {
                float2 r = q-swi2(Q,x,y);
                float2 v = vel(q,b,to_float4_s(0))-vel(q,Q,t);
                float l = length(r);
                float2 im = -1.0f*norm(q,b,to_float4_s(0),0.5f*swi2(R,x,y))+v;
                tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                f += im*_fabs(dot(normalize(r),normalize(im)));
                h += 1.0f;
            }
        }
        if (h>0.0f) f/=h, tor/=h;
        
        //swi2(Q,z,w) += dt*(f);
        Q.z += dt*(f.x);
        Q.w += dt*(f.y);
        
        //swi2(Q,x,y) += dt*(0.5f*f+swi2(Q,z,w));
        Q.x += dt*(0.5f*f.x+Q.z);
        Q.y += dt*(0.5f*f.y+Q.w);
        
        t.y += dt*(tor);
        t.x += dt*(0.5f*tor+t.y);

        if (M.z>0.0f)                      { float2 Qzw = swi2(Q,z,w) + 1e-3*(swi2(Q,x,y)-swi2(M,x,y))*_expf(-0.001f*length(swi2(Q,x,y)-swi2(M,x,y))); Q.z=Qzw.x;Q.w=Qzw.y;}
        if (length(swi2(Q,z,w))>1.0f/dt)   { float2 Qzw = 1.0f/dt*normalize(swi2(Q,z,w)); Q.z=Qzw.x;Q.w=Qzw.y;}
        if (_fabs(t.y)>dt)                 t.y = dt*sign_f(t.y);
    }
    
    if(iFrame < 1) {
        //float2 size = size(_floor(U.x).iResolution);
        float2 tmp = 0.5f*R+0.1f*R.y*(1.0f+6.0f*U.x/N)*to_float2(_cosf(36.0f*U.x/N),_sinf(36.0f*U.x/N));
        Q = to_float4(tmp.x,tmp.y,0,0);
        
        t = to_float4(10.0f*U.x/N,0,0,0);
    }
    if (mod_f(U.y,2.0f)>1.0f) Q = t;



  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void MagnetSimFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

    U += 0.5f;
    
    float2 R=iResolution; float T=iTime; int I=iFrame; float4 M=iMouse;
    
    if (U.y>4.0f) { SetFragmentShaderComputedColor(Q); return;}//discard;}
    
    Q = A(to_float2(U.x,0.5f));
    float4 t = A(to_float2(U.x,1.5f));
    float4 oQ = A(to_float2(U.x,2.5f));
    float4 ot = A(to_float2(U.x,3.5f));
    float2 w = size(_floor(U.x),iResolution);
    
    if (U.y<2.0f) {
        float2 f = to_float2_s(0);
        float tor = 0.0f;
        float h = 0.0f;
        for(float j = 0.0f; j < N; j++) if (j!=_floor(U.x)) {
            float4 b = A(to_float2(j,0)+0.5f);
            float4 bt = A(to_float2(j,1)+0.5f);
            float2 bw = size(j,iResolution);
            for (float x = 0.0f; x<=1.0f; x+=1.0f)
            for (float y = 0.0f; y<=1.0f; y+=1.0f)
            {
                { 
                    float2 q = mul_f2_mat2(w*(to_float2(x,y)-0.5f),ei(t.x))+swi2(Q,x,y);
                    // magnetic force
                    {
                        float2 r = q-swi2(b,x,y);
                        float2 im = MF(q,Q,t,b,bt,I);
                        if (length(r)>0.0f) {
                            tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                            f += im*_fabs(dot(normalize(r),normalize(im)));
                        }
                    }
                    // my point in their box
                    float m = map(q,b,bt,0.5f*bw);
                    if (m<0.0f) {
                        float2 r = q-swi2(Q,x,y);
                        float2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        float2 im = norm(q,b,bt,0.5f*bw)+v;
                        tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                        f += im*_fabs(dot(normalize(r),normalize(im)));
                        h += 1.0f;
                    }
                }
                { // their point in my box
                    float2 q = mul_f2_mat2(bw*(to_float2(x,y)-0.5f),ei(bt.x))+swi2(b,x,y);
                    float m = map(q,Q,t,0.5f*w);
                    if (m<0.0f) {
                        float2 r = q-swi2(Q,x,y);
                        float2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        float2 im = norm(q,b,bt,0.5f*bw)+v;
                        tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                        f += im*_fabs(dot(normalize(r),normalize(im)));
                        h += 1.0f;
                    }
                }
            }
        }
        for (float x = 0.0f; x<=1.0f; x+=1.0f)
        for (float y = 0.0f; y<=1.0f; y+=1.0f)
        {
            float2 q = mul_f2_mat2(w*(to_float2(x,y)-0.5f),ei(t.x))+swi2(Q,x,y);
            float4 b = to_float4(0.5f*R.x,0.5f*R.y,0,0);
            float m = -map(q,b,to_float4_s(0),swi2(b,x,y));
            if (m<0.0f) {
                float2 r = q-swi2(Q,x,y);
                float2 v = vel(q,b,to_float4_s(0))-vel(q,Q,t);
                float l = length(r);
                float2 im = -1.0f*norm(q,b,to_float4_s(0),0.5f*swi2(R,x,y))+v;
                tor -= dot(r,to_float2(-im.y,im.x))/(w.x*w.y);
                f += im*_fabs(dot(normalize(r),normalize(im)));
                h += 1.0f;
            }
        }
        if (h>0.0f) f/=h, tor/=h;
        
        //swi2(Q,z,w) += dt*(f);
        Q.z += dt*(f.x);
        Q.w += dt*(f.y);
        
        //swi2(Q,x,y) += dt*(0.5f*f+swi2(Q,z,w));
        Q.x += dt*(0.5f*f.x+Q.z);
        Q.y += dt*(0.5f*f.y+Q.w);
        
        t.y += dt*(tor);
        t.x += dt*(0.5f*tor+t.y);
        
        if (M.z>0.0f)                      { float2 Qzw = swi2(Q,z,w) + 1e-3*(swi2(Q,x,y)-swi2(M,x,y))*_expf(-0.001f*length(swi2(Q,x,y)-swi2(M,x,y))); Q.z=Qzw.x;Q.w=Qzw.y;}
        if (length(swi2(Q,z,w))>1.0f/dt)   { float2 Qzw = 1.0f/dt*normalize(swi2(Q,z,w)); Q.z=Qzw.x;Q.w=Qzw.y;}
        if (_fabs(t.y)>dt)                 t.y = dt*sign_f(t.y);
    }
    
    if(iFrame < 1) {
        //float2 size = size(_floor(U.x).iResolution);
        
        float2 tmp = 0.5f*R+0.1f*R.y*(1.0f+6.0f*U.x/N)*to_float2(_cosf(36.0f*U.x/N),_sinf(36.0f*U.x/N));
        Q = to_float4(tmp.x,tmp.y,0,0);
        t = to_float4(10.0f*U.x/N,0,0,0);
    }
    if (mod_f(U.y,2.0f)>1.0f) Q = t;



  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Fork of "Rigid Body Test 123" by wyatt. https://shadertoy.com/view/fsscRr
// 2022-01-10 00:45:24

__KERNEL__ void MagnetSimFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{


  float2 R=iResolution; float T=iTime; int I=iFrame; float4 M=iMouse;
  //if (iFrame % 8 > 0) discard;
  Q = sin_f4(5.3f+U.y/R.y+(1.0f+U.x/R.x)*to_float4(1,2,3,4));
  Q *= 0.5f*(_expf(-30.0f*U.x/R.x)+_expf(-30.0f*(R.x-U.x)/R.x)+_expf(-30.0f*U.y/R.y)+_expf(-30.0f*(R.y-U.y)/R.y));
  Q = to_float4_s(1.0f)-Q;
  float2 m = to_float2_s(0);
  float2 p = U;
  for (float i = 0.0f; i < N; i+=1.0f) {
      float4 a = A(to_float2(i,0)+0.5f);
      float4 at= A(to_float2(i,1)+0.5f);
      float2 aw = size(i,iResolution)/6.0f;
      float d = map(p,a,at,aw);
      if (_fabs(d)<2.0f) Q = to_float4_s(0);
      else if (dot(p-swi2(a,x,y), mul_f2_mat2(to_float2(0,1),ei(at.x)))>0.0f&&
          d<0.0f);
      else if (d<0.0f) Q = to_float4(1,0.2f,0.5f,0);
      else {
          m += mag(p,a,at);
      }
  }
  Q *= to_float4_s(1.0f)-0.8f*(Q.y)*length(m)*(0.5f+0.5f*sin_f4(_atan2f(m.x,m.y)+to_float4(1,2,3,4)));


  SetFragmentShaderComputedColor(Q);
}