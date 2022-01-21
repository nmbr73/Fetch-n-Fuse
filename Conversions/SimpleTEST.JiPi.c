//*************************** Print Helper ***********************************
__DEVICE__ float DigitBin(const int x)
{
return x==0?480599.0f:x==1?139810.0f:x==2?476951.0f:x==3?476999.0f:x==4?350020.0f:x==5?464711.0f:x==6?464727.0f:x==7?476228.0f:x==8?481111.0f:x==9?481095.0f:0.0f;
}
__DEVICE__ float PrintValue(float2 fragCoord, float2 pixelCoord, float2 fontSize, float value, float digits, float decimals)
{
   float2 charCoord = (fragCoord - pixelCoord) / fontSize;
   if(charCoord.y < 0.0f || charCoord.y >= 1.0f) return 0.0f;
   float bits = 0.0f;
   float digitIndex1 = digits - _floor(charCoord.x)+ 1.0f;
   if(-digitIndex1 <= decimals)
	{
	float pow1 = _powf(10.0f, digitIndex1);
	float absValue = _fabs(value);
	float pivot = _fmaxf(absValue, 1.5f) * 10.0f;
	if(pivot < pow1)
		{
		if(value < 0.0f && pivot >= pow1 * 0.1f) bits = 1792.0f;
		}
	else if(digitIndex1 == 0.0f)
			{
			if(decimals > 0.0f) bits = 2.0f;
			}
		else	{
			value = digitIndex1 < 0.0f ? fract_f(absValue) : absValue * 10.0f;
			bits = DigitBin((int)(_fmod(value / pow1, 10.0f)));
			}
	}
   return _floor(_fmod(bits / _powf(2.0f, _floor(fract_f(charCoord.x) * 4.0f) + _floor(charCoord.y * 5.0f) * 4.0f), 2.0f));
}
__DEVICE__ float2 grid(int x, int y, float2 fontSize) { return to_float2(fontSize.x,fontSize.x) * to_float2(1,_ceil(fontSize.y/fontSize.x)) * to_float2(x,y) + to_float2_s(2); }
//****************************** Print helper end *****************************************




__KERNEL__ void SimpleTESTFuse(float4 fragColor, float2 fragCoord, float2 iResolution)
{
  CONNECT_COLOR0(JiPi_Color, 0.290196078431373f, 0.254901960784314f, 0.164705882352941f, 1.0f);
  CONNECT_COLOR3(AnotherFunnyColor, 0.9f, 0.2f, 0.1f, 1.0f);
  CONNECT_SLIDER0(JiPiSlider, 0.0f, 300.0f, 200.0f);
  CONNECT_SLIDER1(SmallJiPi, -1.5f, 1.5f, -0.5f);
  CONNECT_INTSLIDER0(IntegerEinfach, -5, 5, 2);
  CONNECT_CHECKBOX2(BlackisBeautiful, 0);
  CONNECT_CHECKBOX3(CheckThis, 0);

  CONNECT_POINT0(Look, 0.8f, 0.35f);
  CONNECT_SCREW0(JiPiScrewer,0.1f ,0.5f , 0.25f );

  #define typ  0

  CONNECT_BUTTON0(JiPiBtn, 0, Left, Right, Mid );


  if (typ == 0)  JiPiBtn = (int)JiPiBtn-1;
  if (typ == 1)  JiPiBtn = (int)JiPiBtn>>1;  //Korrektur, weil Basicbutton nicht ausblendbar -> Fertige Fuse hat kein Basicbutton, daher hier die Anpassung, kann ins Define !toDo! - ev auch Defines für die Namen anlegen . also #define Left = 0

  float red          = JiPi_Color.x*AnotherFunnyColor.x;
  float green        = JiPi_Color.y*AnotherFunnyColor.y;
  float blue         = JiPi_Color.z*AnotherFunnyColor.z;
  float alpha        = JiPi_Color.w*AnotherFunnyColor.w;

  if (length(fragCoord-(Look*iResolution))<JiPiSlider && CheckThis)  red=1.0f,green=0.0,blue=1.0f,alpha=1.0f;

  if(BlackisBeautiful) red=0.0f,green=0.0,blue=0.0f,alpha=0.5f;


  fragColor=to_float4(red,green,blue,alpha);

  float4 var = to_float4(JiPiScrewer,JiPiSlider,IntegerEinfach,JiPiBtn);

    //************************* Print Values *******************************
    float printsize = _fmaxf(iResolution.x/960,1.0f);
    float2 fontSize = to_float2(4,5) * to_float2(5,3)*printsize;// * 1.5f;
    float3 vColour = to_float3(fragColor.x,fragColor.y,fragColor.z);//to_float3_s(0.0f);// to_float3(0.2f, 0.05f, 0.1f);
    float fDecimalPlaces = 4.0f;
    float fDigits = 6.0f;

    int column = 0;
    int row   = 16;

    if (JiPiBtn == 1)
    {
      column = 30;
      row    =  8;
    }

    if (JiPiBtn == 2)
    {
      column = 15;
      row    = 10;
    }

    vColour = _mix( vColour, to_float3(01.0f, 00.0f, 0.0f), PrintValue(fragCoord, grid(column,row+6,fontSize), fontSize, var.x, fDigits, fDecimalPlaces+4.0f));
    vColour = _mix( vColour, to_float3(00.0f, 01.0f, 0.0f), PrintValue(fragCoord, grid(column,row+4,fontSize), fontSize, var.y, fDigits, fDecimalPlaces));
    vColour = _mix( vColour, to_float3(00.0f, 00.0f, 01.0f), PrintValue(fragCoord, grid(column,row+2,fontSize), fontSize, var.z, fDigits, fDecimalPlaces));
    vColour = _mix( vColour, to_float3(01.0f, 01.0f, 01.0f), PrintValue(fragCoord, grid(column,row,fontSize), fontSize, var.w, fDigits, fDecimalPlaces));
    //************************* End Print Values ***************************


  fragColor = to_float4_aw(vColour,alpha);




  SetFragmentShaderComputedColor(fragColor);
}
