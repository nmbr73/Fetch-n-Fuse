

__KERNEL__ void SimpleJiPiFuse(float4 fragColor)
{
  CONNECT_COLOR0(JiPi_Color, 0.290196078431373f, 0.254901960784314f, 0.164705882352941f, 1.0f);
  CONNECT_COLOR3(AnotherFunnyColor, 0.9f, 0.2f, 0.1f, 1.0f);
  CONNECT_TINYSLIDER0(JiPiSlider, 0.0f, 10.0f, 5.0f);
  CONNECT_SMALLSLIDER0(SmallJiPi, -1.5f, 1.5f, -0.5f);
  CONNECT_TINYINT0(IntegerEinfach, -5, 5, 2);

  float red          = JiPi_Color.x*AnotherFunnyColor.x*JiPiSlider;
  float green        = JiPi_Color.y*AnotherFunnyColor.y;
  float blue         = JiPi_Color.z*AnotherFunnyColor.z;
  float alpha        = JiPi_Color.w*AnotherFunnyColor.w;

  fragColor=to_float4(red,green,blue,alpha);

  SetFragmentShaderComputedColor(fragColor);
}
