
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0


#define TUTORIAL 12 // Default to 12, my favorite


/* TUTORIAL LIST
  0 = Normal/No effect
  1 = Contrast
  2 = Invert
  3 = Intensity
  4 = Grey Scale Average
  5 = Grey Scale Basic
  6 = Grey Scale Fancy
  7 = Single Color Channel Gray Scale
  8 = Grey Scale HSL
  9 = Grey Scale Range
  10 = Black Light
  11 = Swizzle
  12 = Wave
  13 = sepia
  14 = invert black and white, keep color
  15 = multiply colorchannel
  16 = dithering // use to fix https://www.shadertoy.com/view/XsVBW1 (needs work)
  17 = Mystery (Not sure what this is but it's cool)
  18 Black and White Threshold

// TODO
// Brightness, Bloom, Glow, Posterization, sat, lum, gamma correction, hsv, hsb, infared, quantize, threshold

// Neighboring pixels
// Neon, bloom, dream, emboss, blurs, edge detection, halftone dots, outline, sharpen, sketch, water color

// shift pixels / neighbors
// chromatic aberration, 

// gradients, dithering

// kaleidoscope

// iterative
// mandle brot, julia, 
  
*/

// #1 = Contrast

__DEVICE__ float3 Contrast(float3 color, float contrast)
{
  // increase values above 0.5f and decrease below 0.5
  color -= 0.5f;
  color *= contrast;
  color += 0.5f;
  return color;
}

// #2 = Invert

__DEVICE__ float3 Invert(float3 color)
{
  float zzzzzzzzzzzzzzzzz;  
  return 1.0f - color;
}

// #3 = Intensity

__DEVICE__ float3 Intensity(float3 color, float factor)
{
  return color = color * factor;
}

// #4 = Grey Scale Average

__DEVICE__ float3 GreyScaleAverage(float3 color)
{
  return to_float3_s((color.x + color.y + color.z) / 3.0f);
}

// #5 = Grey Scale Basic

// Tint green
__DEVICE__ float3 GreyScaleBasic(float3 color)
{ 
  return to_float3_s(color.x * 0.3f + color.y * 0.59f + color.z * 0.11f);
}

// #6 = Grey Scale Fancy

__DEVICE__ float3 GreyScaleFancy(float3 color)
{
  float3 lumCoeff = to_float3(0.3f, 0.59f, 0.11f);
  color = to_float3_s(dot(color, lumCoeff));
  return color;
}

// #7 = Grey Scale Single Channel

// could use any of the 3 channels
__DEVICE__ float3 SingleColorChannelGrayScale(float3 color)
{
    return to_float3_s(color.x);
    // return to_float3(color.y);
    // return to_float3(color.z);
}

// #8 = Grey Scale HSL

__DEVICE__ float3 GrayScaleHSL(float3 color)
{
  return to_float3_s(_fmaxf(color.x, _fmaxf(color.y, color.z)) + _fminf(color.x, _fminf(color.y, color.z)) / 2.0f);
}

// #9 = Grey Scale Range

__DEVICE__ float3 GrayScaleRange(float3 color, float NumberOfShades)
{
  float ConversionFactor = 255.0f / (NumberOfShades - 1.0f);
  float AverageValue = (color.x + color.y + color.z) / 3.0f;
  float Gray = ((AverageValue / ConversionFactor) + 0.5f) * ConversionFactor;
  return to_float3_s(Gray);
}

// #10 = Black Light

__DEVICE__ float3 BlackLight(float3 color, float fxWeight)
{
  float lum = color.x + color.y + color.z / 3.0f;
  lum = (222.0f * color.x + 707.0f * color.y + 71.0f * color.z) / 1000.0f;
  // R = Abs(R - L) * fxWeight
  color.x = _fabs(color.x - lum) * fxWeight;
  color.x = clamp(color.x, 0.0f, 1.0f);
  color.y = _fabs(color.y - lum) * fxWeight;
  color.y = clamp(color.y, 0.0f, 1.0f);
  color.z = _fabs(color.z - lum) * fxWeight;
  color.z = clamp(color.z, 0.0f, 1.0f);

  return color;
}

// TODO It would be nice to slide show thru options with iTime
// # 11 Swizzle
__DEVICE__ float3 Swizzle(float3 color)
{
    // each color used once
    return to_float3(color.y, color.z, color.x);
    // return to_float3(color.z, color.x, color.y);
    // return to_float3(color.z, color.x, color.y);
    // return to_float3(color.z, color.y, color.x);
    // return to_float3(color.y, color.x, color.z);
    // return to_float3(color.x, color.z, color.y);
    
    // one channel used twice(one channel not used)
    // return to_float3(color.x, color.x, color.y);
    // return to_float3(color.x, color.x, color.z);
    // return to_float3(color.y, color.y, color.x);
    // return to_float3(color.y, color.y, color.z);
    // return to_float3(color.z, color.z, color.x);
    // return to_float3(color.z, color.z, color.y);
    
    // one channel used twice(one channel not used) but in different order
    // return to_float3(color.y, color.x, color.x);
    // return to_float3(color.z, color.x, color.x);
    // return to_float3(color.x, color.y, color.y);
    // return to_float3(color.z, color.y, color.y);
    // return to_float3(color.x, color.z, color.z);
    // return to_float3(color.y, color.z, color.z);
    
    // one channel used twice(one channel not used) but in yet another order
    // return to_float3(color.x, color.y, color.x);
    // return to_float3(color.x, color.z, color.x);
    // return to_float3(color.y, color.x, color.y);
    // return to_float3(color.y, color.z, color.y);
    // return to_float3(color.z, color.x, color.z);
    // return to_float3(color.z, color.y, color.z);
}

