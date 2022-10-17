
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define PI 3.14159265359f

__DEVICE__ float2 robotAn;
__DEVICE__ float robotH;
__DEVICE__ float3 robotPos;
__DEVICE__ float legLength;

__DEVICE__ mat2 rmatrix(float a)    //Rotation matrix;
{
  float c = _cosf(a);
  float s = _sinf(a);

  return to_mat2(c, -s, s, c);
}

__DEVICE__ float3 getRayDir(float3 cameraDir, float2 coord, float cameraAngle, float2 iResolution)
{
  coord.y /= iResolution.x / iResolution.y;
  float3 xAxis = normalize(to_float3(-cameraDir.z, 0, cameraDir.x)) * _tanf(cameraAngle / 2.0f);
  float3 yAxis = normalize(cross(cameraDir, xAxis)) * _tanf(cameraAngle / 2.0f) * -1.0f;
  float3 result = normalize(cameraDir + xAxis * coord.x + yAxis * coord.y);

  return (result);
}

__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r )
{
  float3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
  return length( pa - ba*h ) - r;
}

__DEVICE__ float sdfStruts(float3 p) //sdf for strut parts
{
    float3 p1;
    
    p1 = to_float3_aw(swi2(p,x,y), _fabs(p.z)) - to_float3(0.3f, -0.2f, 0.16f);
    float strut = length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.40f), p1.z)) - 0.02f;
    
    //--forehead--
    
    p1 = to_float3_aw(swi2(p,x,y), _fabs(p.z)) - to_float3(0.03f, 0.34f, 0.16f);
    
    swi2S(p1,x,y, mul_f2_mat2(swi2(p1,x,y) , rmatrix(PI / 7.0f + PI / 2.0f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.30f), p1.z)) - 0.02f);
    
    //--upper horizontal---
    
    p1 = (p) - to_float3(0.03f, 0.34f, 0.16f);
    
    swi2S(p1,z,y, mul_f2_mat2(swi2(p1,z,y) , rmatrix(-PI / 2.0f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.32f), p1.z)) - 0.02f);
    
    //--lower horizontal---
    
    p1 = (p) - to_float3(0.31f, 0.21f, 0.16f);
    
    swi2S(p1,z,y, mul_f2_mat2(swi2(p1,z,y) , rmatrix(-PI / 2.0f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.32f), p1.z)) - 0.02f);
    
    //--chin horizontal---
    
    p1 = (p) - to_float3(0.31f, -0.21f, 0.16f);
    
    swi2S(p1,z,y, mul_f2_mat2(swi2(p1,z,y) , rmatrix(-PI / 2.0f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.32f), p1.z)) - 0.02f);
    
    //--chin - spine---
    
    p1 = (to_float3_aw(swi2(p,x,y), _fabs(p.z))) - to_float3(0.31f, -0.21f, 0.16f);
    
    swi2S(p1,x,y, mul_f2_mat2(swi2(p1,x,y) , rmatrix(-PI / 1.9f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.42f), p1.z)) - 0.02f);
    
    //--spine - back---
    
    p1 = (to_float3_aw(swi2(p,x,y), _fabs(p.z))) - to_float3(-0.1086f, -0.2446f, 0.16f);
    
    swi2S(p1,x,y, mul_f2_mat2(swi2(p1,x,y) , rmatrix(-PI / 4.9f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.32f), p1.z)) - 0.02f);
    
    //--back - up---
    
    p1 = (to_float3_aw(swi2(p,x,y), _fabs(p.z))) - to_float3(-0.3f, 0.0118f, 0.16f);
    
    swi2S(p1,x,y, mul_f2_mat2(swi2(p1,x,y) , rmatrix(PI / 10.0f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.34f), p1.z)) - 0.02f);
    
    //--up - front---
    
    p1 = to_float3_aw(swi2(p,x,y), _fabs(p.z)) - to_float3(0.03f, 0.34f, 0.16f);
    
    swi2S(p1,x,y, mul_f2_mat2(swi2(p1,x,y) , rmatrix(-PI / 2.0f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.22f), p1.z)) - 0.02f);
    
    //--up - back horizontal---
    
    p1 = (p) - to_float3(0.03f - 0.22f, 0.34f, 0.16f);
    
    swi2S(p1,z,y, mul_f2_mat2(swi2(p1,z,y) , rmatrix(-PI / 2.0f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.32f), p1.z)) - 0.02f);
    
    //--back horizontal---
    
    p1 = (p) - to_float3(-0.3f, 0.0118f, 0.16f);
    
    swi2S(p1,z,y, mul_f2_mat2(swi2(p1,z,y) , rmatrix(-PI / 2.0f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.32f), p1.z)) - 0.02f);
    
     //--spine horizontal---
    
    p1 = (p) - to_float3(-0.1086f, -0.2446f, 0.16f);
    
    swi2S(p1,z,y, mul_f2_mat2(swi2(p1,z,y) , rmatrix(-PI / 2.0f)));
    
    strut = _fminf(strut, length(to_float3(p1.x, p1.y - clamp(p1.y, 0.0f, 0.32f), p1.z)) - 0.02f);
        
    return strut;
}

