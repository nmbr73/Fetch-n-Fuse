
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

iR = iResolution.xy;

    ivec2 xy = ivec2(fragCoord);
    
    float time = mod(max(0.0, iTime - kTimeDelay) / kTimeStretch, kAnimationCycle);
    
    if(xy.x == 0 && xy.y == 2) { fragColor = vec4(time); }
    //float lastTime = texelFetch(iChannel0, ivec2(0, 2), 0).x;
    float lastTime = texture(iChannel0, (vec2(0, 2)+0.5)/iR).x;
    
    if(xy.x >= kNumMetaballs || xy.y >= 2) { return; }
    
    int y0 = iFrame % 2;
    int y1 = (iFrame + 1) % 2;
    
    //vec4 texel = texelFetch(iChannel0, xy, 0);
    vec4 texel = texture(iChannel0, (vec2(xy)+0.5)/iR);
    
    if(xy.y == y1) 
    { 
        fragColor = texel; 
        return; 
    }
        
    vec2 aspectRatio = vec2(iResolution.x / iResolution.y, 1.0);
    
    pcgInitialise(0);
    
    vec4 p[kNumMetaballs]; 
    for(int i = 0; i < kNumMetaballs; i++)
    {
        vec4 xi1 = rand();        
        vec4 xi2 = rand();
        vec3 v = charge(xi1, xi2, time);  
        
        p[i] = vec4(v.xy, xi2.y, xi2.x);        
    }
    
    if(time < 1.0 || length2(texel.rgb) == 0.0)
    {
        fragColor = vec4(metaColour(p[xy.x].zw), 0.0);
        return;
    }
    
    /*float delta = 1.0 - (kAnimationCycle - time) / kCloseTransition;
    if(delta > 0.0)
    {
        float delta = cos(kPi + kPi * delta) * 0.5 + 0.5;            
        fragColor = mix(texelFetch(iChannel0, xy, 0), vec4(metaColour(p[xy.x].zw), 0.0), pow(delta, 3.0));
        return;
    } */ 
    
    float sumWeights = 0.0;        
    vec3 colour = vec3(0.0);
    for(int i = 0; i < kNumMetaballs; i++)
    {               
        #define kMaxDist 0.1
        float weight = pow(max(0.0, 1.0 - length(p[xy.x].xy - p[i].xy) / kMaxDist), 1.0); 

        //colour += texelFetch(iChannel0, ivec2(i, y1), 0).rgb * weight;
        colour += texture(iChannel0, (vec2(i, y1)+0.5)/iR).rgb * weight;
        
        sumWeights += weight;
    }
    
    colour /= sumWeights;
    //vec3 thisColour = texelFetch(iChannel0, ivec2(xy.x, y1), 0).rgb;   
    vec3 thisColour = texture(iChannel0, (vec2(xy.x, y1)+0.5)/iR).rgb;  
    
    //thisColour = mix(thisColour, kOne * 0.7, 0.001);
    fragColor.xyz = mix(thisColour, colour, kMetaDiffusion / kTimeStretch);
    
}



// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
iR = iResolution.xy;

    #if kApplyBloom == 1   
    {    
        fragColor = bloom(fragCoord, iResolution, ivec2(1, 0), iChannel0);
    }
    #endif
}



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    iR = iResolution.xy;
    
    kFragCoord = ivec2(fragCoord);
    
    float time = mod(max(0.0, iTime - kTimeDelay) / kTimeStretch, kAnimationCycle);
    if(time > 0.0) { time += kTimeStartAt; }
    
    #if kApplyBloom == 1   
    {
        fragColor = renderMetaballs(fragCoord, iResolution, time, iChannel0, iChannel1);
    }
    #endif
}



// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------



vec2 iR;

#define kApplyBloom           1
#define kApplyVignette            true            // Apply vignette as a post-process

#define kBloomGain            10.0             // The strength of the bloom effect 
#define kBloomTint            vec4(1.0)       // The tint applied to the bloom effect
#define kBloomWidth           0.08             // The width of the bloom effect as a proportion of the buffer width
#define kBloomHeight          0.08             // The height of the bloom effect as a proportion of the buffer height
#define kBloomShape           4.0             // The fall-off of the bloom shape. Higher value = steeper fall-off
#define kBloomDownsample      3              // How much the bloom buffer is downsampled. Higher value = lower quality, but faster
#define kDebugBloom           false           // Show only the bloom in the final comp
#define kBloomBurnIn          vec4(0.5)

