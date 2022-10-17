
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define A(U) texture(cha,(U)/R)
//#define B(U) texture(chb,(U)/R)
#define R iResolution
#define M iMouse
#define T iTime
#define I iFrame

#define A(U) _tex2DVecN(cha,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(chb,(U).x/R.x,(U).y/R.y,15)


//#define Main void mainImage (out float4 Q, in float2 U)

__DEVICE__ float signe (float _x) {return _atan2f(100.0f*_x, 1.0f);}
__DEVICE__ void prog (float2 U, out float4 *a, out float4 *b, __TEXTURE2D__ cha, __TEXTURE2D__ chb, float2 R) {
  
  float pppppppppppppppppppppppppppp;
    *a = to_float4_s(0); *b = to_float4_s(0);
    float n = 0.0f;
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++)
    {
        float2 u = to_float2(_x,_y);
        float4 aa = A(U+u), bb = B(U+u);
        #define q 1.075f
        float4 w = clamp(to_float4_f2f2(swi2(aa,x,y)-0.5f*q,swi2(aa,x,y)+0.5f*q),swi4(U,x,y,x,y) - 0.5f,swi4(U,x,y,x,y) + 0.5f);
        float m = (w.w-w.y)*(w.z-w.x)/(q*q);
        swi2S(aa,x,y, 0.5f*(swi2(w,x,y)+swi2(w,z,w)));
        *a += aa*bb.x*m;
        (*b).x += bb.x*m;
        swi3S(*b,y,z,w, swi3(*b,y,z,w) + swi3(bb,y,z,w)*bb.x*m);
        n += bb.x;
    }
    if ((*b).x>0.0f) {
        *a/=(*b).x;
        //b.yzw/=b.x;
        (*b).y/=(*b).x;(*b).z/=(*b).x;(*b).w/=(*b).x;
        
        
        swi3S(*b,y,z,w, swi3(B(swi2(*a,x,y)-swi2(*a,z,w)),y,z,w));
        swi2S(*a,z,w, _mix(swi2(A(swi2(*a,x,y)-swi2(*a,z,w)),z,w),swi2(*a,z,w),clamp(2.0f*n,0.0f,1.0f)));
    }
}

__DEVICE__ void prog2 (float2 U, out float4 *a, out float4 *b, __TEXTURE2D__ cha, __TEXTURE2D__ chb, __TEXTURE2D__ cht, float2 R, float4 M, float T, int I, bool Tex) {
  
    *a = A(U); *b = B(U);
    float2 f = to_float2_s(0); float m = 0.0f, p = 0.0f, z = 0.0f;
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++)
    {
        float2 u = to_float2(_x,_y);
        float4 aa = A(U+u), bb = B(U+u);
        float l = length(u);
        if (l>0.0f) {
            f += 0.125f*(*b).x*bb.x*(0.1f+bb.x*bb.y)*u/l;
            f += 0.125f*(*b).x*bb.x*((*b).z*(*b).w)*to_float2(-u.y,u.x)/l;
            p += 0.125f*(*b).x*bb.x*dot(u/l,swi2(aa,z,w)-swi2(*a,z,w));
            z += 0.125f*(*b).x*bb.x*dot(to_float2(-u.y,u.x)/l,swi2(aa,z,w)-swi2(*a,z,w));
            m += bb.x;
        }
    }
    if (m>0.0f) {
       swi2S(*a,z,w, swi2(*a,z,w) + f/m);
       swi2S(*a,x,y, swi2(*a,x,y) + f/m);
       
       (*b).y += p/m;
       (*b).z += z/m;
    }
    //swi2(a,x,y) += swi2(a,z,w);
    (*a).x += (*a).z;
    (*a).y += (*a).w;
    
    
    // Boundaries:
    (*a).w -= 0.01f/R.y*signe((*b).x);
    if ((*a).x<10.0f) {(*a).z -= -0.1f;(*b).y*=0.9f;}if (R.x-(*a).x<10.0f) {(*a).z -= 0.1f;(*b).y*=0.9f;}if ((*a).y<10.0f) {(*a).w -= -0.1f;(*b).y*=0.9f;}if (R.y-(*a).y<10.0f) {(*a).w -= 0.1f;(*b).y*=0.9f;}
    if (I<1||U.x<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f||R.x-U.x<1.0f) {
        *a = to_float4(U.x,U.y,0,0);
        *b = to_float4_s(0);
        
        if (Tex)
        {
          float4 tex = texture(cht, U/R);
          if (tex.w>0.0f)
          {
            (*b).x = 2.0f; swi2S(*a,z,w, -0.2f*normalize(U-0.5f*R));
          }
        
        }
        else
          if (length(U-0.5f*R) < 0.3f*R.y&&length(U-0.5f*R)>0.0f) {(*b).x = 2.0f; swi2S(*a,z,w, -0.2f*normalize(U-0.5f*R)); }
        (*b).w = 0.0f;
    }
    if (M.z>0.0f) {
        float l = length(U-swi2(M,x,y));
        float s = smoothstep(1.0f,8.0f,l);
        (*b).x = _mix(2.0f,(*b).x,s);
        swi2S(*a,x,y, _mix(U,swi2(*a,x,y),s));
        swi2S(*a,z,w, _mix(0.25f*to_float2(_cosf(0.4f*T),_sinf(0.4f*T)),swi2(*a,z,w),s));
        if (l<8.0f) (*b).w = 1.0f+_sinf(0.1f*T);
    }
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1
// Connect Buffer A 'Texture: Blending' to iChannel2
// Connect Buffer A 'Cubemap: Uffizi Gallery Blurred_0' to iChannel3

