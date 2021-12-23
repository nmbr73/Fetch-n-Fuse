

__KERNEL__ void SimpleDCTLKernel(
    __CONSTANTREF__ Params*  params,
    __TEXTURE2D__            iChannel0,
    __TEXTURE2D_WRITE__      dst
    )
{

  PROLOGUE(fragColor,fragCoord);
  USE_CTRL_COLOR3(farbenspiel);
  USE_CTRL_TINYSLIDER2(zumausprobieren,0.0f,1.0f,0.5f); // Name der Variablen, min, max, default

  float red          = farbenspiel.x;
  float green        = farbenspiel.y;
  float blue         = zumausprobieren;
  float alpha        = farbenspiel.w;

  fragColor=to_float4(red,green,blue,alpha);

  EPILOGUE(fragColor);
}
