
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define TEX(uv) _tex2DVecN(iChannel0,(uv).x,(uv).y,15).x
__DEVICE__ mat2 rot (float a) { return to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blue Noise' to iChannel1
// Connect Buffer A 'Texture: Background' to iChannel2


// fbm gyroid cyclic noise
__DEVICE__ float gyroid (float3 seed) { return dot(sin_f3(seed),cos_f3(swi3(seed,y,z,x))); }
__DEVICE__ float fbm (float3 seed, float iTime) {
    float result = 0.0f;
    float a = 0.5f;
    for (int i = 0; i < 4; ++i) {
        result += _sinf(gyroid(seed/a)*3.14f+iTime/a)*a;
        a /= 2.0f;
    }
    return result;
}

// the fluidish simulacre
__KERNEL__ void SmellOfBurningPlasticFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, int iFrame, float2 iResolution, float4 iMouse, float iTimeDelta, sampler2D iChannel0, sampler2D iChannel1)
{
  
    CONNECT_CHECKBOX0(Reset, 0);
  
    CONNECT_CHECKBOX1(Wind, 0);
    
    CONNECT_POINT0(Shape, 0.0f, 0.0f );
    CONNECT_SLIDER0(Shape0, -1.0f, 1.0f, 0.0f);
    
    CONNECT_SLIDER1(Smoke, -10.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER2(Expansion, -1.0f, 1.0f, 0.0f);
    
    CONNECT_SLIDER3(Turbulence1, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(Turbulence2, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER5(Turbulence3, -10.0f, 10.0f, 1.0f);
    
    CONNECT_SLIDER6(EnergyLoss, -10.0f, 10.0f, 1.0f);
    
    CONNECT_SLIDER7(DrawSize, 0.0f, 0.2f, 0.0f);
    
    
        //Blending
    CONNECT_SLIDER8(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER9(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER10(BlendMul, -10.0f, 10.0f, 1.0f);
    //CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    //CONNECT_POINT2(Par1, 0.0f, 0.0f);
    

    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    float2 p = (fragCoord-iResolution/2.0f)/iResolution.y;
    float3 blue = swi3(texture(iChannel1, fragCoord/1024.0f),x,y,z);
    
    float dt = iTimeDelta;
    float current = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
    
    // shape
    float shape = p.y+0.5f+Shape.y;
    
    // mouse interaction
    if (iMouse.z > 0.5f) {
        float2 mouse = (swi2(iMouse,x,y)-iResolution/2.0f)/iResolution.y;
        shape = _fminf(shape, length(p-mouse)-(0.01f+DrawSize));
    }
    
    
    // masks
    float shade = smoothstep(0.01f,0.0f,shape);
    float smoke = _powf(uv.y,0.5f);
    float flame = 1.0f-uv.y;
    float steam = _powf(current, 0.2f);
    float cycle = 0.5f + 0.5f * _sinf(iTime-uv.x*3.0f);
    
    float2 offset = to_float2_s(0);
    
    // gravity
    offset += to_float2(0,-1) * flame * cycle * steam;
    
    // wind
    if (Wind)
    {
      offset.x += _sinf(iTime*0.2f);
      offset += 3.0f*normalize(mul_f2_mat2(p,rot(0.1f))-p) * smoothstep(0.1f,0.0f,_fabs(length(p)-0.5f));
    }
    // expansion
    float4 data = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float3 unit = to_float3_aw(20.0f*blue.x/iResolution,0);
    float3 normal = normalize(to_float3(
                                      TEX(uv - swi2(unit,x,z))-TEX(uv + swi2(unit,x,z)),
                                      TEX(uv - swi2(unit,z,y))-TEX(uv + swi2(unit,z,y)),
                                      data.x*data.x*data.x)+0.001f+Expansion);
                                      
                                                                           
    offset -= swi2(normal,x,y) * (smoke + cycle) * steam;
    
    // turbulence
    float3 seed = to_float3_aw(p*2.0f*Turbulence1,p.y);
    float angle = fbm(seed,iTime)*6.28f*2.0f*Turbulence2;
    offset += to_float2(_cosf(angle),_sinf(angle)) * flame * Turbulence3;
    
    // energy loss
    float4 frame = texture(iChannel0, uv+offset/iResolution);
    shade = _fmaxf(shade, frame.x-dt*0.2f)*EnergyLoss;
    fragColor = to_float4_s(shade);

    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(iChannel2,uv);

      if (tex.w > 0.0f)
      { 
         fragColor = _mix(fragColor,(tex+BlendOff)*BlendMul,Blend);
      }
    }

    if(iFrame<1 || Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1
// Connect Image 'Texture: Background' to iChannel2


// Smell of Burning Plastic
// revisiting Liquid Toy https://www.shadertoy.com/view/fljBWc
// with gyroid turbulences and a burning floor context

__KERNEL__ void SmellOfBurningPlasticFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);

    float2 uv = fragCoord/iResolution;
    float3 blue = swi3(texture(iChannel1, fragCoord/1024.0f),x,y,z);
    //float3 color = to_float3_s(0);
    float3 color = swi3(texture(iChannel2, uv),x,y,z); // to_float3_s(0);
    
    // masks
    float shade = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
    float flame = _powf(shade, 6.0f);
    float smoke = _powf(shade, 0.5f);
    float height = flame;
    
    // normal
    float range = 30.0f*blue.x;
    float3 unit = to_float3_aw(range/iResolution,0);
    float3 normal = normalize(to_float3(
        TEX(uv + swi2(unit,x,z))-TEX(uv - swi2(unit,x,z)),
        TEX(uv - swi2(unit,z,y))-TEX(uv + swi2(unit,z,y)),
        height));
        
        
    if (isnan(normal.x)) normal.x = -0.1f;
    if (isnan(normal.y)) normal.y = -0.1f;
    if (isnan(normal.z)) normal.z = -0.1f;    
        
      
        
    // lighting
    float3 tint = 0.5f+0.5f*cos_f3(to_float3(0,0.3f,0.6f)*6.28f-flame*4.0f-4.0f);
    float3 dir = normalize(to_float3(0,-0.5f,0.5f));
    float light = dot(normal, dir)*0.5f+0.5f; //0.0f;//
    
    if (isinf(light)) light = 0.0f;
    
    light = _powf(light,0.5f);
    light *= (uv.y+0.5f); 
    color += tint * flame;
    color += to_float3_s(0.5f) * light;
    color *= smoke;
    color -= 0.1f*blue.x;
    color += smoothstep(0.1f,0.0f,1.0f-shade);
    
    // show layers
    if (iMouse.z > 0.5f) {
        if (iMouse.x < 20.0f) {
            if (uv.x < 0.25f) {
                color = to_float3_s(shade);
            } else if (uv.x < 0.5f) {
                color = normal;
            } else if (uv.x < 0.75f) {
                color = tint;
            } else {
                color = to_float3_s(0.5f)*light;
            }
        }
    }
    
    fragColor = to_float4_aw(color+swi3(Color,x,y,z)-0.5f, Color.w);
    
  SetFragmentShaderComputedColor(fragColor);
}