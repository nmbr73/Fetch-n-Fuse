
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define pi2_inv 0.159154943091895335768883763372f

__DEVICE__ float2 complex_mul(float2 factorA, float2 factorB){
    return to_float2( factorA.x*factorB.x - factorA.y*factorB.y, factorA.x*factorB.y + factorA.y*factorB.x);
}

__DEVICE__ float2 spiralzoom(float2 domain, float2 center, float n, float spiral_factor, float zoom_factor, float2 pos){
    float2 uv = domain - center;
    float d = length(uv);
    return to_float2( _atan2f(uv.y, uv.x)*n*pi2_inv + d*spiral_factor, -_logf(d)*zoom_factor) + pos;
}

__DEVICE__ float2 complex_div(float2 numerator, float2 denominator){
    return to_float2( numerator.x*denominator.x + numerator.y*denominator.y,
                      numerator.y*denominator.x - numerator.x*denominator.y)/
           to_float2_s(denominator.x*denominator.x + denominator.y*denominator.y);
}

__DEVICE__ float circle(float2 uv, float2 aspect, float scale){
    return clamp( 1.0f - length((uv-0.5f)*aspect*scale), 0.0f, 1.0f);
}

__DEVICE__ float sigmoid(float x) {
    return 2.0f/(1.0f + _exp2f(-x)) - 1.0f;
}

__DEVICE__ float smoothcircle(float2 uv, float2 aspect, float radius, float ramp){
    return 0.5f - sigmoid( ( length( (uv - 0.5f) * aspect) - radius) * ramp) * 0.5f;
}

__DEVICE__ float conetip(float2 uv, float2 pos, float size, float min, float2 iResolution)
{
    float2 aspect = to_float2(1.0f,iResolution.y/iResolution.x);
    return _fmaxf( min, 1.0f - length((uv - pos) * aspect / size) );
}

__DEVICE__ float warpFilter(float2 uv, float2 pos, float size, float ramp, float2 iResolution)
{
    return 0.5f + sigmoid( conetip(uv, pos, size, -16.0f,iResolution) * ramp) * 0.5f;
}

__DEVICE__ float2 vortex_warp(float2 uv, float2 pos, float size, float ramp, float2 rot, float2 iResolution)
{
    float2 aspect = to_float2(1.0f,iResolution.y/iResolution.x);

    float2 pos_correct = 0.5f + (pos - 0.5f);
    float2 rot_uv = pos_correct + complex_mul((uv - pos_correct)*aspect, rot)/aspect;
    float _filter = warpFilter(uv, pos_correct, size, ramp,iResolution);
    return _mix(uv, rot_uv, _filter);
}

__DEVICE__ float2 vortex_pair_warp(float2 uv, float2 pos, float2 vel, float2 iResolution)
{
    float2 aspect = to_float2(1.0f,iResolution.y/iResolution.x);
    float ramp = 8.0f;

    float d = 0.1f;

    float l = length(vel);
    float2 p1 = pos;
    float2 p2 = pos;

    if(l > 0.0f){
        float2 normal = normalize(swi2(vel,y,x) * to_float2(-1.0f, 1.0f))/aspect;
        p1 = pos - normal * d / 2.0f;
        p2 = pos + normal * d / 2.0f;
    }

    float w = l / d * 2.0f;

    // two overlapping rotations that would annihilate when they were not displaced.
    float2 circle1 = vortex_warp(uv, p1, d, ramp, to_float2(_cosf(w),_sinf(w)),iResolution);
    float2 circle2 = vortex_warp(uv, p2, d, ramp, to_float2(_cosf(-w),_sinf(-w)),iResolution);
    return (circle1 + circle2) / 2.0f;
}

__DEVICE__ float2 mouseDelta(float4 iMouse, float2 iResolution, __TEXTURE2D__ iChannel2){
    float2 pixelSize = 1.0f / iResolution;
    float eighth = 1.0f/8.0f;
    float4 oldMouse = texture(iChannel2, to_float2(7.5f * eighth, 2.5f * eighth));
    float4 nowMouse = to_float4_f2f2(swi2(iMouse,x,y) * swi2(pixelSize,x,y), swi2(iMouse,z,w) * swi2(pixelSize,x,y));
    if(oldMouse.z > pixelSize.x && oldMouse.w > pixelSize.y && 
       nowMouse.z > pixelSize.x && nowMouse.w > pixelSize.y)
    {
        return swi2(nowMouse,x,y) - swi2(oldMouse,x,y);
    }
    return to_float2_s(0.0f);
}

