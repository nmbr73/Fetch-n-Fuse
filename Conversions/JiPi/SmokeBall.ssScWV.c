
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Shows how to use Bitangent Noise (a fast divergence-free noise generator) to mimic fluid motion.
// Details: https://atyuwen.github.io/posts/bitangent-noise/
// Source: https://github.com/atyuwen/bitangent_noise/
// The checkerboard ground is stolen from iq's work: https://www.shadertoy.com/view/Xds3zN

// Set to 1 to make the ball moves.
// Set to 0 to disable movement and get higher framerate.
#define ENABLE_MOVEMENT 1 

//  --------------------------------------------------------------------
//  Optimized implementation of 3D/4D bitangent noise.
//  Based on stegu's simplex noise: https://github.com/stegu/webgl-noise.
//  Contact : atyuwen@gmail.com
//  Author : Yuwen Wu (https://atyuwen.github.io/)
//  License : Distributed under the MIT License.
//  --------------------------------------------------------------------

// Permuted congruential generator (only top 16 bits are well shuffled).
// References: 1.0f Mark Jarzynski and Marc Olano, "Hash Functions for GPU Rendering".
//             2.0f UnrealEngine/Random.ush. https://github.com/EpicGames/UnrealEngine
__DEVICE__ uint2 _pcg3d16(uint3 p)
{
  uint3 v = p * 1664525u + 1013904223u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  v.x += v.y*v.z; v.y += v.z*v.x;
  //return swi2(v,x,y); 
  return make_uint2(v.x,v.y);
}
__DEVICE__ uint2 _pcg4d16(uint4 p)
{
  uint4 v = p * 1664525u + 1013904223u;
  v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
  v.x += v.y*v.w; v.y += v.z*v.x;
  //return swi2(v,x,y);
  return make_uint2(v.x,v.y);
}

// Get random gradient from hash value.
__DEVICE__ float3 _gradient3d(uint hash)
{
  
  //float3 g = to_float3(make_uint3(hash) & make_uint3(0x80000, 0x40000, 0x20000));
  float3 g = to_float3(hash&0x80000, hash&0x40000, hash&0x20000);
  return g * (1.0f / to_float3(0x40000, 0x20000, 0x10000)) - 1.0f;
}
__DEVICE__ float4 _gradient4d(uint hash)
{
  float4 g = to_float4(hash&0x80000, hash&0x40000, hash&0x20000, hash&0x10000);
  return g * (1.0f / to_float4(0x40000, 0x20000, 0x10000, 0x8000)) - 1.0f;
}

