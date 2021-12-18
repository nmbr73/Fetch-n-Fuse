

__KERNEL__ void SimpleDCTLKernel(
    __CONSTANTREF__ Params*  params,
    __TEXTURE2D__            iChannel0,
    __TEXTURE2D_WRITE__      dst
    )
{

  SHADER_PREAMBLE;

  float red          = 1.0f; //params->r;
  float green        = params->g;
  float blue         = params->b;
  float alpha        = params->a;

  fragColor=to_float4(red,green,blue,alpha);

  SHADER_EPILOGUE;
}
