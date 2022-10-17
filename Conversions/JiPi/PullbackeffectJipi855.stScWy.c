
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/pablo-andrioli/bolacircuitera-version-bien-1' to iChannel0


#define time (iChannelTime[0]+1.2f)

#define resolution iResolution

#define rot(a) mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))
#define sm(a,b) smoothstep(a,b,time)
#define hash(p) fract(_sinf(dot(p, to_float2(12.9898f, 78.233f))) * 43758.5453f)

float st, det = 0.01f, t, sc = 0.0f, on = 0.0f, tr = 1.0f, mat = 0.0f, y;
float3 col = to_float3(0), carpos, cardir, pal, glcol;
float2 pf1, pf2, pf3, e = to_float2(0, 0.001f);


__DEVICE__ mat3 lookat(float3 dir) {
    dir = normalize(dir);
    float3 rt = normalize(to_float3(-dir.z, 0, dir));
    return mat3(rt, cross(rt, dir), dir);
}


__DEVICE__ float is(float s) {
    return step(_fabs(sc - s), 0.1f);

}

__DEVICE__ float3 path(float tt) {
    float3 p = to_float3(_sinf(tt * 0.5f + _cosf(tt * 0.2f)) + _sinf(tt * 0.1f), 5.0f, tt);
    tt += 70.0f * step(time, 80.0f);
    p.y -= smoothstep(290.0f, 280.0f, tt) * 10.0f + smoothstep(270.0f, 265.0f, tt) * 5.0f + smoothstep(240.0f, 235.0f, tt) * 5.0f;
    p.x *= 0.5f + tr * 0.5f;
    p.x *= sm(57.0f, 55.0f) + sm(89.0f, 91.0f);
    return p;
}

__DEVICE__ float3 carpath(float t) {
    float3 p = path(t);
    p.y += 1.0f - tr + sm(52.0f, 55.0f) * 4.0f * sm(82.0f, 80.0f);
    p.x *= sm(55.0f, 52.0f) + sm(105.0f, 107.0f);
    p.z -= 375.0f;
    swi2(p,x,z) *= rot(-sm(75.0f, 80.0f) * 3.1416f);
    p.z += 375.0f;
    if (time > 89.0f) p = path(p.z);
    return p;
}

__DEVICE__ float3 fractal(float2 p)
{
    p = _fabs(fract(p * 0.05f) - 0.5f);
    float ot1 = 1000.0f, ot2 = ot1;
    for (float i = 0.0f; i < 6.0f; i++) {
        p = _fabs(p) / clamp(_fabs(p.x * p.y), 0.15f, 5.0f) - to_float2(1.5f, 1);
        ot1 = _fminf(ot1, _fabs(p.x) + step(fract(time * 0.7f + float(i) * 0.2f), 0.5f * p.y));
        ot2 = _fminf(ot2, length(p));
    }
    ot1 = smoothstep(0.1f, 0.05f, ot1);
    return time < 75.0f ? to_float3_aw(p, 0) * ot2 * ot1 * 0.3f + ot1 * 0.3f : to_float3(p.x, -1, p.y * 0.5f) * ot2 * ot1 * 0.3f + ot1 * 0.3f; 
}


__DEVICE__ float map(float2 p) {
    if (y > 10.0f) return 0.0f;
    float2 ppp = swi2(p,y,x); ppp.x -= 311.0f;
    float3 pa = path(p.y);
    float h = 0.0f;
    p.x -= pa.x * sm(24.0f, 25.0f);
    float d = _floor(p.y * 3.0f) / 3.0f - carpos.z - sm(52.0f, 57.0f) * 20.0f;
    p.x *= 1.0f + smoothstep(0.0f, 2.0f, d) * 2.0f * is(1.0f);
    pf1 = p;
    if (time < 24.0f) {
        p -= swi2(carpos,x,z);
        p *= rot(-0.5f * time / _fmaxf(0.5f, _floor(length(p))));
        pf2 = to_float2((_atan2f(p.x, p.y) / 3.1416f), length(p));
        return pa.y - 0.5f - _floor(length(p)) * sm(18.0f, 17.0f) * (sm(5.0f, 8.0f) - 0.5f) * 0.7f;
    }
    float b = step(300.0f + step(75.0f, time) * 46.0f, p.y);
    p = _floor(p * 3.0f) / 3.0f;
    pf3 = p;
    h += hash(p) * 3.0f * sm(24.0f, 26.0f) * (1.0f - b * 0.9f) * sm(550.0f, 500.0f);
    if (sc > 1.0f) p = _floor(p), h += (clamp(hash(p + 0.1f), 0.75f, 1.0f) - 0.75f) * 20.0f;
    if (time > 22.0f && b < 0.5f) h *= smoothstep(0.5f, 5.0f - d, _fabs(p.x) * 1.5f) * (sc > 1.0f ? 2.0f : 1.0f); // barre
    h += pa.y - 0.5f;
    return h;
}

