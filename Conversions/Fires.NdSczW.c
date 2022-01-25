
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


//////////////////////
// Fire Flame shader

// procedural noise from IQ
__DEVICE__ float2 hash( float2 p )
{
  p = to_float2( dot(p,to_float2(127.1f,311.7f)),
       dot(p,to_float2(269.5f,183.3f)) );
  return -1.0f + 2.0f*fract(_sinf(p)*43758.5453123f);
}

__DEVICE__ float noise( in float2 p )
{
  const float K1 = 0.366025404f; // (_sqrtf(3)-1)/2;
  const float K2 = 0.211324865f; // (3-_sqrtf(3))/6;
  
  float2 i = _floor( p + (p.x+p.y)*K1 );
  
  float2 a = p - i + (i.x+i.y)*K2;
  float2 o = (a.x>a.y) ? to_float2(1.0f,0.0f) : to_float2(0.0f,1.0f);
  float2 b = a - o + K2;
  float2 c = a - 1.0f + 2.0f*K2;
  
  float3 h = _fmaxf( 0.5f-to_float3_aw(dot(a,a), dot(b,b), dot(c,c) ), 0.0f );
  
  float3 n = h*h*h*h*to_float3_aw( dot(a,hash(i+0.0f)), dot(b,hash(i+o)), dot(c,hash(i+1.0f)));
  
  return dot( n, to_float3_s(70.0f) );
}

__DEVICE__ float fbm(float2 uv)
{
  float f;
  mat2 m = mat2( 1.6f,  1.2f, -1.2f,  1.6f );
  f  = 0.5000f*noise( uv ); uv = m*uv;
  f += 0.2500f*noise( uv ); uv = m*uv;
  f += 0.1250f*noise( uv ); uv = m*uv;
  f += 0.0625f*noise( uv ); uv = m*uv;
  f = 0.5f + 0.5f*f;
  return f;
}

// no defines, standard redish flames
//#define BLUE_FLAME
//#define GREEN_FLAME

__KERNEL__ void FiresFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  float2 uv = fragCoord / iResolution;
  float2 q = uv;
  q.x *= 5.0f;
  q.y *= 2.0f;
  float strength = _floor(q.x+1.0f);
  float T3 = _fmaxf(3.0f,1.25f*strength)*iTime;
  q.x = mod_f(q.x,1.0f)-0.5f;
  q.y -= 0.25f;
  float n = fbm(strength*q - to_float2(0,T3));
  float c = 1.0f - 16.0f * _powf( _fmaxf( 0.0f, length(q*to_float2(1.8f+q.y*1.5f,0.75f) ) - n * _fmaxf( 0.0f, q.y+0.25f ) ),1.2f );
//  float c1 = n * c * (1.5f-_powf(1.25f*uv.y,4.0f));
  float c1 = n * c * (1.5f-_powf(2.50f*uv.y,4.0f));
  c1=clamp(c1,0.0f,1.0f);

  float3 col = to_float3(1.5f*c1, 1.5f*c1*c1*c1, c1*c1*c1*c1*c1*c1);
  
#ifdef BLUE_FLAME
  col = swi3(col,z,y,x);
#endif
#ifdef GREEN_FLAME
  col = 0.85f*swi3(col,y,x,z);
#endif
  
  float a = c * (1.0f-_powf(uv.y,3.0f));
  fragColor = to_float4( _mix(to_float3_s(0.0f),col,a), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}