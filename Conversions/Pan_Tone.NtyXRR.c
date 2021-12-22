
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#if defined (DEVICE_IS_METAL)
#define inout thread
#define out thread
#else
#define inout
#define out
#endif

__DEVICE__ float fract_f(float A){return A - _floor(A);}
__DEVICE__ float mod_f(float value, float divisor) {  return value - divisor * _floor(value / divisor);}


#define swixy(V) to_float2((V).x,(V).y)
#define swizw(V) to_float2((V).z,(V).w)

#define swixyz(V) to_float3((V).x,(V).y,(V).z)


#define kApplyBloom           1
#define kApplyVignette        true            // Apply vignette as a post-process

#define kBloomGain            10.0f             // The strength of the bloom effect 
#define kBloomTint            to_float4_s(1.0f)       // The tint applied to the bloom effect
#define kBloomWidth           0.08f             // The width of the bloom effect as a proportion of the buffer width
#define kBloomHeight          0.08f             // The height of the bloom effect as a proportion of the buffer height
#define kBloomShape           4.0f             // The fall-off of the bloom shape. Higher value = steeper fall-off
#define kBloomDownsample      3              // How much the bloom buffer is downsampled. Higher value = lower quality, but faster
#define kDebugBloom           false           // Show only the bloom the final comp
#define kBloomBurnIn          to_float4_s(0.5f)

#define kVignetteStrength         0.5f             // The strength of the vignette effect
#define kVignetteScale            1.2f             // The scale of the vignette effect
#define kVignetteExponent         2.0f             // The rate of attenuation of the vignette effect

#define kAA 1

#define kMetaSpeed            0.07f
#define kMetaSpeedVariance    0.5f
#define kMetaSpread           0.008f
#define kMetaAA               3.0f
#define kMetaColourPhase      0.0003f
#define kMetaThreshold        8.0f
#define kMetaDiffusion        0.01f
#define kMetaPulse            true

#define kResolution           0
#if kResolution == 0
    #define kNumMetaballs         250
    #define kMetaCharge           0.0018f
    #define kMetaChargeVariance   0.8f
    #define kMetaChargeExponent   2.0f
    #define kMetaSpreadVariance   0.6f
    #define kMetaSpreadExponent   0.3f
#else
    #define kNumMetaballs         500
    #define kMetaCharge           0.0013f
    #define kMetaChargeVariance   0.9f
    #define kMetaChargeExponent   3.0f
    #define kMetaSpreadVariance   0.8f
    #define kMetaSpreadExponent   0.3f
#endif

#define kAnimationCycle       20.0f
#define kOpenTransition       3.0f
#define kCloseTransition      2.0f
#define kTimeStretch          1.0f
#define kTimeDelay            0.0f
#define kTimeStartAt          0.0f

#define kPi                    3.14159265359f
#define kTwoPi                 (2.0f * kPi)
#define kHalfPi                (0.5f * kPi)
#define kRoot2                 1.41421356237f
#define kFltMax                3.402823466ef+38
#define kIntMax                0x7fffffff
#define kOne                   make_float3(1.0f)
#define kZero                  make_float3(0.0f)
#define kPink                  make_float3(1.0f, 0.0f, 0.2f)
__DEVICE__ float toRad(float deg)         { return kTwoPi * deg / 360.0f; }
__DEVICE__ float toDeg(float rad)         { return 360.0f * rad / kTwoPi; }
__DEVICE__ float sqr(float a)             { return a * a; }
__DEVICE__ int sqr(int a)                 { return a * a; }
__DEVICE__ int mod2(int a, int b)         { return ((a % b) + b) % b; }
__DEVICE__ float length2(float2 v)        { return dot(v, v); }
__DEVICE__ float length3(float3 v)        { return dot(v, v); }
__DEVICE__ int sum(int2 a)                { return a.x + a.y; }
__DEVICE__ float luminance(float3 v)      { return v.x * 0.17691f + v.y * 0.8124f + v.z * 0.01063f; }
__DEVICE__ float mean(float3 v)           { return v.x / 3.0f + v.y / 3.0f + v.z / 3.0f; }
//__DEVICE__ float4 mul4(float3 a, mat4 m)  { return f4_multi_mat4(make_float4(a, 1.0f) , m); }
//__DEVICE__ float3 mul3(float3 a, mat4 m)  { return swixyz((f4_multi_mat4(to_float4_aw(a, 1.0f) , m))); }
__DEVICE__ float _saturatef(float a)      { return clamp(a, 0.0f, 1.0f); }
__DEVICE__ float sin01(float a)           { return 0.5f * _sinf(a) + 0.5f; }
__DEVICE__ float cos01(float a)           { return 0.5f * _cosf(a) + 0.5f; }

