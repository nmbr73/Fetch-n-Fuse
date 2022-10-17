

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define A .0 // Amplitude
#define V 8. // Velocity
#define W 9. // Wavelength
#define T .07 // Thickness
#define S 2. // Sharpness
#define GAP vec2(0.006, 0.008) //gap for edge

float sine(vec2 p){
    return pow(T / abs((p.y + 0.0)), S);
}

bool detectEdge(vec2 fCoord, vec2 gap) {
    vec2 uv = fCoord.xy / iResolution.xy;
    vec2 edgeDistance = 0.5 -  abs(uv - 0.5);
    
    bvec2 edgeCompare = lessThan(edgeDistance, gap);
    bool isEdge = edgeCompare.x || edgeCompare.y;
    
    return isEdge;
}

float blurEdge(vec2 fCoord, vec2 gap) {
    vec2 uv = fCoord.xy / iResolution.xy;
    
    if(uv.y > (1.0 - gap.y)) {
    	float blurValue = 1.0 - pow(T / abs(uv.y - (1.0 - gap.y)), S);
        return blurValue;
    } 
    
    if(uv.y < gap.y) {
    	float blurValue = 1.0 - pow(T / abs(uv.y - gap.y), S);
        return blurValue;
    }
    
    if(uv.x > (1.0 - gap.x)) {
    	float blurValue = 1.0 - pow(T / abs(uv.x - (1.0 - gap.x)), S);
        return blurValue;
    } 
    
    if(uv.x < gap.x) {
    	float blurValue = 1.0 - pow(T / abs(uv.x - gap.x), S);
        return blurValue;
    } 
    
    return 1.0;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    if(detectEdge(fragCoord, GAP)) {
        
        // Normalized pixel coordinates (from 0 to 1)
        vec2 uv = fragCoord/iResolution.xy;

        // Time varying pixel color
        vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));

        fragColor = vec4(col,1.0);
    } else {
    	fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}