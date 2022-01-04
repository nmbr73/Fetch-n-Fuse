

__KERNEL__ void SimpleJiPiFuse(float4 fragColor, float2 fragCoord, float2 iResolution)
{
  CONNECT_COLOR0(JiPi_Color, 0.290196078431373f, 0.254901960784314f, 0.164705882352941f, 1.0f);
  CONNECT_COLOR3(AnotherFunnyColor, 0.9f, 0.2f, 0.1f, 1.0f);
  CONNECT_TINYSLIDER0(JiPiSlider, 0.0f, 300.0f, 200.0f);
  CONNECT_SMALLSLIDER0(SmallJiPi, -1.5f, 1.5f, -0.5f);
  CONNECT_TINYINT0(IntegerEinfach, -5, 5, 2);
  CONNECT_CHECKBOX2(BlackisBeautiful, 0);
  CONNECT_CHECKBOX3(CheckThis, 0);
  CONNECT_POINT0(Look, 0.8f, 0.35f);
  


  float red          = JiPi_Color.x*AnotherFunnyColor.x;
  float green        = JiPi_Color.y*AnotherFunnyColor.y;
  float blue         = JiPi_Color.z*AnotherFunnyColor.z;
  float alpha        = JiPi_Color.w*AnotherFunnyColor.w;

  if (length(fragCoord-(Look*iResolution))<JiPiSlider && CheckThis)  red=1.0f,green=0.0,blue=1.0f,alpha=1.0f;

  if(BlackisBeautiful) red=0.0f,green=0.0,blue=0.0f,alpha=0.5f;


  fragColor=to_float4(red,green,blue,alpha);

  SetFragmentShaderComputedColor(fragColor);
}
