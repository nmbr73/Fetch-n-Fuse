
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution

//#define radius 0.43f
//#define emitSize 4.0f
//#define force 1.0f
//#define constraint 0.01f
//#define effect 1.0f

//#define emit(v,s,f) if (length(g-(v)) < emitSize) swi2(res,x,y) = swi2(res,x,y) * (1.0f - f) + f * (s), res.w = 1.0
#define emit(v,s,f) if (length(g-(v)) < emitSize) res.x = res.x * (1.0f - f) + f * (s.x), res.y = res.y * (1.0f - f) + f * (s.y), res.w = 1.0
#define rot(a) to_float2(_cosf(radians(a)),_sinf(radians(a)))


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
          swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}



__DEVICE__ float4 calc(__TEXTURE2D__ sam, float2 g, float2 s, int i)
{
  float4 a = texture(sam, (g+to_float2(1,0))/s);
  float4 b = texture(sam, (g+to_float2(0,1))/s);
  float4 c = texture(sam, (g+to_float2(-1,0))/s);
  float4 d = texture(sam, (g+to_float2(0,-1))/s);
  float4 res = texture(sam, (g-swi2(texture(sam, g/s),x,y))/s);
  float2 gp = to_float2(a.z-c.z,b.z-d.z);
  swi3S(res,x,y,z, to_float3(
                        res.x + gp.x,
                        res.y + gp.y - res.w * 0.0001f,
                        (0.245f /*+ 0.005f * _fabs((g/s)*2.0f-1.0f)*/) * 
                            (a.z + b.z + c.z + d.z) - 0.05f * (c.x - a.x + d.y - b.y)));
    
  res.w += res.z * 0.01f;
    
  if (i < 1) res = to_float4_s(0);
  if (g.x < 1.0f || g.y < 1.0f || g.x > s.x - 1.0f || g.y > s.y - 1.0f) res.x*=0.0f,res.y*=0.0f;//swi2(res,x,y) *= 0.0f;
    

  //emit(s * 0.5f - to_float2(1.5f,1) * radius * _fminf(s.x,s.y), rot(45.0f),1.0f);
  //emit(s * 0.5f - to_float2(-1.5f,1) * radius * _fminf(s.x,s.y), rot(135.0f),0.25f);
  

    
  return res;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void ShadedFluid2Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);
  
  //Blending
  CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
  CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
  CONNECT_POINT2(Par1, 0.0f, 0.0f);

  
  CONNECT_POINT0(Source1, 1.5f, 1.0f);
  CONNECT_POINT1(Source2, -1.5f, 1.0f);
  CONNECT_SLIDER0(radius, 0.0f, 1.0f, 0.43f);
  CONNECT_SLIDER1(emitSize, 0.0f, 10.0f, 4.0f);

  fragCoord+=0.5f;
  int i = iFrame; if (Reset) i = 0;
  fragColor = calc(iChannel0, fragCoord, iResolution, i);

  if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/R, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R);
  
  
#define XXX
#ifdef XXX  
  float2 s = rot(45.0f);
  float f = 1.0f;
  
  if (length(fragCoord-(R * 0.5f - Source1 * radius * _fminf(R.x,R.y))) < emitSize) 
      fragColor.x = fragColor.x * (1.0f - f) + f * (s.x), fragColor.y = fragColor.y * (1.0f - f) + f * (s.y), fragColor.w = 1.0;
  
  s = rot(135.0f);
  f = 0.25f;
  if (length(fragCoord-(R * 0.5f - Source2 * radius * _fminf(R.x,R.y))) < emitSize) 
      fragColor.x = fragColor.x * (1.0f - f) + f * (s.x), fragColor.y = fragColor.y * (1.0f - f) + f * (s.y), fragColor.w = 1.0;
