
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// main reaction-diffusion loop

// actually the diffusion is realized as a separated two-pass Gaussian blur kernel and is stored in buffer C

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
    return 0.5f + sigmoid( conetip(uv, pos, size, -16.0f, iResolution) * ramp) * 0.5f;
}

__DEVICE__ float2 vortex_warp(float2 uv, float2 pos, float size, float ramp, float2 rot, float2 iResolution)
{
    float2 aspect = to_float2(1.0f,iResolution.y/iResolution.x);

    float2 pos_correct = 0.5f + (pos - 0.5f);
    float2 rot_uv = pos_correct + complex_mul((uv - pos_correct)*aspect, rot)/aspect;
    float _filter = warpFilter(uv, pos_correct, size, ramp, iResolution);
    return _mix(uv, rot_uv, _filter);
}

__DEVICE__ float2 vortex_pair_warp(float2 uv, float2 pos, float2 vel, float2 iResolution)
{
    float2 aspect = to_float2(1.0f,iResolution.y/iResolution.x);
    float ramp = 5.0f;

    float d = 0.2f;

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
    float2 circle1 = vortex_warp(uv, p1, d, ramp, to_float2(_cosf(w),_sinf(w)), iResolution);
    float2 circle2 = vortex_warp(uv, p2, d, ramp, to_float2(_cosf(-w),_sinf(-w)), iResolution);
    return (circle1 + circle2) / 2.0f;
}

__DEVICE__ float2 mouseDelta(float2 iResolution, float4 iMouse, __TEXTURE2D__ iChannel2){
    float2 pixelSize = 1.0f / iResolution;
    float eighth = 1.0f/8.0f;
    float4 oldMouse = texture(iChannel2, to_float2(7.5f * eighth, 2.5f * eighth));
    float4 nowMouse = to_float4_f2f2(swi2(iMouse,x,y) / iResolution, swi2(iMouse,z,w) / iResolution);
    if(oldMouse.z > pixelSize.x && oldMouse.w > pixelSize.y && 
       nowMouse.z > pixelSize.x && nowMouse.w > pixelSize.y)
    {
        return swi2(nowMouse,x,y) - swi2(oldMouse,x,y);
    }
    return to_float2_s(0.0f);
}

__KERNEL__ void ExpansivereactionDiffusionJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float2 pixelSize = 1.0f / iResolution;
    

    float2 mouseV = mouseDelta(iResolution, iMouse, iChannel2);
    float2 aspect = to_float2(1.0f,iResolution.y/iResolution.x);
    uv = vortex_pair_warp(uv, swi2(iMouse,x,y)*pixelSize, mouseV*aspect*1.4f, iResolution);

    float4 blur1 = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    
    float4 noise = texture(iChannel3, fragCoord / iResolution + fract(to_float2(42,56)*iTime));

    // get the gradients from the blurred image
    float2 d = pixelSize*4.0f;
    float4 dx = (texture(iChannel1, fract(uv + to_float2(1,0)*d)) - texture(iChannel1, fract(uv - to_float2(1,0)*d))) * 0.5f;
    float4 dy = (texture(iChannel1, fract(uv + to_float2(0,1)*d)) - texture(iChannel1, fract(uv - to_float2(0,1)*d))) * 0.5f;
    
    float2 uv_red = uv + to_float2(dx.x, dy.x)*pixelSize*8.0f; // add some diffusive expansion
    
    float new_red = texture(iChannel0, fract(uv_red)).x + (noise.x - 0.5f) * 0.0025f - 0.002f; // stochastic decay
        
    new_red -= (texture(iChannel1, fract_f2(uv_red + (swi2(noise,x,y)-0.5f)*pixelSize)) -
                texture(iChannel0, fract_f2(uv_red + (swi2(noise,x,y)-0.5f)*pixelSize))).x * 0.047f; // reaction-diffusion
        
    if(iFrame<10 || Reset)
    {
        fragColor = noise; 
    }
    else
    {
        fragColor.x = clamp(new_red, 0.0f, 1.0f);
    }

//    fragColor = noise; // need a restart?

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// horizontal Gaussian blur pass

