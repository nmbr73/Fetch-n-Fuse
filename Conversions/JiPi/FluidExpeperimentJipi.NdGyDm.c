
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__KERNEL__ void FluidExpeperimentJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);

  fragCoord+=0.5f;

  float dx = 8.0f;
  float2 uv = fragCoord;
  float4 left   = texture(iChannel0, (uv-to_float2(-dx,0))/iResolution);
  float4 right  = texture(iChannel0, (uv-to_float2( dx,0))/iResolution);
  float4 top    = texture(iChannel0, (uv-to_float2(0,-dx))/iResolution);
  float4 bottom = texture(iChannel0, (uv-to_float2(0, dx))/iResolution);
  float4 st     = texture(iChannel0, uv/iResolution) * 2.0f - 1.0f;
  float4 lst    = texture(iChannel0, (uv-swi2(st,x,y))/iResolution);
  float2 grad   = to_float2( right.z - left.z, bottom.z - top.z );
  float diff  = ( left.z + right.z + top.z + bottom.z ) / 4.0f;
  float div   = ( ( right.x - left.x ) + ( bottom.y - top.y ) ) / 4.0f;
  //swi2(lst,x,y) += grad;
  lst.x += grad.x;
  lst.y += grad.y;
  
  lst.z = diff - div * 0.05f;
  swi3S(lst,x,y,z, _mix(
                        swi3(lst,x,y,z),
                        normalize( swi3(lst,x,y,z) * 2.0f - 1.0f ) * 0.5f + 0.5f,
                        0.8
                        ));
  lst.w = 1.0f;
    
  if( iFrame < 1 || iMouse.z > 0.0f || Reset)
    {
        lst = to_float4_s(0);
        if     ( fragCoord.x < 1.0f )                 { lst.x = -1.0f; }
        else if( fragCoord.x > iResolution.x - 1.0f ) { lst.x =  1.0f; }
        if     ( fragCoord.y < 1.0f )                 { lst.y = -1.0f; }
        else if( fragCoord.y > iResolution.y - 1.0f ) { lst.y =  1.0f; }
    }
    fragColor = lst;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void FluidExpeperimentJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    fragColor = _tex2DVecN( iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}