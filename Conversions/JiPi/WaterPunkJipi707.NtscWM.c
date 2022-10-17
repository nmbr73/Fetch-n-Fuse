
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/presets/webcam.png' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define MAX_STEPS 40
#define MAX_DIST  40.0f
#define SURF_DIST 0.005f
#define samples   32
#define LOD       2

__DEVICE__ float hash(float2 n) {
    return fract(_sinf(dot(n, to_float2(12.9898f, 4.1414f))) * 184.5453f);
}

__DEVICE__ float noise(float2 n) {
    const float2 d = to_float2(0.0f, 1.0f);
    float2 b = _floor(n), f = smoothstep(to_float2_s(0.0f), to_float2_s(1.0f), fract_f2(n));
    return _mix(_mix(hash(b), hash(b + swi2(d,y,x)), f.x), _mix(hash(b + swi2(d,x,y)), hash(b + swi2(d,y,y)), f.x), f.y);
}

__DEVICE__ float sdSphere( float3 p, float s ) { return length(p)-s; }


__DEVICE__ float smin( float a, float b, float k ) {
    float h = _fmaxf(k-_fabs(a-b), 0.0f);
    return _fminf(a, b) - h*h*0.25f/k; 
}


__DEVICE__ float getDist(float3 p, float iTime) {
    float matId;
    float final = MAX_DIST;
    //float iTime = iTime; 
    p = p - to_float3(0.0f,0.5f, 5.0f);
    p.x = _fabs(p.x);
    float3 tempP = p;
    for (int i = 0; i < 10; i++) {
        float fi = (float)(i + 1)  + _floor((float)(i) / 5.0f);
        float3 pos = p;
        float xmov = -dot(swi2(p,x,y), swi2(tempP,x,y) + swi2(tempP,x,y) * fi * 0.8f) * 3.0f;
        float ymov = sign_f(mod_f(fi, 2.0f)) - dot(swi2(tempP,x,y), swi2(tempP,x,y)) * 0.2f - xmov * 0.2f;

        float2 xy = to_float2(xmov, ymov);
        
        //swi2(pos,x,y) += xy * 0.2f;
        pos.x += xy.x * 0.2f;
        pos.y += xy.y * 0.2f;
        
        swi2S(pos,x,y, swi2(pos,x,y) - noise(swi2(pos,x,y) * 15.0f / fi) * 0.3f);
        swi2S(pos,x,y, swi2(pos,x,y) + (to_float2(_sinf(iTime + fi) * 2.0f, _cosf(iTime / 2.0f - fi) * 0.5f) * 0.1f * fi));

        pos.z += _sinf(iTime * _cosf((float)(i * 4))) * 0.5f;
        float r = _sinf(fi) * 0.2f;
        float n = _fminf(_sinf(pos.z * (float)(i) * 5.0f), _cosf(pos.x * pos.y * (float)(i) * 10.0f)) * 0.1f;
        float bubble = sdSphere(pos + to_float3_s(n) * 0.1f - to_float3_s(0.05f), r);
        final = smin(final, bubble, 0.3f + final * 0.04f);
        
        tempP = pos;
    }
    
    return final;
}

__DEVICE__ float rayMarch(float3 ro, float3 rd, float iTime) {
    float dO=0.0f;
    float matId = -1.0f;

    for(int i=0; i<MAX_STEPS; i++) {
        float3 p = ro + rd*dO;
        float res = getDist(p,iTime);
        float dS = res;
        dO += dS;
        
        if(dO>MAX_DIST || _fabs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

__DEVICE__ float3 normals(float3 p, float of, float iTime ) {
    float d = getDist(p,iTime);
    float2 e = to_float2(of, 0);
    
    float3 n = d - to_float3(
                        getDist(p-swi3(e,x,y,y),iTime),
                        getDist(p-swi3(e,y,x,y),iTime),
                        getDist(p-swi3(e,y,y,x),iTime));
    
    return normalize(n);
}

__DEVICE__ float diffuse(float3 p, float3 n, float3 lp) {
    float3 l = normalize(lp-p);
    float dif = clamp(dot(n, l), 0.0f, 1.0f);

    return dif;
}

__DEVICE__ float specular(float3 rd, float3 ld, float3 n) {    
    float3 reflection = reflect(-ld, n);
    float spec = _fmaxf(dot(reflection, -1.0f*normalize(rd)), 0.0f); 
    return spec;
}

__DEVICE__ float gaussian(float2 i) {
    float zzzzzzzzzzzzzzzzzzzzzzzzzzz;    
    const float sigma = (float)(samples) * 0.25f;
   
    
    //return _expf( -0.5f* dot(i/=sigma,i) ) / ( 6.28f * sigma*sigma );
    return _expf( -0.5f* dot(i/sigma,i) ) / ( 6.28f * sigma*sigma );
}

__DEVICE__ float4 blur(__TEXTURE2D__ sp, float2 U, float2 scale) {
    const int  sLOD = 1 << LOD;
    float4 O = to_float4_s(0);
    int s = samples/sLOD;
    
    for ( int i = 0; i < s*s; i++ ) {
        float2 d = to_float2(i%s, i/s)*(float)(sLOD) - (float)(samples)/2.0f;
        O += gaussian(d) * texture(sp, U + scale * d);
    }
    
    return O / O.w;
}

__KERNEL__ void WaterPunkJipi707Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float3 col = to_float3_s(0.0f);
    
    float _iTime = iTime * 2.0f;

    float3 ro = to_float3(0.0f, 0.0f, 1.0f);
    float3 rd = normalize(to_float3(uv.x, uv.y + 0.2f , 2.0f));
    float3 ld =  to_float3(0.0f, 0.0f, 1.0f);
    float d = rayMarch(ro, rd, _iTime);
    float3 p = ro + rd * d;
    float3 n = normals(p, 0.003f, _iTime);
    float dif = diffuse(p, n, ld); 
    float spec = specular(rd, ld, n) * 0.1f;
    float fresnel = smoothstep(0.5f, 0.2f, dot(-rd, n));
    float3 dispersion = to_float3(noise(swi2(n,x,y) * 2.7f), noise(swi2(n,x,y) * 3.0f), noise(swi2(n,x,y) * 3.3f)) * 0.4f; 
    
    float2 camUV = fragCoord / iResolution;
    float3 cam1 =  swi3(_tex2DVecN(iChannel0,camUV.x,camUV.y,15),x,y,z) * 0.9f;
    camUV += swi2(n,x,y) * 0.05f * dif;
    float3 cam2 = swi3(blur(iChannel0, camUV, to_float2_s(0.002f)),x,y,z) * 0.9f;

    col = dif * cam2;
    col += spec;
    col += cam2 * 0.15f;         
    col += dispersion;
    col += fresnel * 0.2f;
    
    if (d > MAX_DIST) { col = (cam1);  }

    fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}