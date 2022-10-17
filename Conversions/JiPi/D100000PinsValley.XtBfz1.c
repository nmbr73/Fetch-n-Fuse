
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ float df(float3 p)
{
  float a = p.z * _cosf(p.z * 2.0f) * 0.001f;
  swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , to_mat2(_cosf(a),_sinf(a),-_sinf(a), _cosf(a))));
  p.y += _sinf(p.x * 0.5f + p.z * 0.5f);
  float sp = 10.0f - _fabs(p.y * 0.5f);
  float cy = _fmaxf(1.0f-0.2f*_sinf(p.z*0.5f)-_fabs(p.y*0.5f),length(mod_f2(swi2(p,x,z), 1.0f)-0.5f) - 0.2f);
  return _fminf(sp,cy);
}

__DEVICE__ float3 nor( float3 pos, float prec )
{
  float3 eps = to_float3( prec, 0.0f, 0.0f );
  float3 nor = to_float3(
      df(pos+swi3(eps,x,y,y)) - df(pos-swi3(eps,x,y,y)),
      df(pos+swi3(eps,y,x,y)) - df(pos-swi3(eps,y,x,y)),
      df(pos+swi3(eps,y,y,x)) - df(pos-swi3(eps,y,y,x)) );
  return normalize(nor);
}

__KERNEL__ void D100000PinsValleyFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  float2 g = fragCoord;
  float2 si = iResolution;
  float2 uv = (g+g-si)/si.y/0.8f;
  float3 ro = to_float3(0,0, (iTime + 30.0f) * 2.0f); 
  float3 cv = ro + to_float3(0,0,1); 
  float3 cu = normalize(to_float3(0,1,0));
  float3 z = normalize(cv-ro);
  float3 x = normalize(cross(cu,z));
  float3 y = cross(z,x);
  float fov = 0.9f;
  float3 rd = normalize(fov * (uv.x * x + uv.y * y) + z);
  
  float s = 1.0f, d = 0.0f;
  for (int i=0; i<150; i++) 
  {
    if (_logf(d*d/s/1e6)>0.0f) break;
    s=df(ro+rd*d);
    d += s * 0.15f;
  }
  
  float3 p = ro + rd * d;
  float3 lid = normalize(ro-p);
  float3 n = nor(p, 0.1f);
  float3 refl = reflect(rd,n);
  float diff = clamp( dot( n, lid ), 0.0f, 1.0f );
  float fre = _powf( clamp( 1.0f + dot(n,rd),0.0f,1.0f), 4.0f );
  float spe = _powf(clamp( dot( refl, lid ), 0.0f, 1.0f ),16.0f);
  float3 col = to_float3(0.8f,0.5f,0.2f);
    
  float sss = df(p - n*0.001f)/0.05f;
  
  float3 Color = _mix(
                      (diff * to_float3(0.8f,0.2f,0.5f) + fre + sss * to_float3(0.8f,0.5f,0.5f)) * 0.25f,
                      to_float3_s(spe * 0.5f), 
                      _expf(-0.01f * d*d));

  float2 q = g/si;
  fragColor = to_float4_aw( Color * (0.5f + 0.5f*_powf( 16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.55f )), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}