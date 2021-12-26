

/*
Shader Inputs
uniform vec3      iResolution;           // viewport resolution (in pixels)
uniform float     iTime;                 // shader playback time (in seconds)
uniform float     iTimeDelta;            // render time (in seconds)
uniform int       iFrame;                // shader playback frame
uniform float     iChannelTime[4];       // channel playback time (in seconds)
uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)
uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
uniform samplerXX iChannel0..3;          // input channel. XX = 2D/Cube
uniform vec4      iDate;                 // (year, month, day, time in seconds)
uniform float     iSampleRate;           // sound sample rate (i.e., 44100)
*/





__KERNEL__ void SimpleDCTLFuse(
  float4 fragColor,
  float2 fragCoord,
  float     iChannelTime [ ],
  float iTime   ,
  float2 iResolution,
  float4 iMouse,
  float3 * iChannelResolution
  )
{

  CONNECT_COLOR0(Farbenspiel); // Name der 'float4' Variable - wird nachher fuer das Control in der Fuse verwendet
  CONNECT_TINYSLIDER0(Ich_mach_Blau,0.0f,1.0f,0.5f); // Name der 'float' Variable, Min, Max, und Default-Wert (Default wird hier nicht, aber spaeter in der Fuse verwendet)
  CONNECT_TINYSLIDER1(slider,0.0f,1.0f,0.5f);

  float red          = iMouse.x/iResolution.x; // rot klebe ich mal an die Maus
  float green        = Farbenspiel.y; // gruen haengt am Gruen des Color0-Sliders
  float blue         = Ich_mach_Blau; // blue haengt am 0ten TinySlider
  float alpha        = slider; // alpha haengt am TinySlider 1

  fragColor=to_float4(red,green,blue,alpha);

  SetFragmentShaderComputedColor(fragColor);
}
