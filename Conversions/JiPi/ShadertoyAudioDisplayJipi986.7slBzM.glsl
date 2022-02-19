

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Not mine!!!
vec3 hsv2rgb( in vec3 c ) {
    vec3 rgb = clamp( abs(mod(c.x*6.+vec3(0.,4.,2.),6.)-3.)-1., 0., 1.);
	rgb = rgb*rgb*(3.-2.*rgb); // cubic smoothing
	return c.z * mix(vec3(1.), rgb, c.y);
}

void mainImage(out vec4 O, in vec2 u) {
    vec2 U = u/iResolution.xy;
	
    float N = 32.,
        x = fract(U.x * N),
        y = texture(iChannel0, vec2((floor(U.x * N)+0.5) / N, 0)).x;
    
    if(length(x*2.-1.) < .75)
        O.rgb = clamp(1. - 90.*(U.y - y) ,0.,1.)
        * hsv2rgb(vec3((1. - y)*.6, .5, .9));
            }