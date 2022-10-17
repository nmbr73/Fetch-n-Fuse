
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------


__DEVICE__ float rand(float2 p){
    p = 50.0f*fract_f2(p*0.3183099f + to_float2(2.424f, -3.145f));
    return fract(p.x*p.y*(p.x+p.y));
}

__DEVICE__ float noise(float2 _x){
    float2 p = _floor(_x);
    float2 w = fract_f2(_x);
    float2 u = w*w*w*(w*(w*6.0f-15.0f)+10.0f);
    
    float2 h = to_float2(1,0);
    float a = rand(p+swi2(h,y,y));
    float b = rand(p+swi2(h,x,y));
    float c = rand(p+swi2(h,y,x));
    float d = rand(p+swi2(h,x,x));
    
    return -1.0f + 2.0f*(a + (b-a)*u.x + (c-a)*u.y + (a - b - c + d)*u.x*u.y);
}

__DEVICE__ float sat(float _x){
    return clamp(_x, 0.0f, 1.0f);
}

__DEVICE__ float fbm(float2 p, int oct, float lac, float gain){
    p += to_float2(0.0f, 15.0f);
    
    mat2 r = to_mat2(3.0f, -4.0f, 4.0f, 3.0f); //0.2f*
    
    float f = 0.0f;
    float s = 1.0f;
    for(int i = 0; i < oct; i++){
        f += noise(p) * s;
        p = lac*mul_mat2_f2(r,p) * 0.2f;
        s *= gain;
    }
    
    return f;
}

__DEVICE__ float fbm(float2 p){
    return fbm(p, 12, 2.0f, 0.45f);
}

__DEVICE__ float fbm2(float2 p){
    return fbm(p, 4, 2.0f, 0.45f);
}

__DEVICE__ float smin( float a, float b, float k )
{
    float res = _exp2f( -k*a ) + _exp2f( -k*b );
    return -_log2f( res )/k;
}

