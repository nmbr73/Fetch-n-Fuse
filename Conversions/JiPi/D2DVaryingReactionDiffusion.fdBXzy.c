
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1
#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float stepAndOutputRNGFloat(inout uint *rngState)
{
  // Condensed version of pcg_output_rxs_m_xs_32_32, with simple conversion to floating-point [0,1].
  *rngState  = *rngState * 747796405u + 1u;
  uint word = ((*rngState >> ((*rngState >> 28) + 4u)) ^ *rngState) * 277803737u;
  word      = (word >> 22) ^ word;
  
  return (float)(word) / 4294967295.0f;
}


__DEVICE__ float2 Blending( __TEXTURE2D__ channel, float2 uv, float2 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 fragCoord, int iFrame)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2) //Standard XY
          
          Q = _mix(Q,(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend);
          
        if ((int)Modus&4) // Original
        {  
          uint rngState = ((uint)(iFrame) << 20) + ((uint)(fragCoord.x/16.0f) << 10) + (uint)(fragCoord.y/16.0f);
          Q = to_float2(stepAndOutputRNGFloat(&rngState), uv.x*stepAndOutputRNGFloat(&rngState));
        
          //Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        }
        
        if ((int)Modus&8) // XY mit separatem Parameter
          
          Q = _mix(Q,to_float2((tex.x+MulOff.y)*MulOff.x, (tex.y+Par.y)*Par.x),Blend);
          
        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float2(Par.x,Par.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}



__KERNEL__ void D2DVaryingReactionDiffusionFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Textur, 0);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;

    if(iFrame == 0 || Reset){
        // Reinitialize
        uint rngState = ((uint)(iFrame) << 20) + ((uint)(fragCoord.x/16.0f) << 10) + (uint)(fragCoord.y/16.0f);
        fragColor = to_float4(stepAndOutputRNGFloat(&rngState), uv.x*stepAndOutputRNGFloat(&rngState), 0.0f, 0.0f);
        
        if(Textur)
        {
          float4 tex = texture(iChannel1, uv);
          fragColor = to_float4(tex.x,tex.y,0.0f,0.0f);
        }
        
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    int2 iFC = to_int2_cfloat(fragCoord);
    //float2 ab = texelFetch(iChannel0, iFC, 0).rg;
    float2 ab = swi2(texture(iChannel0, (make_float2(iFC)+0.5f)/R),x,y);
    float weights[] = { 0.05f, 0.2f, 0.05f,
                        0.2f, -1.0f, 0.2f,
                        0.05f, 0.2f, 0.05f};
    float2 laplacian = ab * weights[4];
    for(int y = -1; y <= 1; y++){
        for(int x = -1; x <= 1; x++){
            if(x == 0 && y == 0) continue;
            //laplacian += weights[3*y+x+4] * texelFetch(iChannel0, iFC + to_int2(x, y), 0).rg;
            laplacian += weights[3*y+x+4] * swi2(texture(iChannel0, (make_float2(iFC + to_int2(x, y))+0.5f)/R),x,y);
        }
    }
    
    if(iMouse.z > 0.0f){
        uv = swi2(iMouse,x,y) / iResolution;
    }
    
    float high = _mix(0.4f+0.3f*(1.0f-_powf(1.0f-uv.x,3.0f)), 0.9f-(0.9f-0.57f)*uv.x, 1.0f-_powf(1.0f-uv.x, 2.0f));
    float low = _mix(0.2f*(1.0f-_powf(1.0f-uv.x,3.0f)), 0.74f-(0.74f-0.54f)*uv.x, 1.0f-_powf(1.0f-uv.x, 3.8f));
    float f = 0.1f * uv.x;
    float k = 0.1f * _mix(low, high, uv.y);
    
    float dt = 0.9f;
    
    // Update A and B
    ab = ab + dt*(
        to_float2(1.0f, 0.5f)*laplacian
        + to_float2(-1.0f, 1.0f) * ab.x * ab.y * ab.y
        + to_float2(f*(1.0f-ab.x), -(k+f)*ab.y));
        
    ab = clamp(ab, to_float2_s(0.0f), to_float2_s(1.0f));

    if (Blend1>0.0) ab = Blending(iChannel1, fragCoord/R, ab, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iFrame);
    
    fragColor = to_float4(ab.x, ab.y, 0.0f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define MAXZ (100.0f/iResolution.x)

__DEVICE__ float heightAtLod(float2 uv, float lod, float2 R, __TEXTURE2D__ iChannel0){
    return MAXZ*texture(iChannel0, uv).y;
}

#define NSTEPS 32
__DEVICE__ float visibleAmt(float3 o, float3 d, float2 R, __TEXTURE2D__ iChannel0){
  // Determine the t at which o + d t reaches the z=MAXZ plane
  float tMax = (MAXZ-o.z)/d.z;
  // Difference in t between sample points - note that we sample at halves:
  float dt = tMax/(float)(NSTEPS);
  
  float amt = 1.0f;
  for(int i = 0; i < NSTEPS; i++){
    float t = ((float)(i) + 0.5f)*dt;
    float3 p = o + d*t;
    float height = heightAtLod(swi2(p,x,y), 4.0f+_log2f(t), R, iChannel0);
    
    float insideness = height - p.z; // >spreadZ -> 0, -spreadZ -> 1
    float spreadZ = 0.2f * t;
    amt = _fminf(amt, 0.5f - 0.5f*insideness/spreadZ);
  }
  
  return _fmaxf(0.0f, amt);
}

__KERNEL__ void D2DVaryingReactionDiffusionFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    
    // Get the height at the pixel
    float z = heightAtLod(uv, 0.0f, R, iChannel0);
    float3 origin = to_float3_aw(uv, z);
    
    float3 visibility = to_float3_s(0.0f);
    float aspect = iResolution.x/iResolution.y;
    float3 L0 = normalize(to_float3_aw(normalize(to_float2(0.866f, 0.5f*aspect)), 1.0f));
    float3 L1 = normalize(to_float3_aw(normalize(to_float2(-0.866f, 0.5f*aspect)), 1.0f));
    float3 L2 = normalize(to_float3(0.0f, -1.0f, 1.0f));
    visibility.x = visibleAmt(origin, L0,R,iChannel0);
    visibility.y = visibleAmt(origin, L1,R,iChannel0);
    visibility.z = visibleAmt(origin, L2,R,iChannel0);
    
    // Estimate local curvature for an AO look
    float gaussianZEst = 0.25f*z + 0.25f*heightAtLod(uv, 2.0f,R,iChannel0) + 0.2f*heightAtLod(uv, 4.0f,R,iChannel0) + 0.15f*heightAtLod(uv, 6.0f,R,iChannel0) + 0.1f*heightAtLod(uv, 8.0f,R,iChannel0);
    float aoEst = clamp(1.0f + 3.0f*(z - gaussianZEst)/MAXZ, 0.0f, 1.0f);
    
    // Estimate normal for some specularity
    float3 drez = to_float3_aw(1.0f/iResolution, 0.0f);
    float3 n = to_float3(-(heightAtLod(uv+swi2(drez,x,z), 0.0f,R,iChannel0)-z)*iResolution.x, -(heightAtLod(uv+swi2(drez,z,y), 0.0f,R,iChannel0)-z)*iResolution.y, 1.0f);
    n = normalize(n);

    float3 diffuse = _fmaxf(to_float3(dot(n, L0), dot(n, L1), dot(n, L2)), to_float3_s(0.0f)) * aoEst * visibility;
    
    float3 halfVec = normalize(n+to_float3(0.0f,0.0f,1.0f));
    float3 spec = _fmaxf(to_float3(dot(halfVec, L0), dot(halfVec, L1), dot(halfVec, L2)), to_float3_s(0.0f));
    spec = 17.0f * pow_f3(spec, to_float3_s(128.0f)) * visibility;
    
    float3 col = _mix(diffuse, spec, 0.05f);
    
    // Output to screen
    fragColor = to_float4_aw(pow_f3(col * aoEst, to_float3_s(1.0f/2.2f)), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}