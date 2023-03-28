
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


//CC0 1.0f Universal https://creativecommons.org/publicdomain/zero/1.0f/
//To the extent possible under law, Blackle Mori has waived all copyright and related or neighboring rights to this work.

__DEVICE__ float comp (float3 p) {
    p = asin(_sinf(p)*0.9f);
    return length(p)-1.0f;
}

__DEVICE__ float3 erot(float3 p, float3 ax, float ro) {
    return _mix(dot(p,ax)*ax,p,_cosf(ro))+_sinf(ro)*cross(ax,p);
}

__DEVICE__ float smin(float a, float b, float k) {
    float h = _fmaxf(0.0f,k-_fabs(b-a))/k;
    return _fminf(a,b)-h*h*h*k/6.0f;
}

__DEVICE__ float4 wrot(float4 p) {
    float3 f(swi3(p,y,z,w) + swi3(p,z,w,y) - swi3(p,w,y,z) - swi3(p,x,x,x));
    return to_float4(dot(p,to_float4_s(1.0)), f.x,f.y,f.z)/2.0f;
//    return to_float4_aw(dot(p,to_float4_s(1.0)), swi3(p,y,z,w) + swi3(p,z,w,y) - swi3(p,w,y,z) - swi3(p,x,x,x))/2.0f;
}

// float bpm = 125.0f;
#define bpm 125.0f;

#define GLOBAL_ARGUMENTS   p2,t,iTime,doodad,lazors,d1,d2,d3
#define GLOBAL_PARAMETERS  out float3 &p2, float t, float iTime, out float &doodad, out float &lazors, out float &d1, out float &d2, out float &d3

__DEVICE__ float scene(float3 p, GLOBAL_PARAMETERS) {
    p2 = erot(p, to_float3(0,1,0), t);
    p2 = erot(p2, to_float3(0,0,1), t/3.0f);
    p2 = erot(p2, to_float3(1,0,0), t/5.0f);

    float bpt = iTime/60.0f*bpm;
        float4 p4 = to_float4_aw(p2,0);
        p4=_mix(p4,wrot(p4),smoothstep(-0.5f,0.5f,_sinf(bpt/4.0f)));
        p4 =_fabs(p4);
        p4=_mix(p4,wrot(p4),smoothstep(-0.5f,0.5f,_sinf(bpt)));
    float fctr = smoothstep(-0.5f,0.5f,_sinf(bpt/2.0f));
    float fctr2 = smoothstep(0.9f,1.0f,_sinf(bpt/16.0f));
        doodad = length(_fmaxf(_fabs(p4)-_mix(0.05f,0.07f,fctr),0.0f)+_mix(-0.1f,0.2f,fctr))-_mix(0.15f,0.55f,fctr*fctr)+fctr2;
    /*
        float4 p4 = to_float4_aw(p2,0);
        p4=wrot(p4);
        p4 = _fabs(p4);
        p4=_mix(p4,wrot(p4),smoothstep(-0.5f,0.5f,_sinf(t)));
        doodad = length(_fmaxf(_fabs(p4)-0.07f,0)+0.2f)-0.55f;
    }*/

    p.x += asin(_sinf(t/80.0f)*0.99f)*80.0f;

    lazors = length(asin(_sinf(erot(p,to_float3(1,0,0),t*0.2f).yz*0.5f+1.0f))/0.5f)-0.1f;
    d1 = comp(p);
    d2 = comp(erot(p+5.0f, normalize(to_float3(1,3,4)),0.4f));
    d3 = comp(erot(p+10.0f, normalize(to_float3(3,2,1)),1.0f));
    return _fminf(doodad,_fminf(lazors,0.3f-smin(smin(d1,d2,0.05f),d3,0.05f)));
}



__DEVICE__ float3 norm(float3 p, GLOBAL_PARAMETERS) {
    float precis = length(p) < 1.0f ? 0.005f : 0.01f;
    mat3 k = mat3(p,p,p)-mat3(precis);

    // scene(float3 p, ) {
    // return normalize(scene(p)-to_float3_aw(scene(k[0]),scene(k[1]),scene(k[2])));

    float3 a(scene(p,GLOBAL_ARGUMENTS));
    float3 b(scene(k[0],GLOBAL_ARGUMENTS),scene(k[1],GLOBAL_ARGUMENTS),scene(k[2],GLOBAL_ARGUMENTS));

    return normalize(a-b);
}

