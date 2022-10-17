

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define VELOCITY 0 
#define PRESSURE 1
#define DIVERGENCE 2

#define SHOW VELOCITY


vec4 showPressure(vec2 uv)
{
    return abs(texture(iChannel2, uv)) * 0.05;
}

vec4 showVelocity(vec2 uv)
{
    vec4 color = texture(iChannel0, uv);
    if(color.z > 0.0) // obstacle
    {
        return vec4(0.5);
    }
    else
    {
        return abs(color) * 0.008;
    }
}

vec4 showDivergence(vec2 uv)
{
    // Divergence should be as close to 0 as possible.. sadly it isn't.
    return abs(texture(iChannel1, uv));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;

    #if SHOW == VELOCITY
    fragColor = showVelocity(uv);
    #elif SHOW == PRESSURE
    fragColor = showPressure(uv);
    #elif SHOW == DIVERGENCE
    fragColor = showDivergence(uv);
    #endif
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Advection & force

// Magic force within a rectangle.
const vec2 Force = vec2(100.0, 0.0);
const vec2 ForceAreaMin = vec2(0.0, 0.2); 
const vec2 ForceAreaMax = vec2(0.06, 0.8);

// Circular barrier.
const vec2 BarrierPosition = vec2(0.2, 0.5);
const float BarrierRadiusSq = 0.01;

#define VelocityTexture iChannel3

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 inverseResolution = vec2(1.0) / iResolution.xy;
    vec2 uv = fragCoord.xy * inverseResolution;

    // Simple advection by backstep.
    // Todo: Try better methods like MacCormack (http://http.developer.nvidia.com/GPUGems3/gpugems3_ch30.html)
    vec2 oldVelocity = texture(VelocityTexture, uv).xy;
    vec2 samplePos = uv - oldVelocity * iTimeDelta * inverseResolution;
    vec2 outputVelocity = texture(VelocityTexture, samplePos).xy;
    
    // Add force.
    if(uv.x > ForceAreaMin.x && uv.x < ForceAreaMax.x &&
       uv.y > ForceAreaMin.y && uv.y < ForceAreaMax.y)
    {
    	outputVelocity += Force * iTimeDelta;
    }
    
    // Clamp velocity at borders to zero.
    if(uv.x > 1.0 - inverseResolution.x ||
      	uv.y > 1.0 - inverseResolution.y ||
      	uv.x < inverseResolution.x ||
      	uv.y < inverseResolution.y)
    {
        outputVelocity = vec2(0.0, 0.0);
    }
    
    // Circle barrier.
    vec2 toBarrier = BarrierPosition - uv;
    toBarrier.x *= inverseResolution.y / inverseResolution.x;
    if(dot(toBarrier, toBarrier) < BarrierRadiusSq)
    {
        fragColor = vec4(0.0, 0.0, 999.0, 0.0);
    }
    else
    {
        fragColor = vec4(outputVelocity, 0.0, 0.0);
    } 
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Compute divergence.

#define VelocityTexture iChannel0

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 inverseResolution = vec2(1.0) / iResolution.xy;
    vec2 uv = fragCoord.xy * inverseResolution;
    
    // Obstacle?
    if(texture(VelocityTexture, uv).z > 0.0)
    {
        fragColor = vec4(0.0);
        return;
    }

    float x0 = texture(VelocityTexture, uv - vec2(inverseResolution.x, 0)).x;
    float x1 = texture(VelocityTexture, uv + vec2(inverseResolution.x, 0)).x;
    float y0 = texture(VelocityTexture, uv - vec2(0, inverseResolution.y)).y;
    float y1 = texture(VelocityTexture, uv + vec2(0, inverseResolution.y)).y;
    float divergence = ((x1-x0) + (y1-y0)) * 0.5;
    fragColor = vec4(divergence);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Jacobi iteration
// For a more accurate result, this should be executed multiple times.

#define DivergenceTexture iChannel1
#define PressureTexture iChannel2
#define VelocityTexture iChannel0

vec2 inverseResolution;
vec2 border;
vec2 uv;

float samplePressure(vec2 pos)
{
    // Obstacle?
    if(texture(VelocityTexture, pos).z > 0.0)
    {
        return 0.0;
    }
    
    // Boundary condition: Vanish for at walls.
    if(pos.x > 1.0 - border.x || pos.y > 1.0 - border.y ||
      	pos.x < border.x || pos.y < border.y)
    {
        return 0.0;
    }
   	else
    {
    	return texture(PressureTexture, pos).x;
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    inverseResolution = vec2(1.0) / iResolution.xy;
    border = inverseResolution * 2.0;
    uv = fragCoord.xy * inverseResolution;
    
    float div = texture(DivergenceTexture, uv).x;
    float x0 = samplePressure(uv - vec2(inverseResolution.x, 0));
    float x1 = samplePressure(uv + vec2(inverseResolution.x, 0));
    float y0 = samplePressure(uv - vec2(0, inverseResolution.y));
    float y1 = samplePressure(uv + vec2(0, inverseResolution.y));
    
   	fragColor = vec4((x0 + x1 + y0 + y1 - div) * 0.25);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Subtract pressure gradient to ensure zero divergence.

#define PressureTexture iChannel2
#define VelocityTexture iChannel0

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 inverseResolution = vec2(1.0) / iResolution.xy;
    vec2 uv = fragCoord.xy * inverseResolution;
    
    float x0 = texture(PressureTexture, uv - vec2(inverseResolution.x, 0)).x;
    float x1 = texture(PressureTexture, uv + vec2(inverseResolution.x, 0)).x;
    float y0 = texture(PressureTexture, uv - vec2(0, inverseResolution.y)).x;
    float y1 = texture(PressureTexture, uv + vec2(0, inverseResolution.y)).x;
    vec2 pressureGradient = (vec2(x1, y1) - vec2(x0, y0)) * 0.5;
    vec2 oldV = texture(VelocityTexture, uv).xy;
    
    fragColor = vec4(oldV - pressureGradient, 0.0, 0.0);
}