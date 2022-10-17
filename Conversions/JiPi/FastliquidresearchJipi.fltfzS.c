
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rock Tiles' to iChannel0
// Connect Image 'Texture: Rusty Metal' to iChannel1
// Connect Image '/presets/webcam.png' to iChannel2



__DEVICE__ float3 permute(in  float3 x) { return mod_f( x*x*34.0f+x, 289.0f); }

__DEVICE__ float snoise(in  float2 v) {
  float2 i = _floor((v.x+v.y)*0.36602540378443f + v),
  x0 = (i.x+i.y)*0.211324865405187f + v - i;
  float s = step(x0.x,x0.y);
  float2 j = to_float2(1.0f-s,s),
  x1 = x0 - j + 0.211324865405187f, 
  x3 = x0 - 0.577350269189626f; 
  i = mod_f(i,289.0f);
  float3 p = permute( permute( i.y + to_float3(0, j.y, 1 ))+ i.x + to_float3(0, j.x, 1 )   ),
  m = _fmaxf( 0.5f - to_float3(dot(x0,x0), dot(x1,x1), dot(x3,x3)), to_float3_s(0.0f)),
  x = fract_f3(p * 0.024390243902439f) * 2.0f - 1.0f,
  h = abs_f3(x) - 0.5f,
  a0 = x - _floor(x + 0.5f);
  return 0.5f + 65.0f * dot( pow_f3(m,to_float3_s(4.0f))*(- 0.85373472095314f*( a0*a0 + h*h )+1.79284291400159f ), a0 * to_float3(x0.x,x1.x,x3.x) + h * to_float3(x0.y,x1.y,x3.y));
}

__KERNEL__ void FastliquidresearchJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    CONNECT_SLIDER0(UVX1, -1.0f, 10.0f, 4.3f);
    CONNECT_SLIDER1(UVX2, -1.0f, 10.0f, 3.7f);
    CONNECT_SLIDER2(UVX3, -1.0f, 10.0f, 1.2f);
    

    CONNECT_POINT0(PosXY, 0.0f, 0.0f );
    
    CONNECT_POINT1(Area, 0.0f, 0.0f );
    
    CONNECT_CHECKBOX0(Test, 0);
    CONNECT_CHECKBOX1(Negativ, 0);

    float2 uv = fragCoord / iResolution;
    uv /= 1.1f;
    uv += 0.05f;
    uv *= 1.0f;
    float t = iTime*0.8f;
    
    if(Negativ) t *= -1.0f;
    
    float s = smoothstep(0.5f+Area.x,1.0f+Area.y,uv.x);
    
    if(Test)
      uv.y += s * snoise(uv*2.0f+1.0f+t*0.3f);
    else
      uv.y += s * _sinf(t+uv.x * 5.0f) * 0.05f;// * snoise(uv*2.0f+1.0f+t*0.3f);
    
    
    
    //uv.x += s * snoise(uv*(4.3f*(s/3.7f+1.2f))-to_float2(t*1.2f,0.0f)+Pos);
    uv.x += s * snoise(uv*(UVX1*(s/UVX2+UVX3))-to_float2(t*1.2f,0.0f)+PosXY);
    
    float tt = mod_f(t,10.0f);
    if(tt<3.0f){
      fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    } else if(tt<6.0f){
      fragColor = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    } else {
      fragColor = _tex2DVecN(iChannel2,uv.x,uv.y,15);    
    }
    
    SetFragmentShaderComputedColor(fragColor);
}

 