__DEVICE__ float4 getHead(float3 p, float2 a) // sdf for head
{
    float2 uv = to_float2_s(0.0f);
    float mat = 0.0f;
    float t = 1000000.0f;
    
    float3 p1;
    
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rmatrix(a.x)));
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rmatrix(a.y)));
    
    if (length(p) > 0.7f)  // This "if" is an optimization, which increases fps 3 times
        return to_float4(length(p) - 0.6f, uv.x,uv.y, mat);
    
    //--FOREHEAD--
    
    p1 = p;
    
    swi2S(p1,x,y, mul_f2_mat2(swi2(p1,x,y) , rmatrix(PI / 7.0f)));
    p1 -= to_float3(0.055f, 0.25f, 0.0f);
    
    float forehead = length(_fmaxf(abs_f3(p1) - to_float3(0.145f, 0.03f, 0.16f), to_float3_s(0))) - 0.01f;
    
    t = _fminf(t, forehead);
    
    if (t == forehead)
    {
        if (p1.y > 0.0f && _fabs(p1.x) <= 0.145f && _fabs(p1.z) <= 0.16f)
        {
            uv = swi2(p1,z,x) / to_float2(-0.145f, -0.16f) * to_float2(1.0f, 1.5f);
            mat = 9.0f;
        }
    }
    
    p1 = p;
    p1.y -= 0.1f;
    
    forehead = length(_fmaxf(abs_f3(p1 - to_float3(-0.08f, 0.16f, 0.0f)) - to_float3(0.1f, 0.03f, 0.16f), to_float3_s(0))) - 0.01f;
   
    t = _fminf(t, forehead);
    
    
    if (t == forehead)
        mat = 5.0f;
    
    //--BASE--
    
    float cube = length(_fmaxf(abs_f3(p - to_float3(0.12f, -0.05f, 0.0f)) - to_float3(0.18f, 0.2f, 0.16f), to_float3_s(0))) - 0.01f;
       
    p1 = p;
    
    float2 d = abs_f2(to_float2(length(swi2(p1,x,y)), p1.z)) - to_float2(0.24f, 0.20f);
    
    float cilinder = _fminf(_fmaxf(d.x, d.y), 0.0f) + length(_fmaxf(d, to_float2_s(0.0f))) - 0.01f;
    
    t = _fminf(t, cube);
    t = _fminf(t, cilinder);
    
    if (t == cube)
    {
        mat = 4.0f;
    }
    else if (t == cilinder)
    {
        mat = 3.0f;
        if (_fabs(p.z) < 0.16f)
            mat = 4.0f;
    }
    
    //--STRUTS--
    
    float strut = sdfStruts(p + to_float3(0.0f, 0.05f, 0.0f));
    
    t = _fminf(t, strut);
    
    if (t == strut)
        mat = 6.0f;
    
    //--Display--
    
    float display = length(_fmaxf(abs_f3(p - to_float3(0.32f, -0.05f, 0.0f)) - to_float3(0.01f, 0.16f, 0.16f), to_float3_s(0))) - 0.01f;
   
    t = _fminf(t, display);
    
    if (t == display)
    {
        p1 = p - to_float3(0.32f, -0.05f, 0.0f)
        ;
        if (p1.x > 0.0f && _fabs(p1.y) <= 0.16f && _fabs(p1.z) <= 0.16f)
        {
            mat = 7.0f;
            uv = swi2(p1,z,y) / to_float2_s(0.16f);
        }
        else
        {
            mat = 8.0f;
        }
    }
    
    //--Torus--
    
    p1 = to_float3_aw(swi2(p,x,y), _fabs(p.z)) - to_float3(0.0f, 0.00f, 0.18f);
    
    float3 c = normalize(to_float3_aw(swi2(p1,x,y), 0)) * 0.25f;
    float torus = length(p1 - c) - 0.02f;
    
    t = _fminf(t, torus);
    
    
    if (t == torus)
        mat = 8.0f;
    
    //--axels----
    
    
    p1 = p;
    
    d = abs_f2(to_float2(length(swi2(p1,x,y)), p1.z)) - to_float2(0.15f, 0.22f);
    float cilinder1 = _fminf(_fmaxf(d.x, d.y), 0.0f) + length(_fmaxf(d, to_float2_s(0.0f))) - 0.01f;
    
    t = _fminf(t, cilinder1);
    
    if (t == cilinder1)
    {
        mat = 4.0f;
    }
    
    d = abs_f2(to_float2(length(swi2(p1,x,y)), p1.z)) - to_float2(0.05f, 0.3f);
    float cilinder2 = _fminf(_fmaxf(d.x, d.y), 0.0f) + length(_fmaxf(d, to_float2_s(0.0f))) - 0.01f;
    
    t = _fminf(t, cilinder2);
    
    if (t == cilinder2)
    {
        mat = 8.0f;
    }
    
    float num = 6.0f;
    
    float sector = round(_atan2f(p1.x, p1.y) / (PI * 2.0f / num));
    
    swi2S(p1,x,y, mul_f2_mat2(swi2(p1,x,y) , rmatrix(sector * (PI * 2.0f / num))));
    
    p1.z = _fabs(p1.z);
    
    float tooths = length(_fmaxf(abs_f3(p1 - to_float3(-0.0f, 0.16f, 0.18f)) - to_float3(0.04f, 0.03f, 0.03f), to_float3_s(0))) - 0.01f;
   
    t = _fminf(t, tooths);
    
    
    if (t == tooths)
        mat = 4.0f;
    
    
  return to_float4(t, uv.x,uv.y, mat);
}

