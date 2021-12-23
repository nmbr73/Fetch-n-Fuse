

__KERNEL__ void SimpleDCTLKernel(
    __CONSTANTREF__ Params*  params,
    __TEXTURE2D__            iChannel0,
    __TEXTURE2D_WRITE__      dst
    )
{

  PROLOGUE(fragColor,fragCoord);
  USE_CTRL_COLOR3(farbenspiel);
  USE_CTRL_SLIDER2(zumausprobieren);

  float red          = farbenspiel.x;
  float green        = farbenspiel.y;
  float blue         = zumausprobieren / 100.0f;
  float alpha        = farbenspiel.w;

  fragColor=to_float4(red,green,blue,alpha);

  EPILOGUE(fragColor);
}
