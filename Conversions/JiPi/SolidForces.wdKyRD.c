
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


//vec2 R;
#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define A(U) texture(cha,(U)/R)
#define B(U) texture(chb,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel2,(U)/R)


__DEVICE__ float signe (float _x) {return _atan2f(100.0f*_x,1.0f);}
__DEVICE__ void prog (float2 U, out float4 *a, out float4 *b, float2 R, __TEXTURE2D__ cha, __TEXTURE2D__ chb) {
  
    *a = to_float4_s(0); *b = to_float4_s(0);
    float n = 0.0f;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        float2 u = to_float2(x,y);
        float4 aa = A(U+u), bb = B(U+u);
      #define q 1.075
        float4 w = clamp(to_float4_f2f2(swi2(aa,x,y)-0.5f*q,swi2(aa,x,y)+0.5f*q),swi4(U,x,y,x,y) - 0.5f,swi4(U,x,y,x,y) + 0.5f);
        float m = (w.w-w.y)*(w.z-w.x)/(q*q);
        swi2S(aa,x,y, 0.5f*(swi2(w,x,y)+swi2(w,z,w)));
        *a += aa*bb.x*m;
        (*b).x += bb.x*m;
        swi3S(*b,y,z,w, swi3(*b,y,z,w) + swi3(bb,y,z,w)*bb.x*m);
        n += bb.x;
    }
    if ((*b).x>0.0f) {
        (*a)/=(*b).x;
        //b.yzw/=b.x;
        (*b).y/=(*b).x;
        (*b).z/=(*b).x;
        (*b).w/=(*b).x;
        
        //swi3(b,y,z,w) = B(swi2(a,x,y)-swi2(a,z,w)).yzw;
        //swi2(a,z,w) = _mix(A(swi2(a,x,y)-swi2(a,z,w)).zw,swi2(a,z,w),clamp(2.0f*n,0.0f,1.0f));
    }
}