__DEVICE__ float4 getLeg(float3 p, float2 an, float k)  // sdf of a leg
{
    float t = 1000000.0f;
    float2 uv = to_float2_s(0.0f);
    float mat = 10.0f;
    
    float3 p1;
    
    if (length(swi2(p,x,z)) > legLength * 0.7f)  // Another optimization
        return to_float4(length(swi2(p,x,z)) - legLength * 0.6f, uv.x,uv.y, mat);
    
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rmatrix(an.x)));
    
    float l = legLength / 2.0f;
    
    float2 a, b, c;
    
    a = to_float2(0.0f, 0.0f);
    
    b = to_float2(-l * _sqrtf(1.0f - k * k), l * k);
    
    c = to_float2(0.0f, 2.0f * l * k);
    
    float2 r;
    
    r = normalize( mul_f2_mat2((b - a) , to_mat2(0.0f, -1.0f, 1.0f, 0.0f))) * 0.07f;
    
    p1 = p;
    p1.z = _fabs(p1.z);
    
    float leg1 = sdCapsule(p1, to_float3_aw(a + r, 0.32f), to_float3_aw(b + r, 0.2f), 0.02f);
    
    t = leg1;
    
    float leg2 = sdCapsule(p1, to_float3_aw(a - r, 0.32f), to_float3_aw(b -r, 0.2f), 0.02f);
    
    t = _fminf(t, leg2);
    
    float leg3 = sdCapsule(p1, to_float3_aw(b, 0.1f), to_float3_aw(c, 0.1f), 0.04f);
    
    t = _fminf(t, leg3);
    
    p1 = p;
    p1.z = _fabs(p1.z);
    p1 -= to_float3_aw(a, 0.32f);
    
    float2 d = abs_f2(to_float2(length(swi2(p1,x,y)), p1.z)) - to_float2(0.1f, 0.02f);
    float joinDown = _fminf(_fmaxf(d.x, d.y), 0.0f) + length(_fmaxf(d, to_float2_s(0.0f))) - 0.01f;
    
    t = _fminf(t, joinDown);
    
    p1 = p;
    p1.z = _fabs(p1.z);
    p1 -= to_float3_aw(b, 0.22f);
    
    d = abs_f2(to_float2(length(swi2(p1,x,y)), p1.z)) - to_float2(0.1f, 0.02f);
    float joinMiddle1 = _fminf(max(d.x, d.y), 0.0f) + length(_fmaxf(d, to_float2_s(0.0f))) - 0.01f;
    
    t = _fminf(t, joinMiddle1);
    
    p1 = p;
    p1 -= to_float3_aw(b, 0.0f);
    
    d = abs_f2(to_float2(length(swi2(p1,x,y)), p1.z)) - to_float2(0.05f, 0.2f);
    float joinMiddle2 = _fminf(_fmaxf(d.x, d.y), 0.0f) + length(_fmaxf(d, to_float2_s(0.0f))) - 0.01f;
    
    t = _fminf(t, joinMiddle2);
        
    return to_float4(t, uv.x,uv.y, mat);
}

