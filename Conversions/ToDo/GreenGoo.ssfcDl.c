
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


// psrdnoise (c) Stefan Gustavson and Ian McEwan,
// ver. 2021-12-02, published under the MIT license:
// https://github.com/stegu/psrdnoise/

__DEVICE__ float psrdnoise(float2 x, float2 period, float alpha, out float2 gradient)
{
  float2 uv = to_float2(x.x+x.y*0.5f, x.y);
  float2 i0 = _floor(uv), f0 = fract(uv);
  float cmp = step(f0.y, f0.x);
  float2 o1 = to_float2(cmp, 1.0f-cmp);
  float2 i1 = i0 + o1, i2 = i0 + 1.0f;
  float2 v0 = to_float2(i0.x - i0.y*0.5f, i0.y);
  float2 v1 = to_float2(v0.x + o1.x - o1.y*0.5f, v0.y + o1.y);
  float2 v2 = to_float2(v0.x + 0.5f, v0.y + 1.0f);
  float2 x0 = x - v0, x1 = x - v1, x2 = x - v2;
  float3 iu, iv, xw, yw;
  if(any(greaterThan(period, to_float2_s(0.0f)))) {
    xw = to_float3(v0.x, v1.x, v2.x);
    yw = to_float3(v0.y, v1.y, v2.y);
    if(period.x > 0.0f)
      xw = mod_f(to_float3(v0.x, v1.x, v2.x), period.x);
    if(period.y > 0.0f)
      yw = mod_f(to_float3(v0.y, v1.y, v2.y), period.y);
    iu = _floor(xw + 0.5f*yw + 0.5f); iv = _floor(yw + 0.5f);
  } else {
    iu = to_float3(i0.x, i1.x, i2.x); iv = to_float3(i0.y, i1.y, i2.y);
  }
  float3 hash = mod_f(iu, 289.0f);
  hash = mod_f((hash*51.0f + 2.0f)*hash + iv, 289.0f);
  hash = mod_f((hash*34.0f + 10.0f)*hash, 289.0f);
  float3 psi = hash*0.07482f + alpha;
  float3 gx = _cosf(psi); float3 gy = _sinf(psi);
  float2 g0 = to_float2(gx.x, gy.x);
  float2 g1 = to_float2(gx.y, gy.y);
  float2 g2 = to_float2(gx.z, gy.z);
  float3 w = 0.8f - to_float3_aw(dot(x0, x0), dot(x1, x1), dot(x2, x2));
  w = _fmaxf(w, 0.0f); float3 w2 = w*w; float3 w4 = w2*w2;
  float3 gdotx = to_float3_aw(dot(g0, x0), dot(g1, x1), dot(g2, x2));
  float n = dot(w4, gdotx);
  float3 w3 = w2*w; float3 dw = -8.0f*w3*gdotx;
  float2 dn0 = w4.x*g0 + dw.x*x0;
  float2 dn1 = w4.y*g1 + dw.y*x1;
  float2 dn2 = w4.z*g2 + dw.z*x2;
  gradient = 10.9f*(dn0 + dn1 + dn2);
  return 10.9f*n;
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__KERNEL__ void GreenGooFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    float2 uv = fragCoord/_fminf(iResolution.x, iResolution.y);

    const float nscale = 8.0f;
    float2 v = nscale*(uv-0.5f);
    const float2 p = to_float2(0.0f, 0.0f);
    float alpha = iTime;
    float2 g, gsum;
     
    float n = 0.5f;
    n += 0.4f * psrdnoise(v, p, alpha, g);
    gsum = g;
    n += 0.2f * psrdnoise(v*2.0f+0.1f*gsum, p*2.0f,
        alpha*2.0f, g);
    gsum += g; // Lower amp, higher freq => same weight
    float3 N = normalize(to_float3_aw(-gsum, 1.0f));
    float3 L = normalize(to_float3(1.0f,1.0f,1.0f));
    float s = _powf(_fmaxf(dot(N,L), 0.0f), 10.0f); // Shiny!
    float3 scolor = to_float3(1.0f,1.0f,1.0f);
    float3 ncolor = n*to_float3(0.5f, 1.0f, 0.2f); // Gooey green

    fragColor = to_float4(_mix(ncolor, scolor, s), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}