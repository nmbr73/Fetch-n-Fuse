
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


//#define load(frag) texelFetch(iChannel0, frag, 0)
#define load(frag) _tex2DVecN(iChannel0, ((float)(frag.x)+0.5f)/iR.x,((float)(frag.y)+0.5f)/iR.y,15 )

#define JOINTS 10
#define SPACING 0.1
#define GRAVITY 0.05

#define BOUNDARY_FRICTION 1.0
#define JOINT_FRICTION 0.85

#define dt 0.2

__DEVICE__ float4 restrain(in float4 p, in float2 a, in float2 b) {
    if (p.x < a.x) {
        p.x = a.x;
        if (p.z < 0.0f) p.z = -BOUNDARY_FRICTION * p.z;
    }

    if (p.x > b.x) {
        p.x = b.x;
        if (p.z > 0.0f) p.z = -BOUNDARY_FRICTION * p.z;
    }

    if (p.y < a.y) {
        p.y = a.y;
        if (p.w < 0.0f) p.w = -BOUNDARY_FRICTION * p.w;
    }

    if (p.y > b.y) {
        p.y = b.y;
        if (p.w > 0.0f) p.w = -BOUNDARY_FRICTION * p.w;
    }

    return p;
}

// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float Hash11(in float x) {
    x = fract_f(x * 0.1031f);
    x *= x + 33.33f;
    x *= x + x;
    return fract_f(x);
}

__DEVICE__ float snoise(in float x) {
    return _mix(Hash11(_floor(x)), Hash11(_ceil(x)), smoothstep(0.0f, 1.0f, fract_f(x)));
}

__DEVICE__ float fbm(in float x, in float scale, in int octaves) {
    x *= scale;

    float value = 0.0f;
    float nscale = 1.0f;
    float tscale = 0.0f;

    for (int o=0; o < octaves; o++) {
        value += snoise(x) * nscale;
        tscale += nscale;
        nscale *= 0.5f;
        x *= 2.0f;
    }

    return value / tscale;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Buffer A' to iChannel0


__KERNEL__ void ChainThinFuse__Buffer_A(float4 state, float2 addr, float2 iResolution, float4 iMouse, int iFrame)
{

  //CONNECT_TINYSLIDER0(MausX,0.0f,800.0f,400.0f); // Name der 'float' Variable, Min, Max, und Default-Wert (Default wird hier nicht, aber spaeter in der Fuse verwendet)
  //CONNECT_TINYSLIDER1(MausY,0.0f,450.0f,225.0f);
  CONNECT_TINYSLIDER0(Ich_mach_Blau,0.0f,1000.0f,0.0f); // Name der 'float' Variable, Min, Max, und Default-Wert (Default wird hier nicht, aber spaeter in der Fuse verwendet)
  CONNECT_TINYSLIDER1(slider,0.0f,1000.0f,0.0f);  
    
    float2 iR = iResolution;
    
    state = to_float4_s(0.0f);
    int2 iAddr = to_int2_cfloat(addr);
    if (iFrame == 0) {
        if (iAddr.x < JOINTS && iAddr.y == 0) {
            state.x = -addr.x / 50.0f;
            if (iAddr.x == 0) state.z = 0.01f;
        }
    }

    if (iFrame > 0) {
        if (iAddr.x < JOINTS && iAddr.y == 0) {
            float2 mouse = (swi2(iMouse,x,y) - 0.5f * iResolution) / iResolution.y;
            
            // kommt nicht an, aber gibt auch kein Compilerfehler
            //float2 mouse = (to_float2(MausX,MausY) - 0.5f * iResolution) / iResolution.y;
            //float2 mouse = (to_float2(Ich_mach_Blau,slider) - 0.5f * iResolution) / iResolution.y;
            
            mouse /= 2.5f; //warum auch immer - keine Idee mehr 
            mouse -= to_float2(0.5f,0.25); //warum auch immer - keine Idee mehr 
            
            float2 corner = to_float2(0.5f * iResolution.x / iResolution.y, 0.5f) - 0.02f;

            float4 joints[JOINTS];
            for (int n=0; n < JOINTS; n++) {
                joints[n] = load(to_int2(n, 0));
            }

            //joints[0].xy += (mouse - joints[0].xy) * 0.1f;
            joints[0].x += (mouse.x - joints[0].x) * 0.1f;
            joints[0].y += (mouse.y - joints[0].y) * 0.1f;
            
            joints[0] = restrain(joints[0], -corner, corner);

            for (int n=1; n < JOINTS; n++) {
                //joints[n].zw.y -= GRAVITY * dt;
                //joints[n].w -= GRAVITY * dt;
                joints[n].w -= GRAVITY * dt;
                
                //joints[n].xy += joints[n].zw * dt;
                joints[n].x += joints[n].z * dt;
                joints[n].y += joints[n].w * dt;

                float4 next = load(to_int2(iAddr.x - 1, 0));
                float2 toNext = swi2(joints[n - 1],x,y) - swi2(joints[n],x,y);

                float2 dirToNext = normalize(toNext);
                float2 tangent = to_float2(-dirToNext.y, dirToNext.x);
                float2 joints_zw = tangent * dot(swi2(joints[n],z,w), tangent) * JOINT_FRICTION;
                joints[n].z=joints_zw.x;joints[n].w=joints_zw.y;

                float2 joints_xy = swi2(joints[n - 1],x,y) - normalize(toNext) * SPACING;
                joints[n].x=joints_xy.x;joints[n].y=joints_xy.y;
                
                joints[n] = restrain(joints[n], -corner, corner);
            }

            state = joints[iAddr.x];
        }
    }


  SetFragmentShaderComputedColor(state);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Buffer A' to iChannel0


__DEVICE__ float sdLine(in float2 p, in float2 a, in float2 b) {
    float2 pa = p - a, ba = b - a;
    return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0f, 1.0f));
}

#define drawSDF(dist, color) fragColor = to_float4_aw(_mix(swi3(fragColor,x,y,z), color, smoothstep(unit, 0.0f, dist)),fragColor.w)
__KERNEL__ void ChainThinFuse(float4 fragColor, float2 fragCoord, float2 iResolution)
{


    float2 iR = iResolution;
    
    float2 uv = (fragCoord - 0.5f * iResolution) / iResolution.y;
    float unit = 2.0f / iResolution.y;
    fragColor = to_float4_s(1.0f);

    float2 joints[JOINTS];
    for (int n=0; n < JOINTS; n++) {
        joints[n] = swi2(load(to_int2(n, 0)),x,y);
    }

    float2 prev = joints[0];
    drawSDF(length(uv - prev) - 0.01f, to_float3_s(0.0f));
    for (int n=1; n < JOINTS; n++) {
        float2 cur = joints[n];
        drawSDF(length(uv - joints[n]) - 0.01f, to_float3_s(0.0f));
        drawSDF(sdLine(uv, prev, cur) - 0.001f, to_float3_s(0.0f));
        prev = cur;
    }


  SetFragmentShaderComputedColor(fragColor);
}