#endif

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void ShadedFluid2Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);
  
  CONNECT_POINT0(Source1, 1.5f, 1.0f);
  CONNECT_POINT1(Source2, -1.5f, 1.0f);
  CONNECT_SLIDER0(radius, 0.0f, 1.0f, 0.43f);
  CONNECT_SLIDER1(emitSize, 0.0f, 10.0f, 4.0f);

  fragCoord+=0.5f;
  fragColor = calc(iChannel0, fragCoord, iResolution, iFrame);
  

  float2 s = rot(45.0f);
  float f = 1.0f;
  
  if (length(fragCoord-(R * 0.5f - Source1 * radius * _fminf(R.x,R.y))) < emitSize) 
      fragColor.x = fragColor.x * (1.0f - f) + f * (s.x), fragColor.y = fragColor.y * (1.0f - f) + f * (s.y), fragColor.w = 1.0;
  
  s = rot(135.0f);
  f = 0.25f;
  if (length(fragCoord-(R * 0.5f - Source2 * radius * _fminf(R.x,R.y))) < emitSize) 
      fragColor.x = fragColor.x * (1.0f - f) + f * (s.x), fragColor.y = fragColor.y * (1.0f - f) + f * (s.y), fragColor.w = 1.0;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void ShadedFluid2Fuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
  
  CONNECT_POINT0(Source1, 1.5f, 1.0f);
  CONNECT_POINT1(Source2, -1.5f, 1.0f);
  CONNECT_SLIDER0(radius, 0.0f, 1.0f, 0.43f);
  CONNECT_SLIDER1(emitSize, 0.0f, 10.0f, 4.0f);
  
  fragCoord+=0.5f;
  fragColor = calc(iChannel0, fragCoord, iResolution, iFrame);

  float2 s = rot(45.0f);
  float f = 1.0f;
  
  if (length(fragCoord-(R * 0.5f - Source1 * radius * _fminf(R.x,R.y))) < emitSize) 
      fragColor.x = fragColor.x * (1.0f - f) + f * (s.x), fragColor.y = fragColor.y * (1.0f - f) + f * (s.y), fragColor.w = 1.0;
  
  s = rot(135.0f);
  f = 0.25f;
  if (length(fragCoord-(R * 0.5f - Source2 * radius * _fminf(R.x,R.y))) < emitSize) 
      fragColor.x = fragColor.x * (1.0f - f) + f * (s.x), fragColor.y = fragColor.y * (1.0f - f) + f * (s.y), fragColor.w = 1.0;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void ShadedFluid2Fuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);
  
  CONNECT_POINT0(Source1, 1.5f, 1.0f);
  CONNECT_POINT1(Source2, -1.5f, 1.0f);
  CONNECT_SLIDER0(radius, 0.0f, 1.0f, 0.43f);
  CONNECT_SLIDER1(emitSize, 0.0f, 10.0f, 4.0f);

  fragCoord+=0.5f;
  fragColor = calc(iChannel0, fragCoord, iResolution, iFrame);

  float2 s = rot(45.0f);
  float f = 1.0f;
  
  if (length(fragCoord-(R * 0.5f - Source1 * radius * _fminf(R.x,R.y))) < emitSize) 
      fragColor.x = fragColor.x * (1.0f - f) + f * (s.x), fragColor.y = fragColor.y * (1.0f - f) + f * (s.y), fragColor.w = 1.0;
  
  s = rot(135.0f);
  f = 0.25f;
  if (length(fragCoord-(R * 0.5f - Source2 * radius * _fminf(R.x,R.y))) < emitSize) 
      fragColor.x = fragColor.x * (1.0f - f) + f * (s.x), fragColor.y = fragColor.y * (1.0f - f) + f * (s.y), fragColor.w = 1.0;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void ShadedFluid2Fuse(float4 fragColor, float2 g, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);  
    
    g+=0.5f;  
      
    float2 s = iResolution;
    float cc = texture(iChannel0, g/s).w;
    float cc2 = texture(iChannel0, (g-1.0f)/s).w;
    fragColor = to_float4_s(1)*cc*cc*0.8f;
    fragColor += to_float4(0.7f, 0.4f, 0.2f,1)*_fmaxf(cc2*cc2*cc2 - cc*cc*cc, 0.0f)*iResolution.y*0.2f;
    
    if (Invers) fragColor = to_float4_s(1.0f) - fragColor;
    if (ApplyColor)
    {
      fragColor = (fragColor + (Color-0.5f))*fragColor.w;
      fragColor.w = Color.w;
    }

  SetFragmentShaderComputedColor(fragColor);
}