// Optimized 3D Bitangent Noise. Approximately 113 instruction slots used.
// Assume p is in the range [-32768, 32767].
__DEVICE__ float3 BitangentNoise3D(float3 p)
{
  const float2 C = to_float2(1.0f / 6.0f, 1.0f / 3.0f);
  const float4 D = to_float4(0.0f, 0.5f, 1.0f, 2.0f);

  // First corner
  float3 i = _floor(p + dot(p, swi3(C,y,y,y)));
  float3 x0 = p - i + dot(i, swi3(C,x,x,x));

  // Other corners
  float3 g = step(swi3(x0,y,z,x), swi3(x0,x,y,z));
  float3 l = 1.0f - g;
  float3 i1 = _fminf(swi3(g,x,y,z), swi3(l,z,x,y));
  float3 i2 = _fmaxf(swi3(g,x,y,z), swi3(l,z,x,y));

  // x0 = x0 - 0.0f + 0.0f * swi3(C,x,x,x);
  // x1 = x0 - i1  + 1.0f * swi3(C,x,x,x);
  // x2 = x0 - i2  + 2.0f * swi3(C,x,x,x);
  // x3 = x0 - 1.0f + 3.0f * swi3(C,x,x,x);
  float3 x1 = x0 - i1 + swi3(C,x,x,x);
  float3 x2 = x0 - i2 + swi3(C,y,y,y); // 2.0f*C.x = 1/3 = C.y
  float3 x3 = x0 - swi3(D,y,y,y);      // -1.0f+3.0f*C.x = -0.5f = -D.y

  i = i + 32768.5f;

  uint2 hash0 = _pcg3d16(make_uint3(i.x,i.y,i.z));
  float3 itmp = i + i1;
  //uint2 hash1 = _pcg3d16(make_uint3(i + i1));
  uint2 hash1 = _pcg3d16(make_uint3(itmp.x,itmp.y,itmp.z));
  
  itmp = i + i2;
  //uint2 hash2 = _pcg3d16(make_uint3(i + i2));
  uint2 hash2 = _pcg3d16(make_uint3(itmp.x,itmp.y,itmp.z));
  
  itmp = i + 1.0f;
  //uint2 hash3 = _pcg3d16(make_uint3(i + 1.0f ));
  uint2 hash3 = _pcg3d16(make_uint3(itmp.x,itmp.y,itmp.z));

  float3 p00 = _gradient3d(hash0.x); float3 p01 = _gradient3d(hash0.y);
  float3 p10 = _gradient3d(hash1.x); float3 p11 = _gradient3d(hash1.y);
  float3 p20 = _gradient3d(hash2.x); float3 p21 = _gradient3d(hash2.y);
  float3 p30 = _gradient3d(hash3.x); float3 p31 = _gradient3d(hash3.y);

  // Calculate noise gradients.
  float4 m = clamp(to_float4_s(0.5f) - to_float4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0f, 1.0f);
  float4 mt = m * m;
  float4 m4 = mt * mt;

  mt = mt * m;
  float4 pdotx = to_float4(dot(p00, x0), dot(p10, x1), dot(p20, x2), dot(p30, x3));
  float4 temp = mt * pdotx;
  float3 gradient0 = -8.0f * (temp.x * x0 + temp.y * x1 + temp.z * x2 + temp.w * x3);
  gradient0 += m4.x * p00 + m4.y * p10 + m4.z * p20 + m4.w * p30;

  pdotx = to_float4(dot(p01, x0), dot(p11, x1), dot(p21, x2), dot(p31, x3));
  temp = mt * pdotx;
  float3 gradient1 = -8.0f * (temp.x * x0 + temp.y * x1 + temp.z * x2 + temp.w * x3);
  gradient1 += m4.x * p01 + m4.y * p11 + m4.z * p21 + m4.w * p31;

  // The cross products of two gradients is divergence free.
  return cross(gradient0, gradient1) * 3918.76f;
}

