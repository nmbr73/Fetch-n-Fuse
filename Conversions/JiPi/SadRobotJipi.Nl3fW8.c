
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// sad robot
// @simesgreen

//const int   maxSteps = 64;
//const float hitThreshold = 0.001f;
//const int   shadowSteps = 64;
//const float PI = 3.14159f;

#define maxSteps     64
#define hitThreshold  0.001f
#define shadowSteps  64
#define PI  3.14159f

// CSG operations
__DEVICE__ float _union(float a, float b)
{
    return _fminf(a, b);
}

__DEVICE__ float intersect(float a, float b)
{
    return _fmaxf(a, b);
}

__DEVICE__ float difference(float a, float b)
{
    return _fmaxf(a, -b);
}

// transforms
__DEVICE__ float3 rotateX(float3 p, float a)
{
    float sa = _sinf(a);
    float ca = _cosf(a);
    return to_float3(p.x, ca*p.y - sa*p.z, sa*p.y + ca*p.z);
}

__DEVICE__ float3 rotateY(float3 p, float a)
{
    float sa = _sinf(a);
    float ca = _cosf(a);
    return to_float3(ca*p.x + sa*p.z, p.y, -sa*p.x + ca*p.z);
}

__DEVICE__ float3 rotateZ(float3 p, float a)
{
    float sa = _sinf(a);
    float ca = _cosf(a);
    return to_float3(ca*p.x - sa*p.y, sa*p.x + ca*p.y, p.z);
}

__DEVICE__ mat3 rotationMat(float3 v, float angle)
{
  float c = _cosf(angle);
  float s = _sinf(angle);
  
  return to_mat3(c + (1.0f - c) * v.x * v.x, (1.0f - c) * v.x * v.y - s * v.z, (1.0f - c) * v.x * v.z + s * v.y,
                     (1.0f - c) * v.x * v.y + s * v.z, c + (1.0f - c) * v.y * v.y, (1.0f - c) * v.y * v.z - s * v.x,
                     (1.0f - c) * v.x * v.z - s * v.y, (1.0f - c) * v.y * v.z + s * v.x, c + (1.0f - c) * v.z * v.z
                );
}

// based on gluLookAt
__DEVICE__ mat4 lookAt(float3 eye, float3 center, float3 up)
{
  float3 z = normalize(eye - center);
  float3 y = up;
  float3 x = cross(y, z);
  y = cross(z, x);
  x = normalize(x);
  y = normalize(y);
float zzzzzzzzzzzzzzzzzz;  
  mat4 rm = to_mat4(x.x, y.x, z.x, 0.0f,  // 1st column
                    x.y, y.y, z.y, 0.0f,
                    x.z, y.z, z.z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
  mat4 tm = to_mat4_f(1.0f); //Diagonale "Normalform"
  #ifdef XXX
  mat4 tm = to_mat4(1.0f,0.0f,0.0f,0.0f,
                    0.0f,1.0f,0.0f,0.0f,
                    0.0f,0.0f,1.0f,0.0f,
                    0.0f,0.0f,0.0f,1.0f
                    );
  #endif                  
  //tm[3] = to_float4_aw(-eye, 1.0f);
  tm.r3 = to_float4_aw(-eye, 1.0f);
  return mul_mat4_mat4(rm , tm);
}

// primitive functions
// these all return the distance to the surface from a given point

  // n must be normalized
__DEVICE__ float sdPlane( float3 p, float4 n )
{
  return dot(p,swi3(n,x,y,z)) + n.w;
}

__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3  di = abs_f3(p) - b;
  float mc = _fmaxf(di.x, _fmaxf(di.y, di.z));
  return _fminf(mc,length(_fmaxf(di,to_float3_s(0.0f))));
}

__DEVICE__ float sphere(float3 p, float r)
{
    return length(p) - r;
}

__DEVICE__ float hash( float n )
{
    return fract(_sinf(n)*43758.5453123f);
}

// 1d noise
__DEVICE__ float noise( float p )
{
    float i = _floor( p );
    float f = fract( p );
    float u = f*f*(3.0f-2.0f*f);
    return _mix( hash( i ), hash( i + 1.0f ), u);
}



