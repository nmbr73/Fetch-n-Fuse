
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// rotation matrix
//__DEVICE__ mat2 rot (float a) { return to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }

// generated with discrete Fourier transform
__DEVICE__ float2 cookie(float t) {
  return to_float2(0.08f+_cosf(t-1.58f)*0.23f+_cosf(t*2.0f-1.24f)*0.14f+_cosf(t*3.0f-1.12f)*0.09f+_cosf(t*4.0f-0.76f)*0.06f+_cosf(t*5.0f-0.59f)*0.05f+_cosf(t*6.0f+0.56f)*0.03f+_cosf(t*7.0f-2.73f)*0.03f+_cosf(t*8.0f-1.26f)*0.02f+_cosf(t*9.0f-1.44f)*0.02f+_cosf(t*10.0f-2.09f)*0.03f+_cosf(t*11.0f-2.18f)*0.01f+_cosf(t*12.0f-1.91f)*0.02f,
                         _cosf(3.14f)*0.05f+_cosf(t+0.35f)*0.06f+_cosf(t*2.0f+0.54f)*0.09f+_cosf(t*3.0f+0.44f)*0.03f+_cosf(t*4.0f+1.02f)*0.07f+_cosf(t*6.0f+0.39f)*0.03f+_cosf(t*7.0f-1.48f)*0.02f+_cosf(t*8.0f-3.06f)*0.02f+_cosf(t*9.0f-0.39f)*0.07f+_cosf(t*10.0f-0.39f)*0.03f+_cosf(t*11.0f-0.03f)*0.04f+_cosf(t*12.0f-2.08f)*0.02f);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Blending' to iChannel4
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel2


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

__DEVICE__ float2 mouseDelta( float2 iResolution, float4 iMouse, __TEXTURE2D__ iChannel2){
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

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}


__KERNEL__ void ErdSlimeMouldFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_CHECKBOX1(StartShuffle, 1);
  
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

  fragCoord+=0.5f;

  float2 uv = fragCoord / iResolution;
  float2 pixelSize = 1.0f / iResolution;
  

  float2 mouseV = mouseDelta(iResolution, iMouse, iChannel2);
  float2 aspect = to_float2(1.0f,iResolution.y/iResolution.x);
  uv = vortex_pair_warp(uv, swi2(iMouse,x,y)*pixelSize, mouseV*aspect*1.4f, iResolution);

  float4 blur1 = _tex2DVecN(iChannel1,uv.x,uv.y,15);
  
  float4 noise = texture(iChannel3, fragCoord / iResolution + fract_f2(to_float2(42,56)*iTime)*StartShuffle);

  // get the gradients from the blurred image
  float2 d = pixelSize*4.0f;
  float4 dx = (texture(iChannel1, fract_f2(uv + to_float2(1,0)*d)) - texture(iChannel1, fract_f2(uv - to_float2(1,0)*d))) * 0.5f;
  float4 dy = (texture(iChannel1, fract_f2(uv + to_float2(0,1)*d)) - texture(iChannel1, fract_f2(uv - to_float2(0,1)*d))) * 0.5f;
    
  float2 uv_red = uv + to_float2(dx.x, dy.x)*pixelSize*8.0f; // add some diffusive expansion
  
  float new_red = texture(iChannel0, fract_f2(uv_red)).x + (noise.x - 0.5f) * 0.0025f - 0.002f; // stochastic decay
  new_red -= (texture(iChannel1, fract_f2(uv_red + (swi2(noise,x,y)-0.5f)*pixelSize)).x -
              texture(iChannel0, fract_f2(uv_red + (swi2(noise,x,y)-0.5f)*pixelSize)).x) * 0.047f; // reaction-diffusion
      
      
  if(iFrame<10 || Reset)
  {
    fragColor = noise; 
  }
  else
  {
      fragColor.x = clamp(new_red, 0.0f, 1.0f);
  }


  if (Blend1>0.0) fragColor = Blending(iChannel4, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);

  //    fragColor = noise; // need a restart?

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// horizontal Gaussian blur pass

__KERNEL__ void ErdSlimeMouldFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
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
    
  fragColor = to_float4_aw(swi3(sum,x,y,z)/0.98f, 1.0f); // normalize
  
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// vertical Gaussian blur pass

__KERNEL__ void ErdSlimeMouldFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  fragCoord+=0.5f; 

  float2 pixelSize = 1.0f/ iResolution;
  float2 uv = fragCoord * pixelSize;

  float v = pixelSize.y;
  float4 sum = to_float4_s(0.0f);
  sum += texture(iChannel0, fract(to_float2(uv.x, uv.y - 4.0f*v)) ) * 0.05f;
  sum += texture(iChannel0, fract(to_float2(uv.x, uv.y - 3.0f*v)) ) * 0.09f;
  sum += texture(iChannel0, fract(to_float2(uv.x, uv.y - 2.0f*v)) ) * 0.12f;
  sum += texture(iChannel0, fract(to_float2(uv.x, uv.y - 1.0f*v)) ) * 0.15f;
  sum += texture(iChannel0, fract(to_float2(uv.x, uv.y + 0.0f*v)) ) * 0.16f;
  sum += texture(iChannel0, fract(to_float2(uv.x, uv.y + 1.0f*v)) ) * 0.15f;
  sum += texture(iChannel0, fract(to_float2(uv.x, uv.y + 2.0f*v)) ) * 0.12f;
  sum += texture(iChannel0, fract(to_float2(uv.x, uv.y + 3.0f*v)) ) * 0.09f;
  sum += texture(iChannel0, fract(to_float2(uv.x, uv.y + 4.0f*v)) ) * 0.05f;
    
  fragColor = to_float4_aw(swi3(sum,x,y,z)/0.98f, 1.0f); // normalize  

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0


// not used (yet), but hooray for 8 channel feedback

__KERNEL__ void ErdSlimeMouldFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse)
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
// Connect Image 'Texture: Abstract 1' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1



