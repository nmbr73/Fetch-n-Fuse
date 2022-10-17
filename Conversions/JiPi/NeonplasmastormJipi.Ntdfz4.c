
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel1
// Connect Image 'Texture: Organic 4' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Neon Plasma Storm (Unoptomised, so it can be a learning aid to others)
// by Nickenstein79, Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// Inspired by mu6k's Noise plasma, which can found at: https://www.shadertoy.com/view/XsS3Rd

// This is my first attempt with Shadertoy.
// I've heavily commented it and left it completely unoptimised as a learning aid to other newbs.
// There are plenty of tweekable paramaters so folks can see and learn how it all works. :)
// (A fully optomised version will be added later. But that one will be an impenitrable wall of trig to new comers.)

// Updated on 09/12/2015 to include a some additional toggle-able CRT-filter effects.

// Awesome tune: Clockwork, by Binster/.mepegasus - https://soundcloud.com/binster

#define NUM_LAYERS           (7)
#define LAYER_SEPERATION_FACTOR    (0.041f)
#define ZOOM_FACTOR_PERIOD       (40.0f)    // The time taken for a full zoom-out/zoom-in phase
#define ZOOM_FACTOR_MIN       (0.5f)
#define ZOOM_FACTOR_MAX       (2.8f)
#define SCROLL_SPEED_AT_MIN_ZOOM  (4.0f)
#define SCROLL_SPEED_AT_MAX_ZOOM  (12.000001f)
#define ROTATION_MATRIX_MAX_SKEW  (0.4f)    // The maximum skewing/warping of the rotation matrix
#define ROTATION_MATRIX_SKEW_PERIOD (7.4f)    // The time taken to fully skew and un-skew the rotation matrix

//#define CRT_FILTER_ON              // Toggle this line to dissable/enable the CRT Filter
//#define CRT_VIGNETTE_ON            // Toggle this line to dissable/enable the CRT-edge vignette
//#define CRT_EXTRA_NOISE_ON          // Toggle this line to dissable/enable the CRT-Extra Noise

#define TWO_PI            (6.283185307179586476925286766559f)
#define LAYER_STEP_SIZE       (1.0f/float(NUM_LAYERS))

// Returns a psuedo-random float value (in the range 0.0f to 1.0f) generated from a 2D vector
// works by combining the input seed value with prime numbers.
// Using primes causes sequential inputs (such as pixel coordinates) to still generate non-sequential outputs.
__DEVICE__ float Hash_From2D(float2 Vec)
{
    float f = Vec.x + Vec.y * 37.0f;
    return fract(_sinf(f)*104003.9f);
}
 
// Returns a sin wave driven value that oscilates between Min and Max over the given time period
__DEVICE__ float OscilateSinScalar(float Min, float Max, float Period, float iTime)
{
  return (Max-Min) * (_sinf(iTime*TWO_PI/Period)*0.5f+0.5f) + Min;
}

// Returns the interpolant (fraction) of CurrentValue between Min and Max in the range (0.0f to 1.0f)
__DEVICE__ float GetInterpolant(float Min, float Max, float CurrentValue)
{
    return (CurrentValue-Min) / (Max-Min);
}

// returns a skewed Z rotation matrix (The skewing changes over time)
__DEVICE__ mat2 ZRotate_Skewed(float Angle, float iTime)
{
    // Get a value to skew the rotion matrix with, which bounces between 0.0f and ROTATION_MATRIX_MAX_SKEW, over a given time period.
    float Skew = 1.0f - OscilateSinScalar(0.0f, ROTATION_MATRIX_MAX_SKEW, ROTATION_MATRIX_SKEW_PERIOD, iTime);
    
    // As the input angle is based on an incrimental timer multiplied by magic numbers and getting the cosine of these values,
    // this generates an animated angle which drifts chaotically and 
    // unpredictably over time (Like a double-pendulum's motion. Actually, a triple-pendulum as we combine three _cosf() values).
    // twiddle with the multiply values being passed to _cosf() until you like the motion of the simulation.
    Angle = _cosf(Angle*0.1f)*_cosf(Angle*0.7f)*_cosf(Angle*0.73f)*2.0f;    
    
    // build and return the skewed Z-rotation matrix
    return to_mat2(_sinf(Angle * Skew),_cosf(Angle),-_cosf(Angle * Skew),_sinf(Angle));
}

