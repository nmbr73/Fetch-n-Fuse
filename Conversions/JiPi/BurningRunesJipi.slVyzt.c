
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/**
 * Created by Kamil Kolaczynski (revers) - 2015
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
 *
 * This shader uses code written by: 
 * - iq (raymarching, hash, noise)
 * - otaviogood (runes, https://www.shadertoy.com/view/MsXSRn)
 * Thanks for sharing it guys!
 * 
 * The shader was created and exported from Synthclipse (http://synthclipse.sourceforge.net/)
 */

//const float MarchDumping = 1.0f;
//const float Far = 62.82f;
#define MaxSteps 32
//const float FOV = 0.4f;
//const float3 Eye = to_float3(0.14f, 0.0f, 3.4999998f);
//const float3 Direction = to_float3(0.0f, 0.0f, -1.0f);
//const float3 Up = to_float3(0.0f, 1.0f, 0.0f);

// Noise settings:
//const float Power = 5.059f;
//const float MaxLength = 0.9904f;
//const float Dumping = 10.0f;

#define PI 3.141592
#define HALF_PI 1.57079632679

#define DEG_TO_RAD  (PI / 180.0f)
//const float TIME_FACTOR = 0.3f;
//const float ROTATION_DIST = 16.0f;

__DEVICE__ float3 hash3(float3 p) {
  p = to_float3(dot(p, to_float3(127.1f, 311.7f, 74.7f)),
                dot(p, to_float3(269.5f, 183.3f, 246.1f)),
                dot(p, to_float3(113.5f, 271.9f, 124.6f)));

  return -1.0f + 2.0f * fract_f3(sin_f3(p) * 43758.5453123f);
}

__DEVICE__ float noise(float3 p) {
  float3 i = _floor(p);
  float3 f = fract_f3(p);

  float3 u = f * f * (3.0f - 2.0f * f);

  float n0 = dot(hash3(i + to_float3(0.0f, 0.0f, 0.0f)), f - to_float3(0.0f, 0.0f, 0.0f));
  float n1 = dot(hash3(i + to_float3(1.0f, 0.0f, 0.0f)), f - to_float3(1.0f, 0.0f, 0.0f));
  float n2 = dot(hash3(i + to_float3(0.0f, 1.0f, 0.0f)), f - to_float3(0.0f, 1.0f, 0.0f));
  float n3 = dot(hash3(i + to_float3(1.0f, 1.0f, 0.0f)), f - to_float3(1.0f, 1.0f, 0.0f));
  float n4 = dot(hash3(i + to_float3(0.0f, 0.0f, 1.0f)), f - to_float3(0.0f, 0.0f, 1.0f));
  float n5 = dot(hash3(i + to_float3(1.0f, 0.0f, 1.0f)), f - to_float3(1.0f, 0.0f, 1.0f));
  float n6 = dot(hash3(i + to_float3(0.0f, 1.0f, 1.0f)), f - to_float3(0.0f, 1.0f, 1.0f));
  float n7 = dot(hash3(i + to_float3(1.0f, 1.0f, 1.0f)), f - to_float3(1.0f, 1.0f, 1.0f));

  float ix0 = _mix(n0, n1, u.x);
  float ix1 = _mix(n2, n3, u.x);
  float ix2 = _mix(n4, n5, u.x);
  float ix3 = _mix(n6, n7, u.x);

  float ret = _mix(_mix(ix0, ix1, u.y), _mix(ix2, ix3, u.y), u.z) * 0.5f + 0.5f;
  return ret * 2.0f - 1.0f;
}

__DEVICE__ float sdBox(float3 p, float3 b) {
  float3 d = abs_f3(p) - b;
  return _fminf(_fmaxf(d.x, _fmaxf(d.y, d.z)), 0.0f) + length(_fmaxf(d, to_float3_s(0.0f)));
}

__DEVICE__ float3 rotateY(float3 p, float a) {
  float sa = _sinf(a);
  float ca = _cosf(a);
  return to_float3(ca * p.x + sa * p.z, p.y, ca * p.z - sa * p.x);
}

__DEVICE__ float getAngle(float x) {
  return ((1.0f - x) * 100.0f - 15.0f) * DEG_TO_RAD;
}

__DEVICE__ float tween(float time, float TIME_FACTOR) {
  float t = fract(time * TIME_FACTOR);

  float stop = 0.25f;
  float range = 1.0f - stop;
  float k = _sinf((_sinf(_sinf(HALF_PI) * HALF_PI)) * HALF_PI) * 0.9f;

  float ret = _sinf((_sinf(sin(t / range * HALF_PI) * HALF_PI)) * HALF_PI) * 0.9f;
  float stp = step(range, t);

  return ret * (1.0f - stp) + stp * _mix(k, 1.0f, (t - range) / (1.0f - range));
}

