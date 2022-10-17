
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R   (iResolution)
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// charge force range, collision force range ,0,0 does nothing
#define FORCE_RANGE to_float4(   25, 2.5f,     0,0)

// how many blur iterations
#define BLUR_DEPTH 40.0f
// multiplies the force per frame
#define SPEED 2.0f
// restart after changing. Smaller number -> more particles
#define SEPARATION 13.0f
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


// Particle tracking

__DEVICE__ float4 A (float2 U, float2 R, __TEXTURE2D__ iChannel0) {return texture(iChannel0,U/R);}
__DEVICE__ float4 B (float2 U, float2 R, __TEXTURE2D__ iChannel1) {return texture(iChannel1,U/R);}
__DEVICE__ float4 C (float2 U, float2 R, __TEXTURE2D__ iChannel2) {return texture(iChannel2,U/R);}
__DEVICE__ float4 D (float2 U, float2 R, __TEXTURE2D__ iChannel3) {return texture(iChannel3,U/R);}
__DEVICE__ void X (float2 U, inout float4 *Q, float2 u, float2 R, __TEXTURE2D__ iChannel0) {
    float4 p = A(U+u,R,iChannel0);
    if (length(swi2(p,x,y) - U) < length(swi2(*Q,x,y)-U)) *Q = p;
    
}

__KERNEL__ void MolecularDanceFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  
    CONNECT_CHECKBOX0(Reset, 0);
    
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendZ, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(BlendW, -10.0f, 10.0f, 1.0f);
    CONNECT_CHECKBOX1(SpecialInv, 0);
    CONNECT_SLIDER3(SpecialValue, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, XY,  Z, W, Clear, Special);
  
   U+=0.5f;
   Q = A(U,R,iChannel0);
   // measure neighborhood
   for (int _x = -1; _x <= 1; _x++)
     for (int _y = -1; _y <= 1; _y++)
       X(U,&Q,to_float2(_x,_y),R,iChannel0);
    
   float2 u = swi2(Q,x,y);
   float4 
        n = C(u+to_float2(0,1),R,iChannel2),
        e = C(u+to_float2(1,0),R,iChannel2),
        s = C(u+to_float2(0,-1),R,iChannel2),
        w = C(u+to_float2(-1,0),R,iChannel2);

   float3 dx = swi3(e,x,y,z)-swi3(w,x,y,z);
   float3 dy = swi3(n,x,y,z)-swi3(s,x,y,z);
   // THE FORCE HERE IS COMPUTED WITH THE GRADIENT
   // I DONT NEED ANY INFORMATION ABOUT NEIGHBORING PARTICLES
   float2 v = -Q.w*to_float2(dx.x,dy.x)-to_float2(dx.y,dy.y);
   swi2S(Q,x,y, swi2(Q,x,y) + clamp(SPEED*v/(1.0f+0.2f*_fabs(Q.w)),to_float2_s(-1),to_float2_s(1)));
   // init
   if (iFrame < 1 || Reset) {
        float2 u = U;
        U = _floor((u)/SEPARATION)*SEPARATION;
        if (U.x<0.55f*R.x&&U.x>0.45f*R.x) {
            U=_floor(u/SEPARATION*4.0f)*SEPARATION/4.0f;
            Q = to_float4(U.x,U.y,1,-0.5f);
        } else Q = to_float4(U.x,U.y,1,sign_f(U.x-0.5f*R.x)*(U.y>0.5f*R.y?-9.0f:2.0f));
   }


    //Textureblending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = D(U,R,iChannel3);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          swi2S(Q,x,y, _mix(swi2(Q,x,y),U,Blend1));      // Position

        if ((int)Modus&4)
          Q.z = _mix(Q.z, tex.x*BlendZ-BlendZ/2.0f, Blend1);

        if ((int)Modus&8)
        {  
          Q.w = _mix(Q.w, tex.x*BlendW-BlendW/2.0f, Blend1);
        }

        if ((int)Modus&16) //Clear
          Q = _mix(Q,to_float4_s(0.0f),Blend1);

      }
      
      if ((int)Modus&32) //Special
      {
        
        U=_floor(u/SEPARATION*4.0f)*SEPARATION/4.0f;
        
        if (tex.w > 0.0f)
        {
          if(SpecialInv)         
            Q = to_float4(U.x,U.y,SpecialValue,-0.5f);
          else
            Q = to_float4(U.x,U.y,SpecialValue,sign_f(U.x-0.5f*R.x)*(U.y>0.5f*R.y?-9.0f:2.0f));
        }
        else  
        {
          if(SpecialInv) 
            Q = to_float4(U.x,U.y,SpecialValue,sign_f(U.x-0.5f*R.x)*(U.y>0.5f*R.y?-9.0f:2.0f));
          else
            Q = to_float4(U.x,U.y,SpecialValue,-0.5f);
        }          
      }
    }


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


