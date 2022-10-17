
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float3 mod289(float3 x) {
  return x - _floor(x * (1.0f / 289.0f)) * 289.0f;
}

__DEVICE__ float4 mod289_4(float4 x) {
  return x - _floor(x * (1.0f / 289.0f)) * 289.0f;
}

__DEVICE__ float4 permute(float4 x) {
     return mod289_4(((x*34.0f)+10.0f)*x);
}

__DEVICE__ float4 taylorInvSqrt(float4 r)
{
  return to_float4_s(1.79284291400159f) - 0.85373472095314f * r;
}

__DEVICE__ float snoise(float3 v)
{ 
  const float2  C = to_float2(1.0f/6.0f, 1.0f/3.0f) ;
  const float4  D = to_float4(0.0f, 0.5f, 1.0f, 2.0f);

  float3 i  = _floor(v + dot(v, swi3(C,y,y,y)) );
  float3 x0 =   v - i + dot(i, swi3(C,x,x,x)) ;

  float3 g = step(swi3(x0,y,z,x), swi3(x0,x,y,z));
  float3 l = 1.0f - g;
  float3 i1 = _fminf( swi3(g,x,y,z), swi3(l,z,x,y) );
  float3 i2 = _fmaxf( swi3(g,x,y,z), swi3(l,z,x,y) );

  float3 x1 = x0 - i1 + swi3(C,x,x,x);
  float3 x2 = x0 - i2 + swi3(C,y,y,y); // 2.0f*C.x = 1/3 = C.y
  float3 x3 = x0 - swi3(D,y,y,y);      // -1.0f+3.0f*C.x = -0.5f = -D.y

  i = mod289(i); 
  float4 p = permute( permute( permute( 
             i.z + to_float4(0.0f, i1.z, i2.z, 1.0f ))
           + i.y + to_float4(0.0f, i1.y, i2.y, 1.0f )) 
           + i.x + to_float4(0.0f, i1.x, i2.x, 1.0f ));

  float n_ = 0.142857142857f; // 1.0f/7.0
  float3  ns = n_ * swi3(D,w,y,z) - swi3(D,x,z,x);

  float4 j = p - 49.0f * _floor(p * ns.z * ns.z);  //  mod_f(p,7*7)

  float4 x_ = _floor(j * ns.z);
  float4 y_ = _floor(j - 7.0f * x_ );    // mod_f(j,N)

  float4 x = x_ *ns.x + swi4(ns,y,y,y,y);
  float4 y = y_ *ns.x + swi4(ns,y,y,y,y);
  float4 h = to_float4_s(1.0f) - abs_f4(x) - abs_f4(y);

  float4 b0 = to_float4_f2f2( swi2(x,x,y), swi2(y,x,y) );
  float4 b1 = to_float4_f2f2( swi2(x,z,w), swi2(y,z,w) );

  float4 s0 = _floor(b0)*2.0f + 1.0f;
  float4 s1 = _floor(b1)*2.0f + 1.0f;
  float4 sh = -1.0f*step(h, to_float4_s(0.0f));

  float4 a0 = swi4(b0,x,z,y,w) + swi4(s0,x,z,y,w)*swi4(sh,x,x,y,y) ;
  float4 a1 = swi4(b1,x,z,y,w) + swi4(s1,x,z,y,w)*swi4(sh,z,z,w,w) ;

  float3 p0 = to_float3_aw(swi2(a0,x,y),h.x);
  float3 p1 = to_float3_aw(swi2(a0,z,w),h.y);
  float3 p2 = to_float3_aw(swi2(a1,x,y),h.z);
  float3 p3 = to_float3_aw(swi2(a1,z,w),h.w);
float zzzzzzzzzzzzzzzzzz;
  float4 norm = taylorInvSqrt(to_float4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;
//zz=0;
  float4 m = _fmaxf(to_float4_s(0.5f) - to_float4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), to_float4_s(0.0f));
  m = m * m;
  return 105.0f * dot( m*m, to_float4( dot(p0,x0), dot(p1,x1), 
                                       dot(p2,x2), dot(p3,x3) ) );
}

__DEVICE__ float powcf(float x, float y) {
    float ret = _powf(x,y);
    if (isnan(ret)) {
        ret = 0.0001f;
    }
    return ret;
}

	__DEVICE__ float lpowf(float a, float b) {
		float la = _logf(a);
		float bl = b * la;		
		return _expf(bl);
	}

__DEVICE__ float circle(float2 center, float radius, float2 uv, float blur, inout float *dist)
{
    float2 offset = abs_f2(uv - center);
    *dist = _sqrtf(_powf(offset.x, 2.0f) + _powf(offset.y, 2.0f));
    return 1.0f - smoothstep(radius - blur / 2.0f, radius + blur / 2.0f, *dist);
}

__DEVICE__ float ring(float2 center, float radius, float thickness, float2 uv, float blur, inout float *dist)
{
    float c1 = circle(center, radius, uv, blur, dist);
    float c2 = circle(center, radius + thickness, uv, blur, dist);
    return (1.0f - c1) * c2;
}

__KERNEL__ void WellBottomFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
	
	CONNECT_COLOR0(Color, 1.0f, 1.0f, 1.0f, 1.0f);
  CONNECT_SLIDER0(blur, -1.0f, 5.0f, 1.0f);
	CONNECT_SLIDER1(Radius, -1.0f, 500.0f, 228.0f);
	CONNECT_SLIDER2(Thickness, -1.0f, 5.0f, 2.0f);
	
	CONNECT_CHECKBOX0(Tex, 0);
	CONNECT_SLIDER3(TexDist, -1.0f, 500.0f, 100.0f);
	float dist = 1.0f;

  float2 uv = fragCoord;
  float2 center = iResolution / 2.0f;
  float2 offset = to_float2(
								snoise(to_float3_aw(uv/64.0f, iTime)),
								snoise(to_float3_aw(-uv/64.0f, iTime))
							  ) * 32.0f;
  float3 col = ring(center, Radius,Thickness, uv + offset, blur, &dist) * swi3(Color,x,y,z);
  //float col = offset.x;
	
	if (Tex && dist < TexDist) fragColor = texture(iChannel0, fragCoord/iResolution);
  else                       fragColor = to_float4_aw(col, Color.w);

  SetFragmentShaderComputedColor(fragColor);
}