
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------



__KERNEL__ void Pan_ToneKernel( out float4 fragColor, in float2 fragCoord )
{

iR = swixy(iResolution);

    ivec2 xy = ito_float2(fragCoord);
    
    float time = mod_f(_fmaxf(0.0f, iTime - kTimeDelay) / kTimeStretch, kAnimationCycle);
    
    if(xy.x == 0 && xy.y == 2) { fragColor = to_float4(time); }
    //float lastTime = texelFetch(iChannel0, ito_float2(0, 2), 0).x;
    float lastTime = texture(iChannel0, (to_float2(0, 2)+0.5f)/iR).x;
    
    if(xy.x >= kNumMetaballs || xy.y >= 2) { return; }
    
    int y0 = iFrame % 2;
    int y1 = (iFrame + 1) % 2;
    
    //vec4 texel = texelFetch(iChannel0, xy, 0);
    float4 texel = texture(iChannel0, (to_float2(xy)+0.5f)/iR);
    
    if(xy.y == y1) 
    { 
        fragColor = texel; 
        return; 
    }
        
    float2 aspectRatio = to_float2(iResolution.x / iResolution.y, 1.0f);
    
    pcgInitialise(0);
    
    float4 p[kNumMetaballs]; 
    for(int i = 0; i < kNumMetaballs; i++)
    {
        float4 xi1 = rand();        
        float4 xi2 = rand();
        float3 v = charge(xi1, xi2, time);  
        
        p[i] = to_float4(swixy(v), xi2.y, xi2.x);        
    }
    
    if(time < 1.0f || length2(texel.rgb) == 0.0f)
    {
        fragColor = to_float4_aw(metaColour(p[xy.x].zw), 0.0f);
        return;
    }
    
    /*float delta = 1.0f - (kAnimationCycle - time) / kCloseTransition;
    if(delta > 0.0f)
    {
        float delta = _cosf(kPi + kPi * delta) * 0.5f + 0.5f;            
        fragColor = _mix(texelFetch(iChannel0, xy, 0), to_float4_aw(metaColour(p[xy.x].zw), 0.0f), _powf(delta, 3.0f));
        return;
    } */ 
    
    float sumWeights = 0.0f;        
    float3 colour = to_float3_s(0.0f);
    for(int i = 0; i < kNumMetaballs; i++)
    {               
        #define kMaxDist 0.1
        float weight = _powf(_fmaxf(0.0f, 1.0f - length(p[xy.x].xy - p[i].xy) / kMaxDist), 1.0f); 

        //colour += texelFetch(iChannel0, ito_float2(i, y1), 0).rgb * weight;
        colour += texture(iChannel0, (to_float2(i, y1)+0.5f)/iR).rgb * weight;
        
        sumWeights += weight;
    }
    
    colour /= sumWeights;
    //vec3 thisColour = texelFetch(iChannel0, ito_float2(xy.x, y1), 0).rgb;   
    float3 thisColour = texture(iChannel0, (to_float2(xy.x, y1)+0.5f)/iR).rgb;  
    
    //thisColour = _mix(thisColour, kOne * 0.7f, 0.001f);
    swixyz(fragColor) = _mix(thisColour, colour, kMetaDiffusion / kTimeStretch);
    
}



// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------



__KERNEL__ void Pan_ToneKernel( out float4 fragColor, in float2 fragCoord )
{
iR = swixy(iResolution);

    #if kApplyBloom == 1   
    {    
        fragColor = bloom(fragCoord, iResolution, ito_float2(1, 0), iChannel0);
    }
    #endif
}



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------



__KERNEL__ void Pan_ToneKernel( out float4 fragColor, in float2 fragCoord )
{

    iR = swixy(iResolution);
    
    kFragCoord = ito_float2(fragCoord);
    
    float time = mod_f(_fmaxf(0.0f, iTime - kTimeDelay) / kTimeStretch, kAnimationCycle);
    if(time > 0.0f) { time += kTimeStartAt; }
    
    #if kApplyBloom == 1   
    {
        fragColor = renderMetaballs(fragCoord, iResolution, time, iChannel0, iChannel1);
    }
    #endif
}



// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------



float2 iR;

#define kApplyBloom           1
#define kApplyVignette            true            // Apply vignette as a post-process

