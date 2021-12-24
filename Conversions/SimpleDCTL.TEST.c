

__KERNEL__ void SimpleDCTLKernel(
    __CONSTANTREF__ Params*  params,
    __TEXTURE2D__            iChannel0,
    __TEXTURE2D_WRITE__      dst
    )
{

  PROLOGUE(fragColor,fragCoord); // Standard Shader-Controls bereitstellen (iTime, iMouse und so)
  USE_CTRL_COLOR0(Farbenspiel); // Name der 'float4' Variable - wird nachher fuer das Control in der Fuse verwendet
  USE_CTRL_TINYSLIDER0(Ich_mach_Blau,0.0f,1.0f,0.5f); // Name der 'float' Variable, Min, Max, und Default-Wert (Default wird hier nicht, aber spaeter in der Fuse verwendet)
  USE_CTRL_TINYSLIDER1(slider,0.0f,1.0f,0.5f);

  float red          = iMouse.x/iResolution.x; // rot klebe ich mal an die Maus
  float green        = Farbenspiel.y; // gruen haengt am Gruen des Color0-Sliders
  float blue         = Ich_mach_Blau; // blue haengt am 0ten TinySlider
  float alpha        = slider; // alpha haengt am TinySlider 1

  fragColor=to_float4(red,green,blue,alpha);

  EPILOGUE(fragColor);
}
