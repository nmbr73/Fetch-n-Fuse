
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float2 hash( float2 p ) {
  p = to_float2( dot(p,to_float2(127.1f,311.7f)), dot(p,to_float2(269.5f,183.3f)) );
  return -1.0f + 2.0f*fract_f2(sin_f2(p)*43758.5453123f);
}

__DEVICE__ float simplexNoise( in float2 p ) {
    const float K1 = 0.366025404f; // (_sqrtf(3)-1)/2;
    const float K2 = 0.211324865f; // (3-_sqrtf(3))/6;

    float2  i = _floor( p + (p.x+p.y)*K1 );
    float2  a = p - i + (i.x+i.y)*K2;
    float m = step(a.y,a.x); 
    float2  o = to_float2(m,1.0f-m);
    float2  b = a - o + K2;
    float2  c = a - 1.0f + 2.0f*K2;
    float3  h = _fmaxf( to_float3_s(0.5f)-to_float3(dot(a,a), dot(b,b), dot(c,c) ), to_float3_s(0.0f) );
    float3  n = h*h*h*h*to_float3( dot(a,hash(i+0.0f)), dot(b,hash(i+o)), dot(c,hash(i+1.0f)));
    return dot( n, to_float3_s(70.0f) );
}

__DEVICE__ float3 fractalNoise(in float2 p) {
    float2 uv;
    float3 res;
    mat2 m = to_mat2( 1.6f,  1.2f, -1.2f,  1.6f );
    uv = p*5.0f + to_float2(10.76543f, 30.384756f);
    res.x  = 0.5000f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    res.x += 0.2500f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    res.x += 0.1250f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    res.x += 0.0625f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    uv = p*5.0f + to_float2(14.87443f, 508.12743f);
    res.y  = 0.5000f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    res.y += 0.2500f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    res.y += 0.1250f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    res.y += 0.0625f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    uv = p*5.0f + to_float2(83.21675f, 123.45678f);
    res.z  = 0.5000f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    res.z += 0.2500f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    res.z += 0.1250f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    res.z += 0.0625f*simplexNoise( uv ); uv = mul_mat2_f2(m,uv);
    return res;
}

__DEVICE__ float3 laplacian(in float2 uv, float2 R, __TEXTURE2D__ iChannel0) {
    float2 step = to_float2(1.0f/iResolution.y, 0.0f);
    return (
        swi3(texture(iChannel0, mod_f(uv + swi2(step,x,y),1.0f)),x,y,z) +
        swi3(texture(iChannel0, mod_f(uv - swi2(step,x,y),1.0f)),x,y,z) +
        swi3(texture(iChannel0, mod_f(uv + swi2(step,y,x),1.0f)),x,y,z) +
        swi3(texture(iChannel0, mod_f(uv - swi2(step,y,x),1.0f)),x,y,z) +
        -4.0f * swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z)
    ) / (step.x*step.x);
}

__KERNEL__ void Rps3ReaFuse__Buffer_A(float4 vals, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, int iFrame, float4 iDate, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER2(diffusionCoef, 0.0f, 1.0f, 0.00003f);
    CONNECT_SLIDER3(reactionCoef, 0.0f, 100.0f, 50.0f);
    CONNECT_SLIDER4(RHOdiv, -1.0f, 10.0f, 3.0f);
  
    fragCoord+=0.5f;

    //const float diffusionCoef = 3.0e-5;
    //const float reactionCoef = 50.0f;

    float2 uv = fragCoord/iResolution;
    if (iFrame==0 || Reset) {
        //swi3S(vals,x,y,z, clamp(fractalNoise(uv+hash(swi2(iDate,z,w))),0.0f,1.0f));
        swi3S(vals,x,y,z, clamp(fractalNoise(uv+hash(to_float2(iTime/60.0f,iTime))),0.0f,1.0f));
    } else {
        vals = _tex2DVecN(iChannel0,uv.x,uv.y,15);
        float rho = vals.x + vals.y + vals.z;
        swi3S(vals,x,y,z, swi3(vals,x,y,z) + iTimeDelta * (
                          diffusionCoef * swi3(laplacian(uv,R,iChannel0),x,y,z) +
                          swi3(vals,x,y,z) * (1.0f - rho/RHOdiv - reactionCoef*swi3(vals,y,z,x))) );
        swi3S(vals,x,y,z, clamp(swi3(vals,x,y,z), 0.0f, 1.0f));
    }

  SetFragmentShaderComputedColor(vals);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0

__KERNEL__ void Rps3ReaFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(c1, 0.8f, 0.3f, 0.3f, 1.0f);
    CONNECT_COLOR1(c2, 0.2f, 0.9f, 0.5f, 1.0f);
    CONNECT_COLOR2(c3, 0.95f, 0.65f, 0.25f, 1.0f);
    CONNECT_SLIDER0(Brightness1, 0.0f, 10.0f, 3.0f);
    CONNECT_SLIDER1(Brightness2, 0.0f, 1.0f, 0.4f);
    
    fragCoord+=0.5f;

    //const float3 c1 = to_float3(0.8f, 0.3f, 1.0f);
    //const float3 c2 = to_float3(0.2f, 0.9f, 0.5f);
    //const float3 c3 = to_float3(0.95f, 0.65f, 0.25f);   

    float3 rps = swi3(texture(iChannel0, fragCoord/iResolution),x,y,z);
    rps.x = 1.0f - _powf(1.0f - rps.x, Brightness1);
    rps.y = 1.0f - _powf(1.0f - rps.y, Brightness1);
    rps.z = 1.0f - _powf(1.0f - rps.z, Brightness1);
    fragColor = rps.x * c1 +
                rps.y * c2 +
                rps.z * c3;
               
    fragColor = _mix(sqrt_f4(fragColor),fragColor,Brightness2) ;
    //fragColor = texture(iChannel0, fragCoord/iResolution);

    fragColor.w = c1.w;

  SetFragmentShaderComputedColor(fragColor);
}