__KERNEL__ void PartyConcertVisuals2020Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
    float d1, d2, d3;
    float t;
    float lazors, doodad;
    float3 p2;



    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;

    float bpt = iTime/60.0f*bpm;
    float bp = _mix(_powf(sin(fract(bpt)*3.14f/2.0f),20.0f)+_floor(bpt), bpt,0.4f);
    t = bp;
    float3 cam = normalize(to_float3(0.8f+_sinf(bp*3.14f/4.0f)*0.3f,uv.x,uv.y));
    float3 init = to_float3(-1.5f+_sinf(bp*3.14f)*0.2f,0,0)+cam*0.2f;
    init = erot(init,to_float3(0,1,0),_sinf(bp*0.2f)*0.4f);
    init = erot(init,to_float3(0,0,1),_cosf(bp*0.2f)*0.4f);
    cam = erot(cam,to_float3(0,1,0),_sinf(bp*0.2f)*0.4f);
    cam = erot(cam,to_float3(0,0,1),_cosf(bp*0.2f)*0.4f);
    float3 p = init;
    bool hit = false;
    float atten = 1.0f;
    float tlen = 0.0f;
    float glo = 0.0f;
    float dist;
    float fog = 0.0f;
    float dlglo = 0.0f;
    bool trg = false;
    for (int i = 0; i <80 && !hit; i++) {
        dist = scene(p,GLOBAL_ARGUMENTS);
        hit = dist*dist < 1e-6;
        glo += 0.2f/(1.0f+lazors*lazors*20.0f)*atten;
        dlglo += 0.2f/(1.0f+doodad*doodad*20.0f)*atten;
        if (hit && ((_sinf(d3*45.0f)<-0.4f && (dist!=doodad )) || (dist==doodad && _sinf(pow(length(p2*p2*p2),0.3f)*120.0f)>0.4f )) && dist != lazors) {
            trg = trg || dist==doodad;
            hit = false;
            float3 n = norm(p,GLOBAL_ARGUMENTS);
            atten *= 1.0f-_fabs(dot(cam,n))*0.98f;
            cam = reflect(cam,n);
            dist = 0.1f;
        }
        p += cam*dist;
        tlen += dist;
        fog += dist*atten/30.0f;
    }
    fog = smoothstep(0.0f,1.0f,fog);
    bool lz = lazors == dist;
    bool dl = doodad == dist;
    float3 fogcol = _mix(to_float3(0.5f,0.8f,1.2f), to_float3(0.4f,0.6f,0.9f), length(uv));
    float3 n = norm(p,GLOBAL_ARGUMENTS);
    float3 r = reflect(cam,n);
    float ss = smoothstep(-0.3f,0.3f,scene(p+to_float3_s(0.3f),GLOBAL_ARGUMENTS))+0.5f;
    float fact = length(_sinf(r*(dl?4.:3.))*0.5f+0.5f)/_sqrtf(3.0f)*0.7f+0.3f;
    float3 matcol = _mix(to_float3(0.9f,0.4f,0.3f), to_float3(0.3f,0.4f,0.8f), smoothstep(-1.0f,1.0f,_sinf(d1*5.0f+iTime*2.0f)));
    matcol = _mix(matcol, to_float3(0.5f,0.4f,1.0f), smoothstep(0.0f,1.0f,_sinf(d2*5.0f+iTime*2.0f)));
    if (dl) matcol = _mix(to_float3_s(1.0f),matcol,0.1f)*0.2f+0.1f;
    float3 col = matcol*fact*ss + _powf(fact,10.0f);
    if (lz) col = to_float3_s(4.0f);
    swi3(fragColor,x,y,z) = col*atten + glo*glo + fogcol*glo;

    swi3(fragColor,x,y,z) = _mix(swi3(fragColor,x,y,z), fogcol, fog);
    if(!dl)swi3(fragColor,x,y,z) = _fabs(erot(swi3(fragColor,x,y,z), normalize(_sinf(p*2.0f)),0.2f*(1.0f-fog)));
    if(!trg&&!dl)swi3(fragColor,x,y,z)+=dlglo*dlglo*0.1f*to_float3(0.4f,0.6f,0.9f);
    swi3(fragColor,x,y,z) = _sqrtf(swi3(fragColor,x,y,z));
    swi3(fragColor,x,y,z) = smoothstep(to_float3_s(0.0f),to_float3_s(1.2f),swi3(fragColor,x,y,z));

    fragColor.w=1.0f;
  SetFragmentShaderComputedColor(fragColor);
}