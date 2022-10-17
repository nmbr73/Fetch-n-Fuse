
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel0
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define MAX_STEPS 400
#define MAX_DIST 100.0f
#define EPSILON 0.001f
#define PI 3.14159265f
#define COL1 1.0f
#define COL2 2.0f
#define COL3 3.0f


__DEVICE__ mat2 rot(float a) {float s = _sinf(a), c = _cosf(a);return to_mat2(c, -s, s, c);}
__DEVICE__ float opSmoothUnion( float d1, float d2, float k ) {    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );    return _mix( d2, d1, h ) - k*h*(1.0f-h); }
__DEVICE__ float opSmoothSubtraction( float d1, float d2, float k ) {    float h = clamp( 0.5f - 0.5f*(d1+d2)/k, 0.0f, 1.0f );    return _mix( d1, -d2, h ) + k*h*(1.0f-h); }
__DEVICE__ float hash( float n ) { return fract(_sinf(n)*753.5453123f); }
__DEVICE__ float3 mod289(float3 x) { return x - _floor(x * (1.0f / 289.0f)) * 289.0f; }
__DEVICE__ float2 mod289(float2 x) { return x - _floor(x * (1.0f / 289.0f)) * 289.0f; }
__DEVICE__ float3 permute(float3 x) { return mod289(((x*34.0f)+1.0f)*x); }
__DEVICE__ float snoise(float2 v) {
    // Precompute values for skewed triangular grid
    const float4 C = to_float4(0.211324865405187f,
                               0.366025403784439f,
                               -0.577350269189626f,
                               0.024390243902439f);

    // First corner (x0)
    float2 i  = _floor(v + dot(v, swi2(C,y,y)));
    float2 x0 = v - i + dot(i, swi2(C,x,x));

    // Other two corners (x1, x2)
    float2 i1 = to_float2_s(0.0f);
    i1 = (x0.x > x0.y)? to_float2(1.0f, 0.0f):to_float2(0.0f, 1.0f);
    float2 x1 = swi2(x0,x,y) + swi2(C,x,x) - i1;
    float2 x2 = swi2(x0,x,y) + swi2(C,z,z);

    // Do some permutations to avoid
    // truncation effects in permutation
    i = mod289(i);
    float3 p = permute(
               permute( i.y + to_float3(0.0f, i1.y, 1.0f))
                      + i.x + to_float3(0.0f, i1.x, 1.0f ));

    float3 m = _fmaxf(0.5f - to_float3(
                                      dot(x0,x0),
                                      dot(x1,x1),
                                      dot(x2,x2)
                                      ), to_float3_s(0.0f));

    m = m*m ;
    m = m*m ;

    // Gradients:
    //  41 pts uniformly over a line, mapped onto a diamond
    //  The ring size 17*17 = 289 is close to a multiple
    //      of 41 (41*7 = 287)

    float3 x = 2.0f * fract_f3(p * swi3(C,w,w,w)) - 1.0f;
    float3 h = abs_f3(x) - 0.5f;
    float3 ox = _floor(x + 0.5f);
    float3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt(a0*a0 + h*h);
    m *= 1.79284291400159f - 0.85373472095314f * (a0*a0+h*h);

    // Compute final noise value at P
    float3 g = to_float3_s(0.0f);
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    swi2(g,y,z) = swi2(a0,y,z) * to_float2(x1.x,x2.x) + swi2(h,y,z) * to_float2(x1.y,x2.y);
    return 130.0f * dot(m, g);
}




// ------------------
__DEVICE__ float2 getDist(float3 p, float iTime) {

  // p.x+=10.0f*snoise(swi2(p,y,z)*0.03f+iTime*0.2f) * smoothstep(1.0f, 10.0f, length(p));
  // p.y+=10.0f*snoise(swi2(p,y,z)*0.03f+iTime*0.2f) * smoothstep(1.0f, 10.0f, length(p));
  // p=fract(p+0.5f)-0.5f;

  p.x+=0.03f*snoise(swi2(p,y,z)*2.3f+iTime*0.2f);
  // p.z+=amp.z*snoise(swi2(p,x,y)*1.3f+iTime*0.2f);
  // p.y+=amp.y*snoise(swi2(p,x,z)*1.3f+iTime*0.2f);
  return to_float2((length(p)-0.5f)*0.8f, COL1);
}
// -----------------





