
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Texture' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define fb iChannel0

// Code originally from draw_to_fb().
__KERNEL__ void MousepositionfogJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  CONNECT_COLOR0(Color1, 0.2f, 0.25f, 1.0f, 1.0f);
  CONNECT_COLOR1(Color2, 1.0f, 0.5f, 0.0f, 1.0f);
  CONNECT_SLIDER0(Size, -1.0f, 30.0f, 16.0f);
  CONNECT_SLIDER1(AlphaThres, 0.0f, 1.0f, 0.0f);

  CONNECT_CHECKBOX0(Tex, 0);

  fragCoord+=0.5f;

  float2 pos;
  float2 posnorm;
  float2 poscanv;
  float2 zooming;
  float4 color;
  float dist;

  // These have the same size in Shadertoy:
  float2 canvassize = iResolution;
  float2 fbsize = canvassize;

  bool mouse_hold = !(iMouse.z > 0.0f);
  float2 mouse = swi2(iMouse,x,y);

  // Position of the current pixel, normalized between 0.0f and 1.0
  posnorm = fragCoord / iResolution;

  // Distance between the current pixel and the mouse, already taking into
  // account the difference between the coordinate systems
  dist = distance_f2(mouse, fragCoord);
  if ( dist < Size ) {
    // Drawing a circle around the mouse with a solid color
    // Blue if mouse button is not pressed, orange if it is pressed
    color = Color1;//to_float4(0.2f, 0.25f, 1.0f, 0.0f);// + to_float4(1.0f, 0.5f, -1.0f, 0.0f) * float(mouse_hold);
    if (mouse_hold)
      color = Color2;//to_float4(1.0f, 0.5f, -1.0f, 0.0f);
  } else {
    // Zooming trick
    zooming = ((mouse - fragCoord) * (0.015625f)) / canvassize;
    color = texture(fb, posnorm + zooming);

    color -= (color * 0.00390625f) + 0.001953125f;
  }

  if(Tex)
  {
    float4 TexData = texture(iChannel1, posnorm);
    if(TexData.w > AlphaThres)
    {
      color = TexData;
    }
  }

  fragColor = color;

  fragColor.w = Color1.w;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Written by Denilson SÃ¡ <denilsonsa@gmail.com>
// http://denilson.sa.nom.br/
//
// GLSL Sandbox version at:
// http://glslsandbox.com/e#12315.2
//
// The source is also available at:
// https://bitbucket.org/denilsonsa/atmega8-magnetometer-usb-mouse/src/tip/html_javascript/
// https://github.com/denilsonsa/atmega8-magnetometer-usb-mouse/tree/master/html_javascript
//
// The original code (above) was restructured to work with Shadertoy.

// Code originally from draw_to_main().
__KERNEL__ void MousepositionfogJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  float2 uv = fragCoord / iResolution;
  fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}