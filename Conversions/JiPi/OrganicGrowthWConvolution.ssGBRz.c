
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// blur convolution
/*
const int r = 1;
const int l = (2*r+1)*(2*r+1);
float m[l] = float[](
    0.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.
);
float mt = 4.01f;
*/



__KERNEL__ void OrganicGrowthWConvolutionFuse__Buffer_A(float4 color, float2 coord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    
    CONNECT_SLIDER0(mt, -1.0f, 30.0f, 16.1f);
    CONNECT_INTSLIDER0(r, 0, 5, 1);

    coord+=0.5f;

    // organic growth convolution
    //const int r = 1;
    int l = (2*r+1)*(2*r+1);
    float m[9] = {
                  -1.0f, 5.0f, -1.0f,
                   5.0f, 0.0f, 5.0f,
                  -1.0f, 5.0f, -1.0f
                 };
    //float mt = 16.1f;

    //swi3S(color,x,y,z, to_float3_s(0.0f));
    color = to_float4_s(0);

    float2 uv = coord / iResolution;
    float2 mouseUv = swi2(iMouse,x,y) / iResolution;
    
    float ratio = iResolution.x/iResolution.y;
    
    //uv.x*=ratio;
    
    for (int x = -1; x <=1; x++) {
        for (int y = -1; y <=1; y++) {
            int i = (y+1)*3+x+1;
            float v = m[i] / mt;
            color += 0.9f * v * texture(iChannel0,to_float2(uv.x+(float)(x)/iResolution.x,uv.y+(float)(y)/iResolution.y));
        }
    }        
    
    if (distance_f2(to_float2(uv.x*ratio,uv.y), to_float2(mouseUv.x*ratio, mouseUv.y)) < 0.05f && iMouse.z > 0.0f) {
    //  if (distance_f2(uv, mouseUv) < 0.05f && iMouse.z > 0.0f) {
        //swi3S(color,x,y,z, 1.0f - swi3(color,x,y,z));
        color = to_float4_s(1.0f) - color;
    }
    
    //swi3S(color,x,y,z, clamp(swi3(color,x,y,z), 0.0f, 1.0f));
    color = clamp(color, 0.0f, 1.0f);
    
    
    
  SetFragmentShaderComputedColor(color);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// This just paints whatever is in BufferA
__KERNEL__ void OrganicGrowthWConvolutionFuse(float4 color, float2 coord, float2 iResolution, sampler2D iChannel0)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);

    float2 uv = coord / iResolution;
 
    color = _tex2DVecN(iChannel0,uv.x,uv.y,15);    

    color -= Color-0.5f;
    
    if (color.w == 0.0f)
      color.w = Color.w;

  SetFragmentShaderComputedColor(color);
}