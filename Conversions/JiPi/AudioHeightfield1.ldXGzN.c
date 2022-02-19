
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/a6a1cf7a09adfed8c362492c88c30d74fb3d2f4f7ba180ba34b98556660fada1.mp3' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Based on video heightfield by @simesgreen, https://www.shadertoy.com/view/Xss3zr

/* To try:
 - adjust range and scale of sound frequencies.
   x use x^2 or something so they're spread more evenly
   - adjust upward - are we missing some at the bottom?
     No, there are no lower frequencies to show, apparently.
*/



// transforms
__DEVICE__ float3 rotateX(float3 p, float a)
{
    float sa = _sinf(a);
    float ca = _cosf(a);
    float3 r;
    r.x = p.x;
    r.y = ca*p.y - sa*p.z;
    r.z = sa*p.y + ca*p.z;
    return r;
}

__DEVICE__ float3 rotateY(float3 p, float a)
{
    float sa = _sinf(a);
    float ca = _cosf(a);
    float3 r;
    r.x = ca*p.x + sa*p.z;
    r.y = p.y;
    r.z = -sa*p.x + ca*p.z;
    return r;
}

__DEVICE__ bool intersectBox(float3 ro, float3 rd, float3 boxmin, float3 boxmax, out float *tnear, out float *tfar)
{
  // compute intersection of ray with all six bbox planes
  float3 invR = 1.0f / rd;
  float3 tbot = invR * (boxmin - ro);
  float3 ttop = invR * (boxmax - ro);
  // re-order intersections to find smallest and largest on each axis
  float3 tmin = _fminf (ttop, tbot);
  float3 tmax = _fmaxf (ttop, tbot);
  // find the largest tmin and the smallest tmax
  float2 t0 = _fmaxf (swi2(tmin,x,x), swi2(tmin,y,z));
  *tnear = _fmaxf (t0.x, t0.y);
  t0 = _fminf (swi2(tmax,x,x), swi2(tmax,y,z));
  *tfar = _fminf (t0.x, t0.y);
  // check for hit
  bool hit;
  if ((*tnear > *tfar)) 
    hit = false;
  else
    hit = true;
  return hit;
}


__DEVICE__ float normalCurve(float x) {
  const float pi = 3.141592653589f;
  // const float e = 2.71828f;
  // return _powf(e, -x*x*0.5f) / _sqrtf(2.0f * pi);
  // Cauchy:
  return 1.0f/(pi * (1.0f + x*x));
}

// return texture coords from 0 to 1
__DEVICE__ float2 worldToTex(float3 p)
{
  float2 uv = swi2(p,x,z)*0.5f+0.5f;
  uv.y = 1.0f - uv.y;
  return uv;
}

__DEVICE__ float h1(float2 uv, __TEXTURE2D__ iChannel0) {
  float band = _powf(uv.x, 2.0f); // _floor(uv.x * bands) / bands;
  float amp = texture(iChannel0, to_float2(band, 0.25f)).x;
  return amp * normalCurve((uv.y - 0.5f) * 5.0f) * 1.5f; //  * (1.0f - _fabs(p.z - 0.5f));
}

// return a value from 0 to 1
__DEVICE__ float heightField(float3 p, __TEXTURE2D__ iChannel0)
{
  float2 uv = worldToTex(p);
  // Get amplitude of the frequency that corresponds to p.x
  return h1(uv, iChannel0);

  // return _sinf(p.x * 4.0f) * _sinf(p.z * 4.0f) * 0.5f + 0.5f;
}

__DEVICE__ bool traceHeightField(float3 ro, float3 rayStep, out float3 *hitPos, __TEXTURE2D__ iChannel0, int _Steps)
{
  float3 p = ro;
  bool hit = false;
  float pH = 0.0f;
  float3 pP = p;
  for(int i=0; i<_Steps; i++) {
    float h = heightField(p,iChannel0);
    if ((p.y < h) && !hit) {
      hit = true;
      //*hitPos = p;
      // interpolate based on height
      *hitPos = _mix(pP, p, (pH - pP.y) / ((p.y - pP.y) - (h - pH)));
    }
    pH = h;
    pP = p;
    p += rayStep;
  }
  return hit;
}

__DEVICE__ float3 background(float3 rd)
{
     return _mix(to_float3(1.0f, 1.0f, 1.0f), to_float3(0.0f, 0.5f, 1.0f), _fabs(rd.y));
}

__KERNEL__ void AudioHeightfield1Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
  const int _Steps = 64;
  const float3 lightDir = to_float3(0.577f, 0.577f, 0.577f);

  const float bands = 30.0f;

  float2 pixel = (fragCoord / iResolution)*2.0f-1.0f;

  // compute ray origin and direction
  float asp = iResolution.x / iResolution.y;
  float3 rd = normalize(to_float3(asp*pixel.x, pixel.y, -2.0f));
  float3 ro = to_float3(0.0f, 0.0f, 2.0f);
    
  float2 mouse = (swi2(iMouse,x,y)) / iResolution - 0.5f;

  // rotate view
  float a;
  a = (0.25f + mouse.y) * 2.0f - 1.0f;
  //= -1.0f;
  rd = rotateX(rd, a);
  ro = rotateX(ro, a);
    
  //a = -(mouse.x)*3.0f;
  a = _sinf(iTime*0.2f-mouse.x);
  rd = rotateY(rd, a);
  ro = rotateY(ro, a);
  
  // intersect with bounding box
  bool hit;  
  const float3 boxMin = to_float3(-1.0f, -0.01f, -1.0f);
  const float3 boxMax = to_float3(1.0f, 0.5f, 1.0f);
  float tnear, tfar;
  hit = intersectBox(ro, rd, boxMin, boxMax, &tnear, &tfar);

  tnear -= 0.0001f;
  float3 pnear = ro + rd*tnear;
  float3 pfar = ro + rd*tfar;
  
  float stepSize = length(pfar - pnear) / float(_Steps);
  
  float3 rgb = background(rd);
  if(hit)
    {
      // intersect with heightfield
      ro = pnear;
      float3 hitPos;
      hit = traceHeightField(ro, rd*stepSize, &hitPos,iChannel0, _Steps);
      if (hit) {
        // rgb = hitPos*0.5f+0.5f;
        
        float2 uv = worldToTex(hitPos);
        // rgb = _tex2DVecN(iChannel0,uv.x,uv.y,15).xyz;
        float amp = h1(uv,iChannel0) * 2.0f;

        // float amp = hitPos.y * 2.0f;
              // Compute hue
        rgb = to_float3(amp, 4.0f * amp * (1.0f - amp), 0.5f * (1.0f - amp));
              // Add white waveform
        float wave = texture(iChannel0, to_float2(uv.x, 0.75f)).x;
        rgb += 1.0f -  smoothstep( 0.0f, 0.01f, _fabs(wave - uv.y));
        // to_float3(amp, amp * 0.7f + 0.2f, amp * 0.5f + 0.2f);
        //vec2 g = gradient(iChannel0, uv, to_float2_s(1.0f) / iResolution);
        //vec3 n = normalize(to_float3(g.x, 0.01f, g.y));
        //rgb = n*0.5f+0.5f;
#if 0
        // shadows
        hitPos += to_float3(0.0f, 0.01f, 0.0f);
        bool shadow = traceHeightField(hitPos, lightDir*0.01f, &hitPos,iChannel0, _Steps);
        if (shadow) {
          rgb *= 0.75f;
        }
#endif      
      }
    }

    fragColor=to_float4_aw(rgb, 1.0f);
  //fragColor = to_float4_aw(to_float3(tfar - tnear)*0.2f, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}