
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect '/presets/tex00.jpg' to iChannel0


// ---- 8< ---- GLSL Number Printing - @P_Malin ---- 8< ----
// Creative Commons CC0 1.0f Universal (CC-0) 
// https://www.shadertoy.com/view/4sBSWW

__DEVICE__ float DigitBin(const in int x)
{
    return x==0?480599.0:x==1?139810.0:x==2?476951.0:x==3?476999.0:x==4?350020.0:x==5?464711.0:x==6?464727.0:x==7?476228.0:x==8?481111.0:x==9?481095.0:0.0;
}

__DEVICE__ float PrintValue(const in float2 fragCoord, const in float2 vPixelCoords, const in float2 vFontSize, const in float fValue, const in float fMaxDigits, const in float fDecimalPlaces)
{
    float2 vStringCharCoords = (fragCoord - vPixelCoords) / vFontSize;
    if ((vStringCharCoords.y < 0.0f) || (vStringCharCoords.y >= 1.0f)) return 0.0f;
  float fLog10Value = _log2f(_fabs(fValue)) / _log2f(10.0f);
  float fBiggestIndex = _fmaxf(_floor(fLog10Value), 0.0f);
  float fDigitIndex = fMaxDigits - _floor(vStringCharCoords.x);
  float fCharBin = 0.0f;
  if(fDigitIndex > (-fDecimalPlaces - 1.01f)) {
    if(fDigitIndex > fBiggestIndex) {
      if((fValue < 0.0f) && (fDigitIndex < (fBiggestIndex+1.5f))) fCharBin = 1792.0f;
    } else {    
      if(fDigitIndex == -1.0f) {
        if(fDecimalPlaces > 0.0f) fCharBin = 2.0f;
      } else {
        if(fDigitIndex < 0.0f) fDigitIndex += 1.0f;
        float fDigitValue = (_fabs(fValue / (_powf(10.0f, fDigitIndex))));
                float kFix = 0.0001f;
                fCharBin = DigitBin(int(_floor(mod_f(kFix+fDigitValue, 10.0f))));
      }    
    }
  }
    return _floor(mod_f((fCharBin / _powf(2.0f, _floor(fract(vStringCharCoords.x) * 4.0f) + (_floor(vStringCharCoords.y * 5.0f) * 4.0f))), 2.0f));
}

// ---- 8< -------- 8< -------- 8< -------- 8< ----

__DEVICE__ float keyPressed(int keyCode) {
  return texture(iChannel0, to_float2((float(keyCode) + 0.5f) / 256.0f, 0.5f/3.0f)).r;   
}

__KERNEL__ void KeycodeViewerFuse(float4 fragColor, float2 fragCoord, sampler2D iChannel0)
{

  fragColor = to_float4(0,0,0,1);
    if (fragCoord.x < 96.0f && fragCoord.y < 64.0f) {
        float n = -1.0f;
        for (int i = 0; i < 256; i++) if (bool(keyPressed(i))) n = float(i);
        fragColor = _mix(fragColor, to_float4(1), PrintValue(fragCoord, to_float2(0,10), to_float2(16,30), n, 3.0f, 0.0f)); 
    }


  SetFragmentShaderComputedColor(fragColor);
}