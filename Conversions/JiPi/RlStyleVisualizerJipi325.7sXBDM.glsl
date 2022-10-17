

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{   
    //basic background
    vec4 base = vec4(7.0/255.0, 38.0/255.0, 70.0/255.0, 1.0);
    fragColor = base;
    
    //proper ratios
    vec2 uv = fragCoord.xy / iResolution.xy;
    uv.y = uv.y*1.1;
    uv.x = uv.x*2.0 - .45;
    
    
    //lookup conversion (512 frequences returned by input)
    int tx = int(uv.x*512.0);
    
    //bucketed values of current and max frequencies
    int starter = int(floor(float(tx)/57.0))*57;
    int diff = tx-starter;
    float sum = 0.0;
    float maxSum = 0.0;
    for (int i = 0; i<9;i++) {
		sum = sum + texelFetch( iChannel0, ivec2(starter+i,2), 0 ).x;
        maxSum = maxSum + texelFetch( iChannel0, ivec2(starter+i,1), 0 ).x;
    }
    
    //normalize values
    sum = (sum/9.0);
    maxSum = (maxSum/9.0);
    
    //Draw bars
    float height = sum;
    float col = ((sum)-.2)*1.25;
    if (sum > uv.y && diff>20) {
        fragColor = vec4(uv.y + base.x, uv.y+base.y, uv.y+base.z, 1.0);
    }
    
    //draw "max" lines
    float mDiff = abs((uv.y+.01)-maxSum);
    float mVal = 1.0-(mDiff*50.0);
    if (mDiff<.02 && diff>20 && maxSum > 0.001) {
        fragColor = vec4(mix(fragColor.x,1.0, mVal),mix(fragColor.y, 1.0, mVal),mix(fragColor.z,1.0,  mVal), 1.0);
    }
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{   
    //same as usual uv but we offset by one so we grab the previous frame/texture from Buf A one frame higher;
    vec2 ouv = vec2(fragCoord.x, fragCoord.y-1.0) / iResolution.xy;
    //not offset texture for grabbing "max" values
    vec2 uv = vec2(fragCoord.x, fragCoord.y) / iResolution.xy;
    
    //conversion factor for our texture to sound texture
    int tx = int(fragCoord.x);
    
    //grab previous frame but offset by one pixel
    fragColor = texture(iChannel0, ouv);
    //old values for grabbing "max" values
    vec4 fragColorOld = texture(iChannel0, uv);
    
    //get frequency data
    float freq = texelFetch( iChannel1, ivec2(tx,0), 0 ).x;
    
    //only overwrite pixel if its the bottom one!
    //fragColor = mix(fragColor, vec4(vec3(freq), 1.0), clamp(1.0-fragCoord.y,0.0,1.0));
    
    //simpler code for overwriting third to bottom pixel
    if (int(fragCoord.y) == 2) {
        fragColor = vec4(vec3(freq),1.0);
    }
    //write max in second to bottom pixel
    if (int(fragCoord.y) == 1) {
        if (freq > fragColorOld.x) {
        	fragColor = vec4(freq, 0.0, 0.0,1.0);
        } else {
            //reduce max over time
        	fragColor = vec4(fragColorOld.x-.005, 0.0, 0.0,1.0);
        }
    }
}

//Also see https://www.shadertoy.com/view/XtKGzm by ttoinou for a similar effect