__DEVICE__ float4 map(float3 p) 
{
    float2 uv = to_float2_s(0.0f);
    float mat = 0.0f;
    float t = 1000000.0f;
    
    float3 p1;
float zzzzzzzzzzzzzzzzzzzzz;    
    //---Background---
    uv = swi2(p,x,z);
    mat = 0.0f;
    
    float background = p.x - -1.0f;
    t = background;
   
    if (t == background)
    {
        mat = 11.0f;
        uv  = swi2(p,z,y) / 1.6f;
    }
    
    //---Head---
    
    float4 head = getHead(p - robotPos, robotAn);
    
    t = _fminf(t, head.x);
    if (t == head.x)
    {
        uv = swi2(head,y,z);
        mat = head.w;
    }
    
    //---Leg---
    
    float4 leg = getLeg(p - robotPos, robotAn, robotH);
    
    t = _fminf(t, leg.x);
    if (t == leg.x)
    {
        uv = swi2(leg,y,z);
        mat = leg.w;
    }
    
    return to_float4(t, uv.x,uv.y, mat);
}

__DEVICE__ float4 marchRay(float3 rayOrigin, float3 rayDir)  //almost classic raymarching
{
  float t;
  float d = 0.0f;
  float e = 0.00001f;
  float maxRange = 10.0f;
  float3 pos;
  float4 info;

  for (t = 0.0f; t <= maxRange; t += d)
  {
    pos = rayOrigin + rayDir * t;
        
    info = map(pos);
    d = info.x;

    if (d < e)
      break;
  }
  if (t > maxRange)
    return to_float4_s(-1.0f);
    
  info.x = t;
  return info;
}

__DEVICE__ float3 getNorm(float3 pos)
{
  float2 e = to_float2(0.001f, 0);
  float tp = map(pos).x;

  float3 norm = -1.0f*normalize(to_float3(map(pos - swi3(e,x,y,y)).x - tp,
                                     map(pos - swi3(e,y,x,y)).x - tp,
                                     map(pos - swi3(e,y,y,x)).x - tp));
  return (norm);
}

__DEVICE__ float getEyesAnim(float t)
{
    return 1.0f - step(0.02f, _powf(_cosf(0.3f * t) * _cosf(0.5f * t), 16.0f));
}