#define kBloomGain            10.0f             // The strength of the bloom effect 
#define kBloomTint            to_float4_s(1.0f)       // The tint applied to the bloom effect
#define kBloomWidth           0.08f             // The width of the bloom effect as a proportion of the buffer width
#define kBloomHeight          0.08f             // The height of the bloom effect as a proportion of the buffer height
#define kBloomShape           4.0f             // The fall-off of the bloom shape. Higher value = steeper fall-off
#define kBloomDownsample      3              // How much the bloom buffer is downsampled. Higher value = lower quality, but faster
#define kDebugBloom           false           // Show only the bloom in the final comp
#define kBloomBurnIn          to_float4_s(0.5f)

#define kVignetteStrength         0.5f             // The strength of the vignette effect
#define kVignetteScale            1.2f             // The scale of the vignette effect
#define kVignetteExponent         2.0f             // The rate of attenuation of the vignette effect

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
#define kTwoPi                 (2.0f * kPi)
#define kHalfPi                (0.5f * kPi)
#define kRoot2                 1.41421356237
#define kFltMax                3.402823466e+38
#define kIntMax                0x7fffffff
#define kOne                   to_float3_s(1.0f)
#define kZero                  to_float3_s(0.0f)
#define kPink                  to_float3(1.0f, 0.0f, 0.2f)
float toRad(float deg)         { return kTwoPi * deg / 360.0f; }
float toDeg(float rad)         { return 360.0f * rad / kTwoPi; }
float sqr(float a)             { return a * a; }
//int sqr(int a)                 { return a * a; }
int mod2(int a, int b)         { return ((a % b) + b) % b; }
float length2(float2 v)          { return dot(v, v); }
float length2(float3 v)          { return dot(v, v); }
int sum(ivec2 a)               { return a.x + a.y; }
float luminance(float3 v)        { return v.x * 0.17691f + v.y * 0.8124f + v.z * 0.01063f; }
float mean(float3 v)             { return v.x / 3.0f + v.y / 3.0f + v.z / 3.0f; }
//vec4 mul4(float3 a, mat4 m)      { return to_float4_aw(a, 1.0f) * m; }
//vec3 mul3(float3 a, mat4 m)      { return (to_float4_aw(a, 1.0f) * m).xyz; }
float _saturatef(float a)        { return clamp(a, 0.0f, 1.0f); }
float sin01(float a)           { return 0.5f * _sinf(a) + 0.5f; }
float cos01(float a)           { return 0.5f * _cosf(a) + 0.5f; }

ivec2 kFragCoord;
uvec4 rngSeed; 

// Maps the input xy texel coordinates to UV [0.0, 1.0] and distance R from center
float2 xyToUv(in float2 xy, in float3 iResolution)
{
    float2 uv = to_float2(xy.x / iResolution.x, xy.y / iResolution.y);
    uv.x = (uv.x - 0.5f) * (iResolution.x / iResolution.y) + 0.5f;     
    return uv;
}

// Maps the input xy texel coordinates to UV [-1.0f, 1.0] and distance R from center
float3 xyToUvr(in float2 xy, in float3 iResolution)
{
    float2 uv = xyToUv(xy, iResolution);    
    float x = 2.0f * (uv.x - 0.5f);
    float y = 2.0f * (uv.y - 0.5f);
    
    return to_float3_aw(uv, _sqrtf(x*x + y*y) / kRoot2);
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
    rngSeed = uto_float4(20219u, 7243u, 12547u, 28573u) * uint(frame);
}

// Generates a tuple of canonical random number and uses them to sample an input texture
float4 randX(sampler2D sampler)
{
    //return texelFetch(sampler, (kFragCoord + ito_float2(pcgAdvance() >> 16)) % 1024, 0);
    return texture(sampler, (to_float2((kFragCoord + ito_float2(pcgAdvance() >> 16)) % 1024)+0.5f)/iR);
}

// Generates a tuple of canonical random numbers in the range [0, 1]
float4 rand()
{
    return to_float4_aw(pcgAdvance()) / float(0xffffffffu);
}

