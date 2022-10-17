
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Noise animation - Flow
// 2014 by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/MdlXRS
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License
// Contact the author for other licensing options



//Somewhat inspired by the concepts behind "flow noise"
//every octave of noise is modulated separately
//with displacement using a rotated vector field

//normalization is used to created "swirls"
//usually not a good idea, depending on the type of noise
//you are going for.

//Sinus ridged fbm is used for better effect.

#define time iTime*0.1f
#define tau 6.2831853f
// details 0.0f - 1.0f
#define details 0.3f

__DEVICE__ mat2 makem2(in float theta){float c = _cosf(theta);float s = _sinf(theta);return to_mat2(c,-s,s,c);}
__DEVICE__ float hash(float2 p) {
    p  = 50.0f*fract_f2( p*0.3183099f + to_float2(0.71f,0.113f));
    return -1.0f+2.0f*fract( p.x*p.y*(p.x+p.y) );
}

__DEVICE__ float noise( in float2 x ) {
    float2 i = _floor( x );
    float2 f = fract_f2( x );
  
    float2 u = f*f*(3.0f-2.0f*f);

    return _mix( _mix( hash( i + to_float2(0.0f,0.0f) ), 
                       hash( i + to_float2(1.0f,0.0f) ), u.x),
                 _mix( hash( i + to_float2(0.0f,1.0f) ), 
                       hash( i + to_float2(1.0f,1.0f) ), u.x), u.y);
}


__DEVICE__ float3 hsb2rgb( in float3 c ){
    float3 rgb = clamp(abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f,
                       0.0f,
                       1.0f );
    rgb = rgb*rgb*(3.0f-2.0f*rgb);
    return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

__DEVICE__ float grid(float2 p)
{
  float s = _sinf(p.x)*_cosf(p.y);
  return s;
}

__DEVICE__ float flow(in float2 p,float iTime)
{
  mat2 m2 = to_mat2( 0.80f,  0.60f, -0.60f,  0.80f );
  
  float z=2.0f;
  float rz = 0.0f;
  float2 bp = p;
  for (float i= 1.0f;i < 7.0f;i++ )
  {
    bp += time*1.5f;
    float2 gr = to_float2(grid(p*3.0f-time*2.0f),grid(p*3.0f+4.0f-time*2.0f))*0.4f;
    gr = normalize(gr)*0.4f;
    gr = mul_f2_mat2(gr,makem2((p.x+p.y)*0.3f+time*10.0f));
    p += gr*0.5f;
    
    rz+= (_sinf(noise(p)*8.0f)*0.5f+0.5f) /z;
    
    p = _mix(bp,p,0.5f);
    z *= 1.7f;
    p *= 2.5f;
    p=mul_f2_mat2(p,m2);
    bp *= 2.5f;
    bp=mul_f2_mat2(bp,m2);
  }
  return rz;  
}

__DEVICE__ float spiral(float2 p,float scl) 
{
  float r = length(p);
  r = _logf(r);
  float a = _atan2f(p.y, p.x);
  return _fabs(mod_f(scl*(r-2.0f/scl*a),tau)-1.0f)*2.0f;
}

__KERNEL__ void RainbowSpiralJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  float2 p = fragCoord / iResolution-0.5f;
  float3 color = to_float3_s(1.0f);
        
        
  // rainbow part
  float angle = _atan2f(p.y,p.x)+iTime;
  color = hsb2rgb(to_float3((angle/tau)+0.5f,length(p)*2.0f,1.0f));

    
  p.x *= iResolution.x/iResolution.y;
  p*= 3.0f;
  float rz = flow(p,iTime) * details;
  p /= _expf(mod_f(time*9.0f,2.1f));
  rz += (6.0f-spiral(p,3.0f))*0.5f;

  float3 col = abs_f3(to_float3(0.2f,0.07f,0.01f)/rz);
    
  fragColor = to_float4_aw(col * color,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}