//int2 kFragCoord;
//uint4 rngSeed; 

// Maps the swixy(input) texel coordinates to UV [0.0f, 1.0f] and distance R from center
__DEVICE__ float2 xyToUv(float2 xy, float2 iR)
{
    float2 uv = make_float2(xy.x / iR.x, xy.y / iR.y);
    uv.x = (uv.x - 0.5f) * (iR.x / iR.y) + 0.5f;     
    return uv;
}

// Maps the swixy(input) texel coordinates to UV [-1.0f, 1.0f] and distance R from center
__DEVICE__ float3 xyToUvr(float2 xy, float2 iR)
{
    float2 uv = xyToUv(xy, iR);    
    float x = 2.0f * (uv.x - 0.5f);
    float y = 2.0f * (uv.y - 0.5f);
    
    return make_float3(uv, _sqrtf(x*x + y*y) / kRoot2);
}

// Permuted congruential generator from "Hash Functions for GPU Rendering" (swizy(Ja)nski and Olano)
// http://jcgt.org/published/0009/03/02/paper.pdf
__DEVICE__ uint4 pcgAdvance(uint4 _rngSeed)
{
    uint4 rngSeed = _rngSeed * 1664525u + 1013904223u;
    
    rngSeed.x += rngSeed.y*rngSeed.w; 
    rngSeed.y += rngSeed.z*rngSeed.x; 
    rngSeed.z += rngSeed.x*rngSeed.y; 
    rngSeed.w += rngSeed.y*rngSeed.z;

    //rngSeed ^= make_uint4(rngSeed.x >> 16u,rngSeed.y >> 16u,rngSeed.z >> 16u,rngSeed.w >> 16u);
    rngSeed.x ^= rngSeed.x >> 16u;
    rngSeed.y ^= rngSeed.y >> 16u;
    rngSeed.z ^= rngSeed.z >> 16u;
    rngSeed.w ^= rngSeed.w >> 16u;

    
    rngSeed.x += rngSeed.y*rngSeed.w; 
    rngSeed.y += rngSeed.z*rngSeed.x; 
    rngSeed.z += rngSeed.x*rngSeed.y; 
    rngSeed.w += rngSeed.y*rngSeed.z;
    
    return rngSeed;
}

// Seed the PCG hash function with the current frame multipled by a prime
__DEVICE__ void pcgInitialise(uint4 *rngSeed, int frame)
{    
    *rngSeed = make_uint4(20219u, 7243u, 12547u, 28573u) * (uint)(frame);
}

// Generates a tuple of canonical random number and uses them to sample an input texture
__DEVICE__ float4 rand1(__TEXTURE2D__ sampler,int2 kFragCoord, float2 iR, uint4 *rngSeed )
{

    *rngSeed = pcgAdvance(*rngSeed);
    return _tex2DVecN(sampler, ((float)((kFragCoord.x + ((*rngSeed).x >> 16)) % 1024)+0.5f)/iR.x,(kFragCoord.y + ((*rngSeed).y >> 16)) % 1024, 15);
}

// Generates a tuple of canonical random numbers the range [0, 1]
__DEVICE__ float4 rand2(uint4 *rngSeed)
{
    *rngSeed = pcgAdvance(*rngSeed);
    return make_float4(*rngSeed) / (float)(0xffffffffu);
}

__DEVICE__ float3 hue(float phi)
{
    float phiColour = 6.0f * phi / kTwoPi;
    int i = int(phiColour);
    float3 c0 = make_float3(((i + 4) / 3) & 1, ((i + 2) / 3) & 1, ((i + 0) / 3) & 1);
    float3 c1 = make_float3(((i + 5) / 3) & 1, ((i + 3) / 3) & 1, ((i + 1) / 3) & 1);             
    return clamp(_mix(c0, c1, phiColour - (float)(i)), kZero, kOne);
}

