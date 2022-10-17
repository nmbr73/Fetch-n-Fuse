
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


struct Rect
{
    float2 bottom_left, top_right;
};

__DEVICE__ bool IsPointInsideRect(float2 point, Rect rect)
{
    return (rect.bottom_left.x <= point.x && point.x <= rect.top_right.x &&
            rect.bottom_left.y <= point.y && point.y <= rect.top_right.y);
}

__DEVICE__ float2 WalkPointOnRectPerimeter(float2 dim, float t)
{
    float aspect = dim.x / dim.y;
    float height = 1.0f / (2.0f * (aspect + 1.0f));
    float width = height * aspect;
    
    float tx = mod_f(t, 1.0f);
    float ty = mod_f(t + height, 1.0f);
    
    return to_float2(
        tx < 0.5f ?
        _fminf(tx, width) :
        _fmaxf(width + 0.5f - tx, 0.0f),
        
        ty < 0.5f ?
        _fminf(ty, height) :
        _fmaxf(height + 0.5f - ty, 0.0f)
    ) * dim.x / width;
}

__KERNEL__ void AcgBorderJipi207Fuse(float4 frag_color, float2 frag_coord, float iTime, float2 iResolution)
{

    //CONNECT_COLOR0(center_color,  0.1f, 0.1f, 0.1f, 1.0f); 
    CONNECT_COLOR1(border_color1, 0.8f, 0.2f, 0.3f, 1.0f); 
    CONNECT_COLOR2(border_color2, 0.4f, 0.1f, 0.8f, 1.0f); 
    CONNECT_POINT0(Border, 0.0154f, 13.33f);
    CONNECT_POINT1(BorderOut, 0.0f, 0.0f);
    CONNECT_SLIDER0(Radius, 0.0f, 2.0f, 0.5f);
    CONNECT_SLIDER1(ColFreq, 0.0f, 20.0f, 4.0f);

    //const float pi = 3.1415926535f;
    #define pi 3.1415926535f
    
    const float4 center_color  = to_float4(0.0f, 0.0f, 0.0f, 0.0f);
    //const float4 border_color1 = to_float4(0.8f, 0.2f, 0.3f, 1.0f);
    //const float4 border_color2 = to_float4(0.4f, 0.1f, 0.8f, 1.0f);
    
    float radius = (iResolution.x + iResolution.y) * Radius;//0.5f;
    //float border = iResolution.y * 0.0154f + 13.33f;
    float border = iResolution.y * Border.x + Border.y;
    float borderOut = iResolution.y * BorderOut.x + BorderOut.y;
    
    Rect center_rect = {to_float2_s(0.0f) + to_float2_s(border), iResolution - to_float2_s(border)};
    Rect center_rect_Out = {to_float2_s(0.0f) + to_float2_s(borderOut), iResolution - to_float2_s(borderOut)};
    
    float2 circle_center = WalkPointOnRectPerimeter(iResolution, iTime / ColFreq);//4.0f);
    
    frag_color = center_color;
    if (!IsPointInsideRect(frag_coord, center_rect) &&  IsPointInsideRect(frag_coord, center_rect_Out) )
    {
        float mix_factor = _fminf(1.0f, distance_f2(circle_center, frag_coord) / radius);
        mix_factor = (1.0f - _sinf((mix_factor + 0.5f) * pi)) * 0.5f;
        frag_color = _mix(border_color1, border_color2, mix_factor);
    }

  SetFragmentShaderComputedColor(frag_color);
}