
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };

__DEVICE__ float encode_to_float2(float2 v) {
    
    Zahl z;
    
    const float QUANT_COUNT = 65536.0f - 1.0f;
    v = clamp(v, to_float2_s(-3.0f), to_float2_s(3.0f));
    v /= 3.0f;
    v *= 0.5f;
    v += 0.5f;
    v *= QUANT_COUNT;
    
    z._Uint = (uint)(v.x) << 16 | (uint)(v.y);
    
    //return uintBitsToFloat(uint(v.x) << 16 | uint(v.y));
    return (z._Float);
}

__DEVICE__ float2 decode_to_float2(float vi) {
    
    Zahl z;
    
    const float QUANT_COUNT = 65536.0f - 1.0f;
    
    z._Float = vi;
    
    //float2 v = to_float2(floatBitsToUint(vi) >> 16u, floatBitsToUint(vi) & 0xFFFFu);
    float2 v = to_float2(z._Uint >> 16u, z._Uint & 0xFFFFu);
    
    v /= QUANT_COUNT;
    v -= 0.5f;
    v *= 2.0f;
    v *= 3.0f;
    return v;
}

__DEVICE__ float random(float2 uv)
{
    return fract(_sinf(dot(uv,to_float2(12.9898f,78.233f)))*43758.5453123f);
}

#define PI 3.14f

__DEVICE__ float smin ( float a, float b, float k )
{
  float res = _expf ( -k*a ) + _expf ( -k*b );
  return -_logf( res ) / k;
}

__DEVICE__ float4 smin ( float4 a, float4 b, float k )
{
  float4 res = exp_f4( -k*a ) + exp_f4( -k*b );
  return -1.0f*log_f4( res ) / k;
}


__DEVICE__ mat3 rotateX(float theta) {
    float c = _cosf(theta);
    float s = _sinf(theta);
    return to_mat3_f3(
                        to_float3(1, 0, 0),
                        to_float3(0, c, -s),
                        to_float3(0, s, c)
                    );
}

__DEVICE__ mat3 rotateY(float theta) {
    float c = _cosf(theta);
    float s = _sinf(theta);
    return to_mat3_f3(
                        to_float3(c, 0, s),
                        to_float3(0, 1, 0),
                        to_float3(-s, 0, c)
                    );
}

__DEVICE__ mat3 rotateZ(float theta) {
    float c = _cosf(theta);
    float s = _sinf(theta);
    return to_mat3_f3(
                        to_float3(c, -s, 0),
                        to_float3(s, c, 0),
                        to_float3(0, 0, 1)
                    );
}

__DEVICE__ float sphere(float3 pos, float3 spherePos, float sphereRadius) {
    return length(pos - spherePos) - sphereRadius;
}