__DEVICE__ float3 hue(float phi, float saturation, float brightness)
{
    float3 rgb = hue(phi);

    rgb = _mix(to_float3_s(0.5f), rgb, saturation);
    
    return (brightness < 0.5f) ? _mix(kZero, rgb, brightness * 2.0f) :
                                 _mix(rgb, kOne, (brightness - 0.5f) * 2.0f);
}

__DEVICE__ float2 bezier(float2 u0, float2 u1, float2 u2, float2 u3, float t)
{
    float2 v0 = _mix(u0, u1, t);
    float2 v1 = _mix(u1, u2, t);
    float2 v2 = _mix(u2, u3, t);
    float2 w0 = _mix(v0, v1, t);
    float2 w1 = _mix(v1, v2, t);
    
    return _mix(w0, w1, t);    
}

__DEVICE__ float3 metaColour(float2 t)
{
    #define sat 1.3f
    #define light 0.5f
    
    //t.x = fract_f(t.x * 10.0f);
    
    t.x = fract_f(t.x + 0.01f);    
    
    //h = (fract_f(t * 2.0f) < 0.5f) ? hue(0.0f, 0.0f, 0.9f) : clamp(hue(0.0f, 1.2f, 0.5f), make_float3(0.0f), make_float3(1.0f));
    if(t.x < 0.333f) { return hue(kTwoPi * (0.11f + t.y * 0.03f), sat, light); }
    else if(t.x < 0.666f) { return hue(kTwoPi * (0.92f + t.y * 0.03f), sat, light); }
    return hue(kTwoPi * (0.55f + t.y * 0.03f), sat, light); 
}

__DEVICE__ float2 heartPath(float t)
{
    float2 dir = (t < 0.5f) ? make_float2(1.0f, -1.0f) : make_float2(-1.0f, -1.0f);
    if(t >= 0.5f) { t = 1.0f - t; }
    t *= 2.0f;
    
    if(t < 0.5f)
    {
        return bezier(make_float2(-1.5266667e-6,-24.039329f), 
                      make_float2(-19.654762f,-56.545271f), 
                      make_float2(-50.316625f,-43.202057f), 
                      make_float2(-50.270832f,-19.881592f), t * 2.0f) * dir;
    }
    else
    {
        return bezier(make_float2(-50.270832f,-19.881592f), 
                      make_float2(-50.270832f + 0.04579f,-19.881592f + 23.3204663f), 
                      make_float2(-50.270832f + 38.50771f,-19.881592f+31.238355f), 
                      make_float2(-50.270832f+50.22497425f,-19.881592f+73.193701f), (t - 0.5f) * 2.0f) * dir;
    }   
}

