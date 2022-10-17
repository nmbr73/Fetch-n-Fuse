
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


//CLICK MOUSE AND MOVE
__KERNEL__ void MouseRotTheUniverseJipiFuse(float4 c, float2 o, float iTime, float2 iResolution, float4 iMouse)
{

  c  = to_float4_s(0);
  o -= iResolution/2.0f;
  float t=iTime, dt=256.0f;
  float2 _x,_y,_z;
  float2 v2 = to_float2((iMouse.x-iResolution.x/2.0f)/iResolution.x,(iMouse.y-iResolution.y/2.0f)/iResolution.y)*3.1415f;
  for (float i=0.0f;i<=16.0f;i+=0.25f){
      t  = iTime/16.0f+i*dt;
      _x = to_float2(_cosf(t*1.0f)*100.0f,_sinf(t*1.0f)*100.0f)*v2;
      _y = to_float2(_cosf(t*10.0f)*60.0f,_sinf(t*10.0f)*60.0f)*v2;
      _z = to_float2(_cosf(t*15.0f)*80.0f,_sinf(t*15.0f)*80.0f);
      c +=to_float4(0.0f,0.6f,0.8f,1.0f)*to_float4_s(length(o)/length(_x+_y+_z-o)/16.0f)/(i+0.5f)/2.0f;
  }

  SetFragmentShaderComputedColor(c);
}