#define kVignetteStrength         0.5             // The strength of the vignette effect
#define kVignetteScale            1.2             // The scale of the vignette effect
#define kVignetteExponent         2.0             // The rate of attenuation of the vignette effect

#define kAA 1

#define kMetaSpeed            0.07
#define kMetaSpeedVariance    0.5
#define kMetaSpread           0.008
#define kMetaAA               3.0
#define kMetaColourPhase      0.0003
#define kMetaThreshold        8.0
#define kMetaDiffusion        0.01
#define kMetaPulse            true

#define kResolution           0
#if kResolution == 0
    #define kNumMetaballs         250
    #define kMetaCharge           0.0018
    #define kMetaChargeVariance   0.8
    #define kMetaChargeExponent   2.0
    #define kMetaSpreadVariance   0.6
    #define kMetaSpreadExponent   0.3
#else
    #define kNumMetaballs         500
    #define kMetaCharge           0.0013
    #define kMetaChargeVariance   0.9
    #define kMetaChargeExponent   3.0
    #define kMetaSpreadVariance   0.8
    #define kMetaSpreadExponent   0.3
#endif

#define kAnimationCycle       20.0
#define kOpenTransition       3.0
#define kCloseTransition      2.0
#define kTimeStretch          1.0
#define kTimeDelay            0.0
#define kTimeStartAt          0.0

#define kPi                    3.14159265359
#define kTwoPi                 (2.0 * kPi)
#define kHalfPi                (0.5 * kPi)
#define kRoot2                 1.41421356237
#define kFltMax                3.402823466e+38
#define kIntMax                0x7fffffff
#define kOne                   vec3(1.0)
#define kZero                  vec3(0.0)
#define kPink                  vec3(1.0, 0.0, 0.2)
float toRad(float deg)         { return kTwoPi * deg / 360.0; }
float toDeg(float rad)         { return 360.0 * rad / kTwoPi; }
float sqr(float a)             { return a * a; }
//int sqr(int a)                 { return a * a; }
int mod2(int a, int b)         { return ((a % b) + b) % b; }
float length2(vec2 v)          { return dot(v, v); }
float length2(vec3 v)          { return dot(v, v); }
int sum(ivec2 a)               { return a.x + a.y; }
float luminance(vec3 v)        { return v.x * 0.17691 + v.y * 0.8124 + v.z * 0.01063; }
float mean(vec3 v)             { return v.x / 3.0 + v.y / 3.0 + v.z / 3.0; }
//vec4 mul4(vec3 a, mat4 m)      { return vec4(a, 1.0) * m; }
//vec3 mul3(vec3 a, mat4 m)      { return (vec4(a, 1.0) * m).xyz; }
float saturate(float a)        { return clamp(a, 0.0, 1.0); }
float sin01(float a)           { return 0.5 * sin(a) + 0.5; }
float cos01(float a)           { return 0.5 * cos(a) + 0.5; }

ivec2 kFragCoord;
uvec4 rngSeed; 

// Maps the input xy texel coordinates to UV [0.0, 1.0] and distance R from center
vec2 xyToUv(in vec2 xy, in vec3 iResolution)
{
    vec2 uv = vec2(xy.x / iResolution.x, xy.y / iResolution.y);
    uv.x = (uv.x - 0.5) * (iResolution.x / iResolution.y) + 0.5;     
    return uv;
}

// Maps the input xy texel coordinates to UV [-1.0, 1.0] and distance R from center
vec3 xyToUvr(in vec2 xy, in vec3 iResolution)
{
    vec2 uv = xyToUv(xy, iResolution);    
    float x = 2.0 * (uv.x - 0.5);
    float y = 2.0 * (uv.y - 0.5);
    
    return vec3(uv, sqrt(x*x + y*y) / kRoot2);
}

// Permuted congruential generator from "Hash Functions for GPU Rendering" (Jarzynski and Olano)
// http://jcgt.org/published/0009/03/02/paper.pdf
uvec4 pcgAdvance()
{
    rngSeed = rngSeed * 1664525u + 1013904223u;
    
    rngSeed.x += rngSeed.y*rngSeed.w; 
    rngSeed.y += rngSeed.z*rngSeed.x; 
    rngSeed.z += rngSeed.x*rngSeed.y; 
    rngSeed.w += rngSeed.y*rngSeed.z;
    
    rngSeed ^= rngSeed >> 16u;
    
    rngSeed.x += rngSeed.y*rngSeed.w; 
    rngSeed.y += rngSeed.z*rngSeed.x; 
    rngSeed.z += rngSeed.x*rngSeed.y; 
    rngSeed.w += rngSeed.y*rngSeed.z;
    
    return rngSeed;
}