// shortcut to sample texture
#define TEX(uv) _tex2DVecN(iChannel0,(uv).x,(uv).y,15).x


// Fork of " expansive reaction-diffusion" by Flexi. https://shadertoy.com/view/4dcGW2
// 2022-07-28 10:46:34

// and

// Brush toy by Leon Denise 2022-05-17

// I wanted to play further with shading and lighting from 2D heightmap.
// I started by generating a heightmap from noise, then shape and curves.
// Once the curve was drawing nice brush strokes, I wanted to add motion.
// Also wanted to add droplets of paints falling, but that will be
// for another sketch.

// This is the color pass
// Click on left edge to see layers

// The painting pass (Buffer A) is using FBM noise to simulate brush strokes
// The curve was generated with a discrete Fourier Transform,
// from https://www.shadertoy.com/view/3ljXWK

// Frame buffer sampling get offset from brush motion,
// and the mouse also interact with the buffer.


__KERNEL__ void ErdSlimeMouldFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_COLOR1(Rainbow, 1.0f, 2.0f, 3.0f, 1.0f);
    CONNECT_SLIDER5(Specular, -10.0f, 10.0f, 0.5f);
    CONNECT_SLIDER6(GrayNoise, -10.0f, 10.0f, 5.0f);
    CONNECT_SLIDER7(Tint, -10.0f, 10.0f, 0.5f);
    CONNECT_SLIDER8(UVxNoise, -10.0f, 10.0f, 5.0f);

    float3 color = swi3(Color,x,y,z)-0.5f;//to_float3_s(0.0f);

    // coordinates
    float2 uv = fragCoord / iResolution;
    float3 dither = swi3(texture(iChannel1, fragCoord / 1024.0f),x,y,z);
    
    // value from noise buffer A
    float3 noise = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float gray = noise.x;
    
    // gradient normal from gray value
    float3 unit = to_float3_aw(3.0f/iResolution,0);
    float3 normal = normalize(to_float3(
                                        TEX(uv + swi2(unit,x,z))-TEX(uv - swi2(unit,x,z)),
                                        TEX(uv - swi2(unit,z,y))-TEX(uv + swi2(unit,z,y)),
                                        gray*gray));
    
    
    // specular light
    float3 dir = normalize(to_float3(0,1,2.0f));
    float specular = _powf(dot(normal, dir)*0.5f+0.5f,20.0f);
    color += to_float3_s(Specular)*specular; //to_float3_s(0.5f)*specular;
    
    // rainbow palette
    //float3 tint = 0.5f+0.5f*cos_f3(to_float3(1,2,3)*(1.0f+(0.5f*_sinf(iTime/3.0f)))+gray*5.0f+uv.x*5.0f);
    float3 tint = 0.5f+Tint*cos_f3(swi3(Rainbow,x,y,z)*(1.0f+(0.5f*_sinf(iTime/3.0f)))+gray*GrayNoise+uv.x*UVxNoise);
    dir = normalize(to_float3_aw(uv-0.5f, 0.0f));
    color += tint*_powf(dot(normal, -dir)*0.5f+0.5f, 0.5f);
    
    // background blend
    float3 background = to_float3_s(0.8f)*smoothstep(1.5f,0.0f,length(uv-0.5f));
    color = _mix(background, clamp(color, 0.0f, 1.0f), smoothstep(0.2f,0.5f,noise.x));
    
    // display layers when clic
    if (iMouse.z > 0.5f && iMouse.x/iResolution.x < 0.1f)
    {
        if (uv.x < 0.33f)      color = to_float3_s(gray);
        else if (uv.x < 0.66f) color = normal*0.5f+0.5f;
        else                   color = to_float3_s(0.2f+specular)*gray;
    }

    fragColor = to_float4_aw(color, Color.w);

  SetFragmentShaderComputedColor(fragColor);
}