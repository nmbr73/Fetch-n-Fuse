
__DEVICE__ mat2 m(float a)
{
  float c=_cosf(a), s=_sinf(a);
  mat2 r=to_mat2(c,-s,s,c);
  return r;
}

__DEVICE__ float map(float3 p,float t)
{
    float2 px=to_float2(p.x,p.y);
    px = prod_float2_mat2(px, m(t*0.4f));
    px = prod_float2_mat2(px, m(t*0.3f));
    float3 q = p*2.0f+t;
    p.x=px.x; p.y=px.y;
    return length_float3( p + to_float3_s(sin(t*0.7f)) )*log(length(p)+1.0f) + sin(q.x+sin(q.z+sin(q.y)))*5.5f - 1.0f;
}


__KERNEL__ void CrazynessKernel(
    __CONSTANTREF__ Params*  params,
    __TEXTURE2D__            iChannel0,
    __TEXTURE2D_WRITE__      dst
    )
{

  SHADER_PREAMBLE;

  float red          = params->r;
  float green        = params->g;
  float blue         = params->b;
  float alpha        = params->a;

// FORKED FROM Ether by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/MsjSW3


	float2 p = swixy(fragCoord)/iResolution.y - to_float2(0.9f,0.5f);
  float3 cl = to_float3_s(0.0f);
  float d = 0.9f;

  for(int i=0; i<=5; i++)
  {

    float3 px = to_float3(0.0f,0.0f,5.0f) + normalize(to_float3_aw(p, -1.0f))*d;
    float rz = map(px,iTime);
    float f =  clamp((rz - map(px+0.1f,iTime))*0.5f, -0.1f, 1.0f );
//  float3 l = to_float3(0.1f,0.3f,0.4f) + to_float3(5.0f, 2.5f, 3.0f)*f;
    float3 l = to_float3(red,green,blue) + to_float3(5.0f, 2.5f, 3.0f)*f;
    cl = cl*l + (1.0f-smoothstep(0.0f, 2.5f, rz))*0.7f*l;
    d += min(rz, 1.0f);
	}
  fragColor = to_float4_aw(cl, alpha);


  SHADER_EPILOGUE;


}
