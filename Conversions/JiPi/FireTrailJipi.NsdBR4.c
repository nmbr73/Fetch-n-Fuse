
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define acc 16384.0f
__DEVICE__ float projDist(float2 p, float2 a, float2 b){

    float2 pa = p - a;
    float2 ba = b - a;
    
    float2 t = clamp(dot(pa,ba)/dot(ba,ba),0.0f,1.0f) * ba - pa;
    return length(t);
}


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


__KERNEL__ void FireTrailJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
	CONNECT_CHECKBOX0(Reset, 0);
	
	//Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
	
	fragCoord+=0.5f;

    
    float2 texUV = fragCoord/iResolution;
    float2 uv = fragCoord/iResolution.y;
    float2 m = swi2(iMouse,x,y)/ iResolution.y;
    
	//if (iMouse.z == 0.0f) m = to_float2_s(0.0f);
	
    texUV.y -= 0.003f;
    
    float4 read = _tex2DVecN(iChannel0,texUV.x,texUV.y,15);
    float2 prev = to_float2((float)(((int)(read.w ) & 0xffff0000) >> 16 )/ acc,
                            (float)((int)(read.w ) & 0xffff)/ acc );
    //swi3(read,x,y,z) *= 0.96f;
	read.x *= 0.96f;
	read.y *= 0.96f;
	read.z *= 0.96f;
    
    float write = (float)((((int)(m.x * acc) & 0xffff) << 16 ) |
                          (((int)(m.y * acc) & 0xffff)));
    
    float3 col = to_float3_s(0.0f);
    float size = projDist(uv,prev,m);
    
    col += 1.0f - smoothstep(-0.0f,0.11f,size);
    
    //swi3(read,x,y,z) += col;
	read.x += col.x;
	read.y += col.y;
	read.z += col.z;
	
    fragColor = to_float4_aw(swi3(read,x,y,z), write);
	
	if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);

	if(iFrame < 1 || Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define scale 10.0f 
__DEVICE__ float2 rand22(float2 seed)
{
  return fract_f2(to_float2(_sinf(dot(seed,to_float2(12.788f,72.133f))),_sinf(dot(seed,to_float2(12.788f,72.133f)))) * 522734.567f);
}

__DEVICE__ float voronoise(float2 uv)
{
	
    float2 f = fract_f2(uv);
    f -= 0.5f;
    float2 i = _floor(uv);
    float dist = distance_f2(f,rand22(i) - 0.5f);
    for(int x=-1; x<=1; x++)
    {
        for(int y=-1; y<=1; y++)
        {
            float2 p = i + to_float2(x,y);
            float nDist = distance_f2(f,rand22(p) + to_float2(x,y)  - 0.5f);
            if(nDist < dist){
                dist = nDist;
            }
        }
    }
    return dist;
}
__KERNEL__ void FireTrailJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{


    float2 uv = (fragCoord - 0.5f * to_float2(iResolution.x,0.0f))/iResolution.y;
    float2 unscaledUV = (fragCoord - 0.5f * to_float2(iResolution.x,0.0f))/iResolution.y;
    float2 texUV = fragCoord/iResolution;
    float2 m = (swi2(iMouse,x,y) - 0.5f * to_float2(iResolution.x,0.0f))/iResolution.y;
    uv *= scale;
    float t = iTime * 0.7f;
    float f = 0.0f;
    //fire noise
    f += voronoise(uv * to_float2(3.0f,1.0f) - (to_float2(-2.5f,1.0f) * t));
    f += voronoise((uv + to_float2(13.1312f, 1555.23f)) * to_float2(3.0f,1.0f) - (to_float2(2.5f,1.0f) * t));
    f = _sqrtf(f + 0.1f);
    f *= voronoise(uv - (to_float2(0.0f,5.0f) * t))/2.0f;
    
    //fire mask
    f *= _powf(_tex2DVecN(iChannel0,texUV.x,texUV.y,15).x,0.8f);
    //fill circle in center of fire
    f += clamp(1.0f - distance_f2(unscaledUV, m)*16.0f,0.0f,1.0f);
    
    //color fire
    float3 col = to_float3_s(0.0f);
    col = _mix(to_float3(1.0f,5.0f,2.0f), to_float3(1.0f,1.0f,1.0f), step(0.96f,f));
    col = _mix(to_float3(1.0f,0.15f,0.15f), col,smoothstep(0.1f,0.9f,f));
    col = _mix(to_float3_s(0.0f), col, step(0.1f,f));
    
    // Output to screen
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}