
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect '/presets/webcam.png' to iChannel0


__KERNEL__ void VideoFiltersFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  float2 p = fragCoord/iResolution;

  float4 col = _tex2DVecN(iChannel0,p.x,p.y,15);


  //Desaturate
  if(p.x<0.25f)
  {
    col = to_float4_s( (col.x+col.y+col.z)/3.0f );
  }
  //Invert
  else if (p.x<0.5f)
  {
    col = to_float4_s(1.0f) - _tex2DVecN(iChannel0,p.x,p.y,15);
  }
  //Chromatic aberration
  else if (p.x<0.75f)
  {
    float2 offset = to_float2(0.01f,0.0f);
    col.x = _tex2DVecN(iChannel0, p.x+offset.x,p.y+offset.y,15).x;
    col.y = _tex2DVecN(iChannel0,p.x,p.y,15).y;
    col.z = _tex2DVecN(iChannel0, p.x+offset.y,p.y+offset.x,15).z;
  }
  //Color switching
  else
  {
    swi3(col,x,y,z) = _tex2DVecN(iChannel0,p.x,p.y,15).brg;
  }


  //Line
  if( mod_f(_fabs(p.x+0.5f/iResolution.y),0.25f)<0.5f/iResolution.y )
    col = to_float4_s(1.0f);


    fragColor = col;


  SetFragmentShaderComputedColor(fragColor);
}