__DEVICE__ void prog2 (float2 U, out float4 *a, out float4 *b, float2 R, __TEXTURE2D__ cha, __TEXTURE2D__ chb, float T, int I, float4 M) {
    
    *a = A(U); *b = B(U);
    float2 f = to_float2_s(0); float m = 0.0f, p = 0.0f, z = 0.0f;
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++)
    {
        float2 u = to_float2(_x,_y);
        float4 aa = A(U+u), bb = B(U+u);
        float l = dot(u,u);
        if (l>0.0f) {
            f += 0.1f*0.125f*(*b).x*(bb.x*(0.2f+bb.x*bb.y)-(*b).x*(0.2f+(*b).x*(*b).y))*u/l;
            f += 0.125f*(*b).x*(bb.z*bb.x-(*b).z*(*b).x)*to_float2(-u.y,u.x)/l;
            p += (*b).x*bb.x*dot(u/l,swi2(aa,z,w)-swi2(*a,z,w));
            z += (*b).x*bb.x*(dot(to_float2(-u.y,u.x)/l,swi2(aa,z,w)-swi2(*a,z,w)));
            m += bb.x;
        }
    }
    if (m>0.0f) {
       //swi2(*a,z,w) += f/m;
       (*a).z+=f.x/m;
       (*a).w+=f.y/m;
       
       //swi2(*a,x,y) += f/m;
       (*a).x+=f.x/m;
       (*a).y+=f.y/m;         
       
       //swi2(*a,x,y) += f/m;
       (*a).x+=f.x/m;
       (*a).y+=f.y/m;
       
       (*b).y += p/m;
       (*b).z += z/m;
    }
    //swi2(*a,x,y) += swi2(*a,z,w);
    (*a).x+=(*a).z;
    (*a).y+=(*a).w;
    
    // Boundaries:
    (*a).w -= 0.003f/R.y*signe((*b).x);
    if ((*a).x<10.0f) {(*a).z -= -0.1f;(*b).y*=0.9f;}if (R.x-(*a).x<10.0f) {(*a).z -= 0.1f;(*b).y*=0.9f;}if ((*a).y<10.0f) {(*a).w -= -0.1f;(*b).y*=0.9f;}if (R.y-(*a).y<10.0f) {(*a).w -= 0.1f;(*b).y*=0.9f;}
    if (I<1||U.x<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f||R.x-U.x<1.0f) {
      *a = to_float4(U.x,U.y,0,0);
      *b = to_float4_s(0);
      if (length(U-0.5f*R) < 0.3f*R.y&&length(U-0.5f*R)>0.0f) {(*b).y = 0.0f;(*b).x = 1.0f; swi2S(*a,z,w, -0.01f*normalize(U-0.5f*R))}
      (*b).w = 0.0f;
    }
    if (M.z>0.0f) {
        float l = length(U-swi2(M,x,y));
        if (l<8.0f) {
            (*b).x = 2.0f;
            //swi2(*a,x,y) = U;
            (*a).x=U.x; (*a).y=U.y;
            swi2S(*a,z,w, 0.25f*to_float2(_cosf(0.4f*T),_sinf(0.4f*T)));
            (*b).w = 0.4f+0.4f*_sinf(0.1f*T);
        }
    }
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void SolidForcesFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
    //R = iResolution;
    float4 M = iMouse;
    int I = iFrame;
    float T = iTime;
    float4 a, b;
    
    prog (U,&a,&b,R,iChannel0,iChannel1);

    Q = a;
    
    if(Reset) Q=to_float4_s(0.0f);
    
  SetFragmentShaderComputedColor(Q);  
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer C' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void SolidForcesFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
    //R = iResolution;
    float4 M = iMouse;
    int I = iFrame;
    float T = iTime;
    float4 a, b;

    prog (U,&a,&b,R,iChannel0,iChannel1);
    
    Q = b;
    
    if(Reset) Q=to_float4_s(0.0f);
    
  SetFragmentShaderComputedColor(Q);  
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void SolidForcesFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
  
    CONNECT_SLIDER0(Blenda, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendaZ_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(BlendaW_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER8(BlendaS_Thr, -10.0f, 10.0f, 0.4f);
    
    CONNECT_BUTTON0(Modus, 1, Z,  W, Particle, PS);
  
    U+=0.5f; 
    //R = iResolution;
    float4 M = iMouse;
    int I = iFrame;
    float T = iTime;
    float4 a, b;
    
    prog2 (U,&a,&b,R,iChannel0,iChannel1,T,I,M);
    
    Q = a;
    
    //Textureblending
    if (Blenda > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel2,U/R);

      if (tex.w > 0.0f)
      {
        //swi3S(Co,x,y,z, _mix(swi3(Co,x,y,z),(swi3(tex,x,y,z)*2.0f-0.5f)*Blend1_Thr, Blenda));       
        
        if ((int)Modus&2)
          Q.z = _mix(Q.z,(tex.x-0.5f)*BlendaZ_Thr, Blenda);

        if ((int)Modus&4)
          Q.w = _mix(Q.w,(tex.y-0.5f)*BlendaW_Thr, Blenda);
        
        if ((int)Modus&8)
        {  
          //swi2S(Q,x,y, _mix(swi2(Q,x,y), U, Blenda));
          //if (U.x>0.1f && U.x<0.9f && U.y > 0.1f && U.y < 0.9f)
          float2 uv = U/R;  
          if (uv.x>0.01f && uv.x<0.99f && uv.y > 0.01f && uv.y < 0.99f)
             Q.x=U.x, Q.y=U.y; //Q.z=tex.x; Q
        }
        
        if ((int)Modus&16)
        {  
          //swi2S(Q,x,y, _mix(swi2(Q,x,y), U, Blenda));
          //if (U.x>0.1f && U.x<0.9f && U.y > 0.1f && U.y < 0.9f)
          float2 uv = U/R;  
          if (uv.x>0.01f && uv.x<0.99f && uv.y > 0.01f && uv.y < 0.99f)
          {
            Q.x=U.x, Q.y=U.y; //Q.z=tex.x;
            Q.z= 0.25f*_cosf(0.4f*T);//BlendaS_Thr);
            Q.w= 0.25f*_sinf(0.4f*T);//BlendaS_Thr);
          }
        }
        
      }
    }

    
  SetFragmentShaderComputedColor(Q);  
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void SolidForcesFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER3(Blendb, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER4(BlendbX_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER5(BlendbW_Thr, 0.0f, 10.0f, 1.0f);
    
    CONNECT_BUTTON1(Modus, 1, X,  W, S);
    
    U+=0.5f;
    //R = iResolution;
    float4 M = iMouse;
    int I = iFrame;
    float T = iTime;
    float4 a, b;
    
    prog2 (U,&a,&b,R,iChannel0,iChannel1,T,I,M);
    
    Q = b;
    
    //Textureblending
    if (Blendb > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel2,U/R);

      if (tex.w > 0.0f)
      {
        //swi3S(Co,x,y,z, _mix(swi3(Co,x,y,z),(swi3(tex,x,y,z)*2.0f-0.5f)*Blend1_Thr, Blenda));       
        
        if ((int)Modus&2)
          Q.x = _mix(Q.x,(tex.x-0.5f)*BlendbX_Thr, Blendb);

        if ((int)Modus&4)
          Q.w = _mix(Q.w,(tex.y-0.5f)*BlendbW_Thr, Blendb);
        
        if ((int)Modus&8)
          Q.x = 0.25f*_sinf(0.4f*T);
      }
    }

    
  SetFragmentShaderComputedColor(Q);  
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery Blurred_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


// Mykhailo/Michael's tutorial : 

//    https://t.co/G2aPHqVEo7?amp=1

// Solid Forces : 
// https://www.shadertoy.com/view/WtK3zK

// Delete line 48 in common to see the difference

__KERNEL__ void SolidForcesFuse(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER9(Alpha, 0.0f, 1.0f, 1.0f);
    U+=0.5f;
    //R = iResolution;
    Q = texture(iChannel1,U/R);
    float4 
        n = texture(iChannel1,(U+to_float2(0,1))/R),
        e = texture(iChannel1,(U+to_float2(1,0))/R),
        s = texture(iChannel1,(U-to_float2(0,1))/R),
        w = texture(iChannel1,(U-to_float2(1,0))/R);
    float3 no = normalize(to_float3(e.x-w.x,-n.x+s.x,0.1f));
    Q.x = _atan2f(0.8f*_logf(1.0f+Q.x),1.0f);
    Q = Q.x*(0.8f+0.6f*abs_f4(cos_f4(0.1f+2.0f*Q.w+(1.0f+Q.y+5.0f*Q.z)*to_float4(1,2,3,4))));
    Q *= 0.9f+0.5f*_tex2DVecN(iChannel2,no.x,no.y,15);
    Q = to_float4_s(0.9f)-1.2f*Q;
    Q.w=Alpha;
    
    SetFragmentShaderComputedColor(Q);
}