__DEVICE__ float3 getFaceTex(float2 uv0, float t)
{
    float3 color = to_float3_s(0.2f);
    
    uv0.x += _sinf(t * 1.0f) * _sinf (t * 1.5f) * 0.016f;
    float2 uv = round(uv0 * 15.0f) / 15.0f;
    //uv = uv0;
    
    float2 eyesSize = to_float2(0.3f, 0.05f + 0.25f * getEyesAnim(t));
    float eyes = length((to_float2(_fabs(uv.x), uv.y) - to_float2(0.4f, 0.2f)) / eyesSize);
    
    color = _mix(to_float3(1.0f, 0.0f, 0.0f), color, step(1.0f, eyes));
    
    float2 d = abs_f2(uv + to_float2(0.0f, 0.4f)) - to_float2(0.3f, 0.05f);
    float mouth = length(_fmaxf(to_float2_s(0.0f), d)) + _fminf(max(d.x,d.y), 0.0f);
    
    color = _mix(to_float3(1.0f, 0.0f, 0.0f), color, step(0.0f, mouth));
    
    float b = _fabs(uv0.y - uv.y);//length(uv0.y - uv.y);
    
    color *= to_float3_s(1.0f - b * 20.0f);
    
    return color;
}

__DEVICE__ float3 getForeheadTex(float2 uv)
{
    float3 col = to_float3_s(0.8f);
    
    float2 uv1 = uv - to_float2(-0.4f, 0.0f);
    
    float2 q = uv1 / to_float2(0.3f, 0.5f);
    
    float zero = _powf(_fabs(q.x), 3.0f) + _powf(_fabs(q.y), 3.0f);
    
    float zeroSlash = _fabs(uv1.y - uv1.x * 2.4f);
    
    col = _mix(col, to_float3(1.0f, 0.8f, 0.0f), 1.0f - step(0.4f, _fabs(1.0f - zero)));
    col = _mix(col, to_float3(1.0f, 0.8f, 0.0f), (1.0f - step(0.1f, zeroSlash)) * (1.0f - step(1.0f, zero)));
    
    uv1 = uv - to_float2(0.4f, 0.0f);
    
    float one = length(_fmaxf(abs_f2(uv1) - to_float2(0.01f, 0.5f), to_float2_s(0.0f))) - 0.05f;
    
    uv1 = uv;
    
    uv1 -= to_float2(0.28f, 0.33f);
    uv1 = mul_f2_mat2(uv1,rmatrix(PI / 5.0f));
    
    one = _fminf(one, length(_fmaxf(abs_f2(uv1) - to_float2(0.01f, 0.2f), to_float2_s(0.0f))) - 0.05f);
    
    uv1 = uv - to_float2(0.4f, -0.5f);
    
    one = _fminf(one, length(_fmaxf(abs_f2(uv1) - to_float2(0.25f, 0.01f), to_float2_s(0.0f))) - 0.05f);
    
    col = _mix(col, to_float3(1.0f, 0.8f, 0.0f), 1.0f - step(0.00f, one));
    
    return (col);
}

