

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// change the ramp values for colors
// a is the position on the gradient. 
// 69 in r = return the original color

vec3 zurple = vec3(0.65, 0.1921, 0.4235);


vec4 ramp[10] = vec4[10](
    vec4(vec3(0.65, 0.65, 0.85), 0.0), // zurple
    vec4(vec3(0.8, 1.0, 0.007), 1.0), // wow yellow
    vec4(vec3(0.8, 1.0, 0.007), 1.0), // wow yellow
    vec4(vec3(0.8, 1.0, 0.007), 1.0), // wow yellow
    vec4(vec3(0.8, 1.0, 0.007), 1.0), // wow yellow
    vec4(vec3(0.8, 1.0, 0.007), 1.0), // wow yellow
    vec4(vec3(1.0, 1.0, 1.0), 2.0), // white, maxed out
    vec4(vec3(1.0, 1.0, 1.0), 2.0), // white, maxed out
    vec4(vec3(1.0, 1.0, 1.0), 2.0), // white, maxed out
    vec4(vec3(1.0, 1.0, 1.0), 2.0) // white, maxed out
);
// change ramp count if you add more points on the gradient
int rampCount = 5;


int getLowerRampIndex(in float key){
    for (int i = 0; i < rampCount - 1; i++) {
        float lowerBound = ramp[i].a;
        float upperBound = ramp[i + 1].a;
        if (key <= ramp[i + 1].a) {
            return i;
        }
    }
    return rampCount - 1;
}

float easeInOutCubic(in float x) {
    if (x < 0.5) {
        return 4.0 * x * x * x;
    }    
    return 1.0 - pow(-2.0 * x + 2.0, 3.0) / 2.0;
}

float getLum(in vec3 color) {
    return 0.2126*color.r + 0.7152*color.g + 0.0722*color.b;
}