__DEVICE__ float3 charge(float4 xi1, float4 xi2, float time)
{           
    float3 p = kZero;
    float t = mod_f(kMetaSpeed * _mix(1.0f - kMetaSpeedVariance, 1.0f, xi2.x) * time + xi2.y, 1.0f);
    float pulse = 1.0f;
    
    if(time < kOpenTransition)
    {
        float delta = smoothstep(0.0f, 1.0f, time / kOpenTransition);   
        pulse = delta;

        p.z = _powf(_saturatef((delta - (xi2.y * 0.8f)) * 5.0f), 0.3f) * kMetaCharge * _mix(1.0f - kMetaChargeVariance, 1.0f, _powf(xi1.y, kMetaChargeExponent));
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
    float2 pxy = heartPath(t) * d + make_float2(0.0f, 0.015f) + swixy(p);
    p.x=pxy.x;p.y=pxy.y;
 
    return p;
}

__DEVICE__ void field(float2 xy, float time, float2 iR, out float *F, out float2 *delF, out float3 *colour, __TEXTURE2D__ sampler, uint4 *rngSeed, int frame)
{      
    pcgInitialise(rngSeed, frame);
    
    *F = 0.0f;
    *delF = to_float2_s(0.0f);
    float denom = 0.0f;
    float sumWeights = 0.0f;
    *colour = kZero;
    
    float2 aspectRatio = make_float2(iR.x / iR.y, 1.0f);
     
    for(int i = 0; i < kNumMetaballs; i++)
    {
        float4 xi1 = rand2(rngSeed);        
        float4 xi2 = rand2(rngSeed);
        float3 p = charge(xi1, xi2, time);    
        
        *F += p.z / length2(xy - swixy(p));

        float n = sqr(xy.x - p.x) + sqr(xy.y - p.y);
        (*delF).x += -p.z * 2.0f * (xy.x - p.x) / sqr(n);
        (*delF).y += -p.z * 2.0f * (xy.y - p.y) / sqr(n);
        
        denom += p.z / n;
        
        float weight = _powf(_fminf(1e5, 1.0f / length2(xy - swixy(p))), 2.0f);
        
        if(weight > 1e-2)
        {        
            //float3 h = swixyz(texelFetch(sampler, to_int2(i, 0), 0));
		float3 h = swixyz(_tex2DVecN(sampler, ((float)i+0.5f)/iR.x,((float)0+0.5)/iR.y, 15));
            
            if(length3(h) == 0.0f)
            {
                h = metaColour(make_float2(xi2.y, xi2.x));
            }
       
            *colour += h * weight;
            sumWeights += weight;
        }
    }
    
    *colour /= sumWeights;    
    *F = 1.0f / _sqrtf(*F) - 1.0f / _sqrtf(kMetaThreshold);   
    *delF = -0.5f * *delF / _powf(denom, 1.5f);
}

__DEVICE__ float4 renderMetaballs(float2 fragCoord, float2 iR, float itime, __TEXTURE2D__ sampler, __TEXTURE2D__ colourMap, uint4 *rngSeed, int frame)
{                

    float2 xy = make_float2((fragCoord.x - 0.5f * iR.x) / iR.y, 
                            (fragCoord.y - 0.5f * iR.y) / iR.y);
                   
    //xy = xy * 0.1f + make_float2(0.27f, 0.27f);
    //xy = xy * 0.1f - make_float2(0.13f, 0.13f);

    float F;
    float2 delF;
    float3 colour;
    field(xy, itime, iR, &F, &delF, &colour, colourMap,rngSeed, frame);   
    
    if(kAnimationCycle - itime < kCloseTransition)
    {
        float delta = 1.0f - (kAnimationCycle - itime) / kCloseTransition;        
        colour = _mix(colour, kZero, _saturatef(_powf(delta * 2.0f, 2.0f)));
    }

    float delFLength = length(delF);
    float2 delFNorm = delF / ((delFLength > 1.0f) ? delFLength : 1.0f);
            
    delFNorm = delFNorm * 0.5f + to_float2_s(0.5f);    
    
    float FMax = (kMetaAA / iR.x) * length(delF);          
    if(F < FMax)
    {        
        float alpha = _fminf(1.0f, (FMax - F) / FMax);
        float z = _powf(_fminf(1.0f, -F * 2.5f), 0.5f);
        float3 normal = normalize(to_float3_aw(delFNorm * (1.0f - z), z));                
                
        float L = _powf(luminance(swixyz(_tex2DVecN(sampler, normal.x,normal.y, 15))), 2.0f);
        //return make_float4(_mix(colour, kOne, L), _mix(0.05f * alpha, 0.05f + L, clamp(-F, 0.0f, 1.0f))); 
        return to_float4_aw(_mix(colour, kOne, L), _mix(0.05f * alpha, 0.05f + L, clamp(-F / 0.01f, 0.0f, 1.0f))); 
    }   
    else
    {
        return to_float4_s(0.0f);
    }  
}

// Seperable bloom function. This filter requires two passes the horizontal and vertical directions which are combined as a post-process
// effect after each frame. The accuracy/cost of the effect can be tuned by dialing the kBloomDownsample parameter. 
__DEVICE__ float4 bloom(float2 fragCoord, float2 iR, int2 delta, __TEXTURE2D__ renderSampler)
{        
    float2 scaledResolution = make_float2(iR.x, iR.y) / (float)((delta.x == 1) ? kBloomDownsample : 1);
   
    if(fragCoord.x > scaledResolution.x || fragCoord.y > scaledResolution.y) { return to_float4_s(0.0f); }
    
    float bloomSize = (delta.x == 1) ? kBloomWidth : kBloomHeight;
    
    int kKernelWidth = (int)(bloomSize * _fmaxf(iR.x, iR.y) + 0.5f) / ((delta.x == 1) ? kBloomDownsample : 1);
    float4 sumWeights = to_float4_s(0.0f);
    float4 sumRgb = to_float4_s(0.0f);
    for(int i = -kKernelWidth; i <= kKernelWidth; i++)
    {      
        float2 xy = make_float2(fragCoord.x + (float)(i * delta.x), fragCoord.y + (float)(i * delta.y));
        
        if(delta.x == 1) { xy *= (float)(kBloomDownsample); }
        else { xy /= (float)(kBloomDownsample); }
        
        if (xy.x < 0.0f || xy.x > iR.x || xy.y < 0.0f || xy.y > iR.y) { continue; }
            
        float4 rgb = _tex2DVecN(renderSampler, xy.x / iR.x, xy.y / iR.y,15);
        float d = (float)(_fabs(i)) / (float)(kKernelWidth);
           
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
// - Buffer A   - iChannel0 Texture                                                                    -
// ----------------------------------------------------------------------------------


__KERNEL__ void Pan_ToneKernelA(              
__CONSTANTREF__ Params*  params,     
__TEXTURE2D__            iChannel0,  
__TEXTURE2D_WRITE__      dst         
 ){
    PROLOGUE;
   
    uint4 rngSeed; 
    
    __TEXTURE2D__ iChannel1 = iChannel0;
  
    float iframe = iTime*30.0f;

    int2 kFragCoord = to_int2_cfloat(fragCoord);
    
    float time = mod_f(_fmaxf(0.0f, iTime - kTimeDelay) / kTimeStretch, kAnimationCycle);
    if(time > 0.0f) { time += kTimeStartAt; }
    
    #if kApplyBloom == 1   
    {
        fragColor = renderMetaballs(fragCoord, iResolution, time, iChannel0, iChannel1, &rngSeed, iframe);
    }
    #endif
    
    _tex2DVec4Write(dst, x, y, fragColor);
  	
}


// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------

__KERNEL__ void Pan_ToneKernelB(              
__CONSTANTREF__ Params*  params,     
__TEXTURE2D__            iChannel0,  
__TEXTURE2D_WRITE__      dst         
 ){
    PROLOGUE;

    #if kApplyBloom == 1   
    {    
        fragColor = bloom(fragCoord, iResolution, to_int2(1, 0), iChannel0);
    }
    #endif
    
    //fragColor = _tex2DVecN(iChannel0,fragCoord.x/iResolution.x,fragCoord.y/iResolution.y,15);
//jetzt muss ein Fehler kommen     - okay kam !
    //EPILOGUE(fragColor);
    _tex2DVec4Write(dst, x, y, fragColor);
}


// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------

__KERNEL__ void Pan_ToneKernelC(              
__CONSTANTREF__ Params*  params,     
__TEXTURE2D__            iChannel0,  
__TEXTURE2D_WRITE__      dst         
 ){
    PROLOGUE;
  
    float2 iR = iResolution;
  
    uint4 rngSeed;
  
    int iFrame = (int)(iTime*30.0f); //notlösung
   
    int2 xy = to_int2_cfloat(fragCoord);
    
    float time = mod_f(_fmaxf(0.0f, iTime - kTimeDelay) / kTimeStretch, kAnimationCycle);
    
    if(xy.x == 0 && xy.y == 2) { fragColor = to_float4_s(time); }
    //float lastTime = texelFetch(iChannel0, ito_float2(0, 2), 0).x;
    float lastTime = _tex2DVecN(iChannel0, ((0)+0.5f)/iR.x,((2)+0.5f)/iR.y,15).x;
    
    if(xy.x >= kNumMetaballs || xy.y >= 2) { EPILOGUE(fragColor);return; }
    
    int y0 = iFrame % 2;
    int y1 = (iFrame + 1) % 2;
    
    //vec4 texel = texelFetch(iChannel0, xy, 0);
    float4 texel = _tex2DVecN(iChannel0, ((float)(xy.x)+0.5f)/iR.x,((float)(xy.y)+0.5f)/iR.y,15);

    if(xy.y == y1) 
    { 
        fragColor = texel; 
        EPILOGUE(fragColor);
        return; 
    }
        
    float2 aspectRatio = to_float2(iResolution.x / iResolution.y, 1.0f);
    
    pcgInitialise(&rngSeed, iFrame);
    
    float4 p[kNumMetaballs]; 
    for(int i = 0; i < kNumMetaballs; i++)
    {
        float4 xi1 = rand2(&rngSeed);        
        float4 xi2 = rand2(&rngSeed);
        float3 v = charge(xi1, xi2, time);  
        
        p[i] = to_float4(v.x, v.y, xi2.y, xi2.x);        
    }
    
    if(time < 1.0f || length3(swixyz(texel)) == 0.0f)
    {
        fragColor = to_float4_aw(metaColour(swizw(p[xy.x])), 0.0f);
        EPILOGUE(fragColor);
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
        float weight = _powf(_fmaxf(0.0f, 1.0f - length(swixy(p[xy.x]) - swixy(p[i])) / kMaxDist), 1.0f); 

        //colour += texelFetch(iChannel0, ito_float2(i, y1), 0).rgb * weight;
        colour += swixyz(_tex2DVecN(iChannel0, ((float)(i)+0.5f)/iR.x,((float)(y1)+0.5f)/iR.y,15)) * weight;
        
        sumWeights += weight;
    }
    
    colour /= sumWeights;
    //vec3 thisColour = texelFetch(iChannel0, ito_float2(xy.x, y1), 0).rgb;   
    float3 thisColour = swixyz(_tex2DVecN(iChannel0, ((float)(xy.x)+0.5f)/iR.x,((float)(y1)+0.5f)/iR.y,15));  
    
    //thisColour = _mix(thisColour, kOne * 0.7f, 0.001f);
    fragColor = to_float4_aw(_mix(thisColour, colour, kMetaDiffusion / kTimeStretch), fragColor.w);
    
    EPILOGUE(fragColor);
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
#define in

__DEVICE__ float vignette(in float2 fragCoord, float2 iR)
{
    float3 uvr = xyToUvr(fragCoord, iR);                
    return _mix(1.0f, _fmaxf(0.0f, 1.0f - _powf(uvr.z * kVignetteScale, kVignetteExponent)), kVignetteStrength);
}

__KERNEL__ void Pan_ToneKernel(              
__CONSTANTREF__ Params*  params,     
__TEXTURE2D__            iChannel0,  
__TEXTURE2D_WRITE__      dst         
 ){
    PROLOGUE;     

    __TEXTURE2D__ iChannel1 = iChannel0;

    float3 rgb = kZero; 
    float4 texel;
    #if kApplyBloom == 1
    {
        rgb += to_float3_s(bloom(fragCoord, iResolution, to_int2(0, 1), iChannel1).w * kBloomGain);
                
        if(kDebugBloom)
        {
            fragColor = to_float4_aw(rgb, 1.0f);
            EPILOGUE(fragColor);
            return;
        }
        
        //texel = texelFetch(iChannel0, ito_float2(fragCoord), 0);
        texel = _tex2DVecN(iChannel0, (fragCoord.x+0.5f)/iResolution.x,(fragCoord.y+0.5f)/iResolution.y,15);
    }
    #else
    {
        texel = renderMetaballs(fragCoord, iResolution, iTime / kTimeStretch, iChannel2);
    }
    #endif
        
    float3 bgColour = to_float3_s(0.13f);
    
    if(kApplyVignette) { bgColour *= vignette(fragCoord, iResolution); }
    
    float alpha = _fminf(0.05f, texel.w) / 0.05f;
    rgb += _mix(bgColour, swixyz(texel), alpha);
    
    rgb = clamp(rgb, to_float3_s(0.0f), to_float3_s(1.0f));
        
    fragColor = to_float4_aw(rgb,1.0f);
        
    EPILOGUE(fragColor);
}
