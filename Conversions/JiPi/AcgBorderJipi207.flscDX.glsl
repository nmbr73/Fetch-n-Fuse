

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
struct Rect
{
    vec2 bottom_left, top_right;
};

bool IsPointInsideRect(vec2 point, Rect rect)
{
    return (rect.bottom_left.x <= point.x && point.x <= rect.top_right.x &&
        rect.bottom_left.y <= point.y && point.y <= rect.top_right.y);
}

vec2 WalkPointOnRectPerimeter(vec2 dim, float t)
{
    float aspect = dim.x / dim.y;
    float height = 1.0 / (2.0 * (aspect + 1.0));
    float width = height * aspect;
    
    float tx = mod(t, 1.0);
    float ty = mod(t + height, 1.0);
    
    return vec2(
        tx < 0.5 ?
        min(tx, width) :
        max(width + 0.5 - tx, 0.0),
        
        ty < 0.5 ?
        min(ty, height) :
        max(height + 0.5 - ty, 0.0)
    ) * dim.x / width;
}

void mainImage( out vec4 frag_color, in vec2 frag_coord )
{
    const float pi = 3.1415926535;
    const vec4 center_color = vec4(0.1, 0.1, 0.1, 1.0);
    const vec4 border_color1 = vec4(0.8, 0.2, 0.3, 1.0);
    const vec4 border_color2 = vec4(0.4, 0.1, 0.8, 1.0);
    
    float radius = (iResolution.x + iResolution.y) * 0.5;
    float border = iResolution.y * 0.0154 + 13.33;
    
    Rect center_rect = Rect(vec2(0.0) + vec2(border), iResolution.xy - vec2(border));
    vec2 circle_center = WalkPointOnRectPerimeter(iResolution.xy, iTime / 4.0);
    
    frag_color = center_color;
    if (!IsPointInsideRect(frag_coord, center_rect))
    {
        float mix_factor = min(1.0, distance(circle_center, frag_coord) / radius);
        mix_factor = (1.0 - sin((mix_factor + 0.5) * pi)) * 0.5;
        frag_color = mix(border_color1, border_color2, mix_factor);
    }
}
