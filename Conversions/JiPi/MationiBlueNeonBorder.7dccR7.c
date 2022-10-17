
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel0
// Connect Image 'Texture: Image' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)




__DEVICE__ float sine(float2 p, float S, float T){
    return _powf(T / _fabs((p.y + 0.0f)), S);
}

__DEVICE__ bool detectEdge(float2 fCoord, float2 gap, float2 iResolution, float Size, float2 Offset) {
    float2 uv = swi2(fCoord,x,y) / iResolution;
    float2 edgeDistance = (0.5f -  abs_f2(uv - 0.5f) - Offset) / Size;
    
    //bvec2 edgeCompare = lessThan(edgeDistance, gap);
    //bool isEdge = edgeCompare.x || edgeCompare.y;
    
    //gap = (gap - Offset) / Size;
    
    bool isEdge = edgeDistance.x < gap.x || edgeDistance.y < gap.y;
    
    return isEdge;
}

__DEVICE__ float blurEdge(float2 fCoord, float2 gap, float2 iResolution, float S, float T) {
    float2 uv = swi2(fCoord,x,y) / iResolution;
    
    if(uv.y > (1.0f - gap.y)) {
      float blurValue = 1.0f - _powf(T / _fabs(uv.y - (1.0f - gap.y)), S);
        return blurValue;
    } 
    
    if(uv.y < gap.y) {
      float blurValue = 1.0f - _powf(T / _fabs(uv.y - gap.y), S);
        return blurValue;
    }
    
    if(uv.x > (1.0f - gap.x)) {
      float blurValue = 1.0f - _powf(T / _fabs(uv.x - (1.0f - gap.x)), S);
        return blurValue;
    } 
    
    if(uv.x < gap.x) {
      float blurValue = 1.0f - _powf(T / _fabs(uv.x - gap.x), S);
        return blurValue;
    } 
    
    return 1.0f;
}





__KERNEL__ void MationiBlueNeonBorderFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
  
    CONNECT_BUTTON0(Modus, 0, Neon, Color, Blue, WhiteNeon, White);

    //Modus+=1;

    CONNECT_COLOR0(Color, 0.1f, 0.6f, 0.8f, 1.0f);

    CONNECT_CHECKBOX0(ControlPoints, 0);


    CONNECT_SLIDER0(Size, 0.0f, 2.0f, 1.0f);
    

    CONNECT_POINT0(P00, 0.0f, 0.0f);
    CONNECT_POINT1(P11, 0.0f, 0.0f);
    CONNECT_POINT2(Offset, 0.0f, 0.0f);

//Modus Color
CONNECT_CHECKBOX1(Blur, 0);
CONNECT_SLIDER1(S, -10.0f, 10.0f, 2.0f);
CONNECT_SLIDER2(T, -10.0f, 10.0f, 0.07f);
//CONNECT_SLIDER3(GAP, -10.0f, 10.0f, 0.0f);
CONNECT_POINT3(GAP, 0.006f, 0.008f);


CONNECT_CHECKBOX2(NeonColor, 0);

//#define T 0.07f // Thickness
//#define S 2.0f  // Sharpness
//#define GAP to_float2(0.006f, 0.008f) //gap for edge


    float ratio = iResolution.x/iResolution.y;



if (Modus == 1) // Neon
{
    float2 p1 = to_float2(0.01f, 0.01f);
    float2 p2 = to_float2(0.99f, 0.99f);
    float2 p3 = to_float2(0.01f, 0.99f);
    float2 p4 = to_float2(0.99f, 0.01f);

  if(ControlPoints)
  {
    P11.x += iResolution.x/iResolution.y-1.0f;

    p1 += P00;
    p2 += P11;
    p3 += to_float2(P00.x,P11.y);
    p4 += to_float2(P11.x,P00.y);
  }
  else
  {
    p2.x += iResolution.x/iResolution.y-1.0f;
    p4.x += iResolution.x/iResolution.y-1.0f; 
    
    p2*=Size;
    p3.y*=Size;
    p4.x*=Size;
    
    p1 += Offset;
    p2 += Offset;
    p3 += Offset;
    p4 += Offset;
  }

    float2 uv = fragCoord / iResolution;
    uv.x *= ratio;
    
    float4 c2 = texture(iChannel0, uv + iTime/10.0f);
    
    float d1 = step(p1.x,uv.x)*step(uv.x,p4.x)*_fabs(uv.y-p1.y)+
               step(uv.x,p1.x)*distance_f2(uv,p1)+step(p4.x,uv.x)*distance_f2(uv,p4);
    d1 = _fminf(step(p3.x,uv.x)*step(uv.x,p2.x)*_fabs(uv.y-p2.y)+
                step(uv.x,p3.x)*distance_f2(uv,p3)+step(p2.x,uv.x)*distance_f2(uv,p2),d1);
    d1 = _fminf(step(p1.y,uv.y)*step(uv.y,p3.y)*_fabs(uv.x-p1.x)+
                step(uv.y,p1.y)*distance_f2(uv,p1)+step(p3.y,uv.y)*distance_f2(uv,p3),d1);
    d1 = _fminf(step(p4.y,uv.y)*step(uv.y,p2.y)*_fabs(uv.x-p2.x)+
                step(uv.y,p4.y)*distance_f2(uv,p4)+step(p2.y,uv.y)*distance_f2(uv,p2),d1);
        
    float f1 = 0.01f / _fabs(d1 + c2.x/100.0f);
    
    //fragColor = to_float4_aw(f1 * to_float3(0.1f, 0.6f, 0.8f), 1.0f);
    //fragColor = to_float4_aw(f1 * swi3(Color,x,y,z), Color.w*f1);
    if(NeonColor)
      fragColor = to_float4_aw(f1 * (0.5 + 0.5*cos_f3(iTime+swi3(uv,x,y,x)+ to_float3(0,2,4))), Color.w*f1);
    else
      fragColor = to_float4_aw(f1 * swi3(Color,x,y,z), Color.w*f1);
  

  if(ControlPoints)
  {
    fragColor += texture(iChannel1, fragCoord / to_float2((p2.x-p1.x)*iResolution.x/ratio, (p2.y-p1.y)*iResolution.y) - to_float2(p1.x,p1.y));
  }
  else
  {
    float2 tuv = (fragCoord / iResolution - to_float2(Offset.x/ratio, Offset.y)) / Size ;
    
    if (tuv.x > (p1.x-Offset.x)/Size && tuv.x < (p2.x-Offset.x)/Size+1.0f-ratio && tuv.y > (p1.y-Offset.y)/Size && tuv.y < (p2.y-Offset.y)/Size)
       fragColor += texture(iChannel1, tuv);
  }

} // Neon


if (Modus == 2) // Color
{
    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    if(detectEdge(fragCoord, GAP, iResolution, Size, Offset)) {
        
        // Time varying pixel color
        float3 col = 0.5f + 0.5f*cos_f3(iTime+swi3(uv,x,y,x)+to_float3(0,2,4));

        if (Blur) 
          col  = to_float3_s(blurEdge(fragCoord, GAP, iResolution, S,T));

        fragColor = to_float4_aw(col,1.0f);
    } else {
        fragColor = texture(iChannel1, uv);//to_float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

}



  
  SetFragmentShaderComputedColor(fragColor);
}