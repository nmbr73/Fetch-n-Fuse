
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// FLUID PART

//float2 ur, U;
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 tint (float2 v, int a, int b, float2 ur, __TEXTURE2D__ iChannel0) {return texture(iChannel0,fract_f2((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v, float2 ur, __TEXTURE2D__ iChannel0)                  {return texture(iChannel0,fract_f2(v/ur));}
__DEVICE__ float area (float2 a, float2 b, float2 c) {
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
__KERNEL__ void DragAPseudoFluidFuse__Buffer_A(float4 Co, float2 uu, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Startimpuls, 1);
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(BlendX_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(BlendY_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER3(BlendZ_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(BlendW_Thr, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER5(BlendC_Thr, -1.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, X,  Y, Z, W, C);

    uu+=0.5f; 
    float2 U = uu;
    float2 ur = iResolution;
    if (iFrame < 1 || Reset) {
        // INIT
        float w = 0.5f+_sinf(0.2f*U.x)*0.5f;
        float q = length(U-0.5f*ur);
        Co = to_float4(0.1f*_expf(-0.001f*q*q),0,0,w);
        if (Startimpuls == 0) Co.x = 0.0f;
        
    } else {
        
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        // ADVECT TO LEARN FROM THE PAST
        for (int i = 0; i < 8; i++) {
            v -= swi2(t(v,ur,iChannel0),x,y);
            A -= swi2(t(A,ur,iChannel0),x,y);
            B -= swi2(t(B,ur,iChannel0),x,y);
            C -= swi2(t(C,ur,iChannel0),x,y);
            D -= swi2(t(D,ur,iChannel0),x,y);
        }
        float4 me = tint(v,0,0,ur,iChannel0);
        float4 n = tint(v,0,1,ur,iChannel0),
            e = tint(v,1,0,ur,iChannel0),
            s = tint(v,0,-1,ur,iChannel0),
            w = tint(v,-1,0,ur,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(t(v,ur,iChannel0),ne,to_float4(0.1f,0.1f,0.9f,0.0f));
        me.z  = me.z  - 0.01f*((area(A,B,C)+area(B,C,D))-4.0f);
    
        // PRESSURE GRADIENT
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + 100.0f*to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        
        // MOUSE MOVEMENT
        float4 mouse = texture(iChannel1,to_float2_s(0.5f));
        float q = ln(U,swi2(mouse,x,y),swi2(mouse,z,w));
        float2 m = swi2(mouse,x,y)-swi2(mouse,z,w);
        float l = length(m);
        if (l>0.0f) m = _fminf(l,10.0f)*m/l;
        swi3S(me,x,y,w, swi3(me,x,y,w) + 0.03f*_expf(-5e-2*q*q*q)*to_float3_aw(m,10.0f));
        Co = me;
        
     
        //Textureblending
        if (Blend1 > 0.0f)
        {
          //float2 tuv = U/R;
          float4 tex = texture(iChannel2,uu/ur);

          if (tex.w > 0.0f)
          {
            //swi3S(Co,x,y,z, _mix(swi3(Co,x,y,z),(swi3(tex,x,y,z)*2.0f-0.5f)*Blend1_Thr, Blend1));       
            
            if ((int)Modus&2)
              Co.x = _mix(Co.x,(tex.x*2.0f-0.5f)*BlendX_Thr, Blend1);

            if ((int)Modus&4)
              Co.y = _mix(Co.y,(tex.y*2.0f-0.5f)*BlendY_Thr, Blend1);
            
            if ((int)Modus&8)
              Co.z = _mix(Co.z,(tex.z*2.0f-0.5f)*BlendZ_Thr, Blend1);
            
            if ((int)Modus&16)
              Co.w = _mix(Co.w,(tex.x)*BlendW_Thr, Blend1); //*2.0f-0.5f
            
            if ((int)Modus&32)
              Co.w = _mix(Co.w,0.0f+BlendC_Thr, Blend1);            
            
            
            //Q.w = 1.0;
            //V = _mix(V,swi2(tex,x,y)*Blend2_Thr,Blend1);
          }
        }   
       
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -0.4f, 0.4f));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


// MOUSE
__KERNEL__ void DragAPseudoFluidFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    fragCoord+=0.5f;
    float4 p = texture(iChannel0,fragCoord/iResolution);
    if (iMouse.z>0.0f) {
      if (p.z>0.0f) fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else          fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else fragColor = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


/*
  I changed this since I first posted it
  it now advects the velocity which makes it more realistic



  Hello thanks for looking at my shader
  buffer A has the fun stuff
  
  it works by taking a square around the pixel
  and advecting it through the velocity feild
  if the square gets bigger, pressure is decreasing
  and visa-versa

  kind of a contrived way of calculating divergence
  but hey look! it worked! kind of...
  
*/
__KERNEL__ void DragAPseudoFluidFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0)
{

    U+=0.5f;
    float4 g = texture(iChannel0,U/iResolution);
    float2 d = to_float2(
                        texture(iChannel0,(U+to_float2(1,0))/iResolution).w-texture(iChannel0,(U-to_float2(1,0))/iResolution).w,
                        texture(iChannel0,(U+to_float2(0,1))/iResolution).w-texture(iChannel0,(U-to_float2(0,1))/iResolution).w
                        );
    float3 n = normalize(to_float3_aw(d,0.1f));
    float a = _acosf(dot(n,normalize(to_float3_s(1))))/3.141593f;
    C = to_float4_aw(
                    (0.5f+0.5f*sin_f3(2.0f*g.w*to_float3(1,2,3)))*(0.7f+0.5f*_powf(a,2.0f))
                   ,1);

  SetFragmentShaderComputedColor(C);
}