// Seed the PCG hash function with the current frame multipled by a prime
void pcgInitialise(int frame)
{    
    rngSeed = uvec4(20219u, 7243u, 12547u, 28573u) * uint(frame);
}

// Generates a tuple of canonical random number and uses them to sample an input texture
vec4 randX(sampler2D sampler)
{
    //return texelFetch(sampler, (kFragCoord + ivec2(pcgAdvance() >> 16)) % 1024, 0);
    return texture(sampler, (vec2((kFragCoord + ivec2(pcgAdvance() >> 16)) % 1024)+0.5)/iR);
}

// Generates a tuple of canonical random numbers in the range [0, 1]
vec4 rand()
{
    return vec4(pcgAdvance()) / float(0xffffffffu);
}

vec3 hue(float phi)
{
    float phiColour = 6.0 * phi / kTwoPi;
    int i = int(phiColour);
    vec3 c0 = vec3(((i + 4) / 3) & 1, ((i + 2) / 3) & 1, ((i + 0) / 3) & 1);
    vec3 c1 = vec3(((i + 5) / 3) & 1, ((i + 3) / 3) & 1, ((i + 1) / 3) & 1);             
    return clamp(mix(c0, c1, phiColour - float(i)), kZero, kOne);
}

vec3 hue(float phi, float saturation, float brightness)
{
    vec3 rgb = hue(phi);
    
    rgb = mix(vec3(0.5), rgb, saturation);
    
    return (brightness < 0.5) ? mix(kZero, rgb, brightness * 2.0) :
                                mix(rgb, kOne, (brightness - 0.5) * 2.0);
}

vec2 bezier(vec2 u0, vec2 u1, vec2 u2, vec2 u3, float t)
{
    vec2 v0 = mix(u0, u1, t);
    vec2 v1 = mix(u1, u2, t);
    vec2 v2 = mix(u2, u3, t);
    vec2 w0 = mix(v0, v1, t);
    vec2 w1 = mix(v1, v2, t);
    
    return mix(w0, w1, t);    
}

vec3 metaColour(vec2 t)
{
    #define sat 1.3
    #define light 0.5
    
    //t.x = fract(t.x * 10.0);
    
    t.x = fract(t.x + 0.01);    
    
    //h = (fract(t * 2.0) < 0.5) ? hue(0.0, 0.0, 0.9) : clamp(hue(0.0, 1.2, 0.5), vec3(0.0), vec3(1.0));
    if(t.x < 0.333) { return hue(kTwoPi * (0.11 + t.y * 0.03), sat, light); }
    else if(t.x < 0.666) { return hue(kTwoPi * (0.92 + t.y * 0.03), sat, light); }
    return hue(kTwoPi * (0.55 + t.y * 0.03), sat, light); 
}

vec2 heartPath(float t)
{
    vec2 dir = (t < 0.5) ? vec2(1.0, -1.0) : vec2(-1.0, -1.0);
    if(t >= 0.5) { t = 1.0 - t; }
    t *= 2.0;
    
    if(t < 0.5)
    {
        return bezier(vec2(-1.5266667e-6,-24.039329), 
                      vec2(-19.654762,-56.545271), 
                      vec2(-50.316625,-43.202057), 
                      vec2(-50.270832,-19.881592), t * 2.0) * dir;
    }
    else
    {
        return bezier(vec2(-50.270832,-19.881592), 
                      vec2(-50.270832 + 0.04579,-19.881592 + 23.3204663), 
                      vec2(-50.270832 + 38.50771,-19.881592+31.238355), 
                      vec2(-50.270832+50.22497425,-19.881592+73.193701), (t - 0.5) * 2.0) * dir;
    }   
}

