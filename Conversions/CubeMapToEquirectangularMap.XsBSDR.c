
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Cubemap: St Peters Basilica_0' to iChannel0


__KERNEL__ void CubeMapToEquirectangularMapFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 texCoord = fragCoord / iResolution;
    float2 thetaphi = ((texCoord * 2.0f) - to_float2_s(1.0f)) * to_float2(3.1415926535897932384626433832795f, 1.5707963267948966192313216916398f);
    float3 rayDirection = to_float3(_cosf(thetaphi.y) * _cosf(thetaphi.x), _sinf(thetaphi.y), _cosf(thetaphi.y) * _sinf(thetaphi.x));
  	//fragColor = texture(iChannel0, rayDirection);
    //fragColor = _tex2DVecN(iChannel0,rayDirection.x,rayDirection.y,15);
    fragColor = unfold_cube_f3(iChannel0,rayDirection);

    // for apply the equirectangular map like a cubemap:
    // rayDirection = normalize(rayDirection);
    // texture(uTexEnvMap, to_float2((_atan2f(rayDirection.z, rayDirection.x) / 6.283185307179586476925286766559f) + 0.5f, _acosf(rayDirection.y) / 3.1415926535897932384626433832795f));


  SetFragmentShaderComputedColor(fragColor);
}