

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col =  texture(iChannel0,uv).rgb;
    
    // Output to screen
    col.r /= 2.; // value data stored in r, lighting in gb
    col.r += col.b;
    fragColor = vec4(col.brr,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define SCANSIZE 2
float DECAY = .9;
// Adjust the decay if you adjust the scansize. Try 1 and .925. There are lots of wild effects with different
// values
//
// It's funny how at scansize 1, there is this stable pattern where a black pixel will be surrounded by lit pixels.
// Each frame, a given pixel adds the direction of nearby brigtness, so if it's surrounded, it will stay dark
// because I'm using the length of the scan vector to estimate the brightness, which is zero when it's surrounded.

vec2 ScanNeighbors(ivec2 p) {
    vec2 dir = vec2(0.);
    for (int y=-SCANSIZE; y<=SCANSIZE; y++) {
        for (int x=-SCANSIZE; x<=SCANSIZE; x++) {
            if(x==0 && y==0) continue;
            dir += vec2(x,y)*texelFetch(iChannel1, p+ivec2(x,y), 0).r;
        }
    }
    // returns the average direction of the nearby brigtness
    return (dir);
}
vec3 iterate(ivec2 ifrag) {
   //float DECAY = .89+sin(iTime*.5)*.02; // animating decay

    vec3 col = vec3(0.);
   // if(ifrag.y == int(iResolution.y/2.+10.*sin(iTime*2.))){  // sweeping line
   //     col.r = 0.;
   // } else {
        vec3 tc = vec3(texelFetch(iChannel1, ifrag, 0).rgb);
        
        
        vec2 dir = ScanNeighbors(ifrag);
        vec2 lightDir = normalize( (vec2(sin(iTime),cos(iTime))));
        tc.bg = vec2(smoothstep(-3.,3.,dot(dir, lightDir)));
        tc.r += smoothstep(0.,7.*float(SCANSIZE*SCANSIZE),length(dir));
        tc.r *= DECAY;
        tc.g = smoothstep(.8,1.,DECAY);
        col = clamp(tc,0.,1.);
    //}
    return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    ivec2 ifrag = ivec2(fragCoord);
    vec3 col = vec3(0);
    
    if(iFrame <10) {
        col = texture(iChannel0, uv).rgb;
    } else {
        col = iterate(ifrag);
    }
    
    fragColor = vec4(col,1.0);
    
}