__DEVICE__ float3 transformCube(float3 p, float iTime, float TIME_FACTOR, float ROTATION_DIST) {
  p.x -= ROTATION_DIST;

  p = rotateY(p, getAngle(tween(iTime, TIME_FACTOR)));
  p.x += ROTATION_DIST;
  return p;
}

__DEVICE__ float map(float3 p, float iTime, float TIME_FACTOR, float ROTATION_DIST) {
  float3 q = transformCube(p, iTime, TIME_FACTOR, ROTATION_DIST);
  return sdBox(q, to_float3(1.0f, 1.0f, 0.0001f));
}

__DEVICE__ float2 castRay(float3 ro, float3 rd, float iTime, float TIME_FACTOR, float ROTATION_DIST, float Far, float MarchDumping) {
  float tmin = 0.0f;
  float tmax = Far;

  float precis = 0.002f;
  float t = tmin;
  float m = -1.0f;

  for (int i = 0; i < MaxSteps; i++) {
    float res = map(ro + rd * t,iTime, TIME_FACTOR,ROTATION_DIST);
    if (res < precis || t > tmax) {
      break;
    }
    t += res * MarchDumping;
    m = 1.0f;
  }

  if (t > tmax) {
    m = -1.0f;
  }
  return to_float2(t, m);
}

__DEVICE__ float udSegment(float2 p, float2 start, float2 end) {
  float2 dir = start - end;
  float len = length(dir);
  dir /= len;

  float2 proj = clamp(dot(p - end, dir), 0.0f, len) * dir + end;
  return distance_f2(p, proj);
}


__DEVICE__ float debug;

/**
 * Rune function by Otavio Good.
 * https://www.shadertoy.com/view/MsXSRn
 */
__DEVICE__ float rune(float2 uv, float2 seed, float2 iResolution, __TEXTURE2D__ iChannel0) {
  float ret = 100.0f;

  for (int i = 0; i < 4; i++) {
    // generate seeded random line endPoints - just about any texture_ should work.
    // Hopefully this randomness will work the same on all GPUs (had some trouble with that)
    float2 posA = swi2(texture(iChannel0, _floor(seed + 0.5f) / iResolution),x,y);
    float2 posB = swi2(texture(iChannel0, _floor(seed + 1.5f) / iResolution),x,y);

    seed += 2.0f;
    // expand the range and mod it to get a nicely distributed random number - hopefully. :)
    posA = fract(posA * 128.0f);
    posB = fract(posB * 128.0f);

    // each rune touches the edge of its box on all 4 sides
    if (i == 0) {
      posA.y = 0.0f;
    }
    if (i == 1) {
      posA.x = 0.999f;
    }
    if (i == 2) {
      posA.x = 0.0f;
    }
    if (i == 3) {
      posA.y = 0.999f;
    }

    // snap the random line endpoints to a grid 2x3
    float2 snaps = to_float2(2.0f, 3.0f);
    posA = (_floor(posA * snaps) + 0.5f) / snaps;  // to center it in a grid cell
    posB = (_floor(posB * snaps) + 0.5f) / snaps;

    if (distance_f2(posA, posB) < 0.0001f) {
      continue; // eliminate dots.
    }

    // Dots (degenerate lines) are not cross-GPU safe without adding 0.001f - divide by 0 error.
    float d = udSegment(uv, posA, posB + 0.001f);
    ret = _fminf(ret, d);
  }
  
  //Test
  //float4 tex = texture(iChannel0, uv);
  //if (tex.w > 0.0f) ret = 1.0f;
  //debug = ret;
  
  ret = texture(iChannel0, uv).x;
  
  return ret;
}

__DEVICE__ float distToObject(float2 p, float iTime, float TIME_FACTOR, float2 iResolution, __TEXTURE2D__ iChannel0) {
  p *= 0.2f;

  float2 newSeed = to_float2_s(iTime * TIME_FACTOR + 1.0f);
  newSeed.y *= 0.2f;
  newSeed = _floor(newSeed);
  newSeed *= 4.0f;
        
  return rune(p, newSeed - 0.41f, iResolution, iChannel0);
}

__DEVICE__ float normalizeScalar(float value, float max) {
  return clamp(value, 0.0f, max) / max;
}

