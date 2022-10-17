
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define PI 3.1415926535897932384626433832795f

#define POSITION_ROW   0
#define VELOCITY_ROW   1
#define COLOR_ROW      2
#define LAST_ROW     COLOR_ROW

#define COLOR_ZERO  to_float4_s(0)
#define VEC2_ZERO   to_float2_s(0)
#define HORIZONTAL_REVERSE  to_float2(-1.0f, 1.0f)
#define VERTICAL_REVERSE    to_float2(1.0f, -1.0f)

/*

BEGIN: store vector on texture functions

taken from here:

https://gist.github.com/Flexi23/1713774

These are the helper functions to store and to restore a 2D vector with a custom 16 floating point precision in a texture.
The 16 bit are used as follows: 1 bit is for the sign, 4 bits are used for the exponent, the remaining 11 bit are for the mantissa.

The exponent bias is asymmetric so that the maximum representable number is 2047 (and bigger numbers will be cut)
the accuracy from 1024 - 2047 is one integer
512-1023 it's 1/2 int
256-511 it's 1/4 int and so forth...
between 0 and 1/16 the accuracy is the highest with 1/2048 (which makes 1/32768 the minimum representable number)

So this is a non-IEEE implementation (which would be a 5 bit exponent with a symmetric bias from 2^-15 to 2^16 and a 10 bit mantissa)

I hope anyone else knows a purpose for such a buffer and can use it too (in a fragment shader). ;)

Felix Woitzel, Jan/Feb 2012
(Twitter: @Flexi23)

attention: this is only tested on a AMD Radeon HD series chip so far and there might be oddities with Intel and Nvidia. I'll try and test it on other chips soon.

store: "gl_FragColor = encode2( v );"
restore: "vec2 v = decode2( texture2D( encoded_sampler, coord) );"

*/

__DEVICE__ float2 encode(float v){
  float2 c = to_float2_s(0.0f);

  int signum = (v >= 0.0f) ? 128 : 0;
  v = _fabs(v);
  int exponent = 15;
  float limit = 1024.0f; // considering the bias from 2^-5 to 2^10 (==1024)
  for(int exp = 15; exp > 0; exp--){
    if( v < limit){
      limit /= 2.0f;
      exponent--;
    }
  }

  float rest;
  if(exponent == 0){
    rest = v / limit / 2.0f;    // "subnormalize" implicite preceding 0.0f 
  }else{
    rest = (v - limit)/limit;  // normalize accordingly to implicite preceding 1.
  }

  int mantissa = (int)(rest * 2048.0f);  // 2048 = 2^11 for the (split) 11 bit mantissa
  int msb = mantissa / 256;    // the most significant 3 bits go into the lower part of the first byte
  int lsb = mantissa - msb * 256;    // there go the other 8 bit of the lower significance

  c.x = (float)(signum + exponent * 8 + msb) / 255.0f;  // color normalization for texture2D
  c.y = (float)(lsb) / 255.0f;

  if(v >= 2048.0f){
    c.y = 1.0f;
  }

  return c;
}

__DEVICE__ float decode(const float2 c){
  float v = 0.0f;

  int ix = (int)(c.x*255.0f); // 1st byte: 1 bit signum, 4 bits exponent, 3 bits mantissa (MSB)
  int iy = (int)(c.y*255.0f);  // 2nd byte: 8 bit mantissa (LSB)

  int s = (c.x >= 0.5f) ? 1 : -1;
  ix = (s > 0) ? ix - 128 : ix; // remove the signum bit from exponent
  int iexp = ix / 8; // cut off the last 3 bits of the mantissa to select the 4 exponent bits
  int msb = ix - iexp * 8;  // subtract the exponent bits to select the 3 most significant bits of the mantissa

  int norm = (iexp == 0) ? 0 : 2048; // distinguish between normalized and subnormalized numbers
  int mantissa = norm + msb * 256 + iy; // implicite preceding 1 or 0 added here
  norm = (iexp == 0) ? 1 : 0; // normalization toggle
  float exponent = _powf( 2.0f, (float)(iexp + norm) - 16.0f); // -5 for the the exponent bias from 2^-5 to 2^10 plus another -11 for the normalized 12 bit mantissa 
  v = (float)( s * mantissa ) * exponent;

  return v;
}

__DEVICE__ float4 encode2(const float2 v){
  return to_float4_f2f2(encode(v.x), encode(v.y));
}
    
