
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



// GLOBALS

// position & direction
__DEVICE__ float3 pos_finn, pos_eyes;
//__DEVICE__ float3 dir_eye;
__DEVICE__ mat3 dir_mouth;
__DEVICE__ float3 dir_light;

// coloring and animation
__DEVICE__ float heye, weye, beye;
__DEVICE__ float hmouth, cmouth;
__DEVICE__ float hfinns, htail;
__DEVICE__ float puff;
//__DEVICE__ float time;
__DEVICE__ float tim_tail;
__DEVICE__ float ani_tail, ani_mouth;

// colors
#ifdef XXX
__DEVICE__ float3 col_water = to_float3(0.3f, 0.7f, 1.0f);
__DEVICE__ float3 col_fish_1 = to_float3(1.0f, 0.4f, 0.2f);
__DEVICE__ float3 col_fish_2 = to_float3(1.0f, 0.8f, 0.5f);
__DEVICE__ float3 col_eyes = to_float3(0.7f, 0.75f, 1.0f);
#else
  #define col_water  to_float3(0.3f, 0.7f, 1.0f)
  #define col_fish_1  to_float3(1.0f, 0.4f, 0.2f)
  #define col_fish_2  to_float3(1.0f, 0.8f, 0.5f)
  #define col_eyes  to_float3(0.7f, 0.75f, 1.0f)
#endif

__DEVICE__ float t = 20.0f;



// marching
__DEVICE__ float maxdist = 5.0f;
__DEVICE__ float det = 0.001f;



// USEFUL LITTLE FUNCTIONS

// 2D rotation
__DEVICE__ mat2 rot2D(float a) {
  a = radians(a);
  float s = _sinf(a);
  float c = _cosf(a);
  return to_mat2(c, s, -s, c);
}

// Align vector
__DEVICE__ mat3 lookat(float3 fw, float3 up) {
  fw = normalize(fw);
  float3 rt = normalize(cross(fw, normalize(up)));
  return to_mat3_f3(rt, cross(rt, fw), fw);
}


// Tile fold 
__DEVICE__ float fmod(float p, float c) { return _fabs(c - mod_f(p, c * 2.0f)) / c; }

// Smooth min
__DEVICE__ float smin(float a, float b, float k) {
  float h = clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
  return _mix(b, a, h) - k * h * (1.0f - h);
}

// Smooth max
__DEVICE__ float smax(float a, float b, float k) {
  float h = clamp(0.5f + 0.5f * (a - b) / k, 0.0f, 1.0f);
  return _mix(b, a, h) - k * h * (1.0f - h);
}

// Torus
__DEVICE__ float sdTorus(float3 p, float2 t, float3 s) {
  float uuuuuuuuuuuuuuuuuuuuuu;
  p = swi3(p,y,x,z) * s;
  float2 q = to_float2(length(swi2(p,x,z)) - t.x, p.y);
  return length(q) - t.y;
}


// PUFFY'S SURFACE DISPLACEMENT FUNCTIONS

__DEVICE__ float thorns(float3 p) {
  swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot2D(-25.0f)));
  float s1 = smoothstep(0.0f, 0.7f, -p.x + p.z + 0.6f);
  float s2 = smoothstep(0.15f, 0.3f, length(swi2(p,x,y))) * smoothstep(0.0f, 0.3f, length(swi2(p,y,z)));
  float s3 = smoothstep(0.0f, 0.25f, _fabs(p.y));
  p.x = fmod(_atan2f(p.x, p.y), 0.31459f / 2.0f);
  p.y = fmod(_atan2f(p.y, p.z), 0.31459f / 2.0f);
  swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot2D(25.0f)));
  return _fminf(1.0f, _expf((-3.0f - puff*3.0f) * length(swi2(p,x,y)))) * s1 * s2 * s3;
}

__DEVICE__ float spiral(float3 p, float3 c) {
  p.y = _fabs(p.y);
  float3 pos = p;
  p = mul_mat3_f3(lookat(c, to_float3(0.0f, 1.0f, 0.0f)) , p);
  float a = length(swi2(p,x,y)) * 35.0f;
  swi2S(p,y,x, mul_f2_mat2(swi2(p,y,x) , to_mat2(_sinf(a), _cosf(a), -_cosf(a), _sinf(a))));
  float s=_powf(_fabs(p.x), 2.0f) * smoothstep(0.7f, 1.0f, _fmaxf(0.0f, 1.0f - length(swi2(p,x,y))));
  return s*smoothstep(0.0f,0.05f,pos.z+0.1f);
}

