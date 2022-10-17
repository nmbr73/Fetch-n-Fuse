

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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

vec3 Contrast(vec3 color, float contrast)
{
  // increase values above 0.5 and decrease below 0.5
  color -= 0.5;
  color *= contrast;
  color += 0.5;
  return color;
}

// #2 = Invert

vec3 Invert(vec3 color)
{
  return 1.0 - color;
}

// #3 = Intensity

vec3 Intensity(vec3 color, float factor)
{
  return color *= factor;
}

// #4 = Grey Scale Average

vec3 GreyScaleAverage(vec3 color)
{
  return vec3((color.r + color.g + color.b) / 3.0);
}

// #5 = Grey Scale Basic

// Tint green
vec3 GreyScaleBasic(vec3 color)
{ 
  return vec3(color.r * 0.3 + color.g * 0.59 + color.b * 0.11);
}

// #6 = Grey Scale Fancy

vec3 GreyScaleFancy(vec3 color)
{
  vec3 lumCoeff = vec3(0.3, 0.59, 0.11);
  color = vec3(dot(color, lumCoeff));
  return color;
}

// #7 = Grey Scale Single Channel

// could use any of the 3 channels
vec3 SingleColorChannelGrayScale(vec3 color)
{
    return vec3(color.r);
    // return vec3(color.g);
    // return vec3(color.b);
}

// #8 = Grey Scale HSL

vec3 GrayScaleHSL(vec3 color)
{
  return vec3(max(color.r, max(color.g, color.b)) + min(color.r, min(color.g, color.b)) / 2.0);
}

// #9 = Grey Scale Range

vec3 GrayScaleRange(vec3 color, float NumberOfShades)
{
  float ConversionFactor = 255.0 / (NumberOfShades - 1.0);
  float AverageValue = (color.r + color.g + color.b) / 3.0;
  float Gray = ((AverageValue / ConversionFactor) + 0.5) * ConversionFactor;
  return vec3(Gray);
}

// #10 = Black Light

vec3 BlackLight(vec3 color, float fxWeight)
{
  float lum = color.r + color.g + color.b / 3.0;
  lum = (222.0 * color.r + 707.0 * color.g + 71.0 * color.b) / 1000.0;
  // R = Abs(R - L) * fxWeight
  color.r = abs(color.r - lum) * fxWeight;
  color.r = clamp(color.r, 0.0, 1.0);
  color.g = abs(color.g - lum) * fxWeight;
  color.g = clamp(color.g, 0.0, 1.0);
  color.b = abs(color.b - lum) * fxWeight;
  color.b = clamp(color.b, 0.0, 1.0);

  return color;
}

// TODO It would be nice to slide show thru options with iTime
// # 11 Swizzle
vec3 Swizzle(vec3 color)
{
    // each color used once
    return vec3(color.g, color.b, color.r);
    // return vec3(color.b, color.r, color.g);
    // return vec3(color.b, color.r, color.g);
    // return vec3(color.b, color.g, color.r);
    // return vec3(color.g, color.r, color.b);
    // return vec3(color.r, color.b, color.g);
    
    // one channel used twice(one channel not used)
    // return vec3(color.r, color.r, color.g);
    // return vec3(color.r, color.r, color.b);
    // return vec3(color.g, color.g, color.r);
    // return vec3(color.g, color.g, color.b);
    // return vec3(color.b, color.b, color.r);
    // return vec3(color.b, color.b, color.g);
    
    // one channel used twice(one channel not used) but in different order
    // return vec3(color.g, color.r, color.r);
    // return vec3(color.b, color.r, color.r);
    // return vec3(color.r, color.g, color.g);
    // return vec3(color.b, color.g, color.g);
    // return vec3(color.r, color.b, color.b);
    // return vec3(color.g, color.b, color.b);
    
    // one channel used twice(one channel not used) but in yet another order
    // return vec3(color.r, color.g, color.r);
    // return vec3(color.r, color.b, color.r);
    // return vec3(color.g, color.r, color.g);
    // return vec3(color.g, color.b, color.g);
    // return vec3(color.b, color.r, color.b);
    // return vec3(color.b, color.g, color.b);
}

// # 12 Wave
vec3 Wave(vec3 color, float amount)
{
 	color.r = (sin(color.r * amount) + 1.0) * 0.5;
    color.g = (sin(color.g * (amount * 2.0 )) + 1.0) * 0.5;
    color.b = (sin(color.b * (amount * 4.0)) + 1.0) * 0.5;
    return color;
}

