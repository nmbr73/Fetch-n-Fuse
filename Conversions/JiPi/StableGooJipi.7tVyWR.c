
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel2
// Connect Buffer A 'Texture: Blending' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0





#define PI 3.14159265358979323846264338327950288419716939937510582097494459230781640f

__DEVICE__ float random(float2 fragCoord)
{
  return fract(_sinf(dot(fragCoord, to_float2(12.9898f,78.233f))) * 43758.5453f);  
}

__DEVICE__ float get_average(float2 uv, float size, float2 iResolution, __TEXTURE2D__ iChannel0, float RADIUS, float points, float Start)
{
    //const float points = 14.0f;
    //const float Start = 2.0f / points;
    float2 scale = (RADIUS * 5.0f / iResolution) + size;

    float res = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;

    for (float point = 0.0f; point < points; point++)
    {
        float r = (PI * 2.0f * (1.0f / points)) * (point + Start);
        res += _tex2DVecN(iChannel0, uv.x + _sinf(r) * scale.x, uv.y + _cosf(r) * scale.y, 15).x;
    }

    res /= points;

    return res;
}

__KERNEL__ void StableGooJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Rotate, 0);
    
    CONNECT_SLIDER0(Angel, -100.0f, 100.0f, 10.0f);
    
    //const float TEMPERATURE = 2.0f;
    //const float RADIUS = 1.33f;
    CONNECT_SLIDER1(TEMPERATURE, -1.0f, 10.0f, 2.0f);
    CONNECT_SLIDER2(RADIUS, -1.0f, 10.0f, 1.33f);
    
    //const float points = 14.0f;
    //const float Start = 2.0f / points;
    CONNECT_SLIDER3(Points, -1.0f, 50.0f, 14.0f);
    CONNECT_SLIDER4(Start, -1.0f, 10.0f, 2.0f);
    
    
    //Blending
    CONNECT_SLIDER5(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(Blend1Mul, -10.0f, 10.0f, 1.0f);
    //CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    //CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    
    float3 noise = swi3(_tex2DVecN(iChannel2, iTime*0.01f + uv.x*0.015f,iTime*0.01f + uv.y*0.015f,15),x,y,z);//.rgb;
    float height = (noise.z*2.0f-1.0f) * 0.25f;
    
    {  
        float2 muv = swi2(noise,x,y);
        
        float2 p = uv - muv;
        float r = length(p);
        float a = _atan2f(p.y, p.x);
        r = _powf(r*2.0f, 1.0f + height * 0.05f);
        p = r * to_float2(_cosf(a)*0.5f, _sinf(a)*0.5f);

        uv = p + muv;
    }
    
    if (Rotate) {
        float rot = radians(Angel);
        uv -= 0.5f;
        uv = mul_f2_mat2(uv, to_mat2(_cosf(rot), -_sinf(rot), _sinf(rot), _cosf(rot))) * 1.0002f;
        uv += 0.5f;
    } 
        
    //uv.y += 0.00025f;

    if (iFrame == 0 || Reset)
    {
        fragColor.x = random(fragCoord);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    float val = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
    float avg = get_average(uv, height*-0.005f, iResolution, iChannel0, RADIUS, Points, Start);
    fragColor.x = _sinf(avg * (2.3f + TEMPERATURE + height*2.0f)) + _sinf(val);
   
    if (iMouse.z > 0.0f) 
        fragColor.x += smoothstep(length(iResolution)/5.0f, 0.5f, length(swi2(iMouse,x,y) - fragCoord)) * _sinf(iTime*10.0f);


    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = _tex2DVecN(iChannel1, fragCoord.x/iResolution.x,fragCoord.y/iResolution.y, 15);

      if (tex.w > 0.0f)
      {      
        fragColor.x = _mix(fragColor.x,(tex.x+Blend1Off)*Blend1Mul,Blend1);
      }
    }
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void StableGooJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER8(Level0, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER9(Level1, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER10(Level2, -1.0f, 1.0f, 0.5f);
    
    CONNECT_SLIDER11(Level3, -1.0f, 1.0f, 1.25f);
    CONNECT_SLIDER12(Level4, -1.0f, 1.0f, 0.1f);
    CONNECT_SLIDER13(Level5, -1.0f, 20.0f, 5.0f);
    
    CONNECT_SLIDER14(Level6, -1.0f, 20.0f, 5.0f);
    CONNECT_SLIDER15(Level7, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER16(Level8, -1.0f, 1.0f, 0.26f);

    float v = _tex2DVecN(iChannel0, fragCoord.x/iResolution.x,fragCoord.y/iResolution.y,15).x;
    v *= Level0;//0.5f;
    v = v * Level1 + Level2; //0.5f + 0.5f;
    v = clamp(v, 0.0f, 1.0f);
    
    
    fragColor.x = v*Level3;//1.25f;
    fragColor.y = _sinf(v*Level4)*Level5+v; //0.1f)*5.0f+v;
    fragColor.z = _powf(v*Level6, Level7)*Level8;//5.0f, 0.5f)*0.26f;

    fragColor += Color-0.5f;
    fragColor.w = Color.w;




  SetFragmentShaderComputedColor(fragColor);
}