__KERNEL__ void ExpansivereactionDiffusionJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
  fragCoord+=0.5f;

  float2 pixelSize = 1.0f/ iResolution;
  float2 uv = fragCoord * pixelSize;
  
  float h = pixelSize.x;
  float4 sum = to_float4_s(0.0f);
  sum += texture(iChannel0, fract_f2(to_float2(uv.x - 4.0f*h, uv.y)) ) * 0.05f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x - 3.0f*h, uv.y)) ) * 0.09f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x - 2.0f*h, uv.y)) ) * 0.12f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x - 1.0f*h, uv.y)) ) * 0.15f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 0.0f*h, uv.y)) ) * 0.16f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 1.0f*h, uv.y)) ) * 0.15f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 2.0f*h, uv.y)) ) * 0.12f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 3.0f*h, uv.y)) ) * 0.09f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 4.0f*h, uv.y)) ) * 0.05f;
    
  fragColor = to_float4_aw(swi3(sum,x,y,z)/0.98f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// vertical Gaussian blur pass

__KERNEL__ void ExpansivereactionDiffusionJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
  fragCoord+=0.5f;

  float2 pixelSize = 1.0f/ iResolution;
  float2 uv = fragCoord * pixelSize;

  float v = pixelSize.y;
  float4 sum = to_float4_s(0.0f);
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y - 4.0f*v)) ) * 0.05f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y - 3.0f*v)) ) * 0.09f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y - 2.0f*v)) ) * 0.12f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y - 1.0f*v)) ) * 0.15f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 0.0f*v)) ) * 0.16f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 1.0f*v)) ) * 0.15f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 2.0f*v)) ) * 0.12f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 3.0f*v)) ) * 0.09f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 4.0f*v)) ) * 0.05f;
    
  fragColor = to_float4_aw(swi3(sum,x,y,z)/0.98f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0


// not used (yet), but hooray for 8 channel feedback

__KERNEL__ void ExpansivereactionDiffusionJipiFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse)
{

  fragCoord+=0.5f;

  float2 uv = fragCoord / iResolution;
  float2 pixelSize = 1.0f / iResolution;
  float eighth = 1.0f/8.0f;
  if(uv.x > 7.0f*eighth && uv.x < 8.0f*eighth && uv.y > 2.0f*eighth && uv.y < 3.0f*eighth)
  {
    fragColor = to_float4_f2f2(swi2(iMouse,x,y) / iResolution, swi2(iMouse,z,w) / iResolution);
  }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void ExpansivereactionDiffusionJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel2, sampler2D iChannel3)
{

  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.0f);


  float2 uv = fragCoord / iResolution;
  float2 pixelSize = 1.0f / iResolution;
  float2 aspect = to_float2(1.0f,iResolution.y/iResolution.x);

  float4 noise = texture(iChannel3, fragCoord / iResolution + fract_f2(to_float2(42,56)*iTime));

  float2 lightSize=to_float2_s(4.0f);

    // get the gradients from the blurred image
  float2 d = pixelSize*2.0f;
  float4 dx = (texture(iChannel2, uv + to_float2(1,0)*d) - texture(iChannel2, uv - to_float2(1,0)*d))*0.5f;
  float4 dy = (texture(iChannel2, uv + to_float2(0,1)*d) - texture(iChannel2, uv - to_float2(0,1)*d))*0.5f;

  // add the pixel gradients
  d = pixelSize*1.0f;
  dx += texture(iChannel0, uv + to_float2(1,0)*d) - texture(iChannel0, uv - to_float2(1,0)*d);
  dy += texture(iChannel0, uv + to_float2(0,1)*d) - texture(iChannel0, uv - to_float2(0,1)*d);

  float2 displacement = to_float2(dx.x,dy.x)*lightSize; // using only the red gradient as displacement vector
  float light = _powf(_fmaxf(1.0f-distance_f2(0.5f+(uv-0.5f)*aspect*lightSize + displacement,0.5f+(swi2(iMouse,x,y)*pixelSize-0.5f)*aspect*lightSize),0.0f),4.0f);

  // recolor the red channel
  float4 rd = to_float4_s(texture(iChannel0,uv+to_float2(dx.x,dy.x)*pixelSize*8.0f).x)*to_float4(0.7f,1.5f,2.0f,1.0f)-to_float4(0.3f,1.0f,1.0f,1.0f);

  // and add the light map
  fragColor = _mix(rd,to_float4(8.0f,6.0f,2.0f,1.0f), light*0.75f*to_float4_s(1.0f-texture(iChannel0,uv+to_float2(dx.x,dy.x)*pixelSize*8.0f).x)); 

  fragColor += Color-0.5f;
  fragColor.w=Color.w;


  //fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15); // bypass    

  SetFragmentShaderComputedColor(fragColor);
}