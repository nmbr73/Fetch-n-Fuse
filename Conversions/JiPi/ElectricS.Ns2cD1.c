
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


// A convenient anti-aliased step() using auto derivatives
__DEVICE__ float aastep(float threshold, float value, float2 fragCoord, float2 iResolution) {
    //float afwidth = 0.7f * length(to_float2(dFdx(value), dFdy(value)));
    float afwidth = 0.7f * length(to_float2((value*fragCoord.x)/iResolution.x, (value*fragCoord.y)/iResolution.y));
    return smoothstep(threshold-afwidth, threshold+afwidth, value);
}

// psrdnoise (c) Stefan Gustavson and Ian McEwan,
// ver. 2021-12-02, published under the MIT license:
// https://github.com/stegu/psrdnoise/
__DEVICE__ float psrdnoise(float2 _x, float2 period, float alpha, out float2 *gradient)
{
  float2 uv = to_float2(_x.x+_x.y*0.5f, _x.y);
  float2 i0 = _floor(uv), f0 = fract_f2(uv);
  float cmp = step(f0.y, f0.x);
  float2 o1 = to_float2(cmp, 1.0f-cmp);
  float2 i1 = i0 + o1, i2 = i0 + 1.0f;
  float2 v0 = to_float2(i0.x - i0.y*0.5f, i0.y);
  float2 v1 = to_float2(v0.x + o1.x - o1.y*0.5f, v0.y + o1.y);
  float2 v2 = to_float2(v0.x + 0.5f, v0.y + 1.0f);
  float2 x0 = _x - v0, x1 = _x - v1, x2 = _x - v2;
  float3 iu, iv, xw, yw;
  //if(any(greaterThan(period, to_float2_s(0.0f)))) {
  if (period.x > 0.0f || period.y > 0.0f) {  
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
  float3 hash = mod_f3(iu, 289.0f);
  hash = mod_f3((hash*51.0f + 2.0f)*hash + iv, 289.0f);
  hash = mod_f3((hash*34.0f + 10.0f)*hash, 289.0f);
  float3 psi = hash*0.07482f + alpha;
  float3 gx = cos_f3(psi); float3 gy = sin_f3(psi);
  float2 g0 = to_float2(gx.x, gy.x);
  float2 g1 = to_float2(gx.y, gy.y);
  float2 g2 = to_float2(gx.z, gy.z);
  float3 w = 0.8f - to_float3(dot(x0, x0), dot(x1, x1), dot(x2, x2));
  w = _fmaxf(w, to_float3_s(0.0f)); float3 w2 = w*w; float3 w4 = w2*w2;
  float3 gdotx = to_float3(dot(g0, x0), dot(g1, x1), dot(g2, x2));
  float n = dot(w4, gdotx);
  float3 w3 = w2*w; float3 dw = -8.0f*w3*gdotx;
  float2 dn0 = w4.x*g0 + dw.x*x0;
  float2 dn1 = w4.y*g1 + dw.y*x1;
  float2 dn2 = w4.z*g2 + dw.z*x2;
  *gradient = 10.9f*(dn0 + dn1 + dn2);
  return 10.9f*n;
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// This is a quick and messy hack. Make of it what you want.

__KERNEL__ void ElectricSFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  float2 st = fragCoord/_fminf(iResolution.x, iResolution.y);
  float time = iTime;
    
  float2 _x, p, gwool, gtemp;
  float nwool, swool, wwool;
  p = to_float2_s(0.0f);
  _x = st;
  swool = 12.0f;
  wwool = 0.5f;
  nwool = 0.0f;
  gwool = to_float2_s(0.0f);
  gtemp = to_float2_s(0.0f);

  for(int i=0; i<4; i++) {
    nwool += wwool*psrdnoise(swool*_x, p, _sqrtf(swool)*0.2f*_sinf(time-nwool), &gtemp);
    gwool += wwool*gtemp;
    wwool *= 0.55f;
    swool *= 2.2f;
  }

  float mwool = 0.5f+0.1f*nwool+0.15f*length(gwool);
  const float3 gray = to_float3_s(0.5f);
  const float3 white = to_float3(1.0f, 1.0f, 1.0f);
  const float3 black = to_float3_s(0.0f);
  const float3 blue = to_float3(0.2f,0.5f,1.0f);

  // Slightly over-saturated on purpose - clips to flat white
  float3 woolcolor = _mix(gray, white, 1.1f*mwool);
    
  float r1 = length(_x-to_float2(0.5f+0.02f*_sinf(time*0.91f), 0.45f+0.02f*_sinf(time*0.7f)));
  float sheep1 = 1.0f-aastep(0.3f, r1+0.02f*nwool+0.01f*length(gwool), fragCoord,iResolution);
  float r2 = length(_x-to_float2(1.3f+0.02f*_sinf(time*0.83f), 0.6f+0.02f*_sinf(time*0.67f)));
  float sheep2 = 1.0f-aastep(0.25f, r2+0.02f*nwool+0.01f*length(gwool), fragCoord,iResolution);
  float sheep = _fmaxf(sheep1, sheep2);
 
  float freqspark = 8.0f;
  float ampspark = 1.0f;
  float nspark = 0.0f;
  float2 g, gspark = to_float2_s(0.0f);

    // Start with two terms of similar frequency to stomp out the
    // regular "beat" of psrdnoise when it's animated rapidly,
    // and then tuck on a fractal sum
  nspark = ampspark*psrdnoise(0.5f*freqspark*_x*0.931f,
           to_float2_s(0.0f), 1.81f*freqspark*time, &g);
  gspark += g*0.5f*ampspark;
  nspark += ampspark*psrdnoise(0.5f*freqspark*_x*1.137f,
            to_float2_s(0.0f), -2.27f*freqspark*time, &g);
  gspark += g*0.5f*ampspark;
  float nflare = nspark; // Save low-frequency part for "lighting effect"
  for (int i=0; i<3; i++) {
      nspark += ampspark*psrdnoise(freqspark*_x-0.2f*gspark,
                to_float2_s(0.0f), 2.0f*freqspark*time, &g);
      gspark += g*ampspark;
      freqspark *=1.82f;
      ampspark *= 0.68f;
  }

  float sparkmask = 1.0f -
                    smoothstep(0.2f, 0.5f, r1+0.02f*nflare) * smoothstep(0.15f, 0.4f, r2+0.02f*nflare);

  // Strongly over-saturated on purpose - clips to cyan and white
  float3 bgcolor = _mix(to_float3(0.0f,0.0f,0.0f), blue, 5.0f*nspark*sparkmask);

  woolcolor = woolcolor + (1.0f-smoothstep(0.5f, 1.2f, sparkmask))*_fmaxf(0.0f, nflare)*blue;
  float3 mixcolor = _mix(bgcolor, woolcolor*(1.0f-_fminf(3.0f*r1*r1,3.0f*r2*r2)), sheep);
  fragColor = to_float4_aw(mixcolor, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}