

__KERNEL__ void SimpleJiPiFuse(float4 fragColor)
{
  CONNECT_COLOR0(color, 0.290196078431373f, 0.254901960784314f, 0.164705882352941f, 1.0f);

  float red          = color.x;
  float green        = color.y;
  float blue         = color.z;
  float alpha        = color.w;

  fragColor=to_float4(red,green,blue,alpha);

  SetFragmentShaderComputedColor(fragColor);
}