__DEVICE__ float2 decode2(const float4 c){
  return to_float2(decode(swi2(c,x,y)), decode(swi2(c,z,w)));
}

/* END: store vector on texture functions */

__DEVICE__ float randImpl(const float2 co) {
    return fract(_sinf(dot(swi2(co,x,y), to_float2(12.9898f,78.233f))) * 43758.5453f);
}

//#define tex(ch, x, y) texelFetch(ch, to_int2(x, y), 0)
#define tex(ch, x, y) texture(ch, (make_float2((int)x,(int)y)+0.5)/R)

#define rand(seed) randImpl(seed + mod_f(iTime, PI)) 

#define getParticleRawColor(row, index) tex(iChannel0, index, row) 
#define getParticleVector(row, index) decode2(getParticleRawColor(row, index))
#define getParticlePosition(index) getParticleVector(POSITION_ROW, index)
#define getParticleVelocity(index) getParticleVector(VELOCITY_ROW, index)
#define getParticleColor(index)    getParticleRawColor(COLOR_ROW, index)

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// true if the space is pressed
//__DEVICE__ bool reset() {
//    return texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
//}

__DEVICE__ float4 getInitialState(
    const int row,
    const int particle, // not used here, but might be useful for other shaders
    float2 seed, float iTime
) {
    float4 state;

    if (row == POSITION_ROW) {
        state = encode2(VEC2_ZERO);        
    } else if (row == VELOCITY_ROW) {
        state = encode2(VEC2_ZERO);
    } else if (row == COLOR_ROW) {
        state = to_float4(
            rand(seed + 0.101f),
            rand(seed + 0.102f),
            rand(seed + 0.103f),
            1.0f
        );
    } else {
        state = COLOR_ZERO;
    }
    return state;
}

__DEVICE__ float4 calculateNewState(
    const int row,
    const int particle,
    const float2 seed,
    float2 R, float iTime, __TEXTURE2D__ iChannel0,
    float MAX_VELOCITY,float MAX_VELOCITY_CHANGE,float FOCAL_POINT_TENDENCY
) {

    float4 state;
    if (row == POSITION_ROW) {
        state = encode2(
            getParticlePosition(particle)
            + getParticleVelocity(particle)
        );
    } else if (row == VELOCITY_ROW) {
        float2 velocity = getParticleVelocity(particle);
        float2 coord  = getParticlePosition(particle);
        velocity += to_float2(
            rand(seed + 0.111f) * MAX_VELOCITY_CHANGE * 2.0f - MAX_VELOCITY_CHANGE,
            rand(seed + 0.112f) * MAX_VELOCITY_CHANGE * 2.0f - MAX_VELOCITY_CHANGE
        );
        float2 focalPoint = coord; // might be different for other shaders, e.g. mouse
        velocity -= focalPoint * FOCAL_POINT_TENDENCY;
        // TODO is there a better way to clamp the vector?
        velocity = to_float2(
            clamp(velocity.x, -MAX_VELOCITY, MAX_VELOCITY),
            clamp(velocity.y, -MAX_VELOCITY, MAX_VELOCITY)
        );
        float2 prediction = coord + velocity;
        if ((prediction.x < -0.5f) || (prediction.x > 0.5f)) {
            velocity *= HORIZONTAL_REVERSE;
        }
        if ((prediction.y < -0.5f) || (prediction.y > 0.5f)) {
            velocity *= VERTICAL_REVERSE;
        }
        state = encode2(velocity);
    } else if (row == COLOR_ROW) {
        state = getParticleColor(particle); // will copy over
    } else { // should never happen
        state = COLOR_ZERO;
    }
    return state;
}        

__DEVICE__ float4 getEncodedState(
    const int row,
    const int particle,
    const float2 seed, 
    int iFrame, float iTime, float2 R, bool reset, __TEXTURE2D__ iChannel0,
    float MAX_VELOCITY,float MAX_VELOCITY_CHANGE,float FOCAL_POINT_TENDENCY
) {        
    float4 state;    
    if (iFrame == 0 || reset) {
        state = getInitialState(row, particle, seed, iTime);
    } else {
        state = calculateNewState(row, particle, seed,R,iTime,iChannel0,
                                  MAX_VELOCITY,MAX_VELOCITY_CHANGE,FOCAL_POINT_TENDENCY);
    }
    return state;
}