__KERNEL__ void MichaelsCoolAlgoPressureFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;

    float4 a, b;
    
    prog (U,&a,&b,iChannel0,iChannel1, R);
    
    Q = a;
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer C' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1
// Connect Buffer B 'Texture: Blending' to iChannel2
// Connect Buffer B 'Cubemap: Uffizi Gallery Blurred_0' to iChannel3

__KERNEL__ void MichaelsCoolAlgoPressureFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    float4 a, b;
    
    prog (U,&a,&b,iChannel0,iChannel1, R);
    
    Q = b;
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Texture: Blending' to iChannel2
// Connect Buffer C 'Cubemap: Uffizi Gallery Blurred_0' to iChannel3

__KERNEL__ void MichaelsCoolAlgoPressureFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Tex, 0); 
    
    U+=0.5f;
    float4 a, b;
    
    prog2 (U,&a,&b,iChannel0,iChannel1,iChannel2,R, iMouse, iTime, iFrame, Tex);
    
    Q = a;
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Texture: Blending' to iChannel2
// Connect Buffer D 'Cubemap: Uffizi Gallery Blurred_0' to iChannel3


__KERNEL__ void MichaelsCoolAlgoPressureFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame, sampler2D iChannel0)
{
     CONNECT_CHECKBOX0(Tex, 0); 
   
     U+=0.5f;
     float4 a, b;
    
     prog2 (U,&a,&b,iChannel0,iChannel1,iChannel2,R, iMouse, iTime, iFrame, Tex);
    
     Q = b;
     SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Texture: Blending' to iChannel2
// Connect Image 'Cubemap: Uffizi Gallery Blurred_0' to iChannel3


__KERNEL__ void MichaelsCoolAlgoPressureFuse(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;

    Q = texture(iChannel1,U/R);
    float4 
        n = texture(iChannel1,(U+to_float2(0,1))/R),
        e = texture(iChannel1,(U+to_float2(1,0))/R),
        s = texture(iChannel1,(U-to_float2(0,1))/R),
        w = texture(iChannel1,(U-to_float2(1,0))/R);
    float3 no = normalize(to_float3(e.x-w.x,-n.x+s.x,0.1f));
    Q.x = _atan2f(0.8f*_logf(1.0f+Q.x),1.0f);
    Q = Q.x*(0.8f+0.6f*abs_f4(cos_f4(2.0f*Q.w+(1.0f+Q.y)*to_float4(1,2,3,4))));
    Q *= 0.9f+0.5f*decube_f3(iChannel3,no);
    Q = to_float4_s(1.0f)-Q;
    
    SetFragmentShaderComputedColor(Q);
}