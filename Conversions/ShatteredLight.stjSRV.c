

__DEVICE__ float sign_f(float value) { if (value == 0.0f) return 0.0f; return value > 0.0f ? 1.0f : -1.0f;}

__DEVICE__ float glowBomb(float2 uv, float2 A, float2 B) {

    float strokeWidth = 0.0f; //24. * 1./iResolution.x; // Scale by N units of the X resolution
    float2 pa = uv-A, ba = B-A;
//  float line = (length(pa-ba*clamp(dot(pa, ba)/dot(ba, ba),0.0f,1.0f)) - strokeWidth) * sign(dot( normalize( float2( ba.y, -ba.x ) ), pa ));
    float line =   (length(pa-ba*clamp(dot(pa, ba)/dot(ba, ba),0.0f,1.0f)) - strokeWidth)
                 * sign_f(dot( to_float2( ba.y, -ba.x ) , pa ));
    float lineSide = step(0.0f, line); // Same as: line < 0. ? 0. : 1.;

    return (1. - smoothstep( _fabs(line)/1.15f, 0.0f, 0.075f )) * lineSide;
//  return (1. - smoothstep( 0.0f, 0.075f, _fabs(line)/1.15f  )) * lineSide;

}



__KERNEL__ void Kernel(
    __CONSTANTREF__ Params*  params,
    __TEXTURE2D__            iChannel0,
    __TEXTURE2D_WRITE__      dst
    )
{

  // DEFINE_KERNEL_ITERATORS_XY(x, y);

  PROLOGUE;


  // _tex2DVec4Write(dst, x, y, to_float4(1.0f,0.0f,0.0f,1.0f));
  // return;


  float radius = 0.3f;
  float PI = 3.14159265358979f;


  float2 uv = (fragCoord -0.5f * iResolution)/iResolution.y;

  float angle = (iTime + 1.0f) * 0.08f;

  float2 A = to_float2(
        _sinf(PI * angle * 7.0f) * radius,
      - _sinf(PI * angle * 2.0f) * radius
  );

  float2 B = to_float2(
      - _sinf(PI * angle * 3.0f) * radius,
        _sinf(PI * angle * 2.0f) * radius
  );

  float2 C = to_float2(
      - _sinf(PI * angle * 5.0f) * radius,
        _sinf(PI * angle * 2.0f) * radius
  );

  float2 Z = to_float2(
      _cosf(PI * angle * 2.0f) * radius * 0.3f,
      _sinf(PI * angle * 4.0f) * radius * 0.3f
  );

  float3 col =
      glowBomb(uv, Z, A) * to_float3(0.25f, 0.55f, 1.00f)
    + glowBomb(uv, B, Z) * to_float3(0.75f, 1.00f, 0.88f)
    + glowBomb(uv, C, Z) * to_float3(1.00f, 0.65f, 0.30f);

  col *= 0.8f;

  //##########################################################################

  EPILOGUE(to_float4_aw(col,1.0f));
}
