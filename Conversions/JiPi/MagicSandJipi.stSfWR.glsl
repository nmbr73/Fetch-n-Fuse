

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
	fragColor = texture(iChannel0, uv);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
mat2 rotate(float a) {
	return mat2(-sin(a), cos(a),
                 cos(a), sin(a));
}

float rand(vec2 p) {
	return texture(iChannel1, p).r;
}

void mainImage( out vec4 o, in vec2 q ) {
    q /= iResolution.xy;   
    
    vec2 sand = q * rotate(0.) + 0.000001;
    sand *= rotate(rand(q) * 0.02);

    o = texture(iChannel0, sand) * (0.99 - rand(q) * 0.1);
    
    vec2 p = q - 0.5;
    p.x *= iResolution.x / iResolution.y;
    
    if (iMouse.z > 0.) {
        vec2 m = iMouse.xy;
        
        m /= iResolution.xy;
        m -= 0.5;
        m.x *= iResolution.x / iResolution.y;
        
    	p -= m;
    }
	else {
        p += (vec2(cos(iTime * 2.), sin(iTime * 1.15)) + rand(p) * 0.2) * 0.2;
    }
    
    if (length(p + sin(atan(p.x, p.y) * 3.) * 0.005) - 0.05 - abs(sin(iTime * 2.)) * 0.025 < 0.) {
    	o = vec4(0.8 + sin(iTime * 5.) * 0.2, 0.25 + cos(iTime * 5.), 1. - tan(iTime * 100.) * 0.025, 1.);
    }
}