__DEVICE__ float skin(float3 pos) {
  pos *= 2.0f;
  float3 p = pos;
  float m = 1000.0f;
  for (int i = 0; i < 7; i++) {
    p = abs_f3(p) / dot(p, p) - 0.5f;
    m = _fminf(m, length(p));
  }
  return _fmaxf(0.0f, 1.0f - m) * (0.1f + smoothstep(-pos.x + 1.0f, 0.0f, 0.4f)) * 0.003f;
}

// PUFFY'S DE FUNCTIONS

// Body parts

__DEVICE__ float finn(float3 p) {
  p.z += 0.27f;
  p.x += 0.1f;
  p.x *= 1.0f-_powf(smoothstep(0.0f, 0.2f, -p.z),1.5f)*0.3f;
  mat2 ro = rot2D(_cosf(tim_tail*4.0f+(p.x+p.z)*5.0f) *(3.0f-p.x*20.0f));   
  swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , ro));
  swi2S(p,z,y, mul_f2_mat2(swi2(p,z,y) , ro));
  float e = _atan2f(p.x, p.z);
  float o = _sinf(e * 20.0f) * 0.003f;
  float a = 0.19f - p.z * 0.15f;
  float d = _fmaxf(_fabs(p.y + o) - 0.005f, length(swi2(p,x,z)) - a + _cosf(o * 500.0f) * 0.02f);
  d = _fmaxf(p.x - p.z*0.6f, d);
  d = _fmaxf(p.z-p.x*0.3f, d);
  return d * 0.75f;
}


__DEVICE__ float tail(float3 p) {
  p.z += 0.18f;
  p.x += puff * 0.1f;
  p.x += 0.45f + _powf(smoothstep(0.0f, 0.4f, _fabs(p.z)), 5.0f) * 0.1f;
  swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rot2D(_cosf(tim_tail + p.x * 5.0f + p.z * 3.0f) * 25.0f)));
  float e = _atan2f(p.x, p.z);
  float o = _sinf(e * 20.0f) * 0.003f;
  float a = 0.27f - p.z * 0.15f;
  float d = _fmaxf(_fabs(p.y + o) - 0.003f, length(swi2(p,x,z)) - a + _cosf(o * 500.0f) * 0.02f);
  float d1 = smax(p.x - p.z * 0.2f, d, 0.02f);
  d1 = smax(-p.x * 0.4f + p.z, d1, 0.02f);
  float d2 = smax(p.x + p.z * 0.3f, d, 0.02f);
  d2 = smax(-p.x * 0.3f - p.z, d2, 0.02f);
  d = smin(d1, d2, 0.03f);
  return d * 0.7f;
}

__DEVICE__ float finns(float3 p, float time) {
  float amp = (1.0f - puff * 0.3f) * 0.15f;
  float t = time*5.0f + sign_f(p.y) * 0.2f;
  float l = length(p) * 2.0f;
  p.y = _fabs(p.y);
  p += normalize(pos_finn) * (0.28f + puff * 0.05f);
  p*=1.3f;
  p = mul_mat3_f3(lookat(normalize(to_float3(-1.0f, -0.0f, -5.0f)), to_float3(0.0f, 1.0f, 0.0f)) , p);
  amp *= (1.0f + length(p) * 5.0f);
  float a = 0.2f + _cosf(t + _atan2f(p.y, p.z) * 2.0f) * amp * 0.5f;
  float b = 1.2f + puff *1.5f + _sinf(t - amp) * amp;
  swi2S(p,z,x, mul_f2_mat2(swi2(p,z,x) , to_mat2(_sinf(a), _cosf(a), -_cosf(a), _sinf(a))));
  swi2S(p,y,x, mul_f2_mat2(swi2(p,y,x) , to_mat2(_sinf(b), _cosf(b), -_cosf(b), _sinf(b))));
  float e = _atan2f(p.y, p.z);
  float o = _sinf(e * 20.0f) * 0.003f;
  float r = 0.45f - smoothstep(1.0f, 3.0f, _fabs(e)) * 0.25f;
  float d =
      _fmaxf(_fabs(p.x + o) - 0.005f, length(swi2(p,y,z)) - r + _cosf(p.z * 100.0f) * 0.01f) * 0.9f;
  d = _fmaxf(-p.y - p.z * 0.5f, d);
  d = _fmaxf(p.z + p.y * 0.2f, d);
  d = smin(d, length(p) - 0.04f, 0.04f);
  return d * 0.8f;
}

