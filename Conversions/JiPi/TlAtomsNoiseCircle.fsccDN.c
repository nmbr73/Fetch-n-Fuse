
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ mat2 r2d(float a) {
  float c = _cosf(a), s = _sinf(a);
  return to_mat2(c, s, -s, c );
}


__DEVICE__ float hash( float2 p )
{
    return fract( _sinf( dot(p, to_float2( 15.79f, 81.93f  ) ) * 45678.9123f ) );
}

__DEVICE__ float valueNoise( float2 p )
{

    float2 i = _floor( p );
    float2 f = fract_f2( p );
    f = f*f*(3.0f - 2.0f*f);
    
    float bottomOfGrid =    _mix( hash( i + to_float2( 0.0f, 0.0f ) ), hash( i + to_float2( 1.0f, 0.0f ) ), f.x );
    float topOfGrid =       _mix( hash( i + to_float2( 0.0f, 1.0f ) ), hash( i + to_float2( 1.0f, 1.0f ) ), f.x );

    float t = _mix( bottomOfGrid, topOfGrid, f.y );
    
    return t;
}

__DEVICE__ float fbm( float2 uv )
{
    float sum = 0.00f;
    float amp = 0.7f;
    
    for( int i = 0; i < 4; ++i )
    {
      sum += valueNoise( uv ) * amp;
      uv += uv * 1.2f;
      amp *= 0.4f;
    }
  
    return sum;
}
__KERNEL__ void TlAtomsNoiseCircleFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{


    float rotTime = _sinf(iTime);
    
    float3 color1 = to_float3(0.8f, 0.2f, 0.0f);
    float3 color2 = to_float3(0.0f, 0.2f, 0.8f);
    
    float2 uv = ( fragCoord -0.5f*iResolution )/iResolution.y;

    float3 destColor = to_float3(2.0f * rotTime, 0.0f, 0.5f);
    float f = 10.15f;
    float maxIt = 18.0f;
    float3 shape = to_float3_s(0.0f);
    for(float i = 0.0f; i < maxIt; i+=1.0f){
        float s = _sinf((iTime / 111.0f) + i * _cosf(iTime*0.02f+i)*0.05f+0.05f);
        float c = _cosf((iTime / 411.0f) + i * (_sinf(iTime*0.02f+i)*0.05f+0.05f));
        c += _sinf(iTime);
        f = (0.005f) / _fabs(length(uv / to_float2(c, s)) - 0.4f+fbm( uv*2.0f  ));
        f += _expf(-400.0f*distance_f2(uv, to_float2(c,s)*0.5f))*2.0f;
        // Mas Particulas
        f += _expf(-200.0f*distance_f2(uv, to_float2(c,s+fbm(uv))*-0.5f))*2.0f;
        // Circulito
        f += (0.008f+fbm( uv*0.01f)) / _fabs(length(uv/2.0f / to_float2(c/4.0f + _sinf(iTime*4.0f), s/4.0f)));
        f += fbm( uv * 20.5f )*0.05f;
        float idx = (float)(i)/ (float)(maxIt);
        idx = fract(idx*2.0f);
        float3 colorX = _mix(color1, color2,idx);
        shape += f * colorX;
        
        //uv += fbm( uv * 20.5f )*0.02f;
        uv = mul_f2_mat2(uv,r2d(iTime*0.1f + _cosf(i*50.0f)));
    }
    
    // float3 shape = to_float3_aw(destColor * f);
    // Activar modo falopa fuerte
    // shape = _sinf(shape*10.0f+time);
    fragColor = to_float4_aw(shape,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}