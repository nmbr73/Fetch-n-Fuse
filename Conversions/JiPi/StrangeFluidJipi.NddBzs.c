
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define pi   3.1415f
#define pi2  pi/2.0f

__DEVICE__ float random(float2 fragCoord)
{
  return fract(_sinf(dot(fragCoord, to_float2(12.9898f,78.233f))) * 43758.5453f);  
}

__DEVICE__ float4 get_pixel(float x_offset, float y_offset, float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  return texture(iChannel0, fragCoord / iResolution + (to_float2(x_offset, y_offset) / iResolution));
}

__DEVICE__ float step_simulation(float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  float val = get_pixel(0.0f, 0.0f, fragCoord,iResolution,iChannel0).x;

#ifdef ORG
    val += random(fragCoord)*val*0.15f; // errosion
    
	  	val = get_pixel(
    	sin(get_pixel(val, 0.0).r  - get_pixel(-val, 0.0) + pi).r  * val * 0.4, 
      cos(get_pixel(0.0, -val).r - get_pixel(0.0 , val) - pi2).r * val * 0.4
   	).r;
#endif
    
	float tmpA = (get_pixel(val, 0.0f, fragCoord,iResolution,iChannel0).x  - get_pixel(-val, 0.0f, fragCoord,iResolution,iChannel0).x + pi);
	float tmpB = (get_pixel(0.0f, -val, fragCoord,iResolution,iChannel0).x - get_pixel(0.0f , val, fragCoord,iResolution,iChannel0).x - pi2);
	
    val = get_pixel(
        _sinf(tmpA)  * val * 0.4f, 
        _cosf(tmpB)  * val * 0.4f,
        fragCoord,iResolution,iChannel0).x;
    
    val *= 1.0001f;
    
    return val;
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
          Q.x = _mix(Q.x,(tex.x+MulOff.y)*MulOff.x,Blend);
          

        if ((int)Modus&4)
          Q.x = _mix(Q.x,(Par.x*Par.y),Blend);
        
        
        if ((int)Modus&8)
          Q.x = _mix(Q.x,(tex.x+MulOff.y)*MulOff.x - ((tex.y+Par.x)*Par.y),Blend);
          

        if ((int)Modus&16) 
          Q.x = _mix(Q.x, ((tex.x+tex.y+tex.z)/3.0f+MulOff.y)*MulOff.x, Blend);
      }
      else
        if ((int)Modus&32) //Special
          Q.x = _mix(Q.x,(tex.x+MulOff.y)*MulOff.x, Blend);
    }
  
  return Q;
}



__KERNEL__ void StrangeFluidJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  
    CONNECT_CHECKBOX0(Reset, 0);
  
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
  
    fragCoord+=0.5f;
	
    float val = step_simulation(fragCoord,iResolution,iChannel0);
 
    if(iFrame == 0 || Reset)
        val = 
          random(fragCoord)*length(iResolution)/100.0f + 
          smoothstep(length(iResolution)/2.0f, 0.5f, length(iResolution * 0.5f - fragCoord))*25.0f;
    
    if (iMouse.z > 0.0f) 
        val += smoothstep(length(iResolution)/10.0f, 0.5f, length(swi2(iMouse,x,y) - fragCoord));
        
    fragColor.x = val;
    
    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void StrangeFluidJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float val = texture(iChannel0, fragCoord/iResolution).x;
 
     //{fragColor = to_float4(val,val,val,1.0f); return;}
    fragColor = to_float4(val, val, val, 1.0f);
        
    float4 color = pow_f4(to_float4(_cosf(val), _tanf(val), _sinf(val), 1.0f) * 0.5f + 0.5f, to_float4_s(0.5f));
    
    // code below taken from
    //https://www.shadertoy.com/view/Xsd3DB
    
    float2 q = fragCoord/iResolution;
    
    float3 e = to_float3_aw(to_float2_s(1.0f)/iResolution,0.0f);
    float p10 = texture(iChannel0, q-swi2(e,z,y)).x;
    float p01 = texture(iChannel0, q-swi2(e,x,z)).x;
    float p21 = texture(iChannel0, q+swi2(e,x,z)).x;
    float p12 = texture(iChannel0, q+swi2(e,z,y)).x;
        
    float3 grad = normalize(to_float3(p21 - p01, p12 - p10, 1.0f));
    float3 light = normalize(to_float3(0.2f,-0.25f,0.7f));
    float diffuse = dot(grad,light);
    float spec = _powf(_fmaxf(0.0f,-reflect(light,grad).z),32.0f);
    
    fragColor = (color * diffuse) + spec;

  SetFragmentShaderComputedColor(fragColor);
}