__DEVICE__ float mouth(float3 p) {
  p = mul_f3_mat3(p,dir_mouth);
  float mo = length(swi2(p,y,z) * to_float2(0.35f + ani_mouth * 0.1f-p.z*2.0f, 1.0f)) - 0.02f * (1.0f + ani_mouth * 0.4f);
  return _fmaxf(-p.x, mo);
}

__DEVICE__ float body(float3 p) {
  float m = smoothstep(0.0f, 1.5f, -p.x + 1.3f) * 0.2f;
  float s = smoothstep(0.0f, 1.7f, -p.x);
  p.z -= puff * 0.1f;
  p.z -= smoothstep(0.0f, p.z*0.3f + p.x - 0.6f + ani_mouth * 0.1f,-0.1f)*0.05f;
  p.y *= 1.0f + _powf(_fabs(p.z - 0.2f), 2.0f) * 1.5f;
  p.z *= 1.0f - (p.x + 0.1f) * 0.1f;
  swi2S(p,z,y, swi2(p,z,y) * (1.0f+smoothstep(0.0f,0.5f,-p.x)*0.3f));
  float d = length(p*to_float3(1.0f+smoothstep(0.0f,0.5f,-p.x+p.z)*0.5f,1.0f,1.4f)) - 0.47f - s-puff*0.12f;
  p += to_float3(0.14f + puff * 0.0f, 0.0f, 0.2f);
  p.x -= p.z*0.5f;
  p.z += puff * 0.1f;
  d = smin(d, length(p * to_float3(0.6f, 1.2f, 1.7f)) - 0.55f + m, 0.2f) + 0.1f;
  d+=smoothstep(0.0f,0.7f,-p.x)*0.05f;
  return (d+0.05f) * 0.7f;
}

__DEVICE__ float eye(float3 p) {
  float d = length(p) - 0.13f;
  return d;
}

// Main DE function
__DEVICE__ float de(float3 p, float time) {
float ttttttttttttttttttttttttttttt;  
  beye = 0.0f;
  heye = 0.0f;
  weye = step(0.0f, p.y);
  hmouth = 0.0f;
  hfinns = 0.0f;
  htail = 0.0f;
  p.y *= 1.15f;
  float3 rp = p;
  p.y = _fabs(p.y);
  mat2 rotbod=rot2D(smoothstep(0.0f, 1.3f, -p.x + 0.2f) * ani_tail * 25.0f);
  swi2S(rp,x,y, mul_f2_mat2(swi2(rp,x,y) , rotbod));
  swi2S(rp,z,y, mul_f2_mat2(swi2(rp,z,y) , rotbod));
  float t = time * 10.0f;
  p += sin_f3(p * 20.0f + t) * 0.002f;
  float fi = finn(rp);
  float fis = finns(rp, time);
  float ta = tail(rp);
  float mo = mouth(p);
  float sk = skin(rp);
  float res = (body(rp) - thorns(rp) * (0.01f + puff * 0.1f)) * 0.8f - sk;
  res += spiral(rp, -pos_eyes + to_float3(0.1f, 1.0f, -0.3f))*0.4f;
  rp.y = _fabs(rp.y);
  float eyeh = eye(rp + pos_eyes * 0.9f);
  float eyes = eye(rp + pos_eyes);
  res = smax(res, -mo, 0.013f);
  res = smin(res, eyes, 0.02f);
  res = smin(res, eyeh, 0.035f);
  res = smin(res, fis, 0.02f);
  res = smin(res, fi, 0.02f);
  res = smin(res, ta, 0.03f);
  beye = _fabs(res - eyes);
  heye = 1.0f-step(0.005f, beye);
  hfinns = 1.0f-step(0.005f,_fabs(res-fi));
  hfinns = _fmaxf(hfinns,1.0f-step(0.005f,_fabs(res-fis)));
  htail = 1.0f-step(0.02f, _fabs(res-ta));
  hmouth = 1.0f-step(0.01f, _fabs(res-mo));
  return res;
}

// PUFFY'S COLORING FUNCTIONS