// sample the input textures which create our desired material surface effect
__DEVICE__ float4 SampleMaterial(float2 uv, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1)
{
   float t = iTime * 0.5f;
   
   // sample texture 0, just the red channel to get an initial grey-scale value in range 0.0f to 1.0
   float Sample0 = texture(iChannel0, uv * 0.1f ).z;
    
   // Modify Sample0 by subtracting circlular patterns that grow across the texture over time
   // fiddle with the numbers to get differening circular patterns
   Sample0 -= 0.5f + _sinf(t + _sinf(uv.x) + _sinf(uv.y)) * 0.7f; 
   
   // adjust brightness
   Sample0 *= 1.6f;
   
   // ensure no negative values with _fabs(), this also causes a double
   // concentric-circle pattern as _sinf() returns -1 to +1 values.
   Sample0 = _fabs(Sample0);
 
   // invert these fat bright negative-circles to get thin bright circles and curves
   Sample0 = 1.0f/(Sample0*10.0f+1.0f);
    
   // make a greyscale colour from Sample0, then multiply it by a full-colour sample from eleswhere on texture 0
   float4 Colour = to_float4_s(Sample0) * texture(iChannel0, uv * 0.05f);
    
   // multiple this colour by a scrolling sample from texture 1 (random coloured noise texture)
   // Adjust the multiplyer to expand or shrink the range of the coloured noise texture in use
   return Colour * texture(iChannel1,(uv + (iTime*1.3f)) * 0.001735f);
}

// Generate fast downward cycling fat scanlines
__DEVICE__ float scanline(float2 uv, float TexHeight, float2 iResolution, float iTime) 
{
  return _sinf(iResolution.y * uv.y * 0.7f - iTime * 10.0f);
}

// Generate slow upward cycling thin scanlines
__DEVICE__ float slowscan(float2 uv, float TexHeight, float2 iResolution, float iTime) 
{
  return _sinf(iResolution.y * uv.y * 0.02f + iTime * 6.0f);
}

// warp uv coordinates to emulate a curved crt screen
__DEVICE__ float2 crt_bend_uv_coords(float2 coord, float bend)
{
  // put in symmetrical coords
  //coord = (coord - 0.5f) * 2.0f;
  //coord *= 0.5f;  
  
  // deform coords
  coord.x *= 1.0f + _powf((_fabs(coord.y) / bend), 2.0f);
  coord.y *= 1.0f + _powf((_fabs(coord.x) / bend), 2.0f);

  // transform back to 0.0f - 1.0f space
  coord  = (coord / 1.0f) + 0.5f;

  return coord;
}

// wibble the screen, crt magnetic interferance distortion effect
__DEVICE__ float2 scandistort(float2 uv, float iTime, __TEXTURE2D__ iChannel1) 
{
  float scan1 = clamp(_cosf(uv.y * 2.0f + iTime), 0.0f, 1.0f);
  float scan2 = clamp(_cosf(uv.y * 2.0f + iTime + 4.0f) * 10.0f, 0.0f, 1.0f);
  float amount = scan1 * scan2 * uv.x; 
  uv.x -= 0.05f * _mix( texture(iChannel1, to_float2(uv.x, amount)).x * amount, amount, 0.9f );
    
  return uv;
}

// CRT edge vignette
__DEVICE__ float3 CRT_Vignette(float3 Colour, float2 uv, float2 iResolution)
{
  uv.x /= iResolution.x / iResolution.y;
  float Vignette = clamp(_powf(_cosf(uv.x * 3.1415f), 1.3f) * _powf(_cosf(uv.y * 3.1415f), 1.3f) * 50.0f, 0.0f, 1.0f);
  Colour *= Vignette;
    
  return Colour;    
}

// Cathode ray tube filter, to look like an old cathode ray-tube monitor with scanlines, and V-Hold, etc...
__DEVICE__ float3 CRT_Filter(float3 Colour, float2 uv, float2 iResolution, float iTime)
{
#ifdef CRT_EXTRA_NOISE_ON    
    Colour.x += Hash_From2D(uv * iTime * 911.911f * 4.0f) * 0.19f;
    Colour.y += Hash_From2D(uv * iTime * 563.577f * 4.0f) * 0.19f;
    Colour.z += Hash_From2D(uv * iTime * 487.859f * 4.0f) * 0.19f;
#endif    
    
  float2 sd_uv = uv;
  float2 crt_uv = crt_bend_uv_coords(sd_uv, 2.0f);    
  float3 scanline_Colour;
  float3 slowscan_Colour;
  scanline_Colour.x = scanline(crt_uv, iResolution.y, iResolution, iTime);
  slowscan_Colour.x = slowscan(crt_uv, iResolution.y, iResolution, iTime);
  scanline_Colour.y = scanline_Colour.z = scanline_Colour.x;
  slowscan_Colour.y = slowscan_Colour.z = slowscan_Colour.x;
  Colour = _mix(Colour, _mix(scanline_Colour, slowscan_Colour, 0.5f), 0.04f);
    
  // apply the CRT-vignette filter
#ifdef CRT_VIGNETTE_ON
  Colour = CRT_Vignette(Colour, uv, iResolution);
#endif     
    
  return Colour;
}

