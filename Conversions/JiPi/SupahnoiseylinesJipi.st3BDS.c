
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Created by Fabio Ottaviani
// www.supah.it
// instagram.com/supahfunk


// Simplex 2D noise
//
__DEVICE__ float3 permute(float3 x) { return mod_f3(((x*34.0f)+1.0f)*x, 289.0f); }

__DEVICE__ float s(float2 v){
  const float4 C = to_float4(0.211324865405187f, 0.366025403784439f,
                            -0.577350269189626f, 0.024390243902439f);
  float2 i  = _floor(v + dot(v, swi2(C,y,y)) );
  float2 x0 = v -   i + dot(i, swi2(C,x,x));
  float2 i1;
  i1 = (x0.x > x0.y) ? to_float2(1.0f, 0.0f) : to_float2(0.0f, 1.0f);
  float4 x12 = swi4(x0,x,y,x,y) + swi4(C,x,x,z,z);
  swi2S(x12,x,y, swi2(x12,x,y) - i1);
  i = mod_f(i, 289.0f);
  float3 p = permute( permute( i.y + to_float3(0.0f, i1.y, 1.0f )) + i.x + to_float3(0.0f, i1.x, 1.0f ));
  float3 m = _fmaxf(0.5f - to_float3(dot(x0,x0), dot(swi2(x12,x,y),swi2(x12,x,y)),
                                     dot(swi2(x12,z,w),swi2(x12,z,w))), to_float3_s(0.0f));
  m = m*m ;
  m = m*m ;
  float3 x = 2.0f * fract_f3(p * swi3(C,w,w,w)) - 1.0f;
  float3 h = abs_f3(x) - 0.5f;
  float3 ox = _floor(x + 0.5f);
  float3 a0 = x - ox;
  m *= 1.79284291400159f - 0.85373472095314f * ( a0*a0 + h*h );
  float3 g;
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  swi2S(g,y,z, swi2(a0,y,z) * swi2(x12,x,z) + swi2(h,y,z) * swi2(x12,y,w));
  return 130.0f * dot(m, g);
}

__DEVICE__ float2 rotate(float2 v, float a) {
  float s = _sinf(a);
  float c = _cosf(a);
  mat2 m = to_mat2(c, -s, s, c);
  return mul_mat2_f2(m , v);
}

__DEVICE__ float L(float t, float s, float e, float b){
    float s1 = smoothstep(s - b, s + b, t);
    float s2 = smoothstep(e + b, e - b, t);
    return s1 * s2;
}


__KERNEL__ void SupahnoiseylinesJipiFuse(float4 O, float2 I, float iTime, float2 iResolution)
{

    float2 R = iResolution,
         u = (I-0.5f*R)/R.y;
         
    float3 C = to_float3_s(0.0f);
    float num = 20.0f,
          t = iTime;
    for (float i = 0.0f; i < num; i+=1.0f) {
        float n = i/num;
        float2 uv = u;
        float no = s(uv+_sinf(t+i));
        uv += smoothstep(0.1f,0.4f,length(uv))*no*0.1f;
        uv = rotate(u+to_float2(_sinf(t) * 0.2f, _sinf(t*0.3f) * 0.2f), t*0.5f + no*0.15f + n * 3.14f);
        float3 col = to_float3(n*1.5f, 0.4f + n*0.4f, 0.6f);
        C += L(uv.y, 0.0f, 0.0f, 0.01f + _sinf(0.01f+iTime*3.0f)*0.0013f - length(uv)*0.01f) * 1.5f * col;
    }
    C *= C + C + (1.0f-length(u));
    O = to_float4_aw(C,1.0f);

  SetFragmentShaderComputedColor(O);
}