// Blur pass 1

__KERNEL__ void MolecularDanceFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

  U+=0.5f;
  Q = to_float4_s(0);
   for (float i = -BLUR_DEPTH ; i <= BLUR_DEPTH ; i+=1.0f) {
     float4 a = A(U+to_float2(i,0),R,iChannel0);
        float4 c = to_float4(a.w,1.0f,0,0)*smoothstep(1.0f+0.05f*_fabs(a.w),1.0f,length(U+to_float2(i,0)-swi2(a,x,y)));
        Q += c*sqrt_f4(FORCE_RANGE)/FORCE_RANGE*exp_f4(-i*i*0.5f/FORCE_RANGE);
   }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


// Blur pass 2

__KERNEL__ void MolecularDanceFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

  U+=0.5f;
  Q = to_float4_s(0);
   for (float i = -BLUR_DEPTH ; i <= BLUR_DEPTH ; i+=1.0f) {
     float4 c = B(U+to_float2(0,i),R,iChannel1);
        Q += c*sqrt_f4(FORCE_RANGE)/FORCE_RANGE*exp_f4(-i*i*0.5f/FORCE_RANGE);
   }
 float4 
        n = C(U+to_float2(0,1),R,iChannel2),
        e = C(U+to_float2(1,0),R,iChannel2),
        s = C(U+to_float2(0,-1),R,iChannel2),
        w = C(U+to_float2(-1,0),R,iChannel2);
   Q = C(U,R,iChannel2) + 0.5f*(Q-C(U,R,iChannel2));
   if (iMouse.z > 0.0f) swi2S(Q,x,y, swi2(Q,x,y) + to_float2_s(10.0f)*exp_f2(-1.0f*to_float2(0.01f,0.05f)*length(U-swi2(iMouse,x,y))))

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


//Gradient calculation for caustic

__KERNEL__ void MolecularDanceFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
   U+=0.5f;

   float4 a = A(U,R,iChannel0);
   float r=smoothstep(4.0f,1.0f,length(U-swi2(a,x,y)));
   Q = r*to_float4(a.w,_fabs(a.w),-a.w,1);
   Q = _fmaxf(Q,D(U,R,iChannel3));
   float 
        n = C(U+to_float2(0,1),R,iChannel2).x,
        e = C(U+to_float2(1,0),R,iChannel2).x,
        s = C(U+to_float2(0,-1),R,iChannel2).x,
        w = C(U+to_float2(-1,0),R,iChannel2).x;
   //swi2(Q,z,w) = to_float2(e-w,n-s);
   Q.z = e-w;
   Q.w = n-s;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Caustic Drawing and red and blue dot drawing
#define N 2

__DEVICE__ float ln (float3 p, float3 a, float3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}
__DEVICE__ float dI (float2 U, float3 me, float3 light, float mu, float2 R, __TEXTURE2D__ iChannel3) {

    float3 r = to_float3_aw(U,100);
    float3 n = normalize(to_float3_aw(swi2(D(swi2(r,x,y),R,iChannel3),z,w),mu));
    float3 li = reflect((r-light),n);
    float len = ln(me,r,li);
    return 5.e-1*_expf(-len);
}
__DEVICE__ float I (float2 U, float3 me, float3 light, float mu, float2 R, __TEXTURE2D__ iChannel3) {
    float intensity = 0.0f;
    for (int _x = -N; _x <= N; _x++)
        for (int _y = -N; _y <= N; _y++){
            float i = dI(U+to_float2(_x,_y),me,light,10.0f*mu, R, iChannel3);
            intensity += i*i;
        }
        return intensity;
}
__KERNEL__ void MolecularDanceFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    U+=0.5f;

    float3 light = to_float3_aw(0.5f*R,1e5);
    float3 me    = to_float3_aw(U,0);
    float4 a = A(U,R,iChannel0);
    float4 c = C(U,R,iChannel2);
    float l = I(U,me,light,1.0f, R, iChannel3);
    float r = smoothstep(2.0f+0.05f*_fabs(a.w),0.5f,length(U-swi2(a,x,y)));
    Q = l+r*to_float4(_fabs(sign_f(a.w)),-sign_f(a.w),-sign_f(a.w),1);

  SetFragmentShaderComputedColor(Q);
}