// 4D Bitangent noise. Approximately 163 instruction slots used.
// Assume p is in the range [-32768, 32767].
__DEVICE__ float3 BitangentNoise4D(float4 p)
{
  const float4 F4 = to_float4_s( 0.309016994374947451f );
  const float4  C = to_float4( 0.138196601125011f,  // (5 - _sqrtf(5))/20  G4
                               0.276393202250021f,  // 2 * G4
                               0.414589803375032f,  // 3 * G4
                              -0.447213595499958f ); // -1 + 4 * G4

  // First corner
  float4 i  = _floor(p + dot(p, F4) );
  float4 x0 = p -   i + dot(i, swi4(C,x,x,x,x));

  // Other corners

  // Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
  float4 i0;
  float3 isX = step( swi3(x0,y,z,w), swi3(x0,x,x,x) );
  float3 isYZ = step( swi3(x0,z,w,w), swi3(x0,y,y,z) );
  // i0.x = dot( isX, to_float3_s( 1.0f ) );
  i0.x = isX.x + isX.y + isX.z;
  //swi3(i0,y,z,w) = 1.0f - isX;
  i0.y=1.0f - isX.x;
  i0.z=1.0f - isX.y;
  i0.w=1.0f - isX.z;
 
  // i0.y += dot( swi2(isYZ,x,y), to_float2_s( 1.0f ) );
  i0.y += isYZ.x + isYZ.y;
  //swi2(i0,z,w) += 1.0f - swi2(isYZ,x,y);
  i0.z=1.0f - isYZ.x;
  i0.w=1.0f - isYZ.y;


  i0.z += isYZ.z;
  i0.w += 1.0f - isYZ.z;

  // i0 now contains the unique values 0,1,2,3 in each channel
  float4 i3 = clamp( i0, 0.0f, 1.0f );
  float4 i2 = clamp( i0 - 1.0f, 0.0f, 1.0f );
  float4 i1 = clamp( i0 - 2.0f, 0.0f, 1.0f );

  // x0 = x0 - 0.0f + 0.0f * swi4(C,x,x,x,x)
  // x1 = x0 - i1  + 1.0f * swi4(C,x,x,x,x)
  // x2 = x0 - i2  + 2.0f * swi4(C,x,x,x,x)
  // x3 = x0 - i3  + 3.0f * swi4(C,x,x,x,x)
  // x4 = x0 - 1.0f + 4.0f * swi4(C,x,x,x,x)
  float4 x1 = x0 - i1 + swi4(C,x,x,x,x);
  float4 x2 = x0 - i2 + swi4(C,y,y,y,y);
  float4 x3 = x0 - i3 + swi4(C,z,z,z,z);
  float4 x4 = x0 + swi4(C,w,w,w,w);

  i = i + 32768.5f;
  uint2 hash0 = _pcg4d16(make_uint4(i.x,i.y,i.z,i.w));
  
  float4 itmp = i + i1;
  //uint2 hash1 = _pcg4d16(make_uint4(i + i1));
  uint2 hash1 = _pcg4d16(make_uint4(itmp.x,itmp.y,itmp.z,itmp.w));
    
  itmp = i + i2;
  //uint2 hash2 = _pcg4d16(make_uint4(i + i2));
  uint2 hash2 = _pcg4d16(make_uint4(itmp.x,itmp.y,itmp.z,itmp.w));
  
  itmp = i + i3;
  //uint2 hash3 = _pcg4d16(make_uint4(i + i3));
  uint2 hash3 = _pcg4d16(make_uint4(itmp.x,itmp.y,itmp.z,itmp.w));
  
  itmp = i + 1.0f;
  //uint2 hash4 = _pcg4d16(make_uint4(i + 1.0f ));
  uint2 hash4 = _pcg4d16(make_uint4(itmp.x,itmp.y,itmp.z,itmp.w));

  float4 p00 = _gradient4d(hash0.x); float4 p01 = _gradient4d(hash0.y);
  float4 p10 = _gradient4d(hash1.x); float4 p11 = _gradient4d(hash1.y);
  float4 p20 = _gradient4d(hash2.x); float4 p21 = _gradient4d(hash2.y);
  float4 p30 = _gradient4d(hash3.x); float4 p31 = _gradient4d(hash3.y);
  float4 p40 = _gradient4d(hash4.x); float4 p41 = _gradient4d(hash4.y);

  // Calculate noise gradients.
  float3 m0 = clamp(0.6f - to_float3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0.0f, 1.0f);
  float2 m1 = clamp(0.6f - to_float2(dot(x3, x3), dot(x4, x4)             ), 0.0f, 1.0f);
  float3 m02 = m0 * m0; float3 m03 = m02 * m0;
  float2 m12 = m1 * m1; float2 m13 = m12 * m1;

  float3 temp0 = m02 * to_float3(dot(p00, x0), dot(p10, x1), dot(p20, x2));
  float2 temp1 = m12 * to_float2(dot(p30, x3), dot(p40, x4));
  float4 grad0 = -6.0f * (temp0.x * x0 + temp0.y * x1 + temp0.z * x2 + temp1.x * x3 + temp1.y * x4);
  grad0 += m03.x * p00 + m03.y * p10 + m03.z * p20 + m13.x * p30 + m13.y * p40;

  temp0 = m02 * to_float3(dot(p01, x0), dot(p11, x1), dot(p21, x2));
  temp1 = m12 * to_float2(dot(p31, x3), dot(p41, x4));
  float4 grad1 = -6.0f * (temp0.x * x0 + temp0.y * x1 + temp0.z * x2 + temp1.x * x3 + temp1.y * x4);
  grad1 += m03.x * p01 + m03.y * p11 + m03.z * p21 + m13.x * p31 + m13.y * p41;

  // The cross products of two gradients is divergence free.
  return cross(swi3(grad0,x,y,z), swi3(grad1,x,y,z)) * 81.0f;
}

