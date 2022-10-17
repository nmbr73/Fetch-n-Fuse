

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const float SHINE_BOOST = 50.0;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    vec3 colour = texture(iChannel0, uv).rgb;
    vec3 bloom = vec3(0.0, 0.0, 0.0);
    
    int lods = int(log2(iResolution.x));
    
    // fake mip bloom
    for (int i = 1; i < lods; i++)
        bloom += textureLod(iChannel1, uv, float(i)).rgb;
    
    float bloomStrength;
    
    if (iMouse.z > 0.0)
        bloomStrength = iMouse.x / iResolution.x;
    else
        bloomStrength = 0.5;
    
    colour += bloom * bloomStrength * SHINE_BOOST;
    
    fragColor = vec4(colour / float(lods), 1.0);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// just storing the base texture in a buffer so it can be changed from one place

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    
    vec3 colour = texture(iChannel0, uv).rgb;
    
    fragColor = vec4(colour, 1.0);
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// sobel filter

const vec2 KERNEL[9] = vec2[9]
(
    vec2(1.0, -1.0), vec2(0.0, -2.0), vec2(-1.0, -1.0),
    vec2(2.0, 0.0), vec2(0.0, 0.0), vec2(-2.0, 0.0),
    vec2(1.0, 1.0), vec2(0.0, 2.0), vec2(-1.0, 1.0)
);

const vec3 LUMA = vec3(0.299, 0.587, 0.114);

float LumaSobel(vec2 uv, vec2 scales)
{
    vec2 total = vec2(0.0, 0.0);
    float scale;
    
    if (iMouse.z > 0.0)
        scale = iMouse.y / iResolution.y;
    else
        scale = 0.5;
    
    vec2 pixelSize = scale * 4.0 / iResolution.xy;
    
    for (int y = -1, z = 0; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++, z++)
        {
            vec2 pixelUV = uv + pixelSize * vec2(x, y);
            vec3 pixel = textureLod(iChannel0, pixelUV, 0.0).rgb;
            total += KERNEL[z] * dot(LUMA, pixel);
        }
    }

    return total.x * total.y * scales.x;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    
    vec2 scales;
    
    if (iMouse.z > 0.0)
        scales = iMouse.xy / iResolution.xy;
    else
        scales = vec2(cos(iTime), sin(iTime)) * vec2(0.25, 0.125) + vec2(0.75, 0.375);
    
    float sobel = LumaSobel(uv, scales);
    vec3 colour = texture(iChannel0, uv).rgb * sobel;
    
    fragColor = vec4(colour, 1.0);
}
