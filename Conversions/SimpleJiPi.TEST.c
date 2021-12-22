

__KERNEL__ void SimpleJiPiKernel(
    __CONSTANTREF__ Params*  params,
    __TEXTURE2D__            iChannel0,
    __TEXTURE2D_WRITE__      dst
    )
{

  PROLOGUE;
  PARAM_ICOLOR0;

  float red          = params->r1;
  float green        = params->g1;
  float blue         = params->b1;
  float alpha        = params->a1;

  fragColor=to_float4(red,green,blue,alpha);

  EPILOGUE(fragColor);
}