__DEVICE__ mat2 rotate(float angle)
{
    float c = _cosf(angle);
    float s = _sinf(angle);

    return to_mat2(c, -s, s, c);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void ParticlesSdfJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER0(TDelta, -10.0f, 10.0f, 0.0f);
    fragCoord+=0.5f;

    //float4 particleData = texelFetch(iChannel1, to_int2(fragCoord), 0);
    float4 particleData = texture(iChannel1, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution);
    float2 pos = decode_to_float2(particleData.x);
    float2 vel = decode_to_float2(particleData.y);
    if (pos.y < 0.0f || pos.y > 1.0f) {
        vel.y = -vel.y;
    }
    if (pos.x < 0.0f || pos.x > 1.0f) {
        vel.x = -vel.x;
    }
    
    pos += vel * (iTimeDelta+TDelta) * 0.2f;
    // vel.y -= iTimeDelta * 2.0f;
    particleData.x = encode_to_float2(pos);
    particleData.y = encode_to_float2(vel);
    //uint bufferBegin = uint(texelFetch(iChannel0, to_int2(0, 0), 0));
    uint bufferBegin = (uint)(texture(iChannel0, (make_float2(to_int2(0, 0))+0.5f)/iResolution).x);
    //uint bufferEnd = uint(texelFetch(iChannel0, to_int2(1, 0), 0));
    uint bufferEnd = (uint)(texture(iChannel0, (make_float2(to_int2(1, 0))+0.5f)/iResolution).x);
    
    uint idx = (uint)(fragCoord.x);
    bool commonFill = idx >= bufferBegin && idx < bufferEnd;
    bool overflowFill = bufferBegin > bufferEnd && (idx >= bufferBegin || idx < bufferEnd);
    if (idx >= bufferBegin && idx <= bufferEnd) {
        particleData = to_float4(0, 0, 0, 1);
        particleData.x = encode_to_float2(swi2(iMouse,x,y) / iResolution);
        particleData.y = encode_to_float2(to_float2(random(to_float2(fragCoord.x, 5.0f)), random(fragCoord)) * 2.0f - 1.0f);
        //particleData = to_float4_s(1.0f);
    }
    fragColor = particleData;
    // fragColor = to_float4_aw(_sinf(iTime * 2.0f + fragCoord.x) * 0.1f + 0.5f, _cosf(iTime * 2.0f + fragCoord.x) * 0.1f + 0.5f, 1.0f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void ParticlesSdfJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    fragCoord+=0.5f;

float BBBBBBBBBBBBBBBB;
fragColor = to_float4_s(0.0f);
    uint2 coords = make_uint2(fragCoord.x,fragCoord.y);
    //if (coords.y == 0u && coords.x < 2u) {
    if (fragCoord.y < iResolution.y/2.0 && fragCoord.x < iResolution.x/2.0) {
        if (iMouse.z > 0.0f) {
            float spawnCount = 5.0f;
            //fragColor.x = (float)((int)(texelFetch(iChannel0, to_int2(0, 0), 0).x + spawnCount) % 800);
            fragColor.x = (float)((int)(texture(iChannel0, (make_float2(to_int2(0, 0))+0.5f)/iResolution).x + spawnCount) % 800);
        } else {
            //fragColor.x = texelFetch(iChannel0, to_int2(1, 0), 0).x;
            fragColor.x = texture(iChannel0, (make_float2(to_int2(1, 0))+0.5f)/iResolution).x;
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__DEVICE__ float3 rand3( float2 p, __TEXTURE2D__ iChannel2 ) { return swi3(texture( iChannel2, (p*8.0f+0.5f)/256.0f ),x,y,w); }

__DEVICE__ float rand2(float2 co){
    return fract(_sinf(dot(swi2(co,x,y) ,to_float2(12.9898f,78.233f))) * 43758.5453f);
}

__DEVICE__ uint mod_i32( int i32_bas , int i32_div ){

    float   flt_res =  mod_f( (float)(i32_bas), (float)(i32_div));
    uint    i32_res = (uint)( flt_res );
    return( i32_res );
}

__DEVICE__ float4 randomColor(int i) {
    float r = rand2(to_float2(i, mod_i32(i, 7))) * 0.7f;
    float g = rand2(to_float2(mod_i32(i, 19), i));
    float b = rand2(to_float2(mod_i32(i, 6), mod_i32(i, 13)));
    return to_float4(r, g, b, 1.0f);
}



__DEVICE__ float scene(float3 pos, mat3 m, inout float3 *currentColor, float2 iResolution, __TEXTURE2D__ iChannel0) {
    *currentColor = to_float3_s(0.0f);
    float k = 10000.0f;
    float2 scale = 9.0f * iResolution / _fmaxf ( iResolution.x, iResolution.y ) ;
    float3 q = mul_mat3_f3(m , pos);
    for (int i = 0; i < 800; ++i) {
        float radius = 0.3f;
        //float4 particleData = texelFetch(iChannel0, to_int2(i, 0), 0);
        float4 particleData = texture(iChannel0, (make_float2(to_int2(i, 0))+0.5f)/iResolution);
        if (particleData.w == 0.0f)
            continue;
        float2 pos = decode_to_float2(particleData.x);
        pos = scale * ( pos - to_float2_s ( 0.5f ) );
        float4 color = randomColor(i);
        float dist = sphere(q, to_float3(pos.x, -0.00f, pos.y), radius); 
        k = smin(k, dist, 5.0f);
        *currentColor += 2.0f / (20.0f *dist + 1.0f) * swi3(color,x,y,z);
    }
    //*currentColor = clamp(to_float3_s(0.0f), to_float3_s(1.0f), *currentColor);
    
    return k;
}

__DEVICE__ float3 trace ( in float3 from, in float3 dir, out bool hit, mat3 m, inout float3 *currentColor, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  float3 p         = from;
  float  totalDist = 0.0f;
  
  hit = false;
  
  int stepNum = 70;
  for ( int steps = 0; steps < stepNum; steps++ )
  {
        float dist = scene(p, m, currentColor, iResolution, iChannel0);

        p += dist * dir;
        
    if ( dist < 0.01f )
    {
      hit = true;
      break;
    }
    
    totalDist += dist;
    
    if ( totalDist > 10.0f )
      break;  
  }
  
  return p;
}

__DEVICE__ float3 generateNormal ( float3 z, float d, mat3 m, inout float3 *currentColor, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float eps = 0.01f;
    float e   = _fmaxf (d * 0.5f, eps );
    float dx1 = scene(z + to_float3(e, 0, 0), m, currentColor, iResolution, iChannel0);
    float dx2 = scene(z - to_float3(e, 0, 0), m, currentColor, iResolution, iChannel0);
    float dy1 = scene(z + to_float3(0, e, 0), m, currentColor, iResolution, iChannel0);
    float dy2 = scene(z - to_float3(0, e, 0), m, currentColor, iResolution, iChannel0);
    float dz1 = scene(z + to_float3(0, 0, e), m, currentColor, iResolution, iChannel0);
    float dz2 = scene(z - to_float3(0, 0, e), m, currentColor, iResolution, iChannel0);
    
    return normalize ( to_float3 ( dx1 - dx2, dy1 - dy2, dz1 - dz2 ) );
}




__KERNEL__ void ParticlesSdfJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{

    fragCoord+=0.5f;


    float3 lightPos = to_float3(0, -6.0f, 0.0f);    
    float3 currentColor = to_float3_s(0.0f);

    float2 uv = fragCoord/iResolution;
    float minDist = 1000.0f;
    int minDistParticle = -1;
    
    float border = 0.15f;
    bool doVoronoi = uv.x > border && uv.x < 1.0f - border && uv.y > border && uv.y < 1.0f - border;
    
    for (int i = 0; i < 800; ++i) {
        float radius = 0.01f;
        //float4 particleData = texelFetch(iChannel0, to_int2(i, 0), 0);
        float4 particleData = texture(iChannel0,(make_float2(to_int2(i, 0))+0.5f)/iResolution);
        if (particleData.w == 0.0f) {
            continue;
        }
        float2 pos = decode_to_float2(particleData.x);
        float dist = length(pos - uv);
        if (dist < minDist) {
            minDist = dist;
            minDistParticle = i;
        }
        if (dist < radius)
            fragColor = randomColor(i);
    }
    if (doVoronoi) {
        fragColor = randomColor(minDistParticle);
    }
float IIIIIIIIIIIIIIIIIIIII;    
    /////////////////////////////////////////////////////
   
    //fragColor = to_float4(1.0f - clamp( 0.01f, 1.0f, minDist));
    
        // Normalized pixel coordinates (from 0 to 1)
    
    mat3 m;
    
    m = to_mat3_f3(
        to_float3(1, 0, 0),
        to_float3(0, 1, 0),
        to_float3(0, 0, 1)
    );
    
    float3 cameraPos = to_float3(0.0f,-5.0f, 0.0f);
    float3 cameraForward = to_float3(0.0f, 1, 0.0f);
    float3 cameraUp = to_float3 (0.0f, 0.0f, 1.0f);
    float cameraFocus = 5.0f;
    
    float2 scale = 9.0f * iResolution / _fmaxf ( iResolution.x, iResolution.y ) ;
    uv    = scale * ( fragCoord/iResolution - to_float2_s ( 0.5f ) );

    
    float3 from = to_float3(uv.x, (cameraPos + cameraForward * cameraFocus).z, uv.y);
    float3 dir = normalize(from - cameraPos);
    
    bool hit;
    float3 p1 = trace(cameraPos, dir, hit, m, &currentColor, iResolution, iChannel0);
    float d1 = length(cameraPos - p1);
    float c1 = length(p1);
    
    float3 backgroundColor = to_float3(0.0f, 0.0f, 0.0f);
    float3 surfaceColor = to_float3(1.0f, 1.0f, 1.0f);
    
    float4 col;
    
    if (hit) {

       float3 l = normalize(lightPos - p1);
       float3 n = generateNormal(p1, 0.001f, m, &currentColor, iResolution, iChannel0);
       float3 v = normalize(cameraPos - p1);
       float diffuse = _fmaxf(0.0f, dot(n, l));

       float3  h  = normalize ( l + v );
       float hn = _fmaxf ( 0.0f, dot ( h, n ) );
       float specular = _powf ( hn, 150.0f );

       col = 0.5f*to_float4_s( diffuse ) * to_float4_aw(currentColor, 1.0f) + 0.5f * specular * to_float4 ( 1, 1, 1, 1 );
    }
    else {
       col = to_float4_s(0.0f);
    }
   
    // Output to screen
    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}