__DEVICE__ float3 color_eyes(float3 p, float3 n, float3 dir_eye) {
  float3 p1 = p + pos_eyes;
  float3 p2 = p + to_float3(pos_eyes.x, -pos_eyes.y, pos_eyes.z);
  float3 l = p1;
  float3 c = to_float3_s(1.0f);
  p1 = mul_mat3_f3(lookat(dir_eye, to_float3(0.0f, 1.0f, 0.5f)) , p1);
  p2 = mul_mat3_f3(lookat(dir_eye, to_float3(0.0f, 1.0f, -0.5f)) , p2);
  p1.y -= 0.01f;
  p2.y += 0.01f;
  c -= smoothstep(0.07f, 0.085f, length(swi2(p1,x,y)) + 1.0f - weye) * (0.4f + col_eyes * 1.5f);
  c -= smoothstep(0.07f, 0.085f, length(swi2(p2,x,y)) + weye) * (0.4f + col_eyes * 1.5f);
  c *= smoothstep(0.03f + _sinf(_atan2f(p1.x, p1.y) * 25.0f) * 0.02f, 0.07f, length(swi2(p1,x,y)) + 1.0f - weye);
  c *= smoothstep(0.03f + _sinf(_atan2f(p2.x, p2.y) * 25.0f) * 0.02f, 0.07f, length(swi2(p2,x,y)) + weye);
  return _mix(c, -1.0f*col_fish_1 - 0.2f, smoothstep(0.0f, 0.0055f, beye));
}

__DEVICE__ float3 color(float3 p, float3 n, float3 dir_eye) {
  float c=0.1f+_fmaxf(0.0f,p.x*3.0f);
  float th=_powf(_fmaxf(0.0f,0.2f-_fabs(thorns(p)))/0.2f,3.0f);
  float3 col = _mix(col_fish_1, col_fish_2, c);
  col=_mix(col_fish_1, col, 0.3f+th*0.7f);
  if (heye > 0.0f)
    col = color_eyes(p, n, dir_eye);
  if (hmouth > 0.0f)
    col = col_fish_2 - 0.03f;
  if (hfinns > 0.0f)
    col = _mix(col_fish_1, col_fish_2 + 0.15f,
              smoothstep(0.37f, 0.5f, length(p+to_float3(0.0f,0.0f,0.05f)) - puff * 0.05f));
  if (htail > 0.0f)
    col = _mix(col_fish_1, col_fish_2 + 0.2f,
              smoothstep(0.6f, 0.75f, length(p) - puff * 0.1f));
  return abs_f3(col);
}

// BACKGROUND AND FOREGROUND FRACTAL

__DEVICE__ float fractal(float3 p, float time) {
  p += _cosf(p.z * 3.0f + time * 4.0f) * 0.02f;
  float depth = smoothstep(0.0f, 6.0f, -p.z + 5.0f);
  p *= 0.3f;
  p = abs_f3(to_float3_s(2.0f) - mod_f3(p + to_float3(0.4f, 0.7f, time * 0.07f), 4.0f));
  float ls = 0.0f;
  float c = 0.0f;
  for (int i = 0; i < 6; i++) {
    p = abs_f3(p) / _fminf(dot(p, p), 1.0f) - 0.9f;
    float l = length(p);
    c += _fabs(l - ls);
    ls = l;
  }
  return 0.15f + smoothstep(0.0f, 50.0f, c) * depth * 4.0f;
}

// NORMALS AND LIGHTING

__DEVICE__ float3 normal(float3 p, float time) {
  float3 e = to_float3(0.0f, det * 2.0f, 0.0f);

  return normalize(to_float3(de(p + swi3(e,y,x,x),time) - de(p - swi3(e,y,x,x),time),
                             de(p + swi3(e,x,y,x),time) - de(p - swi3(e,x,y,x),time),
                             de(p + swi3(e,x,x,y),time) - de(p - swi3(e,x,x,y),time)));
}

__DEVICE__ float shadow(float3 pos, float time) {
  float sh = 1.0f;
  float totdist = det * 30.0f;
  float d = 10.0f;
  for (int i = 0; i < 8; i++) {
    if (d > det) {
      float3 p = pos - totdist * dir_light;
      d = de(p,time);
      sh = _fminf(sh, 20.0f * d / totdist);
      totdist += d;
    }
  }
  return clamp(sh, 0.0f, 1.0f);
}

__DEVICE__ float light(float3 p, float3 dir, float3 n, float shw) {
  float dif = _powf(_fmaxf(0.0f, dot(dir_light, -n)), 3.0f);
  float amb = _powf(_fmaxf(0.0f, dot(dir, -n)), 3.0f);
  return dif * 0.7f * shw + amb * 0.2f + 0.15f;
}

// RAY MARCHING AND SHADING