// http://iquilezles.org/www/articles/checkerfiltering/checkerfiltering.htm
__DEVICE__ float checkersGradBox( in float2 p, in float2 dpdx, in float2 dpdy )
{
    // filter kernel
    float2 w = abs_f2(dpdx)+abs_f2(dpdy) + 0.001f;
    // analytical integral (box filter)
    float2 i = 2.0f*(abs_f2(fract_f2((p-0.5f*w)*0.5f)-0.5f)-abs_f2(fract_f2((p+0.5f*w)*0.5f)-0.5f))/w;
    // xor pattern
    return 0.5f - 0.5f*i.x*i.y;                  
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv =          ( cross(cu,cw) );
  return to_mat3_f3( cu, cv, cw );
}

#if ENABLE_MOVEMENT
__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r )
{
  float3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );

  return length( pa - ba*h ) - r;
}
__DEVICE__ float map(in float3 p, float iTime)
{
    const float r = 1.0f;
    float _x = r * _sinf(iTime);
    float _z = r * _cosf(iTime);
    
    float3 c = to_float3(_x, 1, _z);
    float3 v = to_float3(_z, 0,-_x) * 2.0f;
    float d = length(p - c) - 0.5f;
    float d2 = sdCapsule(p, c, c - v * 0.2f, 0.5f);
    if (d < -0.2f || d2 > 0.3f) return _fminf(d, d2);
   
    p = p + (normalize(BitangentNoise4D(to_float4_aw(3.0f * p, iTime))) + v) * 0.05f;
    d = length(p - c);
    if (d < 0.5f) return d - 0.5f;

    p = p + (normalize(BitangentNoise4D(to_float4_aw(3.0f * p, iTime))) + v) * 0.05f;
    d = length(p - c);
    if (d < 0.5f) return d - 0.5f;
    
    p = p + (normalize(BitangentNoise4D(to_float4_aw(3.0f * p, iTime))) + v) * 0.05f;
    d = length(p - c);
    if (d < 0.5f) return d - 0.5f;
    
    p = p + (normalize(BitangentNoise4D(to_float4_aw(3.0f * p, iTime))) + v) * 0.05f;
    d = length(p - c);
    return d - 0.5f;
}
#else
__DEVICE__ float map(in float3 p,float iTime)
{
    float d = length(p - to_float3(0, 1, 0));
    if (_fabs(d - 0.5f) > 0.2f)
    {
        // early quit for optimization.
        return d - 0.5f;
    }

    p = p + normalize(BitangentNoise4D(to_float4_aw(3.0f * p, iTime))) * 0.05f;
    p = p + normalize(BitangentNoise4D(to_float4_aw(3.0f * p, iTime))) * 0.05f;
    p = p + normalize(BitangentNoise4D(to_float4_aw(3.0f * p, iTime))) * 0.05f;
    p = p + normalize(BitangentNoise4D(to_float4_aw(3.0f * p, iTime))) * 0.05f;
    d = length(p - to_float3(0, 1, 0)) - 0.5f;
    return d;
}
#endif



__DEVICE__ float4 raymarch(in float3 ro, in float3 rd,float iTime, float3 sundir)
{
    float4 acc = to_float4_s(0.0f);
    float t = 0.0f;
    for (int i = 0; i < 32 && acc.w < 0.95f; ++i)
    {
        float3 pos = ro + t * rd;
        float d = map(pos,iTime);
        float a = clamp(d * -30.0f, 0.0f, 0.2f);
        float s = map(pos + 0.3f * sundir,iTime);
        float diff = clamp((s - d) * 0.4f, 0.0f, 1.0f);
        float3 brdf = to_float3(0.65f,0.68f,0.7f)* 0.2f + 3.0f*to_float3(0.7f, 0.5f, 0.3f)*diff;
        acc.w += (1.0f - acc.w) * a;
        //swi3(acc,x,y,z) += a * brdf;
        acc.x += a * brdf.x;
        acc.y += a * brdf.y;
        acc.z += a * brdf.z;
                
        t += _fmaxf(d * 0.5f, 0.02f);
    }
    
    //swi3(acc,x,y,z) /= (0.001f + acc.w);
    acc.x /= (0.001f + acc.w);
    acc.y /= (0.001f + acc.w);
    acc.z /= (0.001f + acc.w);
    
    return acc;
}

