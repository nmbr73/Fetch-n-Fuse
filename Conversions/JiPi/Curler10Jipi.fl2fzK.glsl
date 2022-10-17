

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage(
    out vec4 outFragColor,
    vec2 fragCoord)
{
    vec4 selfState = texture(iChannel0, (fragCoord / iResolution.xy));
    
    outFragColor = selfState;
    //outFragColor = (selfState * pow(selfState.a, 20.0));
    outFragColor.a = 1.0;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
const int k_blurRadius = 10;

float RandomFloat(
  vec2 testCoord)
{
	// From: https://www.shadertoy.com/view/Xd23Dh
	// (just removed some dimensions)
	float testCoordInGeneratorSpace = 
		dot(testCoord, vec2(127.1, 311.7));

	return fract(sin(testCoordInGeneratorSpace) * 43758.5453);
}

vec2 RandomVec2(
  vec2 testCoord)
{
	// From: https://www.shadertoy.com/view/Xd23Dh
	// (just removed some dimensions)
	vec2 testCoordInGeneratorSpace = vec2(
		dot(testCoord, vec2(127.1, 311.7)),
		dot(testCoord, vec2(269.5, 183.3)));

	return fract(sin(testCoordInGeneratorSpace) * 43758.5453);
}

vec3 RandomVec3(
  vec2 testCoord)
{
    // From: https://www.shadertoy.com/view/Xd23Dh
    // (just removed some dimensions)
    vec3 testCoordInGeneratorSpace = vec3(
        dot(testCoord, vec2(127.1, 311.7)),
        dot(testCoord, vec2(269.5, 183.3)),
        dot(testCoord, vec2(419.2, 371.9)));

	return fract(sin(testCoordInGeneratorSpace) * 43758.5453);
}

vec2 TransformFromCanvasTextureToFramedTexture(
	vec2 canvasTextureCoord,
	vec2 canvasTextureSize,
	vec2 framedTextureSize)
{	
	vec2 result = (canvasTextureCoord / canvasTextureSize);

	float canvasAspectRatio = (canvasTextureSize.x / canvasTextureSize.y);
	float framedAspectRatio = (framedTextureSize.x / framedTextureSize.y);

	if (framedAspectRatio < canvasAspectRatio)
	{
		float relativeAspectRatio = (canvasAspectRatio / framedAspectRatio);

		result.x *= relativeAspectRatio;
		result.x -= (0.5 * (relativeAspectRatio - 1.0));
	}
	else
	{
		float relativeAspectRatio = (framedAspectRatio / canvasAspectRatio);

		result.y *= relativeAspectRatio;
		result.y -= (0.5 * (relativeAspectRatio - 1.0));
	}

	return result;
}

bool TextureCoordIsInBounds(
	vec2 texCoord)
{
	vec2 inBounds = (
        step(vec2(0.0), texCoord) * 
        step(texCoord, vec2(1.0)));
    
    return ((inBounds.x > 0.5) && (inBounds.y > 0.5));
}

vec4 SampleWebcam(
	sampler2D webcamTexture,
	vec2 webcamSize,
    bool horizontallyMirrorWebcam,
    vec2 canvasCoord,
	vec2 canvasSize)
{
	vec2 webcamCoord = 
        TransformFromCanvasTextureToFramedTexture(
            canvasCoord, 
            canvasSize, 
            webcamSize);

    if (horizontallyMirrorWebcam)
    {
    	webcamCoord.x = mix(1.0, 0.0, webcamCoord.x);
    }
    
    vec4 result = TextureCoordIsInBounds(webcamCoord) ? 
        pow(texture(webcamTexture, webcamCoord), vec4(2.0)) : 
    	vec4(0.0, 0.0, 0.0, 1.0);
    
    return result;
}

float GetBrightness(
    vec4 color)
{    
    return (0.333 * (color.r + color.g + color.b));
}

vec4 ComputePseudoGaussianBlur(
	sampler2D source,
    vec2 texelSize,
    vec2 fragCoord,
    vec2 blurDirection) // Unit-length. Typically (1, 0) or (0, 1).
{
    vec4 accumulator = vec4(0.0);
    float totalWeight = 0.0;
    
    float oneOverBlurRadius = (1.0 / 10.0);

    vec2 sampleUvIncrement = (blurDirection * texelSize);
    vec2 sampleUv = (
        (fragCoord * texelSize) - 
        (float(k_blurRadius) * sampleUvIncrement));
    
    float uncurvedSampleWeight = -1.0;
    float uncurvedSampleWeightIncrement = (1.0 / float(k_blurRadius));
    
    for (int index = (-1 * k_blurRadius); index <= k_blurRadius; index++)
    {
        vec4 sampleColor = texture(source, sampleUv);

        // NOTE: Small fudge to make the outermost texels non-zero.
        float sampleWeight = smoothstep(
            (1.0 + oneOverBlurRadius), 
            0.0, 
            abs(uncurvedSampleWeight));
        sampleWeight = (sampleWeight * sampleWeight * sampleWeight); 

        accumulator += (sampleColor * sampleWeight);
        totalWeight += sampleWeight;
        
        sampleUv += sampleUvIncrement;
        uncurvedSampleWeight += uncurvedSampleWeightIncrement;
    }

    return (accumulator / totalWeight);
}


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Pass: Webcam perspective correction.

void mainImage(
    out vec4 outFragColor,
    vec2 fragCoord)
{    
    outFragColor = SampleWebcam(
		iChannel0,
		//iChannelResolution[0].xy,
        iResolution.xy,
    	true, // horizontallyMirrorWebcam
    	fragCoord,
		iResolution.xy);
    
    outFragColor.a = GetBrightness(outFragColor);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Pass: Vertical blur.

void mainImage(
    out vec4 outFragColor,
    vec2 fragCoord)
{
    outFragColor = ComputePseudoGaussianBlur(
        iChannel0,
        (1.0 / iResolution.xy),
        fragCoord,
        vec2(0.0, 1.0));
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Pass: Horizontal blur.

void mainImage(
    out vec4 outFragColor,
    vec2 fragCoord)
{
    outFragColor = ComputePseudoGaussianBlur(
        iChannel0,
        (1.0 / iResolution.xy),
        fragCoord,
        vec2(1.0, 0.0));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Pass: Curl and simulation.

vec2 s_texelSize;

vec2 ComputeGradient(
    vec2 fragUv)
{
    // HAAAAACK!
    
    vec4 center = texture(iChannel0, fragUv);
    vec4 right = texture(iChannel0, (fragUv + vec2(s_texelSize.x, 0.0)));
    vec4 up = texture(iChannel0, (fragUv + vec2(0.0, s_texelSize.y)));

    return vec2((center.a - right.a), (center.a - up.a));
} 

void mainImage(
    out vec4 outFragColor,
    vec2 fragCoord)
{
    s_texelSize = (1.0 / iResolution.xy);
    
    vec2 fragUv = (fragCoord * s_texelSize);
    
    vec2 gradient = ComputeGradient(fragUv);
    vec2 curl = vec2((-1.0 * gradient.y), gradient.x);
    
    vec4 selfState = texture(iChannel1, fragUv);
    
    vec2 frameOffset = vec2(mod(float(iFrame), 97.0), mod(float(iFrame), 101.0));
    vec2 frameRandom = RandomVec2(fragUv + frameOffset);
    
    if (frameRandom.x > selfState.a)
    {
        vec4 camera = texture(iChannel2, fragUv);
        
        outFragColor.rgb = mix(selfState.rgb, camera.rgb, 0.8);
        //outFragColor.rgb = camera.rgb;
        
        //outFragColor.a = mix(outFragColor.a, 1.0, frameRandom.y);
        outFragColor.a = 1.0;
    }
    else
    {
        vec2 normalizedCurl = (curl / (length(curl) + 0.00001));
            
        vec4 curlSourceState = texture(iChannel1, (fragUv + (normalizedCurl * s_texelSize)));
        
        outFragColor = curlSourceState;
        //outFragColor.rg = smoothstep(vec2(-1.0), vec2(1.0), (25.0 * curl));
        
        outFragColor.a -= (0.005 * iTimeDelta);
    }
}