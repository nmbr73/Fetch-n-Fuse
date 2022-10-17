
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
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
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}


__DEVICE__ float2 hash23(float3 p3)
{//https://www.shadertoy.com/view/4djSRW
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

__DEVICE__ float4 t (float2 v, int a, int b, float2 ur, __TEXTURE2D__ iChannel0) {return texture(iChannel0,((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v, float2 ur, __TEXTURE2D__ iChannel0) {return texture(iChannel0,(v/ur));}
__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
__KERNEL__ void MouseableairfoilJipiFuse__Buffer_A(float4 Co, float2 uu, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(TexWall, 0);
    CONNECT_CHECKBOX2(FoilOff, 0);
       
    CONNECT_COLOR0(MixMe, 0.06f, 0.06f, 1.0f, 0.0f);
    CONNECT_SLIDER5(M, -1.0f, 10.0f, 1.0f);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

    uu+=0.5f;

    float2 U = uu;
    float2 ur = iResolution;
    
    if (iFrame < 1||U.x < 3.0f||ur.x-U.x < 3.0f || Reset) {
        Co = to_float4(0.1f,0,0,0);
     
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        for (int i = 0; i < 2; i++) {
            float2 tmp = swi2(t(v,ur,iChannel0),x,y);
            v -= tmp;
        }
        float4 me = t(v,ur,iChannel0);
        for (int i = 0; i < 3; i++) {
            float2 tmp = swi2(t(v,ur,iChannel0),x,y);
            v -= tmp;
        }
        swi2S(me,z,w, swi2( t(v,ur,iChannel0),z,w));
        for (int i = 0; i < 9; i++) {
            A += swi2(t(A,ur,iChannel0),x,y);
            B += swi2(t(B,ur,iChannel0),x,y);
            C += swi2(t(C,ur,iChannel0),x,y);
            D += swi2(t(D,ur,iChannel0),x,y);
        }
        float4 n = t(v,0,1,ur,iChannel0),
               e = t(v,1,0,ur,iChannel0),
               s = t(v,0,-1,ur,iChannel0),
               w = t(v,-1,0,ur,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(me,ne,MixMe);//to_float4(0.06f,0.06f,1,0.0f));
        me.z  = me.z + (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
                
        float o = 0.0f, m=20.0f*M;
        float2 y = U/iResolution*m;
        y = fract_f2(y)*2.0f-1.0f+hash23(to_float3_aw(_floor(y),iFrame))*2.0f-1.0f;
        me.w = me.w + 0.5f*(1.0f+clamp(-0.2f*me.z*(me.z)*me.z,0.0f,2.0f))*smoothstep(0.004f,0.0f,length(y)/m);
                
        U = U-to_float2(0.4f,0.5f)*ur;
        float an = -iMouse.x/ur.x,
        co = _cosf(an), si = _sinf(an);
        swi2S(U,x,y, mul_mat2_f2(to_mat2(co,-si,si,co), swi2(U,x,y)));
        U.x*=0.125f;
        U.y += (iMouse.y/ur.y)*U.x*U.x;
        
        if(TexWall)
        {
          float tex = texture(iChannel1, (uu+0.5f)/iResolution).w; 
          if( tex > 0.0f)
            swi3S(me,x,y,w, to_float3_s(0.0f));
        }
        else
           if(FoilOff==false) swi3S(me,x,y,w, swi3(me,x,y,w) * step(6.0f,length(U)));
                
        Co = me;
        
        if (Blend1>0.0) Co = Blending(iChannel1, uu/iResolution, Co, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, uu, iResolution);
        
        
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -1.0f*to_float3(0.5f,0.5f,40.0f), to_float3(0.5f,0.5f,40.0f)));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Texture: Blending' to iChannel1

//vec2 ur, U;
//__DEVICE__ float4 t (float2 v, int a, int b) {return texture(iChannel0,((v+to_float2(a,b))/ur));}
//__DEVICE__ float4 t (float2 v) {return texture(iChannel0,(v/ur));}
//__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
//    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
//   return _sqrtf(s*(s-A)*(s-B)*(s-C));
//}
__KERNEL__ void MouseableairfoilJipiFuse__Buffer_B(float4 Co, float2 uu, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(TexWall, 0);
    CONNECT_CHECKBOX2(FoilOff, 0);
    
    float2 U = uu+0.5f;
    float2 ur = iResolution;
 
    if (iFrame < 1||U.x < 3.0f||ur.x-U.x < 3.0f || Reset) {
        Co = to_float4(0.1f,0,0,0);
     
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        for (int i = 0; i < 2; i++) {
            float2 tmp = swi2(t(v,ur,iChannel0),x,y);
            v -= tmp;
        }
        float4 me = t(v,ur,iChannel0);
        for (int i = 0; i < 3; i++) {
            float2 tmp = swi2(t(v,ur,iChannel0),x,y);
            v -= tmp;
        }
        swi2S(me,z,w, swi2(t(v,ur,iChannel0),z,w));
        for (int i = 0; i < 9; i++) {
            A += swi2(t(A,ur,iChannel0),x,y);
            B += swi2(t(B,ur,iChannel0),x,y);
            C += swi2(t(C,ur,iChannel0),x,y);
            D += swi2(t(D,ur,iChannel0),x,y);
        }
        float4 n = t(v,0,1,ur,iChannel0),
            e = t(v,1,0,ur,iChannel0),
            s = t(v,0,-1,ur,iChannel0),
            w = t(v,-1,0,ur,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(me,ne,to_float4(0.06f,0.06f,1,0.0f));
        me.z  = me.z + (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        
        U = U-to_float2(0.4f,0.5f)*ur;
        float an = -iMouse.x/ur.x,
        co = _cosf(an), si = _sinf(an);
        swi2S(U,x,y, mul_mat2_f2(to_mat2(co,-si,si,co) , swi2(U,x,y)));
        U.x*=0.125f;
        U.y += (iMouse.y/ur.y)*U.x*U.x;
        
        if(TexWall)
        {
          float tex = texture(iChannel1, (uu+0.5f)/iResolution).w; 
          if( tex > 0.0f)
            swi3S(me,x,y,w, to_float3_s(0.0f));
        }
        else
           if(FoilOff==false) swi3S(me,x,y,w, swi3(me,x,y,w) * step(6.0f,length(U)));
         
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -1.0f*to_float3(0.5f,0.5f,40.0f), to_float3(0.5f,0.5f,40.0f)));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Texture: Blending' to iChannel1

//vec2 ur, U;
//__DEVICE__ float4 t (float2 v, int a, int b) {return texture(iChannel0,((v+to_float2(a,b))/ur));}
//__DEVICE__ float4 t (float2 v) {return texture(iChannel0,(v/ur));}
//__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
//    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
//    return _sqrtf(s*(s-A)*(s-B)*(s-C));
//}
__KERNEL__ void MouseableairfoilJipiFuse__Buffer_C(float4 Co, float2 uu, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(TexWall, 0);
    CONNECT_CHECKBOX2(FoilOff, 0);    

    float2 U = uu+0.5f;
    float2 ur = iResolution;


    if (iFrame < 1||U.x < 3.0f||ur.x-U.x < 3.0f || Reset) {
        Co = to_float4(0.1f,0,0,0);
     
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        for (int i = 0; i < 2; i++) {
            float2 tmp = swi2(t(v,ur,iChannel0),x,y);
            v -= tmp;
        }
        float4 me = t(v,ur,iChannel0);
        for (int i = 0; i < 3; i++) {
            float2 tmp = swi2(t(v,ur,iChannel0),x,y);
            v -= tmp;
        }
        swi2S(me,z,w, swi2(t(v,ur,iChannel0),z,w));
        for (int i = 0; i < 9; i++) {
            A += swi2(t(A,ur,iChannel0),x,y);
            B += swi2(t(B,ur,iChannel0),x,y);
            C += swi2(t(C,ur,iChannel0),x,y);
            D += swi2(t(D,ur,iChannel0),x,y);
        }
        float4 n = t(v,0,1,ur,iChannel0),
               e = t(v,1,0,ur,iChannel0),
               s = t(v,0,-1,ur,iChannel0),
               w = t(v,-1,0,ur,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(me,ne,to_float4(0.06f,0.06f,1,0.0f));
        me.z  = me.z + (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        
        U = U-to_float2(0.4f,0.5f)*ur;
        float an = -iMouse.x/ur.x,
        co = _cosf(an), si = _sinf(an);
        swi2S(U,x,y, mul_mat2_f2(to_mat2(co,-si,si,co) , swi2(U,x,y)));
        U.x*=0.125f;
        U.y += (iMouse.y/ur.y)*U.x*U.x;
        
        if(TexWall)
        {
          float tex = texture(iChannel1, (uu+0.5f)/iResolution).w; 
          if( tex > 0.0f)
            swi3S(me,x,y,w, to_float3_s(0.0f));
        }
        else
           if(FoilOff==false) swi3S(me,x,y,w, swi3(me,x,y,w) * step(6.0f,length(U)));
         
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -1.0f*to_float3(0.5f,0.5f,40.0f), to_float3(0.5f,0.5f,40.0f)));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Texture: Blending' to iChannel1

//vec2 ur, U;
//__DEVICE__ float4 t (float2 v, int a, int b) {return texture(iChannel0,((v+to_float2(a,b))/ur));}
//__DEVICE__ float4 t (float2 v) {return texture(iChannel0,(v/ur));}
//__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
//    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
//    return _sqrtf(s*(s-A)*(s-B)*(s-C));
//}
__KERNEL__ void MouseableairfoilJipiFuse__Buffer_D(float4 Co, float2 uu, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(TexWall, 0);
    CONNECT_CHECKBOX2(FoilOff, 0);    
    
    float2 U = uu+0.5f;
    float2 ur = iResolution;
    
    if (iFrame < 1||U.x < 3.0f||ur.x-U.x < 3.0f || Reset) {
        Co = to_float4(0.1f,0,0,0);
     
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        for (int i = 0; i < 2; i++) {
            float2 tmp = swi2(t(v,ur,iChannel0),x,y);
            v -= tmp;
        }
        float4 me = t(v,ur,iChannel0);
        for (int i = 0; i < 3; i++) {
            float2 tmp = swi2(t(v,ur,iChannel0),x,y);
            v -= tmp;
        }
        swi2(me,z,w) = swi2(t(v,ur,iChannel0),z,w);
        for (int i = 0; i < 9; i++) {
            A += swi2(t(A,ur,iChannel0),x,y);
            B += swi2(t(B,ur,iChannel0),x,y);
            C += swi2(t(C,ur,iChannel0),x,y);
            D += swi2(t(D,ur,iChannel0),x,y);
        }
        float4 n = t(v,0,1,ur,iChannel0),
               e = t(v,1,0,ur,iChannel0),
               s = t(v,0,-1,ur,iChannel0),
               w = t(v,-1,0,ur,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(me,ne,to_float4(0.06f,0.06f,1,0.0f));
        me.z  = me.z + (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        
        U = U-to_float2(0.4f,0.5f)*ur;
        float an = -iMouse.x/ur.x,
        co = _cosf(an), si = _sinf(an);
        swi2S(U,x,y, mul_mat2_f2( to_mat2(co,-si,si,co) , swi2(U,x,y)));
        U.x*=0.125f;
        U.y += (iMouse.y/ur.y)*U.x*U.x;
        
        if(TexWall)
        {
          float tex = texture(iChannel1, (uu+0.5f)/iResolution).w; 
          if( tex > 0.0f)
            swi3S(me,x,y,w, to_float3_s(0.0f));
        }
        else
           if(FoilOff==false) swi3S(me,x,y,w, swi3(me,x,y,w) * step(6.0f,length(U)));
         
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -1.0f*to_float3(0.5f,0.5f,40.0f), to_float3(0.5f,0.5f,40.0f)));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Blending' to iChannel1

__KERNEL__ void MouseableairfoilJipiFuse(float4 C, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(TexWall, 0);
    CONNECT_CHECKBOX2(FoilOff, 0);

    CONNECT_COLOR2(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER6(Level0, -1.0f, 1.0f, 0.0f);

    U+=0.5f;  

    float2 tuv = U/iResolution;

    float4 g = texture(iChannel0,U/iResolution);
    swi3S(C,x,y,z, to_float3_s(g.w));
    
    U = U-to_float2(0.4f,0.5f)*iResolution;
    float an = -iMouse.x/iResolution.x,
    co = _cosf(an), si = _sinf(an);
    swi2S(U,x,y, mul_mat2_f2( to_mat2(co,-si,si,co) , swi2(U,x,y)));
    U.x*=0.125f;
    U.y += (iMouse.y/iResolution.y)*U.x*U.x;
    
    if(TexWall)
    {
       float4 tex = texture(iChannel1,tuv);
       if ( tex.w > 0.0f ) 
         swi3S(C,x,y,z, swi3(tex,x,y,z));
    }
    else
      if(FoilOff==false) if (length(U)<6.0f) swi3S(C,x,y,z, to_float3(_sinf(iTime)*0.5f+0.5f,0.5f,1.0f));
    
    
    C += Color-0.5f;
    
    C.w = Color.w;
    
  SetFragmentShaderComputedColor(C);
}