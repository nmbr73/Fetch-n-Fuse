

__KERNEL__ void SimpleDCTLKernel(
    __CONSTANTREF__ Params*  params,
    __TEXTURE2D__            iChannel0,
    __TEXTURE2D_WRITE__      dst
    )
{

  PROLOGUE;
  PARAM_ICOLOR0;

  float red          = iColor0.x;
  float green        = iColor0.y;
  float blue         = iColor0.z;
  float alpha        = iColor0.w;

  fragColor=to_float4(red,green,blue,alpha);

  EPILOGUE(fragColor);
}
