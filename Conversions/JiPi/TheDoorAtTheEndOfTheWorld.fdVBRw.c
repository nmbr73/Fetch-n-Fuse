
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define repeat(p,r) (mod_f(p,r)-r/2.0f)
#define TEX(uv) _tex2DVecN(iChannel0,(uv).x,(uv).y,15).x
__DEVICE__ mat2 rot (float a) { return to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }
__DEVICE__ float box (float2 p, float2 r) { return _fmaxf(_fabs(p.x)-r.x,_fabs(p.y)-r.y); }
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blue Noise' to iChannel1



// fbm gyroid cyclic noise
__DEVICE__ float gyroid (float3 seed) { return dot(sin_f3(seed),cos_f3(swi3(seed,y,z,x))); }
__DEVICE__ float fbm (float3 seed) {
    float result = 0.0f;
    float a = 0.5f;
    for (int i = 0; i < 4; ++i) {
        result += (gyroid(seed/a))*a;
        a /= 2.0f;
    }
    return result;
}

// the fluidish simulacre
__KERNEL__ void TheDoorAtTheEndOfTheWorldFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, int iFrame, float2 iResolution, float iTimeDelta, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    CONNECT_CHECKBOX1(Environment, 0);
    
    CONNECT_POINT0(Shape, 0.0f, 0.0f );
    CONNECT_SLIDER0(Shape0, -1.0f, 1.0f, 0.0f);
    
    CONNECT_SLIDER1(Smoke, -10.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER2(Expansion, -1.0f, 1.0f, 0.0f);
    
    CONNECT_SLIDER3(Turbulence1, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(Turbulence2, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER5(Turbulence3, -10.0f, 10.0f, 1.0f);
    
    CONNECT_SLIDER6(EnergyLoss, -10.0f, 10.0f, 1.0f);
  
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    float2 p = (fragCoord-iResolution/2.0f)/iResolution.y;
    float3 blue = swi3(texture(iChannel1, fragCoord/1024.0f),x,y,z);
    
    float dt = iTimeDelta;
    float current = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
    
    // shape
    float shape = _fabs(_fmaxf(_fabs(p.x)-0.1f+Shape.x, _fabs(p.y)-0.2f+Shape.y))-0.01f+Shape0;    
    
    // masks
    float shade = smoothstep(0.01f,0.0f,shape);
    float smoke = _powf(uv.y,0.5f+Smoke);
    float flame = 1.0f-uv.y;
    float steam = _powf(current, 0.2f);
    float cycle = 0.5f + 0.5f * _sinf(iTime-uv.x*3.0f);
    
    float2 offset = to_float2_s(0);
    
    // bubble animation cycle
    float2 pp = fract(p*1.0f-to_float2(0.5f,iTime*0.2f))-0.5f;
    float lpp = length(pp);
    float mask = smoothstep(0.5f,0.0f,lpp);
    offset += (-pp)*mask*5.0f;
    
    // ignit with bubble
    shade *= 0.5f+0.5f*mask;
    
    // environment
    if(Environment)
    {
      float2 q = p;
      float c = 0.05f;
      q.x += _floor(q.y/c)*0.02f+0.1f;
      q.y = repeat(q.y, c);
      shape = _fmaxf(p.y+0.22f, _fabs(box(q, to_float2(0.1f,0.002f))));
      shape = _fminf(shape, _fabs(length(p+to_float2(0,0.2f))-0.8f));
      shade = _fminf(1.0f, shade+0.75f*smoothstep(0.01f,0.0f,_fabs(shape)));
    }
    
    // expansion
    float4 data = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float3 unit = to_float3_aw(2.0f/iResolution,0);
    float3 normal = normalize(to_float3(
                                        TEX(uv - swi2(unit,x,z))-TEX(uv + swi2(unit,x,z)),
                                        TEX(uv - swi2(unit,z,y))-TEX(uv + swi2(unit,z,y)),
                                        data.x*data.x*data.x)+0.001f+Expansion);
    offset -= swi2(normal,x,y) * smoke;
    
    // turbulence
    float3 seed = to_float3_aw(p*4.0f*Turbulence1,p.y+iTime);
    float angle = fbm(seed)*6.28f*2.0f*Turbulence2;
    offset += to_float2(_cosf(angle),_sinf(angle)) * flame * Turbulence3;
    
    // energy loss
    float4 frame = texture(iChannel0, uv+offset/iResolution);
    shade = _fmaxf(shade, frame.x-dt*0.4f)*EnergyLoss;
    fragColor = to_float4_s(shade);

    if(iFrame<1 || Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1
// Connect Image 'Texture: Background' to iChannel2



// The door at the end of the world
// revisiting Smell of Burning Plastic https://www.shadertoy.com/view/7dyBRm

__KERNEL__ void TheDoorAtTheEndOfTheWorldFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
    CONNECT_POINT0(Shape, 0.0f, 0.0f );
    CONNECT_SLIDER0(Shape0, -1.0f, 1.0f, 0.0f);
    
    CONNECT_SLIDER7(Light1, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER8(Light2, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER9(Light3, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER10(LightY, -10.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER11(Flame, -10.0f, 10.0f, 1.0f);

    float2 uv = fragCoord/iResolution;
    float2 p = (fragCoord-iResolution/2.0f)/iResolution.y;

    float3 blue = swi3(texture(iChannel1, fragCoord/1024.0f),x,y,z);
    float3 color = swi3(texture(iChannel2, uv),x,y,z); // to_float3_s(0);
    
    // masks
    float shade = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
    float flame = _powf(shade, 3.0f*Flame);
    float smoke = _powf(shade, 0.5f);
    float height = 0.0f;//flame;
    
    // normal
    float range = 30.0f*blue.x;
    float3 unit = to_float3_aw(range/iResolution,0);
    float3 normal = normalize(to_float3(
                                        TEX(uv + swi2(unit,x,z))-TEX(uv - swi2(unit,x,z)),
                                        TEX(uv - swi2(unit,z,y))-TEX(uv + swi2(unit,z,y)),
                                        height));

    if (isnan(normal.x)) normal.x = -1.0f;
    if (isnan(normal.y)) normal.y = -1.0f;
    if (isnan(normal.z)) normal.z = -1.0f;
        
    // lighting
    float3 tint = to_float3_s(1.0f);//0.5f+0.5f*cos_f3(to_float3(1,2,3)-flame*5.0f-4.0f); //to_float3_s(1.0f);//
    float3 dir = normalize(to_float3(0,-0.5f,0.5f));
    
    float light = dot(normal, dir)*(0.5f+Light1) + 0.50f + Light2;
    //light = _powf(light,0.5f+Light3);
    //light *= (uv.y+0.5f+LightY); 
    
    color += tint * flame;
    color += to_float3_s(0.5f) * light;
    

    
    color *= smoke;
    color -= 0.1f*blue.x;
    color += smoothstep(0.1f,0.0f,1.0f-shade);
    
#ifdef XXXX
    // show layers
    if (iMouse.z > 0.5f) {
        if (iMouse.x < 20.0f) {
            if (uv.y < 0.25f) {
                color = to_float3_s(shade);
            } else if (uv.y < 0.5f) {
                color = normal;
            } else if (uv.y < 0.75f) {
                color = tint;
            } else {
                color = to_float3_s(0.5f)*light;
            }
        }
    }
#endif    
    fragColor = to_float4_aw(color+swi3(Color,x,y,z)-0.5f, Color.w);
    
    //fragColor = to_float4_aw(normal, Color.w);

  SetFragmentShaderComputedColor(fragColor);
}