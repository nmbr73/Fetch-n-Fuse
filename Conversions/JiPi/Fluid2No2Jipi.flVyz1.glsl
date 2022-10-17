

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<


precision highp float;


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    vec2 uv = fragCoord/iResolution.xy;

    vec4 col = texture(iChannel0, uv);
    
    vec2 muv = iMouse.xy / iResolution.xy;
    
    //float sound = texture(iChannel1, vec2(.75, .25)).x;
    
    fragColor = 3. *  mix(vec4(col.w), (col), .7);
}



// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
precision highp float;


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    vec2 uv = fragCoord/iResolution.xy;

    vec4 col = texture(iChannel0, uv);
    
    
    
    fragColor = col;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    
    vec2 uvc = uv - vec2(0.5);
    
    vec4 col = vec4(0.);
    
    
    ivec2 fragI = ivec2(fragCoord);
    
    vec2 ex = vec2(1., 0.);
    vec2 ey = vec2(0., 1.);
    
    vec2 coords = fragCoord;
    
    vec2 cfx = coords + ex;
    vec2 cbx = coords - ex;
    vec2 cfy = coords + ey;
    vec2 cby = coords - ey;
    
    
    
    
    if(iFrame == 0){
        col = vec4(0.);
    }
    else if(distance(uvc, vec2(0.)) < 1.) {
        
        
        //advect to new spot
        vec4 u = texture(iChannel0, vec2(coords / iResolution.xy));
        
        vec4 ufx = texture(iChannel0, vec2(cfx / iResolution.xy));
        vec4 ubx = texture(iChannel0, vec2(cbx / iResolution.xy));
        vec4 ufy = texture(iChannel0, vec2(cfy / iResolution.xy));
        vec4 uby = texture(iChannel0, vec2(cby / iResolution.xy));
        
        
        
        
        
        
        float dpx = ufx.z - ubx.z;
        float dpy = ufy.z - uby.z;
        
        
        
        cfx -= ufx.xy;
        cbx -= ubx.xy;
        cfy -= ufy.xy;
        cby -= uby.xy;
        
        
        coords -= u.xy;
        
        
        float densX = distance(coords, cfx) + distance(coords, cbx) - 2.;
        float densY = distance(coords, cfy) + distance(coords, cby) - 2.;
        
        float density = densX + densY;
        

        //apply changes
        col = texture(iChannel0, vec2(coords / iResolution.xy));
        
        col.x -= dpx / 8.;
        col.y -= dpy / 8.;
        
        
        
        
        
        
        //get data at new spot
        ufx = texture(iChannel0, vec2(cfx / iResolution.xy));
        ubx = texture(iChannel0, vec2(cbx / iResolution.xy));
        ufy = texture(iChannel0, vec2(cfy / iResolution.xy));
        uby = texture(iChannel0, vec2(cby / iResolution.xy));
        
        vec4 uAvg = (ufx + ubx + ufy + uby) / 4.;
        
        col.z = uAvg.z + (density / 8.);
        
        
        //col.z = .1 * col.w;
        //col.xy+= vec2(0., -.02) * iTimeDelta;
        
        
        
        if(iMouse.z > 0.){
            vec2 muvc = (iMouse.xy / iResolution.xy) - vec2(.5);
            
            col += vec4(-muvc, 0., 1.) * iTimeDelta * (.1 / (0.001 + distance(uvc, muvc))) * .05;
        }
        
        
        if(abs(coords.x - iResolution.x * .5) >= iResolution.x * .4 || abs(coords.y - iResolution.y * .5) >= iResolution.y * .4){
            col.xyz = vec3(0.); 
        }
        
        if(distance(coords, iResolution.xy * vec2(.25, .5)) < 1.){
            col += vec4(.2, .01 * sin(iTime), 0., .1);
        }
        
        if(distance(coords, iResolution.xy * vec2(.75, .5)) < 1.){
            col += vec4(-.2, -.01 * sin(iTime), 0., .1);
        }
        
        
        //dye will dissipate over time
        //col.w *= .9999;
        
    }
    
    
    
    fragColor = col;
}