
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Gray Noise Medium' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Noise animation - Electric
// by nimitz (stormoid.com) (twitter: @stormoid)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License
// Contact the author for other licensing options

//The domain is displaced by two fbm calls one for each axis.
//Turbulent fbm (aka ridged) is used for better effect.

#define time iTime*0.15f
#define tau 6.2831853f

__DEVICE__ mat2 makem2(in float theta){float c = _cosf(theta);float s = _sinf(theta);return to_mat2(c,-s,s,c);}
__DEVICE__ float noise( in float2 x, __TEXTURE2D__ iChannel0 ){return texture(iChannel0, x*0.01f).x;}

__DEVICE__ float fbm(in float2 p, __TEXTURE2D__ iChannel0)
{  
  float z=2.0f;
  float rz = 0.0f;
  float2 bp = p;
  for (float i= 1.0f;i < 6.0f;i+=1.0f)
  {
    rz+= _fabs((noise(p, iChannel0)-0.5f)*2.0f)/z;
    z = z*2.0f;
    p = p*2.0f;
  }
  return rz;
}

__DEVICE__ float dualfbm(in float2 p, float iTime, __TEXTURE2D__ iChannel0)
{
    //get two rotated fbm calls and displace the domain
  float2 p2 = p*0.7f;
  float2 basis = to_float2(fbm(p2-time*1.6f, iChannel0),fbm(p2+time*1.7f, iChannel0));
  basis = (basis-0.5f)*0.2f;
  p += basis;
  
  //coloring
  return fbm(mul_f2_mat2(p,makem2(time*0.2f)), iChannel0);
}

__DEVICE__ float circ(float2 p) 
{
  float r = length(p);
  r = _logf(sqrt(r));
  return _fabs(mod_f(r*4.0f,tau)-3.14f)*3.0f+0.2f;

}

__KERNEL__ void NoiseAnimationElectricJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

  //setup system
  float2 p = fragCoord / iResolution-0.5f;
  p.x *= iResolution.x/iResolution.y;
  p*=4.0f;
  
  float rz = dualfbm(p, iTime, iChannel0);
  
  //rings
  p /= _expf(mod_f(time*10.0f,3.14159f));
  rz *= _powf(_fabs((0.1f-circ(p))),0.9f);
  
  //final color
  float3 col = to_float3(0.2f,0.1f,0.4f)/rz;
  col = pow_f3(abs_f3(col),to_float3_s(0.99f));
  fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}