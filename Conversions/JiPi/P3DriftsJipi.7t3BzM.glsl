

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float magAmt = 8.0;  // zoom level

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 res = iResolution.xy;
    //vec2 uv = fragCoord / res;
    vec2 uv = (fragCoord + vec2(.01)) / res;
    vec2 p = fragCoord / res.y;
    vec2 ps = 1. / res;
    
    // mouse center
    float mc = length(p-iMouse.xy/res.y);
    
    // apply zoom
    if(iMouse.z>0.)
        uv = uv/magAmt-iMouse.xy/res*(1.-magAmt)/magAmt;
    
    // corrective measure
    //uv += .5/res;
    
    // combine circles from neighboring particles
    float f = 1.;
    for(float y=-1.; y<=1.; y++) {
        for(float x=-1.; x<=1.; x++) {
            vec2 pos = texture(iChannel0, (uv-vec2(x, y)/res)).ba;
            float c = max(0., .5*res.x*length((uv-pos/res)/vec2(ps.x/ps.y, 1.)));
            f = min(f, c);
        }
    }
    
    fragColor = vec4(mix(vec3(0., .25, .0), vec3(1.), f), 1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const float MaxParticleSpeed = 0.2;

// hash without sine
// https://www.shadertoy.com/view/4djSRW
vec2 hash22(vec2 p) {
    vec3 MOD3 = vec3(443.8975, 397.2973, 491.1871);
	vec3 p3 = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract(vec2((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 res = iResolution.xy;
    vec2 ps = 1. / res;
    vec2 uv = fragCoord.xy / res;
    
    vec4 buf[9];
    buf[0] = texture(iChannel0, uv);
    buf[1] = texture(iChannel0, fract(uv-vec2(ps.x, 0.)));
    buf[2] = texture(iChannel0, fract(uv-vec2(-ps.x, 0.)));
    buf[3] = texture(iChannel0, fract(uv-vec2(0., ps.y)));
    buf[4] = texture(iChannel0, fract(uv-vec2(0., -ps.y)));
    buf[5] = texture(iChannel0, fract(uv-vec2(ps.x, ps.y)));
    buf[6] = texture(iChannel0, fract(uv-vec2(-ps.x, ps.y)));
    buf[7] = texture(iChannel0, fract(uv-vec2(ps.x, -ps.y)));
    buf[8] = texture(iChannel0, fract(uv-vec2(-ps.x, -ps.y)));
    
    // this cell's particle direction & position, if any
    vec2 pDir = buf[0].rg;
    vec2 pPos = buf[0].ba;
    
    // update this cell's particle position
    pPos = mod(pPos+pDir, res);
    
    float ct = 0.;
    vec2 pDirAdd = vec2(0.);
    vec2 pPosAdd = vec2(0.);
    vec2 pOffs = vec2(0.);
    if(true){//length(pDir)==0.) {
        for(int i=1; i<9; i++) {
            vec2 pPosI = mod(buf[i].ba+buf[i].rg, res);
            
            // add up incoming particles
            if(floor(pPosI)==floor(fragCoord)){// || (length(buf[i].rg)>0.&&hash12(mod(uv+iTime/100.-8., 100.))>.9125)) {
                pDirAdd += buf[i].rg;
                pPosAdd += pPosI;
                ct ++;
            }
            
            // slow down & 'bounce' particle when near a neighbor
            if(distance(pPos, pPosI)<1.5) {
                pDir *= .5;
                pOffs -= .05*(pPosI-pPos)/(1.+distance(pPos, pPosI));
                //pOffs -= .5*(pPosI-pPos)/(1.+distance(pPos, pPosI))*(length(buf[i].rg));
             	pOffs += .5*buf[i].rg;
            }   
        }

        // if particles were added up, average and transfer them to the current cell
        if(ct>0.) {
            pDir = (pDirAdd / ct);
            pPos = pPosAdd / ct;
        }
        
        // apply 'bounce'
        pDir += pOffs;
        
        // clear cell of data when particle leaves it
        if(floor(pPos)!=floor(fragCoord)) {
            pDir = vec2(0.);
            pPos = vec2(0.);
        }
        
        // make sure particle doesn't travel too fast
        if(length(pDir)>MaxParticleSpeed)
            pDir = MaxParticleSpeed*normalize(pDir);
    }
    
    // fill field with noise
    if(iFrame==0 || texture(iChannel3, vec2(82.5/256., 0.)).r>0.) {
        pDir = .02 * (.5-hash22(mod(uv-iTime/100., vec2(100.))));
        pPos = fragCoord;
	}   
    
    fragColor = vec4(pDir, pPos);
}