__KERNEL__ void PlanetattemptJipi309Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution)
{
      
    CONNECT_POINT2(offA, 0.0f, 0.0f);
    
    CONNECT_SLIDER7(Mask, -10.0f, 10.0f, 0.1f);
    CONNECT_SLIDER8(Mul1, -10.0f, 10.0f, 0.1f);
    CONNECT_SLIDER9(Mul2, -10.0f, 10.0f, 0.5f);
  
    fragCoord+=0.5f;

    float2 uv = fragCoord - 0.5f*iResolution;
    uv /= iResolution;
    //uv.x *= iResolution.x / iResolution.y;

    // generate the terrain texture
    /*float elevation = fbm(uv*10.0f+273.0f);
    elevation *= 0.7f;*/
    float elevation = fbm((uv+offA)*10.0f+273.0f, 8, 2.0f, 0.4f);
    //float mask = _fmaxf(elevation-0.1f, 0.0f);
    float mask = _fmaxf(elevation-Mask, 0.0f);
    elevation = smin(elevation, 0.2f, 32.0f);
    elevation += fbm(uv*20.0f+273.0f, 12, 2.0f, 0.5f) * Mul1;//0.1f;
    //elevation += _fmaxf(fbm((uv+offA)*16.0f+273.0f, 12, 2.0f, 0.5f), 0.0f) * 0.5f * mask;
    elevation += _fmaxf(fbm((uv+offA)*16.0f+273.0f, 12, 2.0f, 0.5f), 0.0f) * Mul2 * mask;

    
    // generate the forest areas
    float forest = fbm2(uv*8.0f+to_float2_s(0.5f));
    forest = sat(forest);
    
    fragColor = to_float4(elevation, forest, 0.0f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


/*
    Use the mouse to rotate around.
*/

#define MAX_RM_ITER 128.0f

__DEVICE__ float mdot(float3 u, float3 v){
    return _fmaxf(0.0f, dot(u, v));
}

__DEVICE__ mat2 rot(float a){
    float c = _cosf(a);
    float s = _sinf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float fbmT(float2 p, __TEXTURE2D__ iChannel0){
    return _tex2DVecN(iChannel0,p.x,p.y,15).x;
}

__DEVICE__ float fbmF(float2 p, __TEXTURE2D__ iChannel0){
      
    return _tex2DVecN(iChannel0,p.x,p.y,15).y;
}

__DEVICE__ float trimapTerrain(float3 p, float3 n, __TEXTURE2D__ iChannel0, float aspect, float4 muloffT, float angle){
    float a = angle;//0.29f;
    float c = _cosf(a);
    float s = _sinf(a);
    mat2 r = to_mat2(c, -s, s, c);
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , r)); // attempt to break symetries
    
    //p = 0.25f*p+0.5f;
    p = muloffT.w*p+0.5f+swi3(muloffT,x,y,z);
    
    //float fx = fbmT(swi2(p,y,z),iChannel0,aspect);
    //float fy = fbmT(swi2(p,x,z),iChannel0,aspect);
    //float fz = fbmT(swi2(p,x,y),iChannel0,aspect);
    
    
    float fx = fbmT(to_float2(p.y*aspect,p.z),iChannel0);
    float fy = fbmT(to_float2(p.x*aspect,p.z),iChannel0);
    float fz = fbmT(to_float2(p.x*aspect,p.y),iChannel0);
    
    n = abs_f3(n);
    
    return dot(n, to_float3(fx, fy, fz));
}

__DEVICE__ float trimapForest(float3 p, float3 n, __TEXTURE2D__ iChannel0, float aspect, float4 muloffF){
    
    //p = 0.25f*p+0.5f;
    p = muloffF.w*p+0.5f+swi3(muloffF,x,y,z);
    
    //float fx = fbmF(swi2(p,y,z),iChannel0,aspect);
    //float fy = fbmF(swi2(p,x,z),iChannel0,aspect);
    //float fz = fbmF(swi2(p,x,y),iChannel0,aspect);
    
    float fx = fbmF(to_float2(p.y*aspect,p.z),iChannel0);
    float fy = fbmF(to_float2(p.x*aspect,p.z),iChannel0);
    float fz = fbmF(to_float2(p.x*aspect,p.y),iChannel0);
    
    n = abs_f3(n);
    
    return dot(n, to_float3(fx, fy, fz));
}



__DEVICE__ float2 sphereItsc(float3 ro, float3 rd, float r){
    float b = dot(ro, rd);
    float d = b*b - dot(ro, ro) + r*r;
    if(d < 0.0f) {
        return to_float2(-1.0f, -1.0f);
    };
    return to_float2(-b-_sqrtf(d), -b+_sqrtf(d));
}

__DEVICE__ float2 seaItsc(float3 ro, float3 rd, float radius)
{
    return sphereItsc(ro, rd, radius);
}

__DEVICE__ float2 atmoItsc(float3 ro, float3 rd, float atmRadius){
    return sphereItsc(ro, rd, atmRadius);
}


__DEVICE__ float sdf(float3 p, float radius, __TEXTURE2D__ iChannel0, float aspect, float4 muloffT, float angle){
    float sd = length(p) - radius;
    
    float3 n = normalize(p);
    
    float dsp = trimapTerrain(p, n, iChannel0, aspect, muloffT, angle);
    dsp *= 0.1f;
    
    return sd - dsp;
}

__DEVICE__ float3 raymarch(float3 ro, float3 rd, float radius, __TEXTURE2D__ iChannel0, float aspect, float4 muloffT, float angle){
    const float eps = 0.0001f;
    const float k = 0.65f;
    float i = 0.0f;
    float t = 0.0f;
    float dmin = 100000.0f;
    float3 p = ro;
    for(; i < MAX_RM_ITER; i++){
        float d = sdf(p,radius, iChannel0, aspect, muloffT, angle);
        dmin = _fminf(32.0f*d / t, dmin);
        if(d < eps){
            break;
        }
        p += k*d*rd;
        t += k*d;
    }
    return to_float3(t, i, dmin);
}

__DEVICE__ float3 normal(float3 p, float radius, __TEXTURE2D__ iChannel0, float aspect, float4 muloffT, float angle){
    float2 h = to_float2(0.001f, 0.0f);
    return normalize(to_float3(
        sdf(p+swi3(h,x,y,y),radius,iChannel0, aspect, muloffT, angle) - sdf(p-swi3(h,x,y,y),radius,iChannel0, aspect, muloffT, angle),
        sdf(p+swi3(h,y,x,y),radius,iChannel0, aspect, muloffT, angle) - sdf(p-swi3(h,y,x,y),radius,iChannel0, aspect, muloffT, angle),
        sdf(p+swi3(h,y,y,x),radius,iChannel0, aspect, muloffT, angle) - sdf(p-swi3(h,y,y,x),radius,iChannel0, aspect, muloffT, angle)
    ));
}

__DEVICE__ float specRef(float3 n, float3 ldir, float3 rd, float a){
    float3 r = reflect(-ldir, n);
    return _powf(mdot(r, -rd), a);
}

__DEVICE__ float3 terrainCol(float3 p, float3 n, float radius, __TEXTURE2D__ iChannel0, float aspect, float4 muloffF, float3 TerrainCol[7]){
    float3 col = to_float3_s(0.0f);
    float h = length(p);
    float r = radius;
    float t = smoothstep(r-0.1f, r, h);
    
    //col = _mix(to_float3(0.6f, 0.5f, 0.4f), to_float3(0.9f, 1.0f, 0.5f), t);
    col = _mix(TerrainCol[0], TerrainCol[1], t);
    
    t = smoothstep(r, r+0.01f, h);
    float f = trimapForest(p, n, iChannel0, aspect, muloffF);
    f = smoothstep(0.1f, 0.5f, f);
    
    //float3 green = _mix(to_float3(0.4f, 0.6f, 0.1f), to_float3(0.0f,0.4f,0.3f), f);
    float3 green = _mix(TerrainCol[2], TerrainCol[3], f);
    
    col = _mix(col, green, t);
    
    t = smoothstep(r+0.02f, r+0.05f, h);
    //col = _mix(col, to_float3(0.7f, 0.5f, 0.3f), t);
    col = _mix(col, TerrainCol[4], t);
    
    float3 up = normalize(p);
    float s = 1.0f-mdot(n, up);
    //col = _mix(col, to_float3_s(0.5f), _fminf(4.0f*s, 1.0f));
    col = _mix(col, TerrainCol[5], _fminf(4.0f*s, 1.0f));
    
    t = smoothstep(r+0.06f, r+0.08f, h);
    //col = _mix(col, to_float3_s(1.0f), t);
    col = _mix(col, TerrainCol[6], t);
    
    return col;
}

__DEVICE__ float atmDensity(float3 p, float radius, float atmRadius){
    float r = (length(p) - radius)/(atmRadius - radius);
    return _expf(-r) * (1.0f - r);
}

__DEVICE__ float3 evalAtmosphere(float3 p1, float3 p2, float3 ldir, float radius, float atmRadius){
    float dt = 0.1f;
    float ds = 0.1f;
    float dst = length(p2 - p1);
    
    float3 coefs = to_float3(
        _powf(400.0f/700.0f, 4.0f),
        _powf(400.0f/530.0f, 4.0f),
        _powf(400.0f/440.0f, 4.0f)
    );
    
    float3 scattered = to_float3_s(0.0f);
    
    for(float t = 0.0f; t <= 1.0f; t += dt){
        float3 p = p1 + (p2 - p1) * t;
float zzzzzzzzzzzzzzzzz;        
        float s = 0.0f;
        float3 q = p;
        float opd = 0.0f;
        while(dot(q, q) < atmRadius * atmRadius){
            q += ds * ldir;
            opd += atmDensity(q, radius, atmRadius);
        }
        
        float3 tsm = exp_f3(-opd * coefs);
        
        scattered += atmDensity(p, radius, atmRadius) * tsm * dst * dt;
    }
    
    return scattered;
}


__DEVICE__ float shadow(float3 p, float3 ldir, float3 n, float radius, __TEXTURE2D__ iChannel0, float aspect, float4 muloffT, float angle){
    float dmin = raymarch(p + n*0.025f, ldir, radius, iChannel0, aspect, muloffT, angle).z;
    return smoothstep(0.0f, 1.0f, dmin);
}

__DEVICE__ float3 background(float3 rd, float3 ldir){
    float f = mdot(rd, ldir);
    f = smoothstep(0.999f, 0.9994f, f);
    return _mix(to_float3_s(0.0f), to_float3(1.0f, 0.99f, 0.7f), f);
}

__KERNEL__ void PlanetattemptJipi309Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(PlanetOnly, 0);
    CONNECT_CHECKBOX1(Textur, 0);
    CONNECT_CHECKBOX2(Aspect, 0);
  
    CONNECT_SLIDER0(radius, -1.0f, 10.0f, 1.5f);
    CONNECT_SLIDER1(atmRadius, -1.0f, 10.0f, 1.9f);
    
    CONNECT_SLIDER2(mulF, -1.0f, 10.0f, 0.25f);
    CONNECT_POINT0(offF, 0.0f, 0.0f);
    CONNECT_SLIDER3(offFZ, -1.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER4(mulT, -1.0f, 10.0f, 0.25f);
    CONNECT_POINT1(offT, 0.0f, 0.0f);
    CONNECT_SLIDER5(offTZ, -1.0f, 10.0f, 0.0f);
    
    float4 muloffF = to_float4(offF.x,offF.y,offFZ,mulF);
    float4 muloffT = to_float4(offT.x,offT.y,offTZ,mulT);
    
    CONNECT_SLIDER6(Angle, -10.0f, 10.0f, 0.29f);
    
    CONNECT_SLIDER7(aspect, -10.0f, 10.0f, 0.5625f);
    
    if (!Textur) aspect = 1.0f;
    else
       aspect = (Aspect == 0) ? iResolution.y/iResolution.x : aspect;
  
  
  CONNECT_COLOR0(Basis1, 0.6f, 0.5f, 0.4f, 1.0f);
  CONNECT_COLOR1(Basis2, 0.9f, 1.0f, 0.5f, 1.0f);
  CONNECT_COLOR2(Green1, 0.4f, 0.6f, 0.1f, 1.0f);
  CONNECT_COLOR3(Green2, 0.0f, 0.4f, 0.3f, 1.0f);
  CONNECT_COLOR4(Basis3, 0.7f, 0.5f, 0.3f, 1.0f);
  CONNECT_COLOR5(Grau,   0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_COLOR6(Mountain, 1.0f, 1.0f, 1.0f, 1.0f);
  
  
  float3 TerrainCol[7] = { swi3(Basis1,x,y,z), swi3(Basis2,x,y,z), swi3(Green1,x,y,z), swi3(Green2,x,y,z), swi3(Basis3,x,y,z), swi3(Grau,x,y,z), swi3(Mountain,x,y,z)}; 
  
  
    __TEXTURE2D__ ch = iChannel0;
    if(Textur) ch = iChannel1;
  
    //const float radius = 1.5f;
    //const float atmRadius = 1.9f;

    float2 uv = fragCoord/iResolution;
    fragColor = to_float4_aw(to_float3_s(_tex2DVecN(ch,uv.x,uv.y,15).x),1.0f);
    uv -= 0.5f;
    uv.x *= iResolution.x / iResolution.y;
    
    float2 mr = swi2(iMouse,x,y)/iResolution;
    mr -= 0.5f;
    mr.x *= iResolution.x / iResolution.y;
    mr *= -3.141592f;
    
    float3 col = to_float3_s(0.0f);

    float3 ro = to_float3(0.0f,0.0f,-5.0f);
    float3 rd = normalize(to_float3_aw(uv, 1.0f));
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , rot(mr.y)));
    swi2S(rd,y,z, mul_f2_mat2(swi2(rd,y,z) , rot(mr.y)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , rot(mr.x)));
    swi2S(rd,x,z, mul_f2_mat2(swi2(rd,x,z) , rot(mr.x)));
    
    float rxz = iTime * 0.1f;
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , rot(rxz)));
    swi2S(rd,x,z, mul_f2_mat2(swi2(rd,x,z) , rot(rxz)));
    
    float3 ldir = normalize(to_float3(1.0f,1.0f,-1.0f));
    
    if (PlanetOnly)
    {
      swi2S(ldir,y,z, mul_f2_mat2(swi2(ldir,y,z) , rot(mr.y)));
      swi2S(ldir,x,z, mul_f2_mat2(swi2(ldir,x,z) , rot(mr.x)));
    }      
    
    swi2S(ldir,x,z, mul_f2_mat2(swi2(ldir,x,z) , rot(rxz)));
    
    float3 rm = raymarch(ro, rd, radius, ch, aspect, muloffT, Angle);
    float t = rm.x;
    float i = rm.y;
    
    float2 ts = seaItsc(ro, rd, radius);
    
    float2 ta = atmoItsc(ro, rd, atmRadius);
    
    col = background(rd, ldir);
    
    if(i < MAX_RM_ITER || ts.x > 0.0f || ta.x > 0.0f){  
        float3 n = to_float3_s(0.0f);
        float3 m = to_float3_s(0.0f);
        
        if(i < MAX_RM_ITER){
            m = ro + t * rd;
            n = normal(ro + t * rd, radius, ch, aspect, muloffT, Angle);
            col = terrainCol(m, n, radius, ch, aspect, muloffF, TerrainCol);
        }
        
        float t2 = t;
        if(ts.x > 0.0f && (ts.x < t || i == MAX_RM_ITER)){
            m = ro + ts.x * rd;
            n = normalize(m);
            t2 = ts.x;
        }
        
        // sea coloring
        float sdepth = _fminf(ts.y-ts.x, t-ts.x);
        if(sdepth > 0.0f){
            float od = 1.0f - _expf(-sdepth*50.0f);
            float a = 1.0f - _expf(-sdepth*200.0f);
            float3 scol = _mix(to_float3(0.0f,0.8f,0.9f), to_float3(0.0f, 0.3f, 0.4f), od);
            scol += specRef(n, ldir, rd, 50.0f);
            col = _mix(col, scol, a);
        }
        
        float lig = mdot(n, ldir);
        if(ts.x > 0.0f || i < MAX_RM_ITER){
            lig *= shadow(m, ldir, n, radius, ch, aspect, muloffT, Angle);
            col *= lig;
        }
        
        // adding the atmosphere
        if(_fminf(ta.y-ta.x, t-ta.x) > 0.0f){
            float3 p1 = ro + ta.x * rd;
            float3 p2 = ro + _fminf(ta.y, t2) * rd;
            float3 scattered = evalAtmosphere(p1, p2, ldir, radius, atmRadius);
            col = mix_f3(col, to_float3(0.5f, 0.8f, 1.0f), scattered);
        }
    }
    
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}