float3 hue(float phi)
{
    float phiColour = 6.0f * phi / kTwoPi;
    int i = int(phiColour);
    float3 c0 = to_float3(((i + 4) / 3) & 1, ((i + 2) / 3) & 1, ((i + 0) / 3) & 1);
    float3 c1 = to_float3_aw(((i + 5) / 3) & 1, ((i + 3) / 3) & 1, ((i + 1) / 3) & 1);             
    return clamp(_mix(c0, c1, phiColour - float(i)), kZero, kOne);
}

float3 hue(float phi, float saturation, float brightness)
{
    float3 rgb = hue(phi);
    
    rgb = _mix(to_float3_s(0.5f), rgb, saturation);
    
    return (brightness < 0.5f) ? _mix(kZero, rgb, brightness * 2.0f) :
                                _mix(rgb, kOne, (brightness - 0.5f) * 2.0f);
}

float2 bezier(float2 u0, float2 u1, float2 u2, float2 u3, float t)
{
    float2 v0 = _mix(u0, u1, t);
    float2 v1 = _mix(u1, u2, t);
    float2 v2 = _mix(u2, u3, t);
    float2 w0 = _mix(v0, v1, t);
    float2 w1 = _mix(v1, v2, t);
    
    return _mix(w0, w1, t);    
}

float3 metaColour(float2 t)
{
    #define sat 1.3
    #define light 0.5
    
    //t.x = fract(t.x * 10.0f);
    
    t.x = fract(t.x + 0.01f);    
    
    //h = (fract(t * 2.0f) < 0.5f) ? hue(0.0f, 0.0f, 0.9f) : clamp(hue(0.0f, 1.2f, 0.5f), to_float3_s(0.0f), to_float3_s(1.0f));
    if(t.x < 0.333f) { return hue(kTwoPi * (0.11f + t.y * 0.03f), sat, light); }
    else if(t.x < 0.666f) { return hue(kTwoPi * (0.92f + t.y * 0.03f), sat, light); }
    return hue(kTwoPi * (0.55f + t.y * 0.03f), sat, light); 
}

float2 heartPath(float t)
{
    float2 dir = (t < 0.5f) ? to_float2(1.0f, -1.0f) : to_float2(-1.0f, -1.0f);
    if(t >= 0.5f) { t = 1.0f - t; }
    t *= 2.0f;
    
    if(t < 0.5f)
    {
        return bezier(to_float2(-1.5266667e-6,-24.039329f), 
                      to_float2(-19.654762f,-56.545271f), 
                      to_float2(-50.316625f,-43.202057f), 
                      to_float2(-50.270832f,-19.881592f), t * 2.0f) * dir;
    }
    else
    {
        return bezier(to_float2(-50.270832f,-19.881592f), 
                      to_float2(-50.270832f + 0.04579f,-19.881592f + 23.3204663f), 
                      to_float2(-50.270832f + 38.50771f,-19.881592f+31.238355f), 
                      to_float2(-50.270832f+50.22497425f,-19.881592f+73.193701f), (t - 0.5f) * 2.0f) * dir;
    }   
}

float3 charge(float4 xi1, float4 xi2, float time)
{           
    float3 p = kZero;
    float t = mod_f(kMetaSpeed * _mix(1.0f - kMetaSpeedVariance, 1.0f, xi2.x) * time + xi2.y, 1.0f);
    float pulse = 1.0f;
    
    if(time < kOpenTransition)
    {
        float delta = smoothstep(0.0f, 1.0f, time / kOpenTransition);   
        pulse = delta;
        
        p.z = _powf(saturate((delta - (xi2.y * 0.8f)) * 5.0f), 0.3f) * kMetaCharge * _mix(1.0f - kMetaChargeVariance, 1.0f, _powf(xi1.y, kMetaChargeExponent));
    }
    else if(kAnimationCycle - time < kCloseTransition)
    {
        float delta = 1.0f - (kAnimationCycle - time) / kCloseTransition;        
        delta = _cosf(kPi + kPi * delta) * 0.5f + 0.5f;
        pulse = 1.0f - delta;
        
        p.y = -_powf(delta, 2.0f + xi1.x) * _mix(1.0f, 2.0f, xi1.y);
        p.z = kMetaCharge * _mix(1.0f - kMetaChargeVariance, 1.0f, _powf(xi1.y, kMetaChargeExponent)) * _mix(1.0f, 0.5f, _powf(delta, 2.0f));  
    }
    else
    {
        p.z = kMetaCharge * _mix(1.0f - kMetaChargeVariance, 1.0f, _powf(xi1.y, kMetaChargeExponent));
    }
    
    float d = kMetaSpread * _mix(1.0f - kMetaSpreadVariance, 1.0f, _powf(xi1.y, kMetaSpreadExponent));    
    if(kMetaPulse) d += kMetaSpread * _mix(0.0f, _fmaxf(0.0f, _sinf(time * 5.0f)), 0.05f * pulse);
    swixy(p) = heartPath(t) * d + to_float2(0.0f, 0.015f) + swixy(p);
 
    return p;
}

