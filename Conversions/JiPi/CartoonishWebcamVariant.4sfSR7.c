
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/e81e818ac76a8983d746784b423178ee9f6cdcdf7f8e8d719341a6fe2d2ab303.webm' to iChannel1
// Connect Image '/presets/webcam.png' to iChannel0
// Connect Image 'Preset: Keyboard' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// variant from okeli4408: https://www.shadertoy.com/view/XdXXzM#

//__DEVICE__ bool keyToggle(int ascii) {
//  return (texture(iChannel2,to_float2((0.5f+float(ascii))/256.0f,0.75f)).x > 0.0f);
//}

__DEVICE__ float showFlag(float2 p, float2 uv, float v, float2 iResolution) {
  float d = length(2.0f*(uv-p)*iResolution/iResolution.y);
  return   1.0f-step(0.06f*v,d) + smoothstep(0.005f,0.0f,_fabs(d-0.06f));
}

__DEVICE__ float showFlag(float2 p, float2 uv, bool flag, float2 iResolution) {
  return showFlag(p, uv, (flag) ? 1.0f: 0.0f, iResolution);
}


__DEVICE__ float dFdx(float value, float2 fragCoord, float2 iResolution)
{
   return ( value*fragCoord.x / iResolution.x );
}
__DEVICE__ float dFdy(float value, float2 fragCoord, float2 iResolution)
{
   return ( value*fragCoord.y / iResolution.y );
}




__KERNEL__ void CartoonishWebcamVariantFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  
  CONNECT_COLOR0(Color1, 0.0f, 0.0f, 0.0f, 1.0f);
  CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.0f);
  
  CONNECT_CHECKBOX0(BG_BW, 0);
  CONNECT_CHECKBOX1(BG_COL, 0);
  CONNECT_CHECKBOX2(FG_COL, 0);
  CONNECT_CHECKBOX3(FLIP, 0);
  CONNECT_CHECKBOX4(GAMMA, 0);
  CONNECT_CHECKBOX5(VID, 0);
         

  float2 uv = fragCoord / iResolution;

  // --- tunings 
  
  float2 mouse = swi2(iMouse,x,y) / iResolution;
  
  //bool BG_BW, BG_COL, FG_COL,FLIP,GAMMA, VID;
float IIIIIIIIIIIIIIIII;  
  if (iMouse.z<=0.0f) { // no mouse: autodemo
    float t = iTime/3.0f;
    float t0 = mod_f(t,3.0f); int i = (int)(t0);
    
    // (!BG_BW) = (i==0)
    BG_BW  = (i==1);
    BG_COL = (i==2);
    FLIP = (mod_f(t/3.0f,2.0f)>1.0f);
    GAMMA = (mod_f(t/6.0f,2.0f)>1.0f);
    VID = (iResolution.y<200.0f) || (iResolution.y<=0.0f) || (mod_f(t/12.0f,2.0f)>1.0f);
    FG_COL=false; // already there for BG + high gamma
    
    mouse = 0.5f*( 1.0f+ 0.5f*to_float2(_cosf(3.0f*t)+_cosf(t), _sinf(3.3f*t)+_cosf(0.7f*t) ) );
  }  
  else 
  {
    //BG_BW  = keyToggle(66);
    //BG_COL = keyToggle(67);
    //FG_COL = keyToggle(70);
    //FLIP   = keyToggle(32);
    //GAMMA  = keyToggle(71);
    //VID    = keyToggle(86);
  }
  
  float panel = showFlag(to_float2(0.25f,0.05f),uv, bool(BG_BW), iResolution)
              + showFlag(to_float2(0.35f,0.05f),uv, bool(BG_COL), iResolution)
              + showFlag(to_float2(0.45f,0.05f),uv, bool(FG_COL), iResolution)
              + showFlag(to_float2(0.55f,0.05f),uv, bool(GAMMA), iResolution)
              + showFlag(to_float2(0.65f,0.05f),uv, bool(FLIP), iResolution)
              + showFlag(mouse,uv, true, iResolution);
  
    // --- display 
  
  float3 col = (VID) ? swi3(texture(iChannel1, to_float2(1.0f-uv.x,uv.y)),x,y,z)
                     : swi3(texture(iChannel0, to_float2(1.0f-uv.x,uv.y)),x,y,z);//.rgb;
  
  // edge = norm of luminance derivative.
  float lum = col.x + col.y + col.z;
  float2 deriv = to_float2(dFdx(lum, fragCoord, iResolution), dFdy(lum, fragCoord, iResolution));
  float edge = _sqrtf(dot(deriv,deriv));
  // improve:
  edge = smoothstep(0.0f,mouse.x,edge);
  if (GAMMA) edge = _powf(edge, _expf(2.0f*2.0f*(mouse.y-0.7f))); // gamma contrasting
  
  if (FLIP) edge = 1.0f-edge;
  
  float3 bg = to_float3_s( (BG_BW) ? 1.0f: 0.0f);  // black vs white background
  if (BG_COL) bg = 1.0f-col;        // background = reverse video
  if (!FG_COL) col = 1.0f-bg;       // forground = rev of background
  
  // key transform: ink + paper
  col = _mix(col,bg,edge); 

  if (!GAMMA) col = pow_f3(col, to_float3_s(_expf(3.0f*mouse.y))); // gamma contrasting
  
  col.z = (col.z+0.2f*(col.x+col.y) < panel) ? panel:  col.z-panel;
  fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}