

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = fragCoord.xy/iResolution.xy;
    
	vec4 col = texture(iChannel0, p);
	
	
	//Desaturate
    if(p.x<.25)
	{
		col = vec4( (col.r+col.g+col.b)/3. );
	}
	//Invert
	else if (p.x<.5)
	{
		col = vec4(1.) - texture(iChannel0, p);
	}
	//Chromatic aberration
	else if (p.x<.75)
	{
		vec2 offset = vec2(.01,.0);
		col.r = texture(iChannel0, p+offset.xy).r;
		col.g = texture(iChannel0, p          ).g;
		col.b = texture(iChannel0, p+offset.yx).b;
	}
	//Color switching
	else 
	{
		col.rgb = texture(iChannel0, p).brg;
	}
	
	
	//Line
	if( mod(abs(p.x+.5/iResolution.y),.25)<0.5/iResolution.y )
		col = vec4(1.);
	
	
    fragColor = col;
}