vec4 evalSpecials(in vec4 color, in vec3 tex_col) {
    if (color.r > 68.5 && color.r < 69.5) {
        return vec4(tex_col, color.a);
    }
    else if (color.r > 1.0 && color.g > 1.0 && color.b > 1.0) {
        return vec4(color.r/255.0, color.g/255.0, color.b/255.0, color.a);
    }
    return color;
    
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 min = ramp[0];
    vec4 max = ramp[1];
    // ramp[1] = vec4(ramp[1].rgb, (sin(iTime * 0.5)/4.0) + 1.0);
    // vec4 color1 = vec4(vec3(0.0, 1.0, 0.0), 1.0);
    bool found = false;
    vec2 uv = fragCoord/iResolution.xy;
    

    
    vec3 tex_col = texture(iChannel0, uv).rgb;
    //float mixVal = getLum(tex_col);
    float mixVal = 1.1 - tex_col.r;
    
    // debug show gradient, try commenting this out
    if (uv.x > 0.95) { mixVal = uv.y; }
    // end show gradient
    
    int lowerRampIndex = getLowerRampIndex(mixVal);
    min = ramp[lowerRampIndex];
    max = ramp[lowerRampIndex + 1];
    mixVal = ((mixVal-min.a)/(max.a - min.a));
    mixVal = easeInOutCubic(mixVal);
    // Normalized pixel coordinates (from 0 to 1)
    // r value of 69 = get the texture color
    min = evalSpecials(min, tex_col);
    max = evalSpecials(max, tex_col);
    vec3 color = mix(min.rgb, max.rgb, mixVal);

    // Time varying pixel color

    // Output to screen
    fragColor = vec4(color, 1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// 2D Random
float random (in vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))
                 * 43758.5453123);
}
float noise (in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    // Smooth Interpolation

    // Cubic Hermine Curve.  Same as SmoothStep()
    vec2 u = f*f*(3.0-2.0*f);
    // u = smoothstep(0.,1.,f);

    // Mix 4 coorners percentages
    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

float quantize(in float color, float steps) {
    return (floor(color * steps)+0.150) /steps;
}
float idk (in float a, in float b) {
    return noise(vec2(a, b/2.0));
}
float getBrightness(in vec3 color) {
    //return (color.r + color.g + color.b)/3.0;
    return (0.299*color.r + 0.587*color.g + 0.114*color.b);
}
float getSaturation(in vec3 color) {
    float colorMax = max(color.b, max(color.r, color.g));
    float colorMin = min(color.b, min(color.r, color.g));
    float luminence = getBrightness(color);
    if (luminence == 1.0) {
        return 0.0;
    }
    return (colorMax - colorMin) / (1.0 - (2.0 * luminence - 1.0));
    

}
vec3 getNewFrameColor(in vec3 lastFrameColor, vec2 uv, sampler2D lastFrame) {
    float pi = 3.14159;

    float speed = 0.0012;

    vec3 newFrameColor = texture(lastFrame, vec2(uv.x + (0.99 - getBrightness(lastFrameColor)) * speed, uv.y)).rgb;
    vec3 maybeNewFrameColor = texture(lastFrame, vec2(uv.x - (0.99 - getBrightness(lastFrameColor)) * speed, uv.y)).rgb;
    /*
    if (random(lastFrameColor.rg) > 0.5) {
    return newFrameColor;
    } else {
    return maybeNewFrameColor;
    }
    */
    if (getSaturation(maybeNewFrameColor) > getSaturation(newFrameColor)){
        return maybeNewFrameColor;
    }
    
    //newFrameColor = newFrameColor/2.0;
    return newFrameColor;
}
float diffuse( in float[9] state, float[9] rules) {
    float retVal = 0.0;
    for (int i = 0; i < 9; i++) {
        retVal = retVal + state[i] * rules[i];
    }
    return retVal;
}
vec2 getUV(in vec2 inFragCoord, vec2 resolution){
  inFragCoord.x = mod(inFragCoord.x, resolution.x);
  inFragCoord.y = mod(inFragCoord.y, resolution.y);
  return inFragCoord/resolution.xy;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    float pi = 3.14159;
    bool zoomers = false;
    bool do_quantize = false;
    vec2 uv = fragCoord/iResolution.xy;
    vec3 lastFrameColor = texture(iChannel0, uv).rgb;
    // vec3 newFrameColor = getNewFrameColor(lastFrameColor, uv, iChannel0);
    fragColor = vec4(lastFrameColor, 1.0);
    //  fragColor.r = random(uv);
    float webcamBrightness = getBrightness(texture(iChannel1, uv).rgb);
    //anotherNoiseVal = mod(iTime, 5.0)/5.0;
    if (iTime < 0.05) {
        fragColor = texture(iChannel1, (uv.xy/vec2(4.0, 5.0)));
        return;
    }
    if (do_quantize) {
        fragColor.r = quantize(fragColor.r, 20.0);
        fragColor.g = quantize(fragColor.g, 10.0);
        fragColor.b = quantize(fragColor.b, 5.0);
    }
    
    float diagonalDiffuse = 0.05;
    float adjacentDiffuse = 0.2;
    float[9] rules = float[9](
    diagonalDiffuse, adjacentDiffuse, diagonalDiffuse,
    adjacentDiffuse, -1.0, adjacentDiffuse,
    diagonalDiffuse, adjacentDiffuse, diagonalDiffuse);
    
    float[9] fstate = float[9](
        texture(iChannel0, getUV(vec2(fragCoord.x - 1.0, fragCoord.y - 1.0), iResolution.xy)).r,
        texture(iChannel0, getUV(vec2(fragCoord.x, fragCoord.y - 1.0), iResolution.xy)).r,
        texture(iChannel0, getUV(vec2(fragCoord.x + 1.0, fragCoord.y - 1.0), iResolution.xy)).r,
        
        texture(iChannel0, getUV(vec2(fragCoord.x - 1.0, fragCoord.y), iResolution.xy)).r,
        texture(iChannel0, getUV(vec2(fragCoord.x, fragCoord.y), iResolution.xy)).r,
        texture(iChannel0, getUV(vec2(fragCoord.x + 1.0, fragCoord.y), iResolution.xy)).r,
        
        texture(iChannel0, getUV(vec2(fragCoord.x - 1.0, fragCoord.y + 1.0), iResolution.xy)).r,
        texture(iChannel0, getUV(vec2(fragCoord.x, fragCoord.y + 1.0), iResolution.xy)).r,
        texture(iChannel0, getUV(vec2(fragCoord.x + 1.0, fragCoord.y + 1.0), iResolution.xy)).r
    );
    
    float[9] bstate = float[9](
        texture(iChannel0, getUV(vec2(fragCoord.x - 1.0, fragCoord.y - 1.0), iResolution.xy)).g,
        texture(iChannel0, getUV(vec2(fragCoord.x, fragCoord.y - 1.0), iResolution.xy)).g,
        texture(iChannel0, getUV(vec2(fragCoord.x + 1.0, fragCoord.y - 1.0), iResolution.xy)).g,
        
        texture(iChannel0, getUV(vec2(fragCoord.x - 1.0, fragCoord.y), iResolution.xy)).g,
        texture(iChannel0, getUV(vec2(fragCoord.x, fragCoord.y), iResolution.xy)).g,
        texture(iChannel0, getUV(vec2(fragCoord.x + 1.0, fragCoord.y), iResolution.xy)).g,
        
        texture(iChannel0, getUV(vec2(fragCoord.x - 1.0, fragCoord.y + 1.0), iResolution.xy)).g,
        texture(iChannel0, getUV(vec2(fragCoord.x, fragCoord.y + 1.0), iResolution.xy)).g,
        texture(iChannel0, getUV(vec2(fragCoord.x + 1.0, fragCoord.y + 1.0), iResolution.xy)).g
    );
    
    
    
    float timestep = 1.0;
    float bunnies = lastFrameColor.g;
    if (mod(float(iFrame), 5.0) != 0.0) {
        // fragColor = vec4(lastFrameColor.rgb, 1);
        // return;
    }
    // diffuse
    float dA = 1.0;
    float dB = 1.0 - webcamBrightness;  // 0.5;  
    dB = (sin(iTime/10.0) + 1.0)/3.0;
    fragColor = vec4(dA * diffuse(fstate, rules), dB * diffuse(bstate, rules), 0, 1);
    /* */

    float maxFood = 1.0;
    // float feedRate = 0.15;
    // feedRate = (webcamBrightness + 0.45)/3.15;
    float feedRate = 0.055;

    float dieRate = 0.062;

    // react

    float eatenFood = lastFrameColor.r * (lastFrameColor.g * lastFrameColor.g);
    fragColor.r = fragColor.r - eatenFood;
    fragColor.g = fragColor.g + eatenFood;
    // kill
    fragColor.g = clamp(lastFrameColor.g + timestep*(fragColor.g - ((dieRate + feedRate) * lastFrameColor.g)), 0.0001, 1.0);
    // feed
    float addedFood = feedRate * clamp(maxFood - lastFrameColor.r, 0.0, 1.0);
    fragColor.r = clamp(timestep*(fragColor.r + addedFood) + lastFrameColor.r, 0.0, 1.0);
    fragColor.b = eatenFood;
    
    if (distance(fragCoord.xy, iMouse.xy) < 40.0 && iMouse.z > 0.0) {
        fragColor.r = 0.5;
        if (distance(fragCoord.xy, iMouse.xy) < 20.0) {
            fragColor.g = 0.1;
        }
    }
    /**/
    /*
        // react
    float eatenFood = lastFrameColor.r * lastFrameColor.g * lastFrameColor.g;
    fragColor.r = fragColor.r - eatenFood;
    fragColor.g += eatenFood;
    // kill
    fragColor.g = fragColor.g - (dieRate * lastFrameColor.g);
    // feed
    float addedFood = feedRate * (maxFood - lastFrameColor.r);
    fragColor.r = fragColor.r + addedFood;
    fragColor.b = 0.0;
    */
    // fragColor.r = noise(vec2(idk(uv.x, iTime), idk(uv.y, iTime))*scale);
}