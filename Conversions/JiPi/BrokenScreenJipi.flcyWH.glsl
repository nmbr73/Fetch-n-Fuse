

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
    vec4 ok = texture(iChannel0,uv);
    fragColor = vec4(vec3(0.02,0.1,0.4) * (ok.b * 10.) +vec3(ok.b),3.);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	
    vec2 dvec;
    if (iMouse.x<20.0)
    {
     	float r =  0.2 + 0.6 * sin(iTime* 0.62);
        dvec = vec2( (iResolution.x/2. + r *iResolution.y/2. * sin(iTime*2.2)), ( iResolution.y/2. + r*iResolution.y/2. * cos( iTime*2.2)));
    }else{
    	dvec = iMouse.xy;
    }
     	
    
    
    vec2 uv = fragCoord.xy/iResolution.xy;
    vec2 tc = fragCoord.xy - dvec.xy;
    tc/=iResolution.x;
    float o= length(tc);
    
    float b = pow(max(1.-o*5.,0.),16.);
    vec3 nv = vec3(tc.x * b * 6.,tc.y * b * 6.,b);
    
    vec3 oldervec = texture(iChannel0, uv).xyz;
    vec3 oldvec = texture(iChannel0, uv).xyz;
    vec2 old2d = oldvec.xy;
    if (length(old2d)>1.)
    {
        old2d = normalize(old2d);
        oldvec = vec3(old2d, oldvec.z);       
    }
    vec3 oldervecnev = texture(iChannel0, uv - oldvec.xy * 0.5).xyz;
    
    
    fragColor = vec4(nv + vec3(oldvec.xy *  0.999, oldervecnev.z * 0.9), 1.0 );


}