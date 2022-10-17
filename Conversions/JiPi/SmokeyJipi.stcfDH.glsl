

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph2007-curlnoise.pdf


vec4 ssamp( vec2 uv )
{
    return texture( iChannel1, uv );
}

//vec3 e = vec3(1./iResolution.xy, 0.);
//vec4 dx( vec2 uv ) { return (ssamp(uv+e.xz) - ssamp(uv-e.xz)) / (2.*e.x); }
//vec4 dy( vec2 uv ) { return (ssamp(uv+e.zy) - ssamp(uv-e.zy)) / (2.*e.y); }

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    fragColor = .5*ssamp(uv);
    
    //vec2 L = normalize(vec2(-0.5,0.5));
    //vec2 n = .005*vec2(dy(uv).x,-dx(uv).x);
    //fragColor.xy = n;
    //fragColor *= .5+.3*max(0.,dot(L,n));
    //fragColor = clamp(fragColor,0.,1.);
    //fragColor = sqrt(fragColor);
    //fragColor = pow(fragColor,2.);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph2007-curlnoise.pdf

vec4 ssamp( vec2 uv, float oct )
{
    uv /= oct;
    
    vec2 d = 2.*vec2(1.,-1.)/iChannelResolution[1].xy;
    
    vec4 tex = (texture( iChannel1, 1.25*uv - 0.*0.004*iTime )
        + texture( iChannel1, 1.25*uv - 0.*0.004*iTime + d.xy )
        + texture( iChannel1, 1.25*uv - 0.*0.004*iTime + d.xx )
        + texture( iChannel1, 1.25*uv - 0.*0.004*iTime + d.yy )
        + texture( iChannel1, 1.25*uv - 0.*0.004*iTime - d.xy )) / 5.;
    
    vec4 noise = .15*(1.-texture( iChannel2, 4.*uv - 0.17*iTime ))
        + .15*(1.-texture( iChannel2, 3.3*uv + 0.1*iTime ));
    
    return tex + noise;
}

vec2 e = vec2(1./256., 0.);
vec4 dx( vec2 uv, float oct ) { return (ssamp(uv+e.xy,oct) - ssamp(uv-e.xy,oct)) / (2.*e.x); }
vec4 dy( vec2 uv, float oct ) { return (ssamp(uv+e.yx,oct) - ssamp(uv-e.yx,oct)) / (2.*e.x); }

vec2 hash2( float n ) { return fract(sin(vec2(n,n+1.0))*vec2(43758.5453123,22578.1459123)); }

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    
    // vel
    //vec2 vel = vec2( dx(uv).x, dy(uv).x );
    
    // curl
    float oct = 1.25;
    vec2 curl1 = .7*vec2( dy(uv,oct).x, -dx(uv,oct).x );
    //oct = 7.37;
    //vec2 curl2 = 0.*vec2( dy(uv,oct).x, -dx(uv,oct).x );
    //oct = 0.1;
    //vec2 curl3 = 0.*vec2( dy(uv,oct).x, -dx(uv,oct).x );
    
    vec2 curl = .0004*curl1;// + .0001*curl2 + .00001*curl3;
    curl *= sqrt(iResolution.x/640.);
    
    vec2 wind = 0.002*vec2(.45 + .1*log(max(iTime,1.)),1.); // grav and wind
    vec2 rand = 0.0005*(hash2(iTime)-0.5);

    fragColor = .991*texture( iChannel0, uv - curl + rand - wind );
    
    //fragColor += .1 * smoothstep(0.9,1.,texture( iChannel1, uv ));
    
    if( iMouse.z > 0. )
        fragColor += 0.5*length(texture( iChannel1, uv ))*.15*smoothstep( iResolution.x/10.,iResolution.x/10.-29.,length(iMouse.xy-fragCoord));
	    //fragColor += .1*smoothstep( iResolution.x/10.,iResolution.x/10.-29.,length(iMouse.xy-fragCoord));
    else
	    fragColor += .035*smoothstep( iResolution.x/8.,iResolution.x/8.-29.,length(fragCoord-iResolution.xy/(3.+iTime/5.)));
}