__DEVICE__ float3 color(float2 p, float iTime, float TIME_FACTOR, float2 iResolution, __TEXTURE2D__ iChannel0, float Power, float MaxLength, float Dumping, float3 Color) {
  float3 coord = to_float3_aw(p, iTime * 0.25f);
  float n = _fabs(noise(coord));
  n += 0.5f * _fabs(noise(coord * 2.0f));
  n += 0.25f * _fabs(noise(coord * 4.0f));
  n += 0.125f * _fabs(noise(coord * 8.0f));

  n *= (100.001f - Power);
  float dist = distToObject(p, iTime,TIME_FACTOR,iResolution,iChannel0);
  float k = normalizeScalar(dist, MaxLength);
  n *= dist / _powf(1.001f - k, Dumping);

  //float3 col = to_float3(1.0f, 0.25f, 0.08f) / n;
  float3 col = Color / n;
  return pow_f3(col, to_float3_s(2.0f));
}

__DEVICE__ float3 render(float3 ro, float3 rd, float iTime, float TIME_FACTOR, 
                         float ROTATION_DIST, float FAR, float MarchDumping, 
                         float2 iResolution, __TEXTURE2D__ iChannel0, float Power, float MaxLength, float Dumping, float3 Color) {
  float3 col = to_float3_s(0.0f);
  float2 res = castRay(ro, rd, iTime, TIME_FACTOR, ROTATION_DIST, FAR, MarchDumping);  

  float t = res.x;
  float m = res.y;

  if (m > 0.0f) {
    float3 pos = ro + t * rd;

    float3 q = transformCube(pos, iTime, TIME_FACTOR, ROTATION_DIST);
    float2 uv = swi2(q,x,y) * 3.0f;

    col = color(uv + 2.5f,iTime,TIME_FACTOR,iResolution,iChannel0,Power,MaxLength,Dumping,Color);
  }

  return (clamp(col, 0.0f, 1.0f));
}

__KERNEL__ void BurningRunesJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float3 iChannelResolution[], sampler2D iChannel0)
{

  CONNECT_COLOR0(Color, 1.0f, 0.25f, 0.08f, 1.0f);
  //CONNECT_SLIDER0(Far, -1.0f, 100.0f, 62.82f);
  CONNECT_SLIDER1(FOV, -1.0f, 10.0f, 0.4f);
  CONNECT_SLIDER2(Power, -1.0f, 10.0f, 5.059f);
  CONNECT_SLIDER3(MaxLength, -1.0f, 10.0f, 0.9904f);  
  CONNECT_SLIDER4(Dumping, -1.0f, 30.0f, 10.0f);
  CONNECT_SLIDER5(TIME_FACTOR, -1.0f, 10.0f, 0.3f);
  CONNECT_SLIDER6(ROTATION_DIST, -1.0f, 30.0f, 16.0f);
  
  CONNECT_POINT0(EyeXY, 0.0f, 0.0f );
  CONNECT_SLIDER7(EyeZ, -1.0f, 10.0f, 0.0f);
  CONNECT_POINT1(DirectionXY, 0.0f, 0.0f );
  CONNECT_SLIDER8(DirectionZ, -1.0f, 10.0f, 0.0f);
  
  CONNECT_POINT2(UPXY, 0.0f, 0.0f );
  CONNECT_SLIDER9(UPZ, -1.0f, 10.0f, 0.0f);
  
  
  const float MarchDumping = 1.0f;
  const float Far = 62.82f;

  //const float FOV = 0.4f;
  const float3 Eye = to_float3(0.14f, 0.0f, 3.4999998f)+to_float3_aw(EyeXY, EyeZ);
  const float3 Direction = to_float3(0.0f, 0.0f, -1.0f)+to_float3_aw(DirectionXY, DirectionZ);
  const float3 Up = to_float3(0.0f, 1.0f, 0.0f)+to_float3_aw(UPXY, UPZ);

  // Noise settings:
  //const float Power = 5.059f;
  //const float MaxLength = 0.9904f;
  //const float Dumping = 10.0f;

  //const float TIME_FACTOR = 0.3f;
  //const float ROTATION_DIST = 16.0f;

float IIIIIIIIIIIIII;

  float2 q = fragCoord / iResolution;
  float2 coord = 2.0f * q - 1.0f;
  coord.x *= iResolution.x / iResolution.y;
  coord *= FOV;

  float3 dir = normalize(Direction);
  float3 up = Up;
  float3 upOrtho = normalize(up - dot(dir, up) * dir);
  float3 right = normalize(cross(dir, upOrtho));

  float3 ro = Eye;
  float3 rd = normalize(dir + coord.x * right + coord.y * upOrtho);

  float3 col = render(ro, rd, iTime, TIME_FACTOR, ROTATION_DIST, Far, MarchDumping, iResolution, iChannel0, Power, MaxLength, Dumping, swi3(Color,x,y,z));
  col = pow_f3(col, to_float3_s(0.4545f));

  fragColor = to_float4_aw(col, 1.0f);

  //fragColor = to_float4_s(debug);
  fragColor.w=1.0f;

  SetFragmentShaderComputedColor(fragColor);
}