// # 13 Sepia
vec3 Sepia(vec3 color)
{
    vec3 outputColor;
    outputColor.r = (color.r * 0.393) + (color.g * 0.769) + (color.b * 0.189);
    outputColor.g = (color.r * 0.349) + (color.g * 0.686) + (color.b * 0.168);    
    outputColor.b = (color.r * 0.272) + (color.g * 0.534) + (color.b * 0.131);
    
    return outputColor;
}

// Used with InvertWithColor
vec3 ColourToYPbPr2(vec3 C)
{
	const mat3 Mat = mat3(
  		0.299,0.587,0.114,
  		-0.168736,-0.331264,0.5,
  		0.5,-0.418688,-0.081312);
	return C * Mat;
}

// Used with InvertWithColor
vec3 YPbPrToColour(vec3 YPbPr)
{
	const mat3 Mat = mat3(
  		1.0,0.0,1.402,
  		1.0,-0.34413,-0.714136,
  		1.0,1.772,0.0);
	return YPbPr * Mat;
}

// # 14 Invert black and white, keep color
vec3 InvertWithColor(vec3 color)
{
   	color = ColourToYPbPr2(color);

  	color.x = 1.0 - color.x;

  	return YPbPrToColour(color);
}

// TODO normalize
// # 15 Multiply colorchannel
vec3 MultiplyColorChannel(vec3 color, float redFactor, float greenFactor, float blueFactor)
{
 	return vec3(color.r * redFactor, color.g * greenFactor, color.b * blueFactor);   
}


// TODO implement
// #16 - dithering // use to fix https://www.shadertoy.com/view/XsVBW1
vec3 Dithering(vec3 color)
{
    return color;
}


// # 17 - Mystery
float Mystery(float component, float colorCountPerComponent)
{
    float a = floor(component * colorCountPerComponent) / colorCountPerComponent;
    float ditherThreshold = 0.5;
    
    float step = 1.0 / colorCountPerComponent;
    if (a + step * ditherThreshold < component)
    return a+step;
}

// # 18 - Black and White Threshold
vec3 BlackAndWhiteThreshold(vec3 color, float threshold)
{
    float bright = 0.333333 * (color.r + color.g + color.b);
    float b = mix(0.0, 1.0, step(threshold, bright));
    return vec3(b);
}



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;

      //Get the pixel at xy from iChannel0
  	vec3 col = texture(iChannel0, uv).rgb;
    
    #if TUTORIAL == 1
		col = Contrast(col, 2.0);

	#elif TUTORIAL == 2
    	col = Invert(col);
    
    #elif TUTORIAL == 3
    	col = Intensity(col, 2.0);
    
    #elif TUTORIAL == 4
    	col = GreyScaleAverage(col);
    
    #elif TUTORIAL == 5
    	col = GreyScaleBasic(col);
    
    #elif TUTORIAL == 6
    	col = GreyScaleFancy(col);
    
    #elif TUTORIAL == 7
    	col = SingleColorChannelGrayScale(col);
    
    #elif TUTORIAL == 8
    	col = GrayScaleHSL(col);
    
    #elif TUTORIAL == 9
        col = GrayScaleRange(col, 500.0);
    
    #elif TUTORIAL == 10
    	// range 1 to 7
  		col = BlackLight(col, 5.0);
    
    #elif TUTORIAL == 11
        col = Swizzle(col);
    
    #elif TUTORIAL == 12
    	col = Wave(col, 10.0);
    
    #elif TUTORIAL == 13
        col = Sepia(col);
    
    #elif TUTORIAL == 14
    	col = InvertWithColor(col);
    
    #elif TUTORIAL == 15
    	col = MultiplyColorChannel(col, 2.4, 1.0, 0.4);
    
    // Dithering
    #elif TUTORIAL == 16
    	
    
    #elif TUTORIAL == 17
    	col.r = Mystery(col.r, 4.0);
        col.g = Mystery(col.g, 4.0);
        col.b = Mystery(col.b, 4.0);
    #elif TUTORIAL == 18
    	col = BlackAndWhiteThreshold(col, 0.5);
    
    #endif
    
    
  	fragColor = vec4(col, 1.0);
}

 