__DEVICE__ float3 getColor(float4 info, float iTime)
{
   float mat = info.w;
   float2 uv = swi2(info,y,z);
   
   if (mat == 1.0f)
   {
       float3 c1 = to_float3(0, 98, 255) / 255.0f;
       float3 c2 = to_float3(255, 21, 0) / 255.0f;
       
       float k = _sinf(fract(uv.y + uv.x * 1.0f) * PI);
       
       float3 color = _mix(c1, c2, k);
       
       color *= _powf(_fabs(_sinf((uv.y + uv.x * 1.0f) * PI / 2.0f * 32.0f)), 10.0f);
       
       return (color);
   }
   if (mat == 0.0f)
   {
       float2 id = _floor(uv);
       if (mod_f((id.x + id.y), 2.0f) == 0.0f)
           return (to_float3_s(0.5f));
       return to_float3_s(1.0f);
   }
   if (mat == 2.0f)
   {
       float2 id = _floor(uv);
       if (mod_f((id.x + id.y), 2.0f) == 0.0f)
           return (to_float3_s(0.5f));
       return to_float3_s(1.0f);
   }
   if (mat == 3.0f)
       return to_float3(245.0f, 133.0f, 54.0f) / to_float3_s(255.0f);
   if (mat == 4.0f)
       return to_float3_s(0.3f);
   if (mat == 5.0f)
       return to_float3_s(0.95f);
   if (mat == 6.0f)
       return to_float3_s(0.2f);
   if (mat == 7.0f)
       return (getFaceTex(uv, iTime * 10.0f));
   if (mat == 8.0f)
       return to_float3_s(0.6f);
   if (mat == 9.0f)
       return getForeheadTex(uv);
   if (mat == 10.0f)
       return to_float3_s(0.5f);
   if (mat == 11.0f)
   {
       if (_fmaxf(abs_f2(uv).x, abs_f2(uv).y) > 1.0f)
           return to_float3_s(1.0f);
       if (_fmaxf(abs_f2(uv).x, abs_f2(uv).y) > 0.9f)
       {
           float3 c1 = to_float3_s(0.1f);
           float3 c2 = to_float3(1.0f, 1.0f, 0.0f);
           
           float k = uv.y - uv.x;
           
           return _mix(c1, c2, step(0.5f, fract(k * 10.0f)));
       }
       return to_float3_s(0.4f);
   }
   return (to_float3_s(0));
}

__DEVICE__ float getShadow(float3 rayOrigin, float3 rayDir)
{
  float t;
  float d = 0.0f;
  float e = 0.00001f;
  float maxRange = 10.0f;
  float4 info;
  float3 pos;
    
  float res = 1.0f;

  for (t = 0.0f; t <= maxRange; t += d)
  {
    pos = rayOrigin + rayDir * t;
    info = map(pos);
    d = info.x;
    res = _fminf(res, d / t * 64.0f);
    if (d < e)
      break;
  }
        
  return res;
}

__KERNEL__ void RobotPrototype1JipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    float d1 = 2.0f;
    float d2 = 1.5f;
    
    float3 cameraPos;
    float3 cameraDir;
    
    float2 mouseuv = (swi2(iMouse,x,y) - iResolution / 2.0f) / iResolution.y * 2.0f;
float IIIIIIIIIIIIIII;    
    cameraPos = to_float3(d1, 0.0f, 0.0f);
    cameraDir = to_float3(-1.0f, 0.0f, 0.0f);
    
    float3 rayDir = getRayDir(cameraDir, (fragCoord / iResolution) * 2.0f - 1.0f, PI / 2.0f, iResolution);
    float3 rayOrigin = cameraPos;
    
    legLength = 2.0f;
    robotPos = to_float3(0.0f, mouseuv.y * -0.4f, 0.0f);
    robotH = (1.4f - robotPos.y) / legLength;
    
    float3 mousePos = getRayDir(cameraDir, (swi2(iMouse,x,y) / iResolution) * 2.0f - 1.0f, PI / 2.0f, iResolution);;
    
    float3 d = (mousePos - robotPos);
    
    robotAn = to_float2(_atan2f(d.z, d.x) + PI, _atan2f(d.y, d.x) + PI);
    
    
  float4 info = marchRay(rayOrigin, rayDir);
  float t = info.x;

  float3 color = to_float3_s(0);

  if (t != -1.0f)
  {
    float3 pos = rayOrigin + rayDir * t;
    float3 lightDir = normalize(to_float3(2.0f, 0.6f, -3.0f) - pos);
    float3 norm = getNorm(pos);
    float l = 1.0f;
    float mat = info.w;
    
    color = getColor(info, iTime);
    
    if (mat != 7.0f)
    {
      l = 0.3f;
      float st = getShadow(pos + norm * 0.001f, lightDir);

      l += _fmaxf(0.0f, dot(lightDir, norm)) * 0.7f * st;
    }

    color *= l;
  }

  fragColor = to_float4_aw(color, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}