// below code is forked from https://www.shadertoy.com/view/MlKXDw
#define sample(x,y) texture(iChannel0, (uv + to_float2(x,y) / iResolution))

#define FEED 0.0367f;
#define KILL 0.0649f;



__KERNEL__ void ReactiondiffusiondualvortexJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{

    fragCoord+=0.5f;

    float zoom = 0.9997f;
    float2 difRate = to_float2(1.0f,0.25f);

    float2 uv = fragCoord / iResolution;
    float2 texel = 1.0f / iResolution;

// begin Felix' edit to the original by Cornus Ammonis
    float2 mouseV = mouseDelta(iMouse,iResolution,iChannel2);
    float2 aspect = to_float2(1.0f,texel.x/texel.y);
    if(length(mouseV)==0.0f && iFrame > 4*1024){
        fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    uv = vortex_pair_warp(uv, swi2(iMouse,x,y)*texel, mouseV*aspect*1.0f,iResolution);
// end      
    
    uv =( uv - to_float2_s(0.5f)) * zoom + to_float2_s(0.5f);
    float4 current = sample(0.0f,0.0f);
    
    float4 cumul = current * -1.0f;
    
    cumul += (   sample( 1.0f, 0.0f) 
               + sample(-1.0f, 0.0f) 
               + sample( 0.0f, 1.0f) 
               + sample( 0.0f,-1.0f)
             ) * 0.2f;

    cumul += (
        sample( 1, 1) +
        sample( 1,-1) +
        sample(-1, 1) +
        sample(-1,-1) 
       )*0.05f;
    
    
    float feed = FEED;
    float kill = KILL;
    
    float dist = distance_f2(uv,to_float2_s(0.5f)) - 0.34f;
    kill = kill + step(0.0f,dist) * dist*0.25f;
    
    float4 lap =  cumul;
    float newR = current.x + (difRate.x * lap.x - current.x * current.y * current.y + feed * (1.0f - current.x));
    float newG = current.y + (difRate.y * lap.y + current.x * current.y * current.y - (kill + feed) * current.y);
    
    newR = clamp(newR,0.0f,1.0f);
    newG = clamp(newG,0.0f,1.0f);
    
    current = to_float4(newR,newG,0.0f,1.0f);
    
    
      uv = (fragCoord / iResolution.y) -  to_float2(iResolution.x /iResolution.y * 0.5f,0.5f);
      float f = step(length(uv),0.25f) - step(length(uv),0.24f);
      f *=  0.25f + fract(_atan2f(uv.y,uv.x)*0.5f + iTime*0.5f) * 0.25f * _sinf(iTime*0.1f);
      current = _fmaxf(current, to_float4(0.0f,1.0f,0.0f,1.0f) * f);
    
    if(iMouse.z > 0.5f)
    {
        uv = (fragCoord - swi2(iMouse,x,y)) / iResolution;
//        current = _fmaxf(current,to_float4_s(1.0f) * step(dot(uv,uv),0.001225f));
    }
    
    fragColor = current;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------


__KERNEL__ void ReactiondiffusiondualvortexJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse)
{

    float2 uv = fragCoord / iResolution;
    float2 pixelSize = 1.0f / iResolution;
    float eighth = 1.0f/8.0f;
    if(uv.x > 7.0f*eighth && uv.x < 8.0f*eighth && uv.y > 2.0f*eighth && uv.y < 3.0f*eighth)
    {
        fragColor = to_float4_f2f2(swi2(iMouse,x,y) * swi2(pixelSize,x,y), swi2(iMouse,z,w) * swi2(pixelSize,x,y));
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define _Smooth(p,r,s) smoothstep(-s, s, p-(r))

__KERNEL__ void ReactiondiffusiondualvortexJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  float2 uv = fragCoord / iResolution;
  float4 state = _tex2DVecN(iChannel0,uv.x,uv.y,15);
  fragColor =  to_float4(0.0f,state.y,state.y/state.x,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}