vec3 charge(vec4 xi1, vec4 xi2, float time)
{           
    vec3 p = kZero;
    float t = mod(kMetaSpeed * mix(1.0 - kMetaSpeedVariance, 1.0, xi2.x) * time + xi2.y, 1.0);
    float pulse = 1.0;
    
    if(time < kOpenTransition)
    {
        float delta = smoothstep(0.0, 1.0, time / kOpenTransition);   
        pulse = delta;
        
        p.z = pow(saturate((delta - (xi2.y * 0.8)) * 5.0), 0.3) * kMetaCharge * mix(1.0 - kMetaChargeVariance, 1.0, pow(xi1.y, kMetaChargeExponent));
    }
    else if(kAnimationCycle - time < kCloseTransition)
    {
        float delta = 1.0 - (kAnimationCycle - time) / kCloseTransition;        
        delta = cos(kPi + kPi * delta) * 0.5 + 0.5;
        pulse = 1.0 - delta;
        
        p.y = -pow(delta, 2.0 + xi1.x) * mix(1.0, 2.0, xi1.y);
        p.z = kMetaCharge * mix(1.0 - kMetaChargeVariance, 1.0, pow(xi1.y, kMetaChargeExponent)) * mix(1.0, 0.5, pow(delta, 2.0));  
    }
    else
    {
        p.z = kMetaCharge * mix(1.0 - kMetaChargeVariance, 1.0, pow(xi1.y, kMetaChargeExponent));
    }
    
    float d = kMetaSpread * mix(1.0 - kMetaSpreadVariance, 1.0, pow(xi1.y, kMetaSpreadExponent));    
    if(kMetaPulse) d += kMetaSpread * mix(0.0, max(0.0, sin(time * 5.0)), 0.05 * pulse);
    p.xy = heartPath(t) * d + vec2(0.0, 0.015) + p.xy;
 
    return p;
}

void field(in vec2 xy, in float time, vec3 iResolution, out float F, out vec2 delF, out vec3 colour, sampler2D sampler)
{      
    pcgInitialise(0);
    
    F = 0.0;
    delF = vec2(0.0);
    float denom = 0.0;
    float sumWeights = 0.0;
    colour = kZero;
    
    vec2 aspectRatio = vec2(iResolution.x / iResolution.y, 1.0);
     
    for(int i = 0; i < kNumMetaballs; i++)
    {
        vec4 xi1 = rand();        
        vec4 xi2 = rand();
        vec3 p = charge(xi1, xi2, time);    
        
        F += p.z / length2(xy - p.xy);
        
        float n = sqr(xy.x - p.x) + sqr(xy.y - p.y);
        delF.x += -p.z * 2.0 * (xy.x - p.x) / sqr(n);
        delF.y += -p.z * 2.0 * (xy.y - p.y) / sqr(n);
        
        denom += p.z / n;
        
        float weight = pow(min(1e5, 1.0 / length2(xy - p.xy)), 2.0);
        
        if(weight > 1e-2)
        {        
            //vec3 h = texelFetch(sampler, ivec2(i, 0), 0).rgb;
            vec3 h = texture(sampler, (vec2(i, 0)+0.5)/iResolution.xy).rgb;
            
            if(length2(h) == 0.0)
            {
                h = metaColour(vec2(xi2.y, xi2.x));
            }
       
            colour += h * weight;
            sumWeights += weight;
        }
    }
    
    colour /= sumWeights;    
    F = 1.0 / sqrt(F) - 1.0 / sqrt(kMetaThreshold);   
    delF = -0.5 * delF / pow(denom, 1.5);
}

vec4 renderMetaballs(vec2 fragCoord, vec3 iResolution, float iTime, sampler2D sampler, sampler2D colourMap)
{                
    vec2 xy = vec2((fragCoord.x - 0.5 * iResolution.x) / iResolution.y, 
                   (fragCoord.y - 0.5 * iResolution.y) / iResolution.y);
                   
    //xy = xy * 0.1 + vec2(0.27, 0.27);
    //xy = xy * 0.1 - vec2(0.13, 0.13);

    float F;
    vec2 delF;
    vec3 colour;
    field(xy, iTime, iResolution, F, delF, colour, colourMap);   
    
    if(kAnimationCycle - iTime < kCloseTransition)
    {
        float delta = 1.0 - (kAnimationCycle - iTime) / kCloseTransition;        
        colour = mix(colour, kZero, saturate(pow(delta * 2.0, 2.0)));
    }

    float delFLength = length(delF);
    vec2 delFNorm = delF / ((delFLength > 1.0) ? delFLength : 1.0);
            
    delFNorm = delFNorm * 0.5 + vec2(0.5);    
    
    float FMax = (kMetaAA / iResolution.x) * length(delF);          
    if(F < FMax)
    {        
        float alpha = min(1.0, (FMax - F) / FMax);
        float z = pow(min(1.0, -F * 2.5), 0.5);
        vec3 normal = normalize(vec3(delFNorm * (1.0 - z), z));                
                
        float L = pow(luminance(texture(sampler, normal.xy, 0.0).xyz), 2.0);
        //return vec4(mix(colour, kOne, L), mix(0.05 * alpha, 0.05 + L, clamp(-F, 0.0, 1.0))); 
        return vec4(mix(colour, kOne, L), mix(0.05 * alpha, 0.05 + L, clamp(-F / 0.01, 0.0, 1.0))); 
    }   
    else
    {
        return vec4(0.0);
    }  
}

