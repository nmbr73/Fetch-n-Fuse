
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



__DEVICE__ float lookup(float2 p, float dx, float dy, float d, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float2 uv = (swi2(p,x,y) + to_float2(dx * d, dy * d)) / iResolution;
    float4 c = texture(iChannel0, uv);
  
  // return as luma
    return 0.2126f*c.x + 0.7152f*c.y + 0.0722f*c.z;
}


__KERNEL__ void CrosshatchJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  CONNECT_CHECKBOX0(ColourHatches, 0);
  CONNECT_CHECKBOX1(BrightnessNull, 0);

  // The brightnesses at which different hatch lines appear
  float hatch_1 = 0.8f;
  float hatch_2 = 0.6f;
  float hatch_3 = 0.3f;
  float hatch_4 = 0.15f;

  // How close together hatch lines should be placed
  float density = 10.0f;

  // How wide hatch lines are drawn.
  float width = 1.0f;

  // enable GREY_HATCHES for greyscale hatch lines
  #define GREY_HATCHES

  // enable COLOUR_HATCHES for coloured hatch lines
  #define COLOUR_HATCHES

  //#ifdef GREY_HATCHES
  float hatch_1_brightness = 0.8f;
  float hatch_2_brightness = 0.6f;
  float hatch_3_brightness = 0.3f;
  float hatch_4_brightness = 0.0f;
  
  if(BrightnessNull)
  {
    hatch_1_brightness = 0.0f;
    hatch_2_brightness = 0.0f;
    hatch_3_brightness = 0.0f;
    hatch_4_brightness = 0.0f;
  }

  float d = 1.0f; // kernel offset


  //
  // Inspired by the technique illustrated at
  // http://www.geeks3d.com/20110219/shader-library-crosshatching-glsl-filter/
  //
  float ratio = iResolution.y / iResolution.x;
  float coordX = fragCoord.x / iResolution.x;
  float coordY = fragCoord.y / iResolution.x;
  float2 dstCoord = to_float2(coordX, coordY);
  float2 srcCoord = to_float2(coordX, coordY / ratio);  
  float2 uv = swi2(srcCoord,x,y);

  float3 res = to_float3(1.0f, 1.0f, 1.0f);
  float4 tex = _tex2DVecN(iChannel0,uv.x,uv.y,15);
  float brightness = (0.2126f*tex.x) + (0.7152f*tex.y) + (0.0722f*tex.z);
//#ifdef COLOUR_HATCHES
  if(ColourHatches)
  {
    // check whether we have enough of a hue to warrant coloring our
    // hatch strokes.  If not, just use greyscale for our hatch color.
    float dimmestChannel = _fminf( _fminf( tex.x, tex.y ), tex.z );
    float brightestChannel = _fmaxf( _fmaxf( tex.x, tex.y ), tex.z );
    float delta = brightestChannel - dimmestChannel;

    if ( delta > 0.1f )
      tex = tex * ( 1.0f / brightestChannel );
    else
      //swi3(tex,x,y,z) = to_float3(1.0f,1.0f,1.0f);
      tex.x=1.0f,tex.y=1.0f,tex.z=1.0f;
  }    
//#endif // COLOUR_HATCHES
  
    if (brightness < hatch_1) 
    {
    if (mod_f(fragCoord.x + fragCoord.y, density) <= width)
    {
//#ifdef COLOUR_HATCHES
    if(ColourHatches)
      res = (swi3(tex,x,y,z) * hatch_1_brightness);
    else
      res = to_float3_s(hatch_1_brightness);
//#endif
    }
    }
  
    if (brightness < hatch_2) 
    {
    if (mod_f(fragCoord.x - fragCoord.y, density) <= width)
    {
//#ifdef COLOUR_HATCHES
    if(ColourHatches)
      res = (swi3(tex,x,y,z) * hatch_2_brightness);
    else
      res = to_float3_s(hatch_2_brightness);
//#endif
    }
    }
float IIIIIIIIIIIIIIIIIIII;  
    if (brightness < hatch_3) 
    {
    if (mod_f(fragCoord.x + fragCoord.y - (density*0.5f), density) <= width)
    {
//#ifdef COLOUR_HATCHES
    if(ColourHatches)
      res = (swi3(tex,x,y,z) * hatch_3_brightness);
    else
      res = to_float3_s(hatch_3_brightness);
//#endif
    }
    }
  
    if (brightness < hatch_4) 
    {
    if (mod_f(fragCoord.x - fragCoord.y - (density*0.5f), density) <= width)
    {
//#ifdef COLOUR_HATCHES
    if(ColourHatches)
      res = (swi3(tex,x,y,z) * hatch_4_brightness);
    else
      res = to_float3_s(hatch_4_brightness);
//#endif
    }
    }
  
  float2 p = fragCoord;
    
  // simple sobel edge detection,
  // borrowed and tweaked from jmk's "edge glow" filter, here:
  // https://www.shadertoy.com/view/Mdf3zr
    float gx = 0.0f;
    gx += -1.0f * lookup(p, -1.0f, -1.0f,d,iResolution,iChannel0);
    gx += -2.0f * lookup(p, -1.0f,  0.0f,d,iResolution,iChannel0);
    gx += -1.0f * lookup(p, -1.0f,  1.0f,d,iResolution,iChannel0);
    gx +=  1.0f * lookup(p,  1.0f, -1.0f,d,iResolution,iChannel0);
    gx +=  2.0f * lookup(p,  1.0f,  0.0f,d,iResolution,iChannel0);
    gx +=  1.0f * lookup(p,  1.0f,  1.0f,d,iResolution,iChannel0);
    
    float gy = 0.0f;
    gy += -1.0f * lookup(p, -1.0f, -1.0f,d,iResolution,iChannel0);
    gy += -2.0f * lookup(p,  0.0f, -1.0f,d,iResolution,iChannel0);
    gy += -1.0f * lookup(p,  1.0f, -1.0f,d,iResolution,iChannel0);
    gy +=  1.0f * lookup(p, -1.0f,  1.0f,d,iResolution,iChannel0);
    gy +=  2.0f * lookup(p,  0.0f,  1.0f,d,iResolution,iChannel0);
    gy +=  1.0f * lookup(p,  1.0f,  1.0f,d,iResolution,iChannel0);
    
  // hack: use g^2 to conceal noise in the video
  float g = gx*gx + gy*gy;
  res *= (1.0f-g);
  
  fragColor = to_float4_aw(res, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}