__DEVICE__ float3 shade(float3 diff, float m, float3 N, float3 L, float3 V)
{
  float3 H = normalize(V + L);
  float F = 0.05f + 0.95f * _powf(1.0f - dot(V, H), 5.0f);
  float R = F * _powf(_fmaxf(dot(N, H), 0.0f), m);
  return diff + R * (m + 8.0f) / 8.0f;
}

// http://iquilezles.org/www/articles/rmshadows/rmshadows.htm
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float mint, in float tmax, float iTime )
{

    float res = 1.0f;
    for( float t=mint; t<tmax; )
    {
        float h = map(ro + rd*t,iTime);
        if( h<0.001f )
            return 0.0f;
        res = _fminf( res, 4.0f*h/t );
        t += h;
    }
    return res;
}

__KERNEL__ void SmokeBallFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
  
  const float3 sundir = to_float3(-1, 1, 0);
  const float3 fog = to_float3(0.242f, 0.334f, 0.42f) * 2.0f;

  float2 mo = swi2(iMouse,x,y)/iResolution;
  float time = 32.0f + iTime * 1.2f;

    // camera
#if ENABLE_MOVEMENT    
    const float dist = 4.5f;
#else
    const float dist = 3.0f;
#endif
    float3 ta = to_float3( 0, 1, 0 );
    float3 ro = ta + to_float3( dist*_cosf(0.1f*time + 7.0f*mo.x), 0.6f + 2.0f*mo.y, dist*_sinf(0.1f*time + 7.0f*mo.x) );
    // camera-to-world transformation
    mat3 ca = setCamera( ro, ta, 0.0f );
    
    float2 p = (2.0f*fragCoord-iResolution)/iResolution.y;
    // focal length
    const float fl = 2.5f;
        
    // ray direction
    float3 rd = mul_mat3_f3(ca , normalize( to_float3_aw(p,fl) ));

    // ray differentials
    float2 px = (2.0f*(fragCoord+to_float2(1.0f,0.0f))-iResolution)/iResolution.y;
    float2 py = (2.0f*(fragCoord+to_float2(0.0f,1.0f))-iResolution)/iResolution.y;
    float3 rdx = mul_mat3_f3(ca , normalize( to_float3_aw(px,fl) ));
    float3 rdy = mul_mat3_f3(ca , normalize( to_float3_aw(py,fl) ));
    
    // raymarch the smoke ball
    float4 col = raymarch(ro, rd, iTime,sundir);

    // raytrace floor plane
    float tp1 = (0.0f-ro.y)/rd.y;
    if( tp1 > 0.0f )
    {
        float3 pos = ro + rd * tp1;
        float3 dpdx = ro.y*(rd/rd.y-rdx/rdx.y);
        float3 dpdy = ro.y*(rd/rd.y-rdy/rdy.y);
        float f = checkersGradBox( 3.0f*swi2(pos,x,z), 3.0f*swi2(dpdx,x,z), 3.0f*swi2(dpdy,x,z) );
        float3 ground = shade(0.15f + f * to_float3_s(0.5f), 5.0f, to_float3(0,1,0), sundir, -rd) * to_float3(1.3f, 1.2f, 1.1f);
        
        float shadow = calcSoftshadow(pos, normalize(sundir), 0.1f, 4.0f,iTime);
        ground = ground * _mix(0.3f, 1.0f, shadow);
        ground = _mix(ground, fog, clamp(tp1 * 0.06f, 0.0f, 1.0f));
        
        swi3S(col,x,y,z, _mix(ground, swi3(col,x,y,z), col.w));
    }
    else
    {
        swi3S(col,x,y,z, _mix(fog, swi3(col,x,y,z), col.w));
    }
    
    float sun = clamp(dot(sundir,rd), 0.0f, 1.0f);
    swi3S(col,x,y,z, swi3(col,x,y,z) + to_float3(1.0f,0.6f,0.1f) * 0.6f * (_powf(sun, 6.0f) * 0.5f + _fmaxf(rd.y * 3.0f, 0.05f)));
     
    // Output to screen
    fragColor = to_float4_aw(pow_f3(swi3(col,x,y,z), to_float3_s(0.4545f)),1.0f);


  SetFragmentShaderComputedColor(fragColor);
}