__KERNEL__ void ExpandingPrismaticSmokeFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, int iFrame, sampler2D iChannel0,sampler2D iChannel1,sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    CONNECT_SLIDER0(MAX_VELOCITY, 0.0f, 0.1f, 0.004f);
    CONNECT_SLIDER1(MAX_VELOCITY_CHANGE, 0.0f, 0.1f, 0.0003f);
    CONNECT_SLIDER2(FOCAL_POINT_TENDENCY, 0.0f, 0.1f, 0.0002f);
    CONNECT_INTSLIDER0(PARTICLE_COUNT, 1, 10, 5);
    
    fragCoord+=0.5f;
    int particle = (int)(fragCoord.x);
    int row = (int)(fragCoord.y);    
    if ((row <= LAST_ROW) && (particle < PARTICLE_COUNT)) {
      fragColor = getEncodedState(row, particle, fragCoord, iFrame, iTime,R, Reset, iChannel0,
                                  MAX_VELOCITY,MAX_VELOCITY_CHANGE,FOCAL_POINT_TENDENCY);        
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Preset: Keyboard' to iChannel3
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


#define rotate(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))

//__DEVICE__ bool reset() {
//    return texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
//}

__DEVICE__ float getColor(
    float dist,
    const float angle,
    float size,
    float phase,
    float iTime, float WALL_THINNESS, float PARTICLE_EDGE_SMOOTHING
) { 
    dist = dist
        + (_sinf(angle * 3.0f + iTime * 1.0f + phase) + 1.0f) * .02
        + (_cosf(angle * 5.0f - iTime * 1.1f + phase) + 1.0f) * 0.01f;
  return 
        _powf(dist / size, WALL_THINNESS)
        * smoothstep(size, size - PARTICLE_EDGE_SMOOTHING, dist)        
    ;
}

__KERNEL__ void ExpandingPrismaticSmokeFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    CONNECT_SLIDER0(MAX_VELOCITY, 0.0f, 0.1f, 0.004f);
    CONNECT_INTSLIDER0(PARTICLE_COUNT, 1, 10, 5);
    
    CONNECT_SLIDER3(PARTICLE_SIZE, 0.0f, 1.0f, 0.2f);
    CONNECT_SLIDER4(PARTICLE_EDGE_SMOOTHING, 0.0f, 0.1f, 0.003f);
    CONNECT_SLIDER5(WALL_THINNESS, 0.0f, 100.0f, 60.0f);
    
    
    fragCoord+=0.5f;

    float2 pixel = (fragCoord - (iResolution / 2.0f)) / iResolution.y;
    pixel = mul_f2_mat2(pixel,rotate(iTime * 0.005f));
    float3 mixedColor = swi3(texture(iChannel1, fragCoord / iResolution - pixel * 0.005
                             * iResolution.y / iResolution
                             ),x,y,z);
    mixedColor *= 0.995f;
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        float2 particle = getParticlePosition(i);
        float dist = distance_f2(particle, pixel);
        if (dist <= PARTICLE_SIZE) { 
            float2 delta = particle - pixel;
            float angle = _atan2f(delta.x, delta.y);
            float phase = (float)(i);
                mixedColor += to_float3(
                getColor(dist, angle, PARTICLE_SIZE, phase,iTime,WALL_THINNESS, PARTICLE_EDGE_SMOOTHING),
                getColor(dist, angle + 0.03f, PARTICLE_SIZE, phase,iTime,WALL_THINNESS, PARTICLE_EDGE_SMOOTHING),
                getColor(dist, angle + 0.06f, PARTICLE_SIZE, phase,iTime,WALL_THINNESS, PARTICLE_EDGE_SMOOTHING)
            ) * 0.09
            * swi3(getParticleColor(i),x,y,z); //, mixedColor, 0.5f;            
        }
    }
    fragColor = to_float4_aw(mixedColor, 1.0f);
    
    if (Reset) {
        fragColor = to_float4_s(0);
    }


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


// Fork of "static prismatic smoke" by morisil. https://shadertoy.com/view/tsXSDH
// 2019-02-26 09:33:58

// Fork of "persisted motion" by morisil. https://shadertoy.com/view/tsjGDD
// 2019-02-21 17:19:31

// Fork of "focused random bubbles" by morisil. https://shadertoy.com/view/Wdj3WR
// 2019-01-31 16:12:56

__KERNEL__ void ExpandingPrismaticSmokeFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  fragColor = texture(iChannel0, fragCoord / iResolution);

  SetFragmentShaderComputedColor(fragColor);
}