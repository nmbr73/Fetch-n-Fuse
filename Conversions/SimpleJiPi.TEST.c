

__KERNEL__ void SimpleJiPiFuse(float4 fragColor)
{
  CONNECT_COLOR0(color);

  float red          = color.x;
  float green        = color.y;
  float blue         = color.z;
  float alpha        = color.w;

  fragColor=to_float4(red,green,blue,alpha);

  SetFragmentShaderComputedColor(fragColor);
}
