
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          Q.x = _mix( Q.x, Par.x , Blend);
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix( swi2(Q,x,y), to_float2_s(0.0f), Blend));

      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
        
    }
  
  return Q;
}


// FLUID PART

//float2 ur, U;
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 t (float2 v, int a, int b, float2 ur, __TEXTURE2D__ iChannel0) {return texture(iChannel0,fract_f2((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v, float2 ur, __TEXTURE2D__ iChannel0) {return texture(iChannel0,fract_f2(v/ur));}
__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
__KERNEL__ void IsThisRealisticFuse__Buffer_A(float4 Co, float2 uu, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 
    CONNECT_SLIDER3(Strength, -10.0f, 10.0f, 1.0f);
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    

    float2 U = uu+0.5f;
    float2 ur = iResolution;
    if (iFrame < 1 || U.x < 3.0f || Reset) {
        
        if (Textur)
        {
          float4 tex = texture(iChannel1, U/R);
          if (tex.w>0.0f)
          {
            Co = tex*Strength;
          }            
        }
        else
          Co = to_float4(0.1f,0,0,0);
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        float to = 0.0f;
        for (int i = 0; i < 2; i++) {
            float2 tmp = swi2(t(v,ur,iChannel0),x,y);
           
            v -= tmp;
        }
        for (int i = 0; i < 6; i++) {
            A -= swi2(t(A,ur,iChannel0),x,y);
            B -= swi2(t(B,ur,iChannel0),x,y);
            C -= swi2(t(C,ur,iChannel0),x,y);
            D -= swi2(t(D,ur,iChannel0),x,y);
        }
        float4 me = t(v,0,0,ur,iChannel0);
        float4 n = t(v,0,1,ur,iChannel0),
               e = t(v,1,0,ur,iChannel0),
               s = t(v,0,-1,ur,iChannel0),
               w = t(v,-1,0,ur,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = mix_f4(me,ne,to_float4(0.04f,0.04f,1,0.01f));
        me.w += 0.9f*(100.0f*to-me.w);
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        
        if(!Textur)
          if (length(U-to_float2(0.2f,0.5f)*ur)<10.0f||length(swi2(iMouse,x,y)-U)<5.0f) me.x*=0.0f,me.y*=0.0f,me.w*=0.0f;//swi3(me,x,y,w) *= 0.0f;
        
        //__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus)
        if (Blend1>0.0) me = Blending(iChannel1,U/ur, me, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus);
        
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -40.0f, 40.0f));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// FLUID PART

__KERNEL__ void IsThisRealisticFuse__Buffer_B(float4 Co, float2 uu, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 
    CONNECT_SLIDER3(Strength, -10.0f, 10.0f, 1.0f);

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

    float2 U = uu+0.5f;;
    float2 ur = iResolution;
    if (iFrame < 1 || U.x < 3.0f) {
        
        if (Textur)
        {
          float4 tex = texture(iChannel1, U/R);
          if (tex.w>0.0f)
          {
            Co = tex*Strength;
          }            
        }
        else
        
        Co = to_float4(0.1f,0,0,0);
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        float to = 0.0f;
        for (int i = 0; i < 2; i++) {
            float2 tmp = swi2(t(v,R,iChannel0),x,y);
           
            v -= tmp;
        }
        for (int i = 0; i < 6; i++) {
            A -= swi2(t(A,R,iChannel0),x,y);
            B -= swi2(t(B,R,iChannel0),x,y);
            C -= swi2(t(C,R,iChannel0),x,y);
            D -= swi2(t(D,R,iChannel0),x,y);
        }
        float4 me = t(v,0,0,R,iChannel0);
        float4 n = t(v,0,1,R,iChannel0),
               e = t(v,1,0,R,iChannel0),
               s = t(v,0,-1,R,iChannel0),
               w = t(v,-1,0,R,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = mix_f4(me,ne,to_float4(0.04f,0.04f,1,0.01f));
        me.w += 0.9f*(100.0f*to-me.w);
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        
        
        if (length(U-to_float2(0.2f,0.5f)*ur)<10.||length(swi2(iMouse,x,y)-U)<5.0f) me.x*=0.0f,me.y*=0.0f,me.w*=0.0f;//swi3(me,x,y,w) *= 0.0f;
        
        if (Blend1>0.0) me = Blending(iChannel1,U/ur, me, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus);
        
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -40.0f, 40.0f));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// FLUID PART
#ifdef XXX
float2 ur, U;
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 t (float2 v, int a, int b) {return texture(iChannel0,fract((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v) {return texture(iChannel0,fract(v/ur));}
__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
#endif
__KERNEL__ void IsThisRealisticFuse__Buffer_C(float4 Co, float2 uu, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 
    CONNECT_SLIDER3(Strength, -10.0f, 10.0f, 1.0f);
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);    

    float2 U = uu+0.5f;
    float2 ur = iResolution;
    if (iFrame < 1 || U.x < 3.0f) {
        
        if (Textur)
        {
          float4 tex = texture(iChannel1, U/R);
          if (tex.w>0.0f)
          {
            Co = tex*Strength;
          }            
        }
        else        
        
        Co = to_float4(0.1f,0,0,0);
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        float to = 0.0f;
        for (int i = 0; i < 2; i++) {
            float2 tmp = swi2(t(v,R,iChannel0),x,y);
           
            v -= tmp;
        }
        for (int i = 0; i < 6; i++) {
            A -= swi2(t(A,R,iChannel0),x,y);
            B -= swi2(t(B,R,iChannel0),x,y);
            C -= swi2(t(C,R,iChannel0),x,y);
            D -= swi2(t(D,R,iChannel0),x,y);
        }
        float4 me = t(v,0,0,R,iChannel0);
        float4 n = t(v,0,1,R,iChannel0),
               e = t(v,1,0,R,iChannel0),
               s = t(v,0,-1,R,iChannel0),
               w = t(v,-1,0,R,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(me,ne,to_float4(0.04f,0.04f,1,0.01f));
        me.w += 0.9f*(100.0f*to-me.w);
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        
        
        if (length(U-to_float2(0.2f,0.5f)*ur)<10.||length(swi2(iMouse,x,y)-U)<5.0f) me.x*=0.0f,me.y*=0.0f,me.w*=0.0f;//swi3(me,x,y,w) *= 0.0f;
        
        if (Blend1>0.0) me = Blending(iChannel1,U/ur, me, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus);
        
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -40.0f, 40.0f));
    }


  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// FLUID PART
#ifdef XXX
float2 ur, U;
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float4 t (float2 v, int a, int b) {return texture(iChannel0,fract((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v) {return texture(iChannel0,fract(v/ur));}
__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
#endif

__KERNEL__ void IsThisRealisticFuse__Buffer_D(float4 Co, float2 uu, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 
    CONNECT_SLIDER3(Strength, -10.0f, 10.0f, 1.0f);
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);    

    float2 U = uu+0.5f;
    float2 ur = iResolution;
    if (iFrame < 1 || U.x < 3.0f) {
        
        if (Textur)
        {
          float4 tex = texture(iChannel1, U/R);
          if (tex.w>0.0f)
          {
            Co = tex*Strength;
          }            
        }
        else        
        
        Co = to_float4(0.1f,0,0,0);
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        float to = 0.0f;
        for (int i = 0; i < 2; i++) {
            float2 tmp = swi2(t(v,R,iChannel0),x,y);
           
            v -= tmp;
        }
        for (int i = 0; i < 6; i++) {
            A -= swi2(t(A,R,iChannel0),x,y);
            B -= swi2(t(B,R,iChannel0),x,y);
            C -= swi2(t(C,R,iChannel0),x,y);
            D -= swi2(t(D,R,iChannel0),x,y);
        }
        float4 me = t(v,0,0,R,iChannel0);
        float4 n = t(v,0,1,R,iChannel0),
               e = t(v,1,0,R,iChannel0),
               s = t(v,0,-1,R,iChannel0),
               w = t(v,-1,0,R,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(me,ne,to_float4(0.04f,0.04f,1,0.01f));
        me.w += 0.9f*(100.0f*to-me.w);
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2(me,x,y) = swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        
        if (length(U-to_float2(0.2f,0.5f)*ur)<10.0f||length(swi2(iMouse,x,y)-U)<5.0f) me.x*=0.0f,me.y*=0.0f,me.w*=0.0f;//swi3(me,x,y,w) *= 0.0f;
        
        if (Blend1>0.0) me = Blending(iChannel1,U/ur, me, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus);
        
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -40.0f, 40.0f));
    }


  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float2 hash22(float2 p)
{//https://www.shadertoy.com/view/4djSRW
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y))*2.0f-1.0f;

}
__DEVICE__ float2 vf (float2 v, float2 R, __TEXTURE2D__ iChannel0) {
  return swi2(_tex2DVecN(iChannel0,v.x,v.y,15),x,y)-to_float2(0.1f,0);
}
#ifdef XXX
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
#endif
__DEVICE__ float ff (float2 U, float2 o, float2 R, __TEXTURE2D__ iChannel0) {
    float q = 0.25f*iResolution.x;
    float2 V = _floor(U*q+0.5f + o)/q;
    V += 0.1f*hash22(_floor(V*iResolution))/q;
    
    float2 v;
    v = vf(V,R,iChannel0);
    float a = 1e3;

    for (int i = 0; i < 3; i++) {
        v = 0.5f*vf(V,R,iChannel0);
        a = _fminf(a,(float)(1+i)*ln(U, V, V+v));
        V += v;
    }
    
    return _fmaxf(1.0f-iResolution.x*0.4f*a,0.0f);
}
__KERNEL__ void IsThisRealisticFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0)
{
    U+=0.5f;
  
    U = U/iResolution;
  
    float c = 0.0f;
    for (int _x = -2; _x <= 2; _x++) {
    for (int _y = -2; _y <= 2; _y++) {
        c += 0.3f*ff(U,to_float2(_x,_y),R,iChannel0);
    }
    }
    
    
    float4 g = _tex2DVecN(iChannel0,U.x,U.y,15);
    //swi3(C,x,y,z) = to_float3(c);
    C.x = c;
    C.y = c;
    C.z = c;

  SetFragmentShaderComputedColor(C);
}