// Perform some post processing effects on the output colour
__DEVICE__ float3 PostProcessColour(float3 Colour, float2 uv, float2 iResolution, float iTime)
{
  // Vignette (Darken the pixels nearer the corners of the screen)
  Colour -= to_float3_s(length(uv*0.1f));
    
  // Add some random noise to the colour with the Hashing function
  Colour += Hash_From2D(uv*iTime*0.01f)*0.02f;
    
  // apply the CRT-screen filter
#ifdef CRT_FILTER_ON
  Colour = CRT_Filter(Colour, uv, iResolution, iTime);
#endif 
    
  // Approximate the brightness of the colour by using it as a 3d spacial vector and getting its length in colour space
  float Brightness = length(Colour);
    
  // inrease the colour contrast, by dimming the darker colours and brightening the lighter ones, 
  // via linear interpolation of the colour and its approximated brightness value
  Colour = _mix(Colour, to_float3_s(Brightness), Brightness - 0.5f);
    
  return Colour;
}

__KERNEL__ void NeonplasmastormJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  // get a UV chord for each texel in the range -1.0f to 1.0f (so we can scale/rotate everything from the center of the screen)
  float2 uv = fragCoord / iResolution - 0.5f;
    
  // modify the X component of the uv, by the screen's aspect ratio to avoid stretching of the input textures in width
  uv.x *= iResolution.x / iResolution.y;
    
  // start with black
  float3 Colour = to_float3(0.0f, 0.0f, 0.0f);
    
  // Determine a scale value that oscilates between MIN_ZOOM and MAX_ZOOM over the desired time period
  float ScaleValue = OscilateSinScalar(ZOOM_FACTOR_MIN, ZOOM_FACTOR_MAX, ZOOM_FACTOR_PERIOD, iTime);
  
  // Determine a speed to scroll through the texture space.
  // This works by generating a speed value between MIN_SCROLL and MAX_SCROLL
  // which is guided by the current zoom value, such that when the texture
  // is fully zoomed out, the scroll speed is at the desired maximum, and when
  // the texture is fully zoomed in, the scroll speed is at the desired minimum.
  float ScrollInterpolant = GetInterpolant(ZOOM_FACTOR_MIN, ZOOM_FACTOR_MAX, ScaleValue);
  // mix performs a linear interpolation between two inputs
  float ScrollValue = _mix(SCROLL_SPEED_AT_MIN_ZOOM, SCROLL_SPEED_AT_MAX_ZOOM, ScrollInterpolant); 
    
  // Sum the colour contribution of each layer   
  for (float i = 0.0f; i < 1.0f; i += LAYER_STEP_SIZE)
  {
    // clone the uv, so the original is preserved for each itteration of the loop
    float2 uv2 = uv;
   
    // Rotate with a Z-Rotation matrix that skewes over time 
    // (Giving a slighlty different input angle for each layer, as i-squared is combined in)
    uv2 = mul_f2_mat2(uv , ZRotate_Skewed(iTime * i*i * 12.0f * LAYER_SEPERATION_FACTOR, iTime));
    
    // Scale (Again, giving a slighlty different scale for each layer, as i-squared is combined in)
    uv2 *= ScaleValue * (i*i+1.0f); 

    // Scroll the sampling position over time
    swi2S(uv2,x,y, swi2(uv2,x,y) + ScrollValue + iTime*0.125f);
    
    // sample the material, building up each layer's colour contribution
    Colour += swi3(SampleMaterial(uv2, iTime, iChannel0, iChannel1),x,y,z)*LAYER_STEP_SIZE*3.5f;
  }
  
  // Perform some post processing on the accumulated colour layers
  Colour = PostProcessColour(Colour, uv, iResolution, iTime);
    
  // set the generated colour as the final output pixel colour
  fragColor = to_float4_aw(Colour, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}