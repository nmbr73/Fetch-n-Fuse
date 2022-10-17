
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ mat2 r2d(float a) {
  float c = _cosf(a), s = _sinf(a);
    return to_mat2(c, s,-s, c);
}
__DEVICE__ float hash( float2 p )
{
    return fract( _sinf( dot(p, to_float2( 15.79f, 81.93f  ) ) * 45678.9123f ) );
}

__DEVICE__ float valueNoise( float2 p )
{
    float2 i = _floor( p );
    float2 f = fract( p );
    
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

__KERNEL__ void TlNoiseVortexJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    CONNECT_COLOR0(Color1, 0.8f, 0.2f, 0.0f, 1.0f);
    CONNECT_COLOR1(Color2, 0.0f, 0.2f, 0.8f, 1.0f);
    CONNECT_COLOR2(DestColor, 2.0f, 0.0f, 0.5f, 1.0f);

    //CONNECT_SLIDER0(f, -1.0f, 20.0f, 10.15f);
    CONNECT_SLIDER1(maxIt, -1.0f, 30.0f, 18.0f);
    
    CONNECT_SLIDER2(Par1, -1.0f, 0.03f, 0.005f);
    CONNECT_SLIDER3(Par2, -1.0f, 3.0f, 0.4f);
    CONNECT_SLIDER4(Par3, 0.0f, 1000.0f, 400.0f);
    CONNECT_SLIDER5(Par4, -1.0f, 50.0f, 2.0f);
    
    CONNECT_SLIDER6(Particulas1, 0.0f, 1000.0f, 200.0f);
    CONNECT_SLIDER7(Particulas2, -1.0f, 50.0f, 2.0f);
    
    CONNECT_SLIDER8(Circulito1, -1.0f, 0.1f, 0.008f);
    CONNECT_SLIDER9(Circulito2, -10.0f, 30.0f, 4.0f);
    CONNECT_SLIDER10(Circulito3, -1.0f, 10.0f, 4.0f);
    CONNECT_SLIDER11(Circulito4, -1.0f, 10.0f, 4.0f);
    
    CONNECT_SLIDER12(FBM1, -1.0f, 50.0f, 20.5f);
    CONNECT_SLIDER13(FBM2, -1.0f, 1.0f, 0.05f);
    CONNECT_SLIDER14(FBM3, -1.0f, 50.0f, 20.5f);
    CONNECT_SLIDER15(FBM4, -1.0f, 1.0f, 0.02f);
     
    
    CONNECT_SLIDER16(FBM5, -1.0f, 1.0f, 0.1f);
    CONNECT_SLIDER17(FBM6, -1.0f, 100.0f, 50.0f);
    

    float time = iTime;
    float rotTime = _sinf(time);
    
    float3 color1 = swi3(Color1,x,y,z);//to_float3(0.8f, 0.2f, 0.0f);
    float3 color2 = swi3(Color2,x,y,z);//to_float3(0.0f, 0.2f, 0.8f);
    
    float2 uv = ( fragCoord -0.5f*iResolution )/iResolution.y;

    float3 destColor = to_float3(DestColor.x * rotTime, DestColor.y, DestColor.z); //to_float3(2.0f * rotTime, 0.0f, 0.5f);
    float f = 10.15f;
    //float maxIt = 18.0f;
    float3 shape = to_float3_s(0.0f);
    for(float i = 0.0f; i < maxIt; i++){
        float s = _sinf((time / 111.0f) + i * _cosf(iTime*0.02f+i)*0.05f+0.05f);
        float c = _cosf((time / 411.0f) + i * (_sinf(time*0.02f+i)*0.05f+0.05f));
        c += _sinf(iTime);
        //f = (0.005f) / _fabs(length(uv / to_float2(c, s)) - 0.4f);
        f = (Par1) / _fabs(length(uv / to_float2(c, s)) - Par2);
        //f += _expf(-400.0f*distance_f2(uv, to_float2(c, s)*0.5f))*2.0f;
        f += _expf(-Par3*distance_f2(uv, to_float2(c, s)*0.5f))*Par4;
        // Mas Particulas
        //f += _expf(-200.0f*distance_f2(uv, to_float2(c, s)*-0.5f))*2.0f;
        f += _expf(-Particulas1*distance_f2(uv, to_float2(c, s)*-0.5f))*Particulas2;
        // Circulito
        //f += (0.008f) / _fabs(length(uv/2.0f / to_float2(c/4.0f + _sinf(time*4.0f), s/4.0f)));
        f += (Circulito1) / _fabs(length(uv/2.0f / to_float2(c/Circulito2 + _sinf(time*Circulito3), s/Circulito4)));
        //f += fbm( uv * 20.5f )*0.05f;
        f += fbm( uv * FBM1 )*FBM2;
        float idx = (float)(i)/ (float)(maxIt);
        idx = fract(idx*2.0f);
        float3 colorX = _mix(color1, color2,idx);
        shape += f * colorX;
        
        //uv += fbm( uv * 20.5f )*0.02f;
        uv += fbm( uv * FBM3 )*FBM4;
        //uv = mul_f2_mat2(uv, r2d(iTime*0.1f + _cosf(i*50.0f)*f));
        uv = mul_f2_mat2(uv, r2d(iTime*FBM5 + _cosf(i*FBM6)*f));
        
    }
    
    // float3 shape = to_float3_aw(destColor * f);
    // Activar modo falopa fuerte
    // shape = _sinf(shape*10.0f+time);
    fragColor = to_float4_aw(shape,Color1.w);

  SetFragmentShaderComputedColor(fragColor);
}