__DEVICE__ float4 rayMarch(float3 ro, float3 rd, float iTime) {
  float d = 0.0f;
  float info = 0.0f;
  float glow = 9999.0f;
  int ii=0;
  for (int i = 0; i < MAX_STEPS; i++) {
    ii=i;
    float2 distToClosest = getDist(ro + rd * d, iTime);
    d += _fabs(distToClosest.x);
    info = distToClosest.y;
    glow = _fminf(glow, _fabs(distToClosest.x));
    if(_fabs(distToClosest.x) < EPSILON || d > MAX_DIST) {
      break;
    }
  }
  return to_float4(d, info, ii, glow);
}

__DEVICE__ float3 getNormal(float3 p, float iTime) {
    float2 e = to_float2(EPSILON, 0.0f);
    float3 n = getDist(p,iTime).x - to_float3(getDist(p - swi3(e,x,y,y),iTime).x,
                                              getDist(p - swi3(e,y,x,y),iTime).x,
                                              getDist(p - swi3(e,y,y,x),iTime).x);
  return normalize(n);
}

__DEVICE__ float3 getRayDirection (float3 ro, float2 uv, float3 lookAt) {
    float3 rd;
    rd = normalize(to_float3_aw(uv - to_float2(0, 0.0f), 1.0f));
    float3 lookTo = lookAt - ro;
    float horizAngle = _acosf(dot(swi2(lookTo,x,z), swi2(rd,x,z)) / length(swi2(lookTo,x,z)) * length(swi2(rd,x,z)));
    swi2S(rd,x,z, mul_f2_mat2(swi2(rd,x,z) , rot(horizAngle)));
    return rd;
}

__DEVICE__ float3 getRayDir(float2 uv, float3 p, float3 l, float z) {
    float3 f = normalize(l-p),
        r = normalize(cross(to_float3(0,1,0), f)),
        u = cross(f,r),
        c = f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i);
    return d;
}

union A2F
 {
   float4  F; //32bit float
   float  A[4];  //32bit unsigend integer
 };

__KERNEL__ void SeifenblaserJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float3 p, color=to_float3_s(0.0f),rd,n,ro,ref;
    //float4 rm;
    A2F rm;
    ro=to_float3(0,0,1.5f);
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , rot(iTime)));
    float d=0.0f, info, dtotal=0.0f, steps, marches=0.0f, glow=0.0f;
    rd = getRayDir(uv, ro, to_float3_s(0), 1.0f);
    // rm.F = rayMarch(ro, rd,iTime);
    // d = rm.A[0];
    // info = rm.A[1];
    // steps = rm.A[2];

    float3 light = to_float3(50, 50, 50);
    // n = getNormal(p,iTime);

    // making several marches outside and inside
    // the surface along the ray
    for (int i = 0; i < 5; i++) {
      rm.F = rayMarch(ro, rd,iTime);
      info = rm.A[1];
      glow += rm.A[3];
      // color+=0.00000002f/glow;
      // marches+=1.0f;
      dtotal += d = rm.A[0];
      if (dtotal > MAX_DIST) break;
      
      p = ro + rd * d;
      n = getNormal(p,iTime);
      
      float refK = 7.0f;
      ref = reflect(rd, n);
      color+=refK*swi3(decube_f3(iChannel0,ref),x,y,z);
      marches+=refK;
      
      color+=2.0f*smoothstep(-0.5f,1.0f,dot(ref, rd));
      color+=2.0f*smoothstep(0.6f,1.0f,dot(ref, rd));
      
      float3 amp = to_float3_s(2.3f);
      n.z+=amp.z*snoise(swi2(n,x,y)*0.6f+iTime*0.05f);
      n.x+=amp.x*snoise(swi2(n,y,z)*0.6f+iTime*0.05f);
      n.y+=amp.y*snoise(swi2(n,x,z)*0.6f+iTime*0.05f);
      color+= n*0.5f+0.5f;
      marches++;

      ro = p + rd * 0.05f;
    }
    color/=marches;

    // float3 dirToLight = normalize(light - p);
    // float3 rayMarchLight = rayMarch(p + dirToLight * 0.06f, dirToLight,iTime);
    // float distToObstable = rayMarchLight.x;
    // float distToLight = length(light - p);

    fragColor = to_float4_aw(color,1);

  SetFragmentShaderComputedColor(fragColor);
}