// distance to scene
__DEVICE__ float scene(float3 p, float iTime, float4 iMouse, mat4 headMat, float3 target)
{    
  float t = iTime*1.5f;
  
  float d;  
  d = sdPlane(p, to_float4(0, 1, 0, 1)); 

  p.y -= _cosf(t*2.0f)*0.1f;  // bounce
  //p.x += _sinf(t)*0.1f;
  
  // head
  float3 hp = p - to_float3(0.0f, 4.0f, 0.0f);
  // rotate head
  hp = swi3(mul_f4_mat4(to_float4_aw(hp, 1.0f) , headMat),x,y,z);

  d = _union(d, sdBox(hp, to_float3(1.5f, 1.0f, 1.0f)));

  // eyes
  float3 eyeScale = to_float3_s(1.0f);
  //eyeScale.y *= 1.0f - _powf(noise(t*10.0f), 10.0f);  // blink
  eyeScale.y *= 1.0f - smoothstep(0.8f, 1.0f, noise(t*5.0f));  // blink
  d = difference(d, sphere((hp - to_float3(0.6f, 0.2f, 1.0f))/eyeScale, 0.15f));
  d = difference(d, sphere((hp - to_float3(-0.6f, 0.2f, 1.0f))/eyeScale, 0.15f));
  
  // mouth
  if (iMouse.z > 0.0f) {
    // surprised mouth
    d = difference(d, sdBox(hp - to_float3(0.0f, -0.4f, 1.0f), to_float3(0.2f, 0.1f, 0.1f)));    
  } else {
    d = difference(d, sdBox(hp - to_float3(0.0f, -0.4f, 1.0f), to_float3(0.25f, 0.05f, 0.1f)));        
  }

  // body
  float3 bp = p;
  bp = rotateY(bp, -target.x*0.05f);
  d = _union(d, sdBox(bp - to_float3(0.0f, 2.0f, 0.0f), to_float3(0.8f, 1.0f, 0.5f)));

  // arms
  //const float arz = -0.1f;
  float arz = -noise(t*0.3f);
  float3 a1 = rotateZ(rotateX(bp- to_float3(1.2f, 2.0f+0.7f, 0.0f), _sinf(t)*0.75f), arz) + to_float3(0, 1.0f, 0);
  float3 a2 = rotateZ(rotateX(bp- to_float3(-1.2f, 2.0f+0.7f, 0.0f), _sinf(t+PI)*0.75f), -arz) + to_float3(0, 1.0f, 0);
  d = _union(d, sdBox(a1, to_float3(0.25f, 1.0f, 0.25f)));
  d = _union(d, sdBox(a2, to_float3(0.25f, 1.0f, 0.25f)));

  // legs
  float3 l1 = rotateX(p - to_float3(0.5f, 1.2f, 0.0f), -_sinf(t)*0.5f) + to_float3(0.0f, 1.2f, 0.0f);
  float3 l2 = rotateX(p - to_float3(-0.5f, 1.2f, 0.0f), -_sinf(t+PI)*0.5f) + to_float3(0.0f, 1.2f, 0.0f);
  d = _union(d, sdBox(l1, to_float3(0.3f, 1.0f, 0.5f)));
  d = _union(d, sdBox(l2, to_float3(0.3f, 1.0f, 0.5f)));
  
  return d;
}

// calculate scene normal
__DEVICE__ float3 sceneNormal(in float3 pos, float iTime, float4 iMouse, mat4 headMat, float3 target )
{
    float eps = 0.0001f;
    float3 n;
    float d = scene(pos, iTime, iMouse, headMat, target);
        n.x = scene( to_float3(pos.x+eps, pos.y, pos.z), iTime, iMouse, headMat, target ) - d;
        n.y = scene( to_float3(pos.x, pos.y+eps, pos.z), iTime, iMouse, headMat, target ) - d;
        n.z = scene( to_float3(pos.x, pos.y, pos.z+eps), iTime, iMouse, headMat, target ) - d;
    return normalize(n);
}

// ambient occlusion approximation
__DEVICE__ float ambientOcclusion(float3 p, float3 n, float iTime, float4 iMouse, mat4 headMat, float3 target)
{
    const int steps = 4;
    const float delta = 0.5f;

    float a = 0.0f;
    float weight = 1.0f;
    for(int i=1; i<=steps; i++) {
        float d = ((float)(i) / (float)(steps)) * delta; 
        a += weight*(d - scene(p + n*d, iTime, iMouse, headMat, target));
        weight *= 0.5f;
    }
    return clamp(1.0f - a, 0.0f, 1.0f);
}

// https://iquilezles.org/articles/rmshadows
__DEVICE__ float softShadow(float3 ro, float3 rd, float mint, float maxt, float k, float iTime, float4 iMouse, mat4 headMat, float3 target)
{
    float dt = (maxt - mint) / (float)(shadowSteps);
    float t = mint;
    t += hash(ro.z*574854.0f + ro.y*517.0f + ro.x)*0.1f;
    float res = 1.0f;
    for( int i=0; i<shadowSteps; i++ )
    {
        float h = scene(ro + rd*t, iTime, iMouse, headMat, target);
    if (h < hitThreshold) return 0.0f;  // hit
        res = _fminf(res, k*h/t);
        //t += h;
    t += dt;
    }
    return clamp(res, 0.0f, 1.0f);
}


