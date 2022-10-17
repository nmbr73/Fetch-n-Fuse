

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = texture( iChannel0, uv );
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float dx = 8.0;
    vec2 uv = fragCoord;
    vec4 left   = texture(iChannel0, (uv-vec2(-dx,0))/iResolution.xy);
	vec4 right  = texture(iChannel0, (uv-vec2( dx,0))/iResolution.xy);
	vec4 top    = texture(iChannel0, (uv-vec2(0,-dx))/iResolution.xy);
	vec4 bottom = texture(iChannel0, (uv-vec2(0, dx))/iResolution.xy);
	vec4 st     = texture(iChannel0, uv/iResolution.xy) * 2.0 - 1.0;
	vec4 lst    = texture(iChannel0, (uv-st.xy)/iResolution.xy);
	vec2 grad   = vec2( right.z - left.z, bottom.z - top.z );
	float diff  = ( left.z + right.z + top.z + bottom.z ) / 4.0;
	float div   = ( ( right.x - left.x ) + ( bottom.y - top.y ) ) / 4.0;
	lst.xy += grad;
	lst.z = diff - div * 0.05;
    lst.xyz = mix(
        lst.xyz,
        normalize( lst.xyz * 2.0 - 1.0 ) * 0.5 + 0.5,
        0.8
    );
	lst.w = 1.0;
    
    if( iFrame < 1 || iMouse.z > 0.0 )
    {
        lst = vec4(0);
        if     ( fragCoord.x < 1.0 )                 { lst.x = -1.0; }
        else if( fragCoord.x > iResolution.x - 1.0 ) { lst.x =  1.0; }
        if     ( fragCoord.y < 1.0 )                 { lst.y = -1.0; }
        else if( fragCoord.y > iResolution.y - 1.0 ) { lst.y =  1.0; }
    }
    fragColor = lst;
}