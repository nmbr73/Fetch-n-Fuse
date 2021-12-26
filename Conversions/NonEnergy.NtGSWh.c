

__DEVICE__ float2 sin_f2(float2 i) {float2 r; r.x = _sinf(i.x); r.y = _sinf(i.y); return r;}
__DEVICE__ float3 abs_f3(float3 a) {return (to_float3(_fabs(a.x), _fabs(a.y),_fabs(a.z)));}

#define swizw(V) to_float2((V).z,(V).w)

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------

#define sampler2D __TEXTURE2D__

__DEVICE__ float2 hash( float2 p ) // replace this by something better
{
  p = to_float2( dot(p,to_float2(227.1f,341.7f)),
        dot(p,to_float2(259.5f,283.3f)) );

  return -1.0f + 2.0f*fract_f2(sin_f2(p)*4378.5453123f);
}

__DEVICE__ float2 rand(float2 uv) {
    return normalize(hash(uv));
}

__DEVICE__ float2 computeGive(float3 uv, float2 dir) {
    float2 flow = swixy(uv);
    if (length(flow) == 0.0f) {
        return to_float2_s(0.0f);
    }
    float amount = dot(flow, normalize(dir)) / length(flow);
    //amount = _fmaxf(amount, 0.0f);
    return amount * flow;
}

// mapping edges into a donut shape
__DEVICE__ float3 texDonut(float2 uv, __TEXTURE2D__ iChannel0) {
    return swixyz(_tex2DVecN(iChannel0, fract_f(uv.x),fract(uv.y),15));
}

__DEVICE__ float3 texTiedCylinder(float2 uv, __TEXTURE2D__ iChannel0) {
    float2 velScalar = to_float2_s(1.0f);
    float2 newUv = uv;
    newUv.x = fract(newUv.x);
    if (newUv.y > 1.0f) {
        newUv.x = 1.0f - newUv.x;
        newUv.y = 2.0f - newUv.y;
        velScalar *= -1.0f;
    }
    if (newUv.y < 0.0f) {
        newUv.x = 1.0f - newUv.x;
        newUv.y = -newUv.y;
        velScalar *= -1.0f;
    }
    float3 tex = swixyz(_tex2DVecN(iChannel0, newUv.x,newUv.y,15));
    return to_float3_aw(swixy(tex) * velScalar, tex.z);
}

#define tex texTiedCylinder

// inspired by suture fluid: https://www.shadertoy.com/view/XddSRX
__KERNEL__ void NonEnergyFuse_BufferA(float4 fragColor, float2 fragCoord, float iTime, float3 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    float2 vUv = fragCoord / swixy(iResolution);
    float2 step = 1.0f / swixy(iResolution);
    float step_x = step.x;
    float step_y = step.y;   
    
    float2 n  = to_float2(0.0f, step_y);
    float2 ne = to_float2(step_x, step_y);
    float2 e  = to_float2(step_x, 0.0f);
    float2 se = to_float2(step_x, -step_y);
    float2 s  = to_float2(0.0f, -step_y);
    float2 sw = to_float2(-step_x, -step_y);
    float2 w  = to_float2(-step_x, 0.0f);
    float2 nw = to_float2(-step_x, step_y);

  // get the 3x3 neighborhood.
    float3 uv =    tex(vUv, iChannel0);
    float3 uv_n =  tex(vUv+n, iChannel0);
    float3 uv_e =  tex(vUv+e, iChannel0);
    float3 uv_s =  tex(vUv+s, iChannel0);
    float3 uv_w =  tex(vUv+w, iChannel0);
    float3 uv_nw = tex(vUv+nw, iChannel0);
    float3 uv_sw = tex(vUv+sw, iChannel0);
    float3 uv_ne = tex(vUv+ne, iChannel0);
    float3 uv_se = tex(vUv+se, iChannel0);
    
    // blur
    float3 sum = uv / 4.0f + (uv_n + uv_e + uv_s + uv_w) / 8.0f + (uv_nw + uv_sw + uv_ne + uv_se) / 16.0f;
    
    
    // compute how much you "give" to the other cells, based on which direction you're pointed at
    float2 n_give_me = computeGive(uv_n, s);
    float2 e_give_me = computeGive(uv_e, w);
    float2 s_give_me = computeGive(uv_s, n);
    float2 w_give_me = computeGive(uv_w, e);
    float2 nw_give_me = computeGive(uv_nw, se);
    float2 sw_give_me = computeGive(uv_sw, ne);
    float2 ne_give_me = computeGive(uv_ne, sw);
    float2 se_give_me = computeGive(uv_se, nw);
    
    float2 me_give_n = computeGive(uv, n);
    float2 me_give_e = computeGive(uv, e);
    float2 me_give_s = computeGive(uv, s);
    float2 me_give_w = computeGive(uv, w);
    float2 me_give_nw = computeGive(uv, nw);
    float2 me_give_sw = computeGive(uv, sw);
    float2 me_give_ne = computeGive(uv, ne);
    float2 me_give_se = computeGive(uv, se);
    
    float2 transfer = +(n_give_me + e_give_me + s_give_me + w_give_me) / 4.
              +(nw_give_me + sw_give_me + ne_give_me + se_give_me) / 16.
              -(me_give_n + me_give_e + me_give_s + me_give_w) / 4.
              -(me_give_nw + me_give_sw + me_give_ne + me_give_se) / 16.0f;

    // float transferAmount = _sinf(iTime) * 1.4f;
    float transferAmount = (1.0f - fragCoord.y / iResolution.y) * 1.5f;
    
    float2 newFlow = swixy(sum) + transfer * transferAmount;
    
    // initialize
    if (length(swizw(iMouse) - fragCoord) < 25.0f) {
        float angle = iTime / 1.0f;
        newFlow = to_float2(_cosf(angle), _sinf(angle)) * 15.0f;
        fragColor = to_float4(newFlow.x,newFlow.y, 0.0f, 1.0f);
    } else if (iFrame < 10) {
        fragColor = to_float4(0.0f,0.0f, 0.0f, 1.0f);
        float2 tmp = rand(vUv) * 10.0f;
        fragColor = to_float4(tmp.x,tmp.y, 0.0f, 1.0f);
    } else {
        float max = 100.0f;
        newFlow = length(newFlow) > max ? normalize(newFlow) * max : newFlow;
        // newFlow = normalize(newFlow);
      fragColor = to_float4(newFlow.x,newFlow.y, 0.0f, 1.0f);
    }


  SetFragmentShaderComputedColor(fragColor);
}



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define PI 3.14159265

// https://github.com/hughsk/glsl-hsv2rgb/blob/master/index.glsl
__DEVICE__ float3 hsv2rgb(float3 c) {
  float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
  float3 p = abs_f3(fract(swixxx(c) + swixyz(K)) * 6.0f - swiwww(K));
  return c.z * _mix(swixxx(K), clamp(p - swixxx(K), 0.0f, 1.0f), c.y);
}



__KERNEL__ void NonEnergyFuse(float4 fragColor, float2 fragCoord, float3 iResolution, sampler2D iChannel0)
{

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/swixy(iResolution);

    float3 values = swixyz(_tex2DVecN(iChannel0, uv.x,uv.y,15));
    
    float2 vector = swixy(values);
    float angle = _atan2f(vector.y, vector.x);
    float mag = length(vector) / 10.0f;
    float3 hsv = hsv2rgb(to_float3(angle / (PI*2.0f), 0.9f, mag));
    
    float3 color = hsv;

    // Output to screen
    fragColor = to_float4_aw(color,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}