__DEVICE__ float3 march(float3 from, float3 dir, float time, float3 dir_eye) {
  float3 odir = dir;
  float3 p = from + dir * 2.0f;
  float fg = fractal(p + dir,time) * 0.55f;
  float3 col = to_float3_s(0.0f);
  float totdist = 0.0f;
  float d;
  float v = 0.0f;
  cmouth = 1.0f;
  for (int i = 0; i < 80; i++) {
    p = from + totdist * dir;
    d = de(p,time);
    if (d < det || totdist > maxdist)
      break;
    totdist += d;
    v += _fmaxf(0.0f, 0.1f - d) / 0.1f;
  }
  float fade = smoothstep(maxdist * 0.2f, maxdist * 0.9f, maxdist - totdist);
  float ref = 1.0f;
  float eyes_ref = heye;
  float shw = 1.0f;
  if (d < det * 2.0f) {
    p -= (det - d) * dir;
    float3 n = normal(p,time);
    col = color(p, n, dir_eye) * (0.1f + 0.9f * cmouth);
    shw = shadow(p,time);
    col *= light(p, dir, n, shw);
    from = p - det * dir * 3.0f;
    dir = reflect(dir, n);
    ref = fade * (0.3f * cmouth + eyes_ref * 0.2f);
    col = _mix(col_water * 0.15f, col, fade);
  }
  col *= normalize(col_water + 1.5f) * 1.7f;
  p = maxdist * dir;
  float3 bk = fractal(p,time) * ref * col_water;
  float glow = _powf(_fmaxf(0.0f, dot(dir, -dir_light)), 1.5f+eyes_ref*1.5f);
  float3 glow_water = normalize(col_water+1.0f);
  bk += glow_water*(glow*(1.0f-eyes_ref*0.7f) + _powf(glow, 8.0f) * 1.5f) * shw * cmouth * ref;
  col += v * 0.06f * glow * ref * glow_water;
  col += bk + fg * col_water;
  return col;
}

//********************* MAIN ****************************
__KERNEL__ void PuffyJipi467Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    
  // Set globals
  float time = mod_f(iTime, 600.0f);
  ani_mouth = _sinf(time * 6.0f);
  puff = -0.03f+0.5f*smoothstep(0.945f, 0.95f, _fabs(_sinf(time * 0.1f)))+ani_mouth*0.04f;
  pos_finn = normalize(to_float3(0.35f, -1, 0.0f));
  pos_eyes = to_float3(-1.0f, -1.1f, 1.0f) * 0.12f;
  //pos_eyes*=1.0f+to_float3(-1.0f,1.0f,0.0f)*puff*0.05f;
  dir_light = normalize(to_float3(-0.3f, 0.2f, 1.0f));
  dir_mouth = lookat(normalize(to_float3(-0.4f-puff*0.1f+ani_mouth*0.03f, 0.0f, -1.0f)), to_float3(0.0f, 1.0f, 0.0f));
  tim_tail = time * 2.0f;
  ani_tail = _cosf(tim_tail);
float IIIIIIIIIIIIIIIIIIIIIIIIIIIIIII;
  // Pixel coordinates
  float2 uv = fragCoord / iResolution - 0.5f;
  float2 uv2 = uv;
  float ar = iResolution.x / iResolution.y; 
  uv.x *= ar;

  // Camera
  float2 mouse = (swi2(iMouse,x,y) / iResolution - 0.5f) * 4.0f;
  float tcam = (time+67.0f)*0.05f;
  float zcam = smoothstep(0.7f, 1.0f, _cosf(tcam)) * 1.8f - 0.3f;
  zcam -= smoothstep(0.7f, 1.0f, -_cosf(tcam)) * 1.6f;
  if (iMouse.z < 0.1f) mouse = to_float2(_sinf(time * 0.15f)*ar, zcam);
  float3 dir = normalize(to_float3_aw(uv, 0.9f));
  float3 from = to_float3(1.0f, 0.0f, -0.5f + mouse.y) * 1.25f;
  swi2S(from,x,y, mul_f2_mat2(swi2(from,x,y) , rot2D(-mouse.x * 40.0f)));
  dir = mul_mat3_f3(lookat(normalize(-from+to_float3(_sinf(time*0.5f)*0.3f,_cosf(time*0.25f)*0.1f,0.0f)), to_float3(0.0f, 0.0f, -1.0f)) , dir);

  // Eyes direction
  float3 dir_eye = normalize(from);
  //dir_eye.x = _fmaxf(dir_eye.x, pos_eyes.x - 0.5f);
  dir_eye.y = _fminf(_fabs(dir_eye.y), pos_eyes.y*sign_f(dir_eye.y)+0.5f*sign_f(dir_eye.y));
  dir_eye.z = _fminf(dir_eye.z, pos_eyes.z - 0.5f);

  // March and color
  float3 col = march(from, dir, time, dir_eye);
  col *= to_float3(1.1f, 0.9f, 0.8f);
  col += dot(uv2, uv2) * to_float3(0.0f, 0.6f, 1.0f) * 0.8f;

  // Output to screen
  fragColor = to_float4_aw(col, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}