// Seperable bloom function. This filter requires two passes in the horizontal and vertical directions which are combined as a post-process
// effect after each frame. The accuracy/cost of the effect can be tuned by dialing the kBloomDownsample parameter. 
vec4 bloom(vec2 fragCoord, vec3 iResolution, ivec2 delta, sampler2D renderSampler)
{        
    vec2 scaledResolution = vec2(iResolution.x, iResolution.y) / float((delta.x == 1) ? kBloomDownsample : 1);
   
    if(fragCoord.x > scaledResolution.x || fragCoord.y > scaledResolution.y) { return vec4(0.0); }
    
    float bloomSize = (delta.x == 1) ? kBloomWidth : kBloomHeight;
    
    int kKernelWidth = int(bloomSize * max(iResolution.x, iResolution.y) + 0.5) / ((delta.x == 1) ? kBloomDownsample : 1);
    vec4 sumWeights = vec4(0.0);
    vec4 sumRgb = vec4(0.0);
    for(int i = -kKernelWidth; i <= kKernelWidth; i++)
    {      
        vec2 xy = vec2(fragCoord.x + float(i * delta.x), fragCoord.y + float(i * delta.y));
        
        if(delta.x == 1) { xy *= float(kBloomDownsample); }
        else { xy /= float(kBloomDownsample); }
        
        if(xy.x < 0.0 || xy.x > iResolution.x || xy.y < 0.0 || xy.y > iResolution.y) { continue; }
            
        vec4 rgb = texture(renderSampler, xy / iResolution.xy);
        float d = float(abs(i)) / float(kKernelWidth);
           
        vec4 weight = vec4(1.0);
        if(i != 0)
        {
            // Currently using a single weight although this effect can be done per-channel
            float kernel = pow(max(0.0, (exp(-sqr(d * 4.0)) - 0.0183156) / 0.981684), kBloomShape);            
            weight = vec4(1.0) * kernel;
        }
            
        sumRgb += ((delta.y == 1) ? rgb : max(vec4(0.0), rgb - kBloomBurnIn)) * weight;         
        sumWeights += weight;
    }
    
    sumRgb = sumRgb / sumWeights;
    
    return (delta.x == 1) ? sumRgb : (sumRgb * kBloomTint);
}



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



float vignette(in vec2 fragCoord)
{
    vec3 uvr = xyToUvr(fragCoord, iResolution);                
    return mix(1.0, max(0.0, 1.0 - pow(uvr.z * kVignetteScale, kVignetteExponent)), kVignetteStrength);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{       

iR = iResolution.xy;

    vec3 rgb = kZero; 
    vec4 texel;
    #if kApplyBloom == 1
    {
        rgb += vec3(bloom(fragCoord, iResolution, ivec2(0, 1), iChannel1).w * kBloomGain);
        
        if(kDebugBloom)
        {
            fragColor = vec4(rgb, 1.0);
            return;
        }
        
        //texel = texelFetch(iChannel0, ivec2(fragCoord), 0);
        texel = texture(iChannel0, (vec2(fragCoord)+0.5)/iR);
    }
    #else
    {
        texel = renderMetaballs(fragCoord, iResolution, iTime / kTimeStretch, iChannel2);
    }
    #endif
        
    vec3 bgColour = vec3(0.13);
    
    if(kApplyVignette) { bgColour *= vignette(fragCoord); }
    
    float alpha = min(0.05, texel.w) / 0.05;
    rgb += mix(bgColour, texel.rgb, alpha);
    
    rgb = clamp(rgb, vec3(0.0), vec3(1.0));
    
    
    
    fragColor.xyz = rgb;
    fragColor.w = 1.0;
}