// trace ray using sphere tracing
__DEVICE__ float3 trace(float3 ro, float3 rd, out bool *hit, float iTime, float4 iMouse, mat4 headMat, float3 target)
{
    *hit = false;
    float3 pos = ro;
    float3 hitPos = ro;

    for(int i=0; i<maxSteps; i++)
    {
      if (!*hit) {
        float d = scene(pos, iTime, iMouse, headMat, target);
        if (_fabs(d) < hitThreshold) {
          *hit = true;
          hitPos = pos;
        }
        pos += d*rd;
      }
    }
    return pos;
}

// lighting
__DEVICE__ float3 shade(float3 pos, float3 n, float3 eyePos, float iTime, float4 iMouse, mat4 headMat, float3 target)
{
  //vec3 color = to_float3_s(0.5f);
  //const float3 lightDir = to_float3(0.577f, 0.577f, 0.577f);
    //vec3 v = normalize(eyePos - pos);
    //vec3 h = normalize(v + lightDir);
    //float diff = dot(n, lightDir);
    //diff = _fmaxf(0.0f, diff);
    //diff = 0.5f+0.5f*diff;
  
    float ao = ambientOcclusion(pos, n, iTime, iMouse, headMat, target);
  //vec3 c = diff*ao*color;
  
  // skylight
  float3 sky = _mix(to_float3(0.3f, 0.2f, 0.0f), to_float3(0.6f, 0.8f, 1.0f), n.y*0.5f+0.5f);
  float3 c = sky*0.5f*ao;
  
  // point light
  const float3 lightPos = to_float3(5.0f, 5.0f, 5.0f);
  const float3 lightColor = to_float3(0.5f, 0.5f, 0.1f);
  
  float3 l = lightPos - pos;
  float dist = length(l);
  l /= dist;
  float diff = _fmaxf(0.0f, dot(n, l));
  //diff *= 50.0f / (dist*dist);  // attenutation
  
#if 1
  float maxt = dist;
    float shadow = softShadow( pos, l, 0.1f, maxt, 5.0f, iTime, iMouse, headMat, target );
  diff *= shadow;
#endif
  
  c += diff*lightColor;
  
//  return to_float3(ao);
//  return n*0.5f+0.5f;
  return c;
}

__DEVICE__ float3 background(float3 rd)
{
  return _mix(to_float3(0.3f, 0.2f, 0.0f), to_float3(0.6f, 0.8f, 1.0f), rd.y*0.5f+0.5f);
    //return to_float3_s(0.0f);
}



__KERNEL__ void SadRobotJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
  
  float3 target = to_float3_s(0.0f);
  mat4 headMat;

  float2 pixel = (fragCoord / iResolution)*2.0f-1.0f;

  // compute ray origin and direction
  float asp = iResolution.x / iResolution.y;
  float3 rd = normalize(to_float3(asp*pixel.x, pixel.y, -2.0f));
  float3 ro = to_float3(0.0f, 2.0f, 8.0f);

  float2 mouse = swi2(iMouse,x,y) / iResolution;
  float roty = 0.0f; // _sinf(iTime*0.2f);
  float rotx = 0.0f;
  /*
  if (iMouse.z > 0.0f) {
    rotx = -(mouse.y-0.5f)*3.0f;
    roty = -(mouse.x-0.5f)*6.0f;
  }
  */
  
    rd = rotateX(rd, rotx);
    ro = rotateX(ro, rotx);
    
    rd = rotateY(rd, roty);
    ro = rotateY(ro, roty);

  if (iMouse.z > 0.0f) {
    // look at mouse
    target = to_float3_aw(mouse*10.0f-5.0f, -10.0f);
    target.x *= asp;
  } else {
    target = to_float3(noise(iTime*0.5f)*10.0f-5.0f, noise(iTime*0.5f+250673.0f)*8.0f-4.0f, -10.0f);
  }
  headMat = lookAt(to_float3(0.0f, 0.0f, 0.0f), target, to_float3(0.0f, 1.0f, 0.0f));  
    
    // trace ray
    bool hit;
    float3 pos = trace(ro, rd, &hit, iTime, iMouse, headMat, target);

    float3 rgb;
    if(hit) {
        // calc normal
        float3 n = sceneNormal(pos, iTime, iMouse, headMat, target);
        // shade
        rgb = shade(pos, n, ro, iTime, iMouse, headMat, target);

     } else {
        rgb = background(rd);
     }
  
    fragColor=to_float4_aw(rgb, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}