__DEVICE__ float de(float3 p) {
    p -= carpos;
    st = 0.1f;
    float bound = length(p * to_float3(1, 2, 1)) - 3.0f + tr;
    if (bound > 0.0f) return bound + 5.0f;
    p = lookat(cardir * to_float3(0.5f, 0, 1)) * p;
    swi2(p,x,y) *= rot(_sinf(time * 1.5f) * 0.2f + cardir.x);
    swi2(p,y,z) *= rot(t * 1.5f * step(0.2f, tr));
    swi2(p,x,z) *= rot(0.5f * tr);
    float mat1 = _expf(-0.8f * length(_sinf(p * 6.0f)));
    float d1 = length(p) - 0.5f;
    float d = 100.0f;
    swi2(p,x,y) *= rot(smoothstep(0.3f, 0.5f, _fabs(p.x)) * sign(p.x) * 0.2f);
    p.y *= 1.2f + smoothstep(0.3f, 0.4f, _fabs(p.x));
    p.x *= 1.0f - _fminf(0.7f, _fabs(p.z - 0.4f));
    p.z += smoothstep(0.0f, 0.6f, p.x * p.x);
    p.z -= smoothstep(0.1f, 0.0f, _fabs(p.x)) * 0.5f * _fminf(p.z, 0.0f);
    d = length(p) - 0.5f;
    d += _fabs(p.y) * smoothstep(0.6f, 0.3f, _fabs(p.x));
    p.y += 5.0f;
    d = _mix(d, d1, _sqrtf(tr));
    mat = _mix(_expf(-0.8f * length(_sinf(p * 6.3f))), mat1, tr) + step(_fabs(p.x), 0.03f) * 0.1f;
    mat *= _fminf(1.0f, on * 4.0f);
    if (d < 2.0f) st = 0.05f;
    return d * 0.6f;
}

__DEVICE__ float4 hit(float3 p) {
    float h = map(swi2(p,x,z)), d = de(p);
    return to_float4(p.y < h, d < det * 2.0f, h, d);
}

__DEVICE__ float3 bsearch(float3 p, float3 dir) {
    float ste = st*-0.5f;
    for (float h2 = 1.0f, i = h2; i++ < 21.0f;)
    {
        p += dir * ste;
        float4 hi = hit(p);
        float h = _fmaxf(hi.x, hi.y);
        if (_fabs(h - h2) > 0.001f) {
            ste *= -0.5f;
            h2 = h;
        }
    }
    return p;
}

__DEVICE__ float3 march(float3 from, float3 dir) {
    float3 p, cl = to_float3_s(0.0f), pr = p;
    float td = 2.0f + hash(swi2(dir,x,y) + time) * 0.1f, g = 0.0f, eg = 0.0f, ref = 0.0f;
    p = from + td * dir;
    float4 h;
    for (int i = 0; i < 300; i++) {
        p += dir * st;
        y = p.y;
        td += st;
        h = hit(p);
        if (h.y > 0.5f && ref == 0.0f) {
            pr = p;
            ref = 1.0f;
            p -= 0.2f * dir;
            for (int i = 0; i < 20; i++) {
                float d = de(p) * 0.5f;
                p += d * dir;
                if (d < det) break;
            }
            dir = reflect(dir, normalize(to_float3(de(p + swi3(e,y,x,x)), de(p + swi3(e,x,y,x)), de(p + swi3(e,x,x,y))) - de(p)));
            p += hash(swi2(dir,x,y) + time) * 0.1f * dir;
        }
        g = _fmaxf(g, _fmaxf(0.0f, 0.2f - h.w) / 0.2f) * mat;
        eg += 0.01f / (0.1f + h * h * 20.0f).w * mat;
        if (h.x > 0.5f || td > 25.0f || (h.y > 0.5f && mat > 0.4f)) break;
    }
    if (h.x > 0.9f) {
        p -= dir * det;
        p = bsearch(p, dir);
        float3 ldir = normalize(p - (carpos + to_float3(0.0f, 2.0f, 0.0f)));
        float3 n = normalize(to_float3(map(swi2(p,x,z) + swi2(e,y,x)) - map(swi2(p,x,z) - swi2(e,y,x)), 2.0f * e.y, map(swi2(p,x,z) + swi2(e,x,y)) - map(swi2(p,x,z) - swi2(e,x,y))));;
        n.y *= -1.0f;
        float cam = _fmaxf(0.2f, dot(dir, n)) * step(on, 0.9f - is(3.0f)) * 0.8f;
        cl = (_fmaxf(cam * 0.3f, dot(ldir, n)) * on + cam) * 0.8f * pal;
        float dl = length(swi2(p,x,z) - swi2(carpos,x,z)) * 1.3f * (1.0f - sm(52.0f, 55.0f) * 0.5f);
        cl *= _fminf(0.8f, _expf(-0.15f * dl * dl));
        cl += (fractal(pf1) * sm(20.0f, 22.0f) + fractal(pf2 * 5.0f) * sm(25.0f, 23.0f) + fractal(pf3 * 0.2f) * 2.0f * float(1.0f < sc) * -n.y + fractal(swi2(p,x,y)).g * n.z * 2.0f * is(2.0f) + 0.7f * step(_fabs(pf1.x), 0.3f) * step(0.7f, fract(pf1.y * 4.0f)) * step(pf1.y, 292.0f)
            * step(1.5f, sc)) * _expf(-0.3f * dl) * 0.7f;
        mat = 0.0f;
    }
    else {
        cl = pal * ref * 0.3f + smoothstep(7.0f, 0.0f, length(swi2(p,x,z))) * 0.13f;
    }
    if (td > 25.0f) cl = fractal(swi2(p,x,z) * 0.2f) * _fmaxf(0.0f,dir.y);
    cl = _mix(cl, to_float3_aw(ref), 0.2f * ref) + _expf(-0.3f * length(p + to_float3(0, 17, -157))) * glcol * 5.0f * is(3.0f);
    p -= carpos;
    if (time > 80.0f && time < 89.0f && length(p) < 2.0f) cl += fractal(swi2(p,z,x)*2.0f);
    return cl + (g + eg) * glcol;
}