// # 12 Wave
__DEVICE__ float3 Wave(float3 color, float amount)
{
    color.x = (_sinf(color.x * amount) + 1.0f) * 0.5f;
    color.y = (_sinf(color.y * (amount * 2.0f )) + 1.0f) * 0.5f;
    color.z = (_sinf(color.z * (amount * 4.0f)) + 1.0f) * 0.5f;
    return color;
}

// # 13 Sepia
__DEVICE__ float3 Sepia(float3 color)
{
    float3 outputColor;
    outputColor.x = (color.x * 0.393f) + (color.y * 0.769f) + (color.z * 0.189f);
    outputColor.y = (color.x * 0.349f) + (color.y * 0.686f) + (color.z * 0.168f);    
    outputColor.z = (color.x * 0.272f) + (color.y * 0.534f) + (color.z * 0.131f);
    
    return outputColor;
}

// Used with InvertWithColor
__DEVICE__ float3 ColourToYPbPr2(float3 C)
{
  const mat3 Mat = to_mat3(
      0.299f,0.587f,0.114f,
      -0.168736f,-0.331264f,0.5f,
      0.5f,-0.418688f,-0.081312f);
  return mul_f3_mat3(C , Mat);
}

// Used with InvertWithColor
__DEVICE__ float3 YPbPrToColour(float3 YPbPr)
{
  const mat3 Mat = to_mat3(
      1.0f,0.0f,1.402f,
      1.0f,-0.34413f,-0.714136f,
      1.0f,1.772f,0.0f);
  return mul_f3_mat3(YPbPr , Mat);
}

// # 14 Invert black and white, keep color
__DEVICE__ float3 InvertWithColor(float3 color)
{
     color = ColourToYPbPr2(color);

    color.x = 1.0f - color.x;

    return YPbPrToColour(color);
}

// TODO normalize
// # 15 Multiply colorchannel
__DEVICE__ float3 MultiplyColorChannel(float3 color, float redFactor, float greenFactor, float blueFactor)
{
   return to_float3(color.x * redFactor, color.y * greenFactor, color.z * blueFactor);   
}


// TODO implement
// #16 - dithering // use to fix https://www.shadertoy.com/view/XsVBW1
__DEVICE__ float3 Dithering(float3 color)
{
    return color;
}


// # 17 - Mystery
__DEVICE__ float Mystery(float component, float colorCountPerComponent)
{
    float a = _floor(component * colorCountPerComponent) / colorCountPerComponent;
    float ditherThreshold = 0.5f;
  
    float step = 1.0f / colorCountPerComponent;
    if (a + step * ditherThreshold < component)
    return a+step;
}

// # 18 - Black and White Threshold
__DEVICE__ float3 BlackAndWhiteThreshold(float3 color, float threshold)
{
    float bright = 0.333333f * (color.x + color.y + color.z);
    float b = _mix(0.0f, 1.0f, step(threshold, bright));
    return to_float3_s(b);
}



__KERNEL__ void ManipulatingcolorsJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_INTSLIDER0(Modus, 1, 18, 12);

    float2 uv = fragCoord / iResolution;
float IIIIIIIIIIIII;
    //Get the pixel at xy from iChannel0
    float3 col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);//.rgb;
    
    if( Modus == 1)
      col = Contrast(col, 2.0f);

    else if( Modus == 2)
      col = Invert(col);
    
    else if( Modus == 3)
      col = Intensity(col, 2.0f);
    
    else if( Modus == 4)
      col = GreyScaleAverage(col);
    
    else if( Modus == 5)
      col = GreyScaleBasic(col);
    
    else if( Modus == 6)
      col = GreyScaleFancy(col);
    
    else if( Modus == 7)
      col = SingleColorChannelGrayScale(col);
    
    else if( Modus == 8)
      col = GrayScaleHSL(col);
    
    else if( Modus == 9)
      col = GrayScaleRange(col, 500.0f);
    
    else if( Modus == 10)
      // range 1 to 7
      col = BlackLight(col, 5.0f);
    
    else if( Modus == 11)
      col = Swizzle(col);
    
    else if( Modus == 12)
      col = Wave(col, 10.0f);
    
    else if( Modus == 13)
      col = Sepia(col);
    
    else if( Modus == 14)
      col = InvertWithColor(col);
    
    else if( Modus == 15)
      col = MultiplyColorChannel(col, 2.4f, 1.0f, 0.4f);
    
    // Dithering
    else if( Modus == 16)
      col = Dithering(col);  
    
    else if( Modus == 17)
    {
      col.x = Mystery(col.x, 4.0f);
      col.y = Mystery(col.y, 4.0f);
      col.z = Mystery(col.z, 4.0f);
    }
    else if( Modus == 18)
      col = BlackAndWhiteThreshold(col, 0.5f);
float qqqqqqqqqqqqqqqqq;    
    fragColor = to_float4_aw(col, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}