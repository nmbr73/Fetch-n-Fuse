

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
	lots o' particles (Image)
	2016 stb

	Drawing from Buf B...
*/

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 uv = fragCoord.xy / iResolution.xy;
    
    float pDot, pTrail;
    
    pDot = texture(iChannel0, uv).r;
    pTrail = texture(iChannel0, uv).g;
    
	fragColor = vec4(vec3(pDot)+vec3(.5, .75, 1.)*vec3(pTrail), 1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/*
	lots o' particles (Buf A)
	2016 stb

	This shader updates the particles.

	No attempt is made to preserve particles upon contact, so only a few will remain after a while :(
*/

const float ParticleDensity = 1.; // 0.0-1.0

// hash without sine
// https://www.shadertoy.com/view/4djSRW
float hash12(vec2 p) {
    vec3 MOD3 = vec3(443.8975, 397.2973, 491.1871);
    vec3 p3 = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 res = iResolution.xy;
    vec2 px = 1. / res;
    vec2 uv = fragCoord.xy / res;
    
    vec4 buf[9];
    buf[0] = texture(iChannel0, uv);
    buf[1] = texture(iChannel0, fract(uv-vec2(px.x, 0.)));
    buf[2] = texture(iChannel0, fract(uv-vec2(-px.x, 0.)));
    buf[3] = texture(iChannel0, fract(uv-vec2(0., px.y)));
    buf[4] = texture(iChannel0, fract(uv-vec2(0., -px.y)));
    buf[5] = texture(iChannel0, fract(uv-vec2(px.x, px.y)));
    buf[6] = texture(iChannel0, fract(uv-vec2(-px.x, px.y)));
    buf[7] = texture(iChannel0, fract(uv-vec2(px.x, -px.y)));
    buf[8] = texture(iChannel0, fract(uv-vec2(-px.x, -px.y)));
    
    // this cell's particle direction & position, if any
    vec2 pDir = buf[0].rg;
    vec2 pPos = buf[0].ba;
    
    // update this cell's particle position
    pPos = mod(pPos+pDir, res);
    
    // clear the current cell if its particle leaves it
    if(floor(pPos)!=floor(fragCoord)) {
        pDir = vec2(0.);
        pPos = vec2(0.);
    }
    
    // add up any incoming particles
    float ct = 0.;
    vec2 pDirAdd = vec2(0.);
    vec2 pPosAdd = vec2(0.);
    for(int i=1; i<9; i++) {
        vec2 pPosI = buf[i].ba;
        pPosI = mod(pPosI+buf[i].rg, res);
        if(floor(pPosI)==floor(fragCoord)) {
            pDirAdd += buf[i].rg;
            pPosAdd += pPosI;
            ct ++;
        }
    }
    
    // if particles were added up, average and transfer them to the current cell
    if(ct>0.) {
        pDir = normalize(pDirAdd / ct);
        pPos = pPosAdd / ct;
    }
    
    // first frame particle setup
    if(iFrame==0)
        if(ParticleDensity>hash12(fragCoord/res)) {
            vec2 randXY =
                vec2(
                    hash12(mod(uv+iTime/100.-4., 100.)),
            		hash12(mod(uv-iTime/100.-8., 100.))
				);
            
            pDir = normalize(randXY-.5);
            pPos = fragCoord;
        }
    
    fragColor = vec4(pDir, pPos);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
/*
	lots o' particles (Buf B)
	2016 stb

	This shader predraws the particles for the Image shader.
*/

const float FadeAmt = 0.1; // 0.0-1.0

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 uv = fragCoord.xy / iResolution.xy;
    
    float pDot, pTrail;
    vec2 pDir = texture(iChannel0, uv).rg;
    
    pDot = texture(iChannel0, uv).r;
    pTrail = texture(iChannel1, uv).g;
    
    // make this cell white if it has a nonzero vector length
    if(length(pDir)>0.)
       	pDot = 1.;
    
    // trail effect
    pTrail = max(pDot, pTrail-FadeAmt);
    
	fragColor = vec4(pDot, pTrail, 0., 1.);
}