__DEVICE__ float4 main2()
{
    float2 uv = swi2(gl_FragCoord,x,y) / swi2(resolution,x,y) - 0.5f; uv.x *= 1.8f;
    tr = sm(50.0f, 48.0f) + sm(86.4f, 89.0f);
    on = sm(14.0f, 15.0f) * _fabs(_sinf(time * 0.7f)) * 0.6f - fract(_sinf(time) * 10.0f) * step(20.0f, time);
    if (time > 21.0f) on = 1.0f;
    if (time > 110.0f) on = step(time, 114.3f) * _fabs(_sinf(time * 8.0f));
    pal = _mix(to_float3(0.6f, 1, 0.5f) * 0.75f, to_float3(1, 0.5f, 0.5f), sm(74.0f, 75.0f));
    glcol = _mix(to_float3(0.5f, 1, 0.5f) * on * 0.8f, to_float3(1.5f, 0.5f, 0.5f), sm(74.0f, 75.0f));
    t = (_fmaxf(21.2f, time) - (time - 114.5f) * sm(90.0f, 116.5f))*5.0f;
    float3 from = carpath(t - 2.0f);
    float3 cam = to_float3(-4.0f, 4.0f, 2.0f);
    if (time < 28.0f) swi2(cam,x,z) *= rot(-_fmaxf(8.0f, time) * 0.7f + 2.5f);
    if (time > 28.0f) from = carpath(t - 2.0f), cam = to_float3(-3, 4, -2), sc = 1.0f;
    if (time > 35.0f) from = carpath(t + 4.0f), cam = to_float3(0, 3, 0);
    if (time > 41.5f) from = carpath(t - 3.0f), cam = to_float3(1, 4.0f - tr * 2.0f, 0);
    if (time > 52.0f) from = carpath(t + 5.0f), cam = to_float3(_sinf(time) * 3.0f, 4, 0);
    if (time > 55.0f) from = carpath(t + 5.0f), cam = _mix(cam, to_float3(-5, 6, -8), sm(55.0f, 58.0f)), sc = 2.0f;
    if (_fabs(time - 67.0f) < 3.0f) from = path(68.0f * 5.0f), cam = to_float3(4, 3, -0.5f);
    if (time > 85.0f) sc = 3.0f;
    cam.z += sm(77.0f, 78.0f) * 4.0f;
    cam.y *= 0.5f + 0.5f * sm(87.0f, 85.0f);
    cam.x += sm(90.0f, 92.0f) * 10.0f;
    cam = _mix(cam, to_float3(3, 2, 10), sm(105.0f, 110.0f));
    from += cam;
    carpos = carpath(t);
    float3 carpos2 = carpath(t + 1.0f * (1.0f - is(0.0f)));
    from = _mix(from, carpos2 + cam * 0.8f, sm(93.0f, 95.0f));
    cardir = normalize(carpath(t + 1.0f) - carpos);
    return march(from, lookat(normalize(carpos2 - from)) * normalize(to_float3(uv, 1.2f + sm(85.0f, 86.0f) - sm(90.0f, 91.0f)))).rgbr * sm(1.0f, 3.0f) * sm(117.5f, 115.5f);
}



__KERNEL__ void PullbackeffectJipi855Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iChannelTime[])
{

    fragColor = main2();


  SetFragmentShaderComputedColor(fragColor);
}