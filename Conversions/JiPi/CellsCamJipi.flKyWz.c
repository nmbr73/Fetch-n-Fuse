
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Blending' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


#define hue(h) clamp( abs_f4( fract_f4(h + to_float4(2,1,4,0)/1.0f) * 6.0f - 3.0f) -1.0f , 0.0f, 1.0f)

__DEVICE__ float2 rand( float2 p ) {
    return fract_f2(sin_f2(to_float2(dot(p,to_float2(127.1f,311.7f)), dot(p,to_float2(269.5f,183.3f))))*43758.5453f);
}

__KERNEL__ void CellsCamJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  
    fragCoord+=0.5f;
  
    //stuff to expose
    const float size = 1.5f;
    const float uvFac = 10.0f;
    const float colFac = 0.5f;
    
    
    float2 ouv = fragCoord/iResolution;        
    float2 uv = (fragCoord - iResolution*0.5f) / (iResolution.y*size);    
    float2 luv = uv;
    
    float4 texIn = _tex2DVecN(iChannel1,ouv.x,ouv.y,15);
    float2 mp = swi2(texIn,x,z);//.rb;
    
    uv *= 100.0f + _sinf(iTime*0.5f+mp.x*uvFac);
   
    float2 iuv = _floor(uv);
    float2 guv = fract_f2(uv);      

    float mDist = 10.0f;
   
    float3 col = to_float3_s(0.1f);
       
    for (float y= -1.0f; y <= 1.0f; y++) {
        for (float x= -1.0f; x <= 1.0f; x++) {            
            float2 neighbor = to_float2(x, y);            
            float2 point = rand(iuv + neighbor);
            point = 0.5f + 0.5f*sin_f2(iTime*2.0f + 6.2831f*point);
            float2 diff = neighbor + point - guv;            
            float dist = length(diff);                      
           
            mDist = _fminf(mDist, dist);                        
        }
    } 
       
    float l = length(luv);    
    col = swi3(hue(fract(mDist*0.95f + iTime*0.1f + l + mp.x*colFac)),x,y,z);//.rgb;
    fragColor = to_float4_aw(col,1.0f)*0.05f + _tex2DVecN(iChannel0,ouv.x,ouv.y,15) *0.95f;    

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void CellsCamJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord/iResolution;
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}