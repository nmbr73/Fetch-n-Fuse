

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float getv(vec2 uv){
 	return max(texture(iChannel0,uv).g, texture(iChannel0,uv).b); // G channel for births, B is deaths
    // mixing in different ways provides different results
    // I like this one
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
     vec2 px = 1./iResolution.xy;

    vec3 norm = vec3(getv(uv-vec2(1,0)*px)-getv(uv-vec2(-1,0)*px),
                     getv(uv-vec2(0,1)*px)-getv(uv-vec2(0,-1)*px),
                     .2
        
        );
    norm = normalize(norm);
    
    vec3 light = normalize(vec3(sin(iTime),cos(iTime),1.));
    
    fragColor=vec4(vec3(dot(norm,light)),1.);
    fragColor = 1. - (1.-fragColor)*(1.-texture(iChannel0,uv).rrra);
	//fragColor = texture(iChannel0,uv); // uncoment for alt/direct rendering
    // I think it looks prettier but my work colleague pushed for 3Dish-based lighting
    // so I embossed the birth and death data, which is blurrified some
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const float KEY_SPACE = 32.5/256.0;

vec3 hash3( vec2 p )
{
    vec3 q = vec3( dot(p,vec2(127.1,311.7)), 
				   dot(p,vec2(269.5,183.3)), 
				   dot(p,vec2(419.2,371.9)) );
	return fract(sin(q)*43758.5453);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 px = 1. / iResolution.xy;
    fragColor.r = texture(iChannel0, fragCoord/iResolution.xy).r; 
    fragColor.b = texture(iChannel0, fragCoord/iResolution.xy).b*.9;
    
    if(iFrame<2 || (texture( iChannel1, vec2(KEY_SPACE,0.25) ).x == 1.0) ){
        if( abs(uv.y-.5)<.2){
        	fragColor=vec4(hash3(fragCoord+iTime).xxx*2.,
                       1.0);
        }else{
            fragColor=vec4(0.);
        }
    }else{
        int count = -int( texture(iChannel0,uv).x+.1);
        for(int x = -1; x <= 1; x++){
            for(int y = -1; y <= 1; y++){
                vec2 checkCoords = mod(uv+vec2(x,y)*px,1.);
                vec4 checkPix = texture(iChannel0,checkCoords);
        		count += int( checkPix.r+.1);
                fragColor.g += checkPix.g*.11;
                fragColor.b += checkPix.b*.01;
            }
        }
        
        if(count==3){ // conditions for new life
            fragColor.r = 1.;
            if(texture(iChannel0,uv).x < .5) // actually new, was previously dead
            	fragColor.g=1.;
        }
        else if(count<2){ // lonely
            fragColor.r=0.;
            if(texture(iChannel0,uv).x > .5) // new death
            	fragColor.b=1.;
        }
        else if(count>3){ // overcrowded
            fragColor.r=0.;
            if(texture(iChannel0,uv).x > .5) // new death
            	fragColor.b=1.;
        }
        
        
    if(length(iMouse.xy-fragCoord)<15. && iMouse.z>0.)
        fragColor.r=step(0.5,hash3(fragCoord).x);
    }
}