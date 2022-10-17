

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const vec3 color1 = vec3(0.0, 0.05, 0.2);
const vec3 color2 = vec3(0.1, 0.0, 0.1);
const vec3 color3 = vec3(0.5, 0.15, 0.25);
const vec3 color4 = vec3(2.0, 1.25, 0.7);
const vec3 color5 = vec3(2.0, 2.0, 2.0);

const vec3 glowColor1 = vec3(1.5, 0.5, 0.0);
const vec3 glowColor2 = vec3(1.5, 1.5, 0.5);

const vec3 lightColor = vec3(1.0, 1.5, 0.75);
const vec3 lightDirection = normalize(vec3(0.0, -1.0, 0.0));

const float a = 0.125;
const float b = 0.35;
const float c = 0.5;

vec3 gradient(float value)
{
    vec4 start = vec4(0.0, a, b, c);
    vec4 end = vec4(a, b, c, 1.0);
    vec4 mixValue = smoothstep(start, end, vec4(value));
    
    vec3 color = mix(color1, color2, mixValue.x);
    color = mix(color, color3, mixValue.y);
    color = mix(color, color4, mixValue.z);
    color = mix(color, color5, mixValue.w);
    
    return color;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    
    vec4 source = texture(iChannel0, uv);
    
    vec2 force = texture(iChannel1, uv).xy;
    force = DecodeForce(force);
    
    float value = length(force);
    
    float glow = source.w + source.z * 0.75;
    glow /= 2.0;
    
    vec3 color = gradient(value);
    color += mix(glowColor1, glowColor2, glow) * glow;
    
    vec3 normal = vec3(force.x, force.y, 1.0) * 0.5;
    normal = normalize(normal);
    
    float NdotL = smoothstep(-0.5, 0.5, dot(normal, lightDirection));
    color += color * NdotL * lightColor;
    
    fragColor = vec4(color, 1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 EncodeForce(vec2 force)
{
    force = clamp(force, -1.0, 1.0);
    return force * 0.5 + 0.5;
}

vec2 DecodeForce(vec2 force)
{
    force = force * 2.0 - 1.0;
    return force;
}

const float pi = 3.14159265359;
const float tau = 6.28318530718;

mat2 rot(float a) 
{
    vec2 s = sin(vec2(a, a + pi/2.0));
    return mat2(s.y,s.x,-s.x,s.y);
}

float linearStep(float a, float b, float x)
{
    return clamp((x - a)/(b - a), 0.0, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Init Fluid

const vec3 noiseSpeed1 = vec3(-0.05, 0.0, 0.2);
const float noiseSize1 = 3.3;
const vec3 noiseSpeed2 = vec3(0.05, 0.0, -0.2);
const float noiseSize2 = 0.8;
const float circleForceAmount = 15.0;
const vec2 randomForceAmount = vec2(0.5, 0.75);
const vec2 upForce = vec2(0.0, 0.8);
const vec2 moveSpeed = vec2(1.0, 2.0);

vec4 GetNoise(vec2 uv, float ratio)
{
    vec3 noiseCoord1;
    noiseCoord1.xy = uv;
    noiseCoord1.x *= ratio;
    noiseCoord1 += iTime * noiseSpeed1;
    noiseCoord1 *= noiseSize1;
    
    vec3 noiseCoord2;
    noiseCoord2.xy = uv;
    noiseCoord2.x *= ratio;
    noiseCoord2 += iTime * noiseSpeed2;
    noiseCoord2 *= noiseSize2;
    
    vec4 noise1 = texture(iChannel2, noiseCoord1);
    vec4 noise2 = texture(iChannel2, noiseCoord2);
    
    vec4 noise = (noise1 + noise2) / 2.0;
    
    return noise;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float ratio = iResolution.x / iResolution.y;
    vec2 uv = fragCoord / iResolution.xy;
    
    vec2 circleCoord = uv;   
    vec2 mousePos = vec2(0.0);
    vec2 circleVelocity = vec2(0.0);
    
    if(iMouse.z > 0.5)
    {     
        circleCoord -= iMouse.xy/iResolution.xy;
    }
    else
    {
        circleCoord -= 0.5;
        circleCoord.xy += sin(iTime * moveSpeed) * vec2(0.35, 0.25);
    }
    
    circleCoord.x *= ratio;
    
    float circle = length(circleCoord);
    float bottom = uv.y;
    
    vec4 masksIN = vec4(0.08, 0.35, 0.05, 0.2);
    vec4 masksOUT = vec4(0.06, 0.0, 0.0, 0.0);
    vec4 masksValue = vec4(circle, circle, bottom, bottom);
    vec4 masks = smoothstep(masksIN, masksOUT, masksValue);

    vec2 mask = masks.xy + masks.zw;
    
    vec4 noise = GetNoise(uv, ratio);
        
    vec2 force = circleCoord * noise.xy * circleForceAmount * masks.x;
    force += (noise.xy - 0.5) * (masks.x * randomForceAmount.x + masks.z * randomForceAmount.y);
    force.y += (0.25 + 0.75 * noise.z) * (masks.x * upForce.x + masks.z * upForce.y);
    force = EncodeForce(force);
    
    fragColor = vec4(force.x, force.y, mask.x, mask.y);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//Move Fluid

const float flow1 = 0.5;
const float flow2 = 0.75;
const float speed = 0.02;
const float gravity = -0.15;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float ratio = iResolution.x / iResolution.y;
    vec2 uv = fragCoord / iResolution.xy;

    vec4 source = texture(iChannel0, uv);
 
    vec2 force = texture(iChannel1, uv).xy;
    force = DecodeForce(force);
    force.y -= gravity;
    
    vec2 s = vec2(speed);
    s.x /= ratio;
    force *= s;
    
    source.z = smoothstep(flow1, flow2, source.z);
    
    vec2 movedForce = texture(iChannel1, uv - force).xy;
    movedForce = mix(movedForce, source.xy, source.z);
    
    fragColor = vec4(movedForce.x, movedForce.y, 0.0, 1.0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
//Update Fluid

const int Xiterations = 2;
const int Yiterations = 2;

const float sampleDistance1 = 0.006;
const float sampleDistance2 = 0.0001;

const float forceDamping = 0.01;

const vec3 noiseSpeed1 = vec3(0.0, 0.1, 0.2);
const float noiseSize1 = 2.7;
const vec3 noiseSpeed2 = vec3(0.0, -0.1, -0.2);
const float noiseSize2 = 0.8;

const float turbulenceAmount = 2.0;

vec4 GetNoise(vec2 uv, float ratio)
{
    vec3 noiseCoord1;
    noiseCoord1.xy = uv;
    noiseCoord1.x *= ratio;
    noiseCoord1 += iTime * noiseSpeed1;
    noiseCoord1 *= noiseSize1;
    
    vec3 noiseCoord2;
    noiseCoord2.xy = uv;
    noiseCoord2.x *= ratio;
    noiseCoord2 += iTime * noiseSpeed2;
    noiseCoord2 *= noiseSize2;
    
    vec4 noise1 = texture(iChannel2, noiseCoord1);
    vec4 noise2 = texture(iChannel2, noiseCoord2);
    
    vec4 noise = (noise1 + noise2) / 2.0;
    
    return noise;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float ratio = iResolution.x / iResolution.y;
    vec2 uv = fragCoord / iResolution.xy;
    
    vec4 source = texture(iChannel0, uv);    
    
    vec2 currentForce = DecodeForce(texture(iChannel1, uv).xy);
    float currentForceMagnitude = length(currentForce);
       
    vec3 sampleDistance; 
    sampleDistance.xy = vec2(mix(sampleDistance1, sampleDistance2, smoothstep(-0.25, 0.65, currentForceMagnitude)));
    sampleDistance.z = 0.0;
    
    vec2 totalForce = vec2(0.0);
    float iterations = 0.0;
    
    for(int x = -Xiterations; x <= Xiterations; x++)
    {
        for(int y = -Yiterations; y <= Yiterations; y++)
        {
            vec3 dir = vec3(float(x), float(y), 0.0);
            vec4 sampledValue = texture(iChannel1, uv + dir.xy * sampleDistance.xy);
            
            vec2 force = DecodeForce(sampledValue.xy); 
            float forceValue = length(force);
            totalForce += force * forceValue;
            iterations += forceValue;
        }
    }
    
    totalForce /= iterations;  
    totalForce -= totalForce * forceDamping;
    
    float turbulence = GetNoise(uv, ratio).z - 0.5;
    turbulence *= mix(0.0, turbulenceAmount, smoothstep(0.0, 1.0, currentForceMagnitude));
    
    totalForce *= rot(turbulence);
    totalForce = EncodeForce(totalForce);
    
    fragColor = vec4(totalForce.x, totalForce.y, 0.0, 1.0);
}