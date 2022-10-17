

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec2 sTexelSize;

void mainImage(
    out vec4 outFragColor,
    in vec2 fragCoord)
{
    sTexelSize = (1.0 / iResolution.xy);
    
    vec4 selfState = texture(iChannel0, (fragCoord * sTexelSize));
    
    outFragColor = vec4(0.25, 0.25, 0.7, 1.0);
    
    if (selfState.g > 0.5)
    {
        outFragColor = vec4(0.85, 0.9, 1.0, 1.0);
    }
    else if (selfState.r > 0.5)
    {
        outFragColor = mix(outFragColor, vec4(0.6, 0.6, 1.0, 1.0), (1.0 - (1.0 / (1.0 + selfState.r))));
    }
        
	// outFragColor = selfState; // Debug.
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Buffer format:
// R - Moving Particle Count (can be temporarily greater than 1 if particles collide)
// G - Frozen Particle Count (should only ever be 0 or 1)

vec2 kDirectionDeltas[8] = vec2[](
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0),
    vec2(-1.0, 1.0),
    vec2(-1.0, 0.0),
    vec2(-1.0, -1.0),
    vec2(0.0, -1.0),
    vec2(1.0, -1.0));

const int kKeySpace = 32;

vec2 sTexelSize;

float randomFloat(
  vec2 testCoord)
{
	// From: https://www.shadertoy.com/view/Xd23Dh
	// (just removed some dimensions)
	float testCoordInGeneratorSpace = 
		dot(testCoord, vec2(127.1, 311.7));

	return fract(sin(testCoordInGeneratorSpace) * 43758.5453);
}

bool coordIsContainedInFragment(
    vec2 testCoord,
    vec2 fragCoord)
{
    vec2 delta = (testCoord - fragCoord);
    return (
        (-0.5 < delta.x) && (delta.x <= 0.5) && 
		(-0.5 < delta.y) && (delta.y <= 0.5));
}

int wrapDirectionIndex(
    int unboundedDirectionIndex)
{
    return (
        unboundedDirectionIndex + 
		((unboundedDirectionIndex < 0) ? 8 : 0) +
		((unboundedDirectionIndex > 7) ? -8 : 0));
}

float getFragRandom(
    vec2 fragCoord)
{
    return randomFloat((fragCoord * sTexelSize) + mod(iTime, 15.0));;
}

int directionFractionToIndex(
    float directionFraction)
{
    return int(directionFraction * 7.999999);
}

void mainImage(
    out vec4 outFragColor,
    in vec2 fragCoord)
{
    sTexelSize = (1.0 / iResolution.xy);
        
    vec4 selfState = texture(iChannel0, (fragCoord * sTexelSize));
    
    outFragColor = selfState;
    
    float selfRandom = getFragRandom(fragCoord);
        
    if ((iFrame == 0) || 
        (texelFetch(iChannel1, ivec2(kKeySpace, 1), 0).x > 0.0))
    {
        // If we're the seed-crystal, else we're possibly a moving particle.
        if (coordIsContainedInFragment((iResolution.xy / 2.0), fragCoord))
        {
            outFragColor = vec4(0.0, 1.0, 0.0, 0.0);
        }
        else
        {
            outFragColor = vec4(step(0.8, selfRandom), 0.0, 0.0, 0.0);
        }
    }
    else
    {        
        // If we're frozen.
        if (selfState.g > 0.5)
        {
            // TODO: Handshake with neighbors to let any remaining moving particles escape to empty cells.
            outFragColor.r = 0.0;
        }
        else
        {
            float selfDirection = floor((selfRandom * 8.0) + 0.5);
            
            // Accept moving particles from neighbors.
            {
                float incomingMovingParticleCount = 0.0;
                for (int directionIndex = 0; directionIndex < 8; directionIndex++)
                {
                    vec2 neighborFragCoord = (fragCoord + kDirectionDeltas[directionIndex]);
                    float neighborRandom = getFragRandom(neighborFragCoord);

                    // If this neighbor is trying to move into our cell.
                    if (directionFractionToIndex(neighborRandom) == wrapDirectionIndex(directionIndex + 4))
                    {
                        vec4 neighborState = texture(iChannel0, (neighborFragCoord * sTexelSize));

                        if (neighborState.r > 0.5)
                        {
                            incomingMovingParticleCount += 1.0;
                        }
                    }
                }

                outFragColor.r += incomingMovingParticleCount;
            }
            
            // If we contained a moving particle.
            if (selfState.r > 0.5)
            {
            	vec2 destinationFragCoord = (fragCoord + kDirectionDeltas[directionFractionToIndex(selfRandom)]);
                vec4 destinationState = texture(iChannel0, (destinationFragCoord * sTexelSize));
                
                // If we just froze.
                if (destinationState.g > 0.5)
                {
                    outFragColor.g = 1.0; // Become frozen.

                    // TODO: Handshake with neighbors to let any remaining moving particles escape to empty cells.
                    outFragColor.r = 0.0;
                }
                else
                {
                    // We're losing one particle to movement towards whichever neighbor is accepting movement from us.
                    outFragColor.r = max(0.0, (outFragColor.r - 1.0));
                }
            }
        }
    }
}