void field(in float2 xy, in float time, float3 iResolution, out float F, out float2 delF, out float3 colour, sampler2D sampler)
{      
    pcgInitialise(0);
    
    F = 0.0f;
    delF = to_float2_s(0.0f);
    float denom = 0.0f;
    float sumWeights = 0.0f;
    colour = kZero;
    
    float2 aspectRatio = to_float2(iResolution.x / iResolution.y, 1.0f);
     
    for(int i = 0; i < kNumMetaballs; i++)
    {
        float4 xi1 = rand();        
        float4 xi2 = rand();
        float3 p = charge(xi1, xi2, time);    
        
        F += p.z / length2(xy - swixy(p));
        
        float n = sqr(xy.x - p.x) + sqr(xy.y - p.y);
        delF.x += -p.z * 2.0f * (xy.x - p.x) / sqr(n);
        delF.y += -p.z * 2.0f * (xy.y - p.y) / sqr(n);
        
        denom += p.z / n;
        
        float weight = _powf(_fminf(1e5, 1.0f / length2(xy - swixy(p))), 2.0f);
        
        if(weight > 1e-2)
        {        
            //vec3 h = texelFetch(sampler, ito_float2(i, 0), 0).rgb;
            float3 h = texture(sampler, (to_float2(i, 0)+0.5f)/swixy(iResolution)).rgb;
            
            if(length2(h) == 0.0f)
            {
                h = metaColour(to_float2(xi2.y, xi2.x));
            }
       
            colour += h * weight;
            sumWeights += weight;
        }
    }
    
    colour /= sumWeights;    
    F = 1.0f / _sqrtf(F) - 1.0f / _sqrtf(kMetaThreshold);   
    delF = -0.5f * delF / _powf(denom, 1.5f);
}

float4 renderMetaballs(float2 fragCoord, float3 iResolution, float iTime, sampler2D sampler, sampler2D colourMap)
{                
    float2 xy = to_float2((fragCoord.x - 0.5f * iResolution.x) / iResolution.y, 
                   (fragCoord.y - 0.5f * iResolution.y) / iResolution.y);
                   
    //xy = xy * 0.1f + to_float2(0.27f, 0.27f);
    //xy = xy * 0.1f - to_float2(0.13f, 0.13f);

    float F;
    float2 delF;
    float3 colour;
    field(xy, iTime, iResolution, F, delF, colour, colourMap);   
    
    if(kAnimationCycle - iTime < kCloseTransition)
    {
        float delta = 1.0f - (kAnimationCycle - iTime) / kCloseTransition;        
        colour = _mix(colour, kZero, _saturatef(pow(delta * 2.0f, 2.0f)));
    }

    float delFLength = length(delF);
    float2 delFNorm = delF / ((delFLength > 1.0f) ? delFLength : 1.0f);
            
    delFNorm = delFNorm * 0.5f + to_float2_s(0.5f);    
    
    float FMax = (kMetaAA / iResolution.x) * length(delF);          
    if(F < FMax)
    {        
        float alpha = _fminf(1.0f, (FMax - F) / FMax);
        float z = _powf(_fminf(1.0f, -F * 2.5f), 0.5f);
        float3 normal = normalize(to_float3_aw(delFNorm * (1.0f - z), z));                
                
        float L = _powf(luminance(texture(sampler, swixy(normal), 0.0f).xyz), 2.0f);
        //return to_float4(_mix(colour, kOne, L), _mix(0.05f * alpha, 0.05f + L, clamp(-F, 0.0f, 1.0f))); 
        return to_float4(_mix(colour, kOne, L), _mix(0.05f * alpha, 0.05f + L, clamp(-F / 0.01f, 0.0f, 1.0f))); 
    }   
    else
    {
        return to_float4_s(0.0f);
    }  
}

// Seperable bloom function. This filter requires two passes in the horizontal and vertical directions which are combined as a post-process
// effect after each frame. The accuracy/cost of the effect can be tuned by dialing the kBloomDownsample parameter. 
float4 bloom(float2 fragCoord, float3 iResolution, ivec2 delta, sampler2D renderSampler)
{        
    float2 scaledResolution = to_float2(iResolution.x, iResolution.y) / float((delta.x == 1) ? kBloomDownsample : 1);
   
    if(fragCoord.x > scaledResolution.x || fragCoord.y > scaledResolution.y) { return to_float4_s(0.0f); }
    
    float bloomSize = (delta.x == 1) ? kBloomWidth : kBloomHeight;
    
    int kKernelWidth = int(bloomSize * _fmaxf(iResolution.x, iResolution.y) + 0.5f) / ((delta.x == 1) ? kBloomDownsample : 1);
    float4 sumWeights = to_float4_s(0.0f);
    float4 sumRgb = to_float4_s(0.0f);
    for(int i = -kKernelWidth; i <= kKernelWidth; i++)
    {      
        float2 xy = to_float2(fragCoord.x + float(i * delta.x), fragCoord.y + float(i * delta.y));
        
        if(delta.x == 1) { xy *= float(kBloomDownsample); }
        else { xy /= float(kBloomDownsample); }
        
        if(xy.x < 0.0f || xy.x > iResolution.x || xy.y < 0.0f || xy.y > iResolution.y) { continue; }
            
        float4 rgb = texture(renderSampler, xy / swixy(iResolution));
        float d = float(_fabs(i)) / float(kKernelWidth);
           
        float4 weight = to_float4_s(1.0f);
        if(i != 0)
        {
            // Currently using a single weight although this effect can be done per-channel
            float kernel = _powf(_fmaxf(0.0f, (_expf(-sqr(d * 4.0f)) - 0.0183156f) / 0.981684f), kBloomShape);            
            weight = to_float4_s(1.0f) * kernel;
        }
            
        sumRgb += ((delta.y == 1) ? rgb : _fmaxf(to_float4_s(0.0f), rgb - kBloomBurnIn)) * weight;         
        sumWeights += weight;
    }
    
    sumRgb = sumRgb / sumWeights;
    
    return (delta.x == 1) ? sumRgb : (sumRgb * kBloomTint);
}



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



float vignette(in float2 fragCoord)
{
    float3 uvr = xyToUvr(fragCoord, iResolution);                
    return _mix(1.0f, _fmaxf(0.0f, 1.0f - _powf(uvr.z * kVignetteScale, kVignetteExponent)), kVignetteStrength);
}

__KERNEL__ void Pan_ToneKernel( out float4 fragColor, in float2 fragCoord )
{       

iR = swixy(iResolution);

    float3 rgb = kZero; 
    float4 texel;
    #if kApplyBloom == 1
    {
        rgb += to_float3(bloom(fragCoord, iResolution, ito_float2(0, 1), iChannel1).w * kBloomGain);
        
        if(kDebugBloom)
        {
            fragColor = to_float4_aw(rgb, 1.0f);
            return;
        }
        
        //texel = texelFetch(iChannel0, ito_float2(fragCoord), 0);
        texel = texture(iChannel0, (to_float2(fragCoord)+0.5f)/iR);
    }
    #else
    {
        texel = renderMetaballs(fragCoord, iResolution, iTime / kTimeStretch, iChannel2);
    }
    #endif
        
    float3 bgColour = to_float3_s(0.13f);
    
    if(kApplyVignette) { bgColour *= vignette(fragCoord); }
    
    float alpha = _fminf(0.05f, texel.w) / 0.05f;
    rgb += _mix(bgColour, texel.rgb, alpha);
    
    rgb = clamp(rgb, to_float3_s(0.0f), to_float3_s(1.0f));
    
    
    
    swixyz(fragColor) = rgb;
    fragColor.w = 1.0f;
}


