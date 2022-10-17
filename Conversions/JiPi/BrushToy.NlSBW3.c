
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

// shortcut to sample texture
#define TEX(uv) _tex2DVecN(iChannel0,(uv).x,(uv).y,15).x
#define TEX1(uv) _tex2DVecN(iChannel1,(uv).x,(uv).y,15).x
#define TEX2(uv) _tex2DVecN(iChannel2,(uv).x,(uv).y,15).x
#define TEX3(uv) _tex2DVecN(iChannel3,(uv).x,(uv).y,15).x

// shorcut for smoothstep uses
#define trace(edge, thin) smoothstep(thin,0.0f,edge)
#define ss(a,b,t) smoothstep(a,b,t)




// rotation matrix
__DEVICE__ mat2 rot(float a) { return to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }

// generated with discrete Fourier transform
__DEVICE__ float2 cookie(float t) {
  
  //return to_float2(0.08f+_cosf(t-1.58f)*0.23f +_cosf(t*2.0f-1.24f)*0.14f , _cosf(3.14f)*0.05f);
  #define ORG
  #ifdef ORG
  return to_float2(0.08f+_cosf(t-1.58f)*0.23f
                        +_cosf(t*2.0f-1.24f)*0.14f
                        +_cosf(t*3.0f-1.12f)*0.09f
                        +_cosf(t*4.0f-0.76f)*0.06f
                        +_cosf(t*5.0f-0.59f)*0.05f
                        +_cosf(t*6.0f+0.56f)*0.03f
                        +_cosf(t*7.0f-2.73f)*0.03f
                        +_cosf(t*8.0f-1.26f)*0.02f
                        +_cosf(t*9.0f-1.44f)*0.02f
                        +_cosf(t*10.0f-2.09f)*0.03f
                        +_cosf(t*11.0f-2.18f)*0.01f
                        +_cosf(t*12.0f-1.91f)*0.02f,
                        
                         _cosf(3.14f)*0.05f
                        +_cosf(t+0.35f)*0.06f
                        +_cosf(t*2.0f+0.54f)*0.09f
                        +_cosf(t*3.0f+0.44f)*0.03f
                        +_cosf(t*4.0f+1.02f)*0.07f
                        +_cosf(t*6.0f+0.39f)*0.03f
                        +_cosf(t*7.0f-1.48f)*0.02f
                        +_cosf(t*8.0f-3.06f)*0.02f
                        +_cosf(t*9.0f-0.39f)*0.07f
                        +_cosf(t*10.0f-0.39f)*0.03f
                        +_cosf(t*11.0f-0.03f)*0.04f
                        +_cosf(t*12.0f-2.08f)*0.02f);
#endif                        
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Blue Noise' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1
// Connect Buffer A 'Texture: Cube' to iChannel2
// Connect Buffer A 'Texture: Blending' to iChannel3



// Brush toy by Leon Denise 2022-05-17

// The painting pass is using FBM noise to simulate brush strokes
// The curve was generated with a discrete Fourier Transform,
// from https://www.shadertoy.com/view/3ljXWK

// Frame buffer sampling get offset from brush motion,
// and the mouse also interact with the buffer.



// fractal brownian motion (layers of multi scale noise)
__DEVICE__ float3 fbm(float3 p, float falloff, __TEXTURE2D__ iChannel2)
{
    float3 result = to_float3_s(0);
    float amplitude = 0.5f;
    for (float index = 0.0f; index < 3.0f; ++index)
    {
        //result += swi3(_tex2DVecN(iChannel2, p.x/amplitude,p.y/amplitude, 15),x,y,z) * amplitude; // Eigentlich ein Cube
        result += swi3(decube_f3(iChannel2, p/amplitude),x,y,z) * amplitude; // Eigentlich ein Cube
        amplitude /= falloff;
    }
    return result;
}


__KERNEL__ void BrushToyFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, float iTimeDelta, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(LiquidToy, 0);
    
    
    CONNECT_CHECKBOX4(BrushPaint, 0);
    CONNECT_POINT0(BrushPaintXY, 0.5f, 0.5f);
    
    CONNECT_SLIDER2(Scale, -1.0f, 5.0f, 0.0f);
    CONNECT_SLIDER3(Falloff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Speed, -1.0f, 5.0f, 0.0f);
    
    CONNECT_SLIDER5(Blend, 0.0f, 1.0f, 0.0f);
    
    fragCoord+=0.5f; 
    
    float scale   = 0.8f+Scale;
    float falloff = 2.0f+Falloff;
    float speed = 1.0f+Speed;
    
    float paint;
        
    // coordinates
    float2 uv = (fragCoord - iResolution / 2.0f)/iResolution.y;
    float2 mouse = (swi2(iMouse,x,y) - iResolution / 2.0f)/iResolution.y;

float AAAAAAAAAAAAAAAAAAAA;

    if (LiquidToy == 0)
    {

      // dithering
      float3 dither = swi3(_tex2DVecN(iChannel0, fragCoord.x / 1024.0f,fragCoord.y / 1024.0f,15),x,y,z);
      
      // sample curve position
      
      float t = -iTime*speed+dither.x*0.01f;
      float2 current = cookie(t);
      
      if(BrushPaint) current = (BrushPaintXY-0.5f)/1.6f, current.x*=(iResolution.x/iResolution.y);
      
      // velocity from current and next curve position
      float2 next = cookie(t+0.01f);
      float2 velocity = normalize(next-current);
      
      // move brush cursor along curve
      float2 pos = uv-current*1.6f;
      
      paint = fbm(to_float3_aw(pos, 0.0f) * scale,falloff,iChannel2).x;
      
      // brush range
      float brush = smoothstep(0.3f,0.0f,length(pos));
      paint *= brush;
      
      // add circle shape to buffer
      paint += smoothstep(0.05f, 0.0f, length(pos));
      
      // motion mask
      float push = smoothstep(0.3f, 0.5f, paint);
      push *= smoothstep(0.4f, 1.0f, brush);
      
      // direction and strength
      float2 offset = 10.0f*push*velocity/iResolution;
      
      // paint mouse
      float2 mousePrevious = swi2(_tex2DVecN(iChannel1, 0.0f,0.0f,15),x,y);
      if (iMouse.z > 0.5f && length(mouse-mousePrevious) > 0.001f)
      {
          uv = (fragCoord - iResolution / 2.0f)/iResolution.y;
          float mask = fbm(to_float3_aw(uv-mouse, 0.0f) * scale * 0.5f,falloff,iChannel2).x;
          push = smoothstep(0.2f,0.0f,length(uv-mouse));
          push *= smoothstep(0.3f,0.6f,mask);
          float2 dir = normalize(mousePrevious-mouse);
          push *= 500.0f*length(mousePrevious-mouse);
          offset += push*dir/iResolution;
      }
      
      // sample frame buffer with motion
      uv = fragCoord / iResolution;
      float4 frame = _tex2DVecN(iChannel1, uv.x + offset.x, uv.y + offset.y,15);
      
      // temporal fading buffer
      paint = _fmaxf(paint, frame.x - 0.0005f);

    }
    else
    {
      
      speed = 0.01f+Speed;
      scale = 0.1f+Scale;
      falloff = 3.0f+Falloff;
      const float fade = 0.4f;
      const float strength = 1.0f;
      const float range = 5.0f;
      
      // noise
      float3 spice = fbm(to_float3_aw(uv*scale,iTime*speed),falloff,iChannel2);
      
      // draw circle at mouse or in motion
      float t = iTime*2.0f;
//      float2 mouse = (swi2(iMouse,x,y) - iResolution / 2.0f)/iResolution.y;
      if (iMouse.z > 0.5f) uv -= mouse;
      else uv -= to_float2(_cosf(t),_sinf(t))*0.3f;
      paint = trace(length(uv),0.1f);
      
      // expansion
      float2 offset = to_float2_s(0);
      uv = fragCoord / iResolution;
      float4 data = _tex2DVecN(iChannel1,uv.x,uv.y,15);
      float3 unit = to_float3_aw(range/iResolution,0);
      float3 normal = normalize(to_float3(
                                          TEX1(uv - swi2(unit,x,z))-TEX1(uv + swi2(unit,x,z)),
                                          TEX1(uv - swi2(unit,z,y))-TEX1(uv + swi2(unit,z,y)),
                                          data.x*data.x)+0.001f);
      offset -= swi2(normal,x,y);
      
      // turbulence
      spice.x *= 6.28f*2.0f;
      spice.x += iTime;
      offset += to_float2(_cosf(spice.x),_sinf(spice.x));
      
      // sample buffer
      float4 frame = _tex2DVecN(iChannel1, uv.x + strength * offset.x / iResolution.x,  uv.y + strength * offset.y / iResolution.y,15);
      
      // temporal fading buffer
      paint = _fmaxf(paint, frame.x - iTimeDelta * fade);

      
    }
    
    // print result
    fragColor = to_float4_s(clamp(paint, 0.0f, 1.0f));

    float tex = _tex2DVecN(iChannel3, uv.x,uv.y,15).x;
    fragColor = _mix(fragColor,to_float4_s(tex), Blend);

    
    // save mouse position for next frame
    if (fragCoord.x < 1.0f && fragCoord.y < 1.0f) fragColor = to_float4(mouse.x,mouse.y, 0, 1);
    
    if (iFrame<1 || Reset)
      fragColor = to_float4_s(0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1



// Brush toy by Leon Denise 2022-05-17

// I wanted to play further with shading and lighting from 2D heightmap.
// I started by generating a heightmap from noise, then shape and curves.
// Once the curve was drawing nice brush strokes, I wanted to add motion.
// Also wanted to add droplets of paints falling, but that will be
// for another sketch.

// This is the color pass
// Click on left edge to see layers

// The painting pass (Buffer A) is using FBM noise to simulate brush strokes
// The curve was generated with a discrete Fourier Transform,
// from https://www.shadertoy.com/view/3ljXWK

// Frame buffer sampling get offset from brush motion,
// and the mouse also interact with the buffer.


__KERNEL__ void BrushToyFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(LiquidToy, 0);
    CONNECT_CHECKBOX2(LiquidToy2, 0);
    CONNECT_CHECKBOX3(Alpha, 1);

    CONNECT_COLOR0(ColorStd, 0.3f, 0.3f, 0.3f, 1.0f);
    CONNECT_COLOR1(Color2, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_COLOR2(ColorBKG, 1.0f, 1.0f, 1.0f, 1.0f);
    
    CONNECT_COLOR3(ColorTint, 1.0f, 2.0f, 3.0f, 1.0f);

    CONNECT_SLIDER0(TintMul1, -1.0f, 10.0f, 5.0f);
    CONNECT_SLIDER1(TintMul2, -1.0f, 10.0f, 5.0f);

    float3 color = to_float3_s(0.0f);


    // coordinates
    float2 uv = fragCoord / iResolution;
    float3 dither = swi3(_tex2DVecN(iChannel1, fragCoord.x / 1024.0f,fragCoord.y / 1024.0f, 15),x,y,z);
    
    // value from noise buffer A
    float3 noise = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float gray = noise.x;
    
    // gradient normal from gray value
    float range = 3.0f;

    float3 unit = to_float3_aw(range/iResolution,0);
    
    float grayLiquid = 1.0f;
    if (LiquidToy2 == 1) grayLiquid = gray;
    
    float3 normal = normalize(to_float3(
                                        TEX(uv + swi2(unit,x,z))-TEX(uv - swi2(unit,x,z)),
                                        TEX(uv - swi2(unit,z,y))-TEX(uv + swi2(unit,z,y)),
                                        gray*gray*grayLiquid));
    
    // backlight    
    if(LiquidToy2)
      //color = to_float3_s(0.3f)*(1.0f-_fabs(dot(normal, to_float3(0,0,1))));
      color = swi3(ColorStd,x,y,z)*(1.0f-_fabs(dot(normal, to_float3(0,0,1))));
    
  
    // specular light
    float3 dir = normalize(to_float3(0,1,2.0f));
    float specular = _powf(dot(normal, dir)*0.5f+0.5f,20.0f);
    
    //float3 tint = 0.5f+0.5f*cos_f3(to_float3(1,2,3)*1.5f+gray*5.0f+uv.x*5.0f);
    float3 tint = 0.5f+0.5f*cos_f3(swi3(ColorTint,x,y,z)*1.5f+gray*TintMul1+uv.x*TintMul2);
    
    if(LiquidToy2==0)
    {
      //color += to_float3_s(0.5f)*specular;
      color += swi3(ColorStd,x,y,z)*specular;
      
      // rainbow palette
      
      dir = normalize(to_float3_aw(uv-0.5f, 0.0f));
      color += tint*_powf(dot(normal, -dir)*0.5f+0.5f, 0.5f);
      
      // background blend
      //float3 background = to_float3_s(0.8f)*smoothstep(1.5f,0.0f,length(uv-0.5f));
      float3 background = swi3(ColorBKG,x,y,z)*smoothstep(1.5f,0.0f,length(uv-0.5f));
      color = _mix(background, clamp(color, 0.0f, 1.0f), smoothstep(0.2f,0.5f,noise.x));
    }
    else
    {
      //color += to_float3_s(0.5f)*ss(0.2f,1.0f,specular);
      color += swi3(Color2,x,y,z)*ss(0.2f,1.0f,specular);
          
      // rainbow
      tint = 0.5f+0.5f*cos_f3(to_float3(1,2,3)*1.0f+dot(normal, dir)*4.0f-uv.y*3.0f-3.0f);
      color += tint * smoothstep(0.15f,0.0f,gray);

      // dither
      color -= dither.x*0.1f;
      
      // background blend
      float3 background = swi3(ColorBKG,x,y,z);//to_float3_s(1);
      background *= smoothstep(1.5f,-0.5f,length(uv-0.5f));
      color = _mix(background, clamp(color, 0.0f, 1.0f), ss(0.01f,0.1f,gray));
      
    }
    
    
    // display layers when clic
    if (iMouse.z > 0.5f && iMouse.x/iResolution.x < 0.1f)
    {
        if (uv.x < 0.33f) color = to_float3_s(gray);
        else if (uv.x < 0.66f) color = normal*0.5f+0.5f;
        else 
          if (LiquidToy2) color = (tint);
          else            color = to_float3_s(0.2f+specular)*gray;
    }

    fragColor = to_float4_aw(color, Alpha==1?ColorBKG.w:gray);

  SetFragmentShaderComputedColor(fragColor);
}