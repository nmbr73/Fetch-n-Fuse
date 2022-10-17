
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define deg (3.14159f/180.0f)
__DEVICE__ mat2 r2d(float a) {
    return to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a));
}

//rgb2hsv2rgb from https://stackoverflow.com/questions/15095909/from-rgb-to-hsv-in-opengl-glsl
// All components are in the range [0…1], including hue.
__DEVICE__ float3 rgb2hsv(float3 c)
{
    float4 K = to_float4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
    float4 p = _mix(to_float4(c.z,c.y,K.w,K.z), to_float4(c.y,c.z,K.x,K.y), step(c.z, c.y));
    float4 q = _mix(to_float4_aw(swi3(p,x,y,w), c.x), to_float4(c.x,p.y,p.z,p.x), step(p.x, c.x));

    float d = q.x - _fminf(q.w, q.y);
    float e = 1.0e-10;
    return to_float3(_fabs(q.z + (q.w - q.y) / (6.0f * d + e)), d / (q.x + e), q.x);
}

// All components are in the range [0…1], including hue.
__DEVICE__ float3 hsv2rgb(float3 c)
{
    float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
    return c.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), c.y);
}
// 2D Random
__DEVICE__ float random (in float2 st) {
    return fract(_sinf(dot(swi2(st,x,y),
                       to_float2(12.9898f,78.233f)))
                       * 43758.5453123f);
}

// 2D Noise based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
__DEVICE__ float noise (in float2 st) {
    float2 i = _floor(st);
    float2 f = fract_f2(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + to_float2(1.0f, 0.0f));
    float c = random(i + to_float2(0.0f, 1.0f));
    float d = random(i + to_float2(1.0f, 1.0f));

    // Smooth Interpolation

    // Cubic Hermine Curve.  Same as SmoothStep()
    float2 u = f*f*(3.0f-2.0f*f);
    // u = smoothstep(0.0f,1.0f,f);

    // Mix 4 coorners percentages
    return _mix(a, b, u.x) +
               (c - a)* u.y * (1.0f - u.x) +
               (d - b) * u.x * u.y;
}

//from https://www.shadertoy.com/view/ttBXRG
__DEVICE__ float staircase( in float _x, in float k )
{
    float i = _floor(_x);
    float f = fract(_x);
    
    float a = 0.5f*_powf(2.0f*((f<0.5f)?f:1.0-f), k);
    f = (f<0.5f)?a:1.0-a;
    
    return i+f;
}


__DEVICE__ float bitm(float2 uv,int c) {
    float h = 5.0f;
    float w = 3.0f;
    int p = (int)(_powf(2.0f,(w)));
    float line1 = 9591.0f;
    uv = _floor(to_float2(uv.x*w,uv.y*h))/to_float2(w,w);
    float c1 = 0.0f;
    float cc = uv.x + uv.y*w;
    c1 = mod_f( _floor( (float)(c) / _exp2f(_ceil(cc*w-0.6f))) ,2.0f);
    c1 *= step(0.0f,uv.x)*step(0.0f,uv.y);
    c1 *= step(0.0f,(-uv.x+0.99f))*step(0.0f,(-uv.y+1.6f));
    return (c1);
}
#define logo 1
__DEVICE__ float3 slogo(float2 uv, float ar, float iTime) {
    if (logo == 0) {
        return to_float3_s(0.0f);
    }
    float2 px = to_float2(1.0f/3.0f,1.0f/5.0f);
    float ls = 4.1f;
    uv.x = 0.993f-uv.x;
    uv *= 8.0f*ls;
    ls += 2.0f;
    float ul = length(uv);
    uv -= swi2(px,y,x)*0.5f*0.5f*ls;
    ul = length(to_float2(uv.x*0.5f,uv.y)-0.5f);
    uv.x *= ar*1.75f;
    int s = 29671;
    int c = 29263;
    int r = 31469;
    int y = 23186;
    uv.x= 5.0f-uv.x;
    float b = bitm(uv,s);
    uv.x -= 1.0f/3.0f*4.0f;
    b += bitm(uv,c);
    uv.x -= 1.0f/3.0f*4.0f;
    b += bitm(uv,r);
    uv.x -= 1.0f/3.0f*4.0f;
    b += bitm(uv,y);
    float rr = step(0.0f,uv.x+px.x*13.0f)*step(0.0f,uv.y+px.y)*step(0.0f,(-uv.x+px.x*4.0f))*step(0.0f,(-uv.y+px.y*6.0f));
    b = clamp(b,0.0f,1.0f);
    //b = rr*_floor(b);
    float ptime = iTime;
    float3 l = hsv2rgb(to_float3(b+ptime,0,rr-b*1.9f))*rr;
    //l -= length(uv)*0.5f;
    //l -= ul*rr*0.6f;
    l -= 0.1f-clamp(ul*0.1f,rr*1.0f-b,0.1f);
    //l -= 3.0f-ul*2;
    //l = clamp(l,-1.0f,1.0f);
    return (l);
}

//iq sdbox and sdvertcalcapsule functions :)
__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f);
}

__DEVICE__ float sdVerticalCapsule( float3 p, float h, float r )
{
  p.y -= clamp( p.y, 0.0f, h );
  return length( p ) - r;
}

__DEVICE__ float abx(float a,float b) {
    return _fabs(_fabs(a)-b)-b;
}

__DEVICE__ float dty(float3 p, float3 s) {
    float ls = s.x;
    float lr = s.y;
    //swi2(p,z,y) *= r2d((3.14159f/180.0f)*-_sinf(iTime+p.y*20));
    //swi2(p,x,y) *= r2d((3.14159f/180.0f)*iTime);
    float d = sdBox(p+to_float3(0.0f,0,0.0f),to_float3_s(lr*1.0f));
    //float d = sdVerticalCapsule(p+to_float3(0.0f,ls/2,0.0f),ls,lr);
    //float d = length(p)-0.001f;
    //d = length(p)-0.014f;
    return d;
}

__DEVICE__ float4 fr(float3 p, float iTime) {
    //p.z = 0.0f;
    //float pfd = 0.95f;
    //p = (fract((p*pfd))-0.5f)/pfd;
    //swi2(p,x,z) *= r2d((3.14159f/180.0f)*-90);
    float st = iTime*0.2f;
    float sm = _sinf(st*10.0f)*0.5f+0.5f;
    //sm = 0.0f;
    //sm += _sinf(st*2.0f)*0.25f;
    //sm *= 1.0f;
    float lp = length(p);
    //sm += _sinf(st*1)*0.125f;
    //float lr = -0.001f-_sinf(lp*20.0f+iTime)*0.0015f;
    float lr = 0.13f;
    float ls = 0.5f;
    //p *= 2.0f;
    //ls += spectrum1.y*16.0f;
    lr += 0.001f;
    float d = 10000.0f;
    float u = 1.0f;

    float lsm = 0.495f;
    float3 dp = p;

    float rt = iTime*2.0f;

    float pd = 0.185f;
    //float pm = 1.0f-(_sinf(iTime)*0.5f+0.5f);
    float pm = 1.0f;
    //p *=  2.0f;
    float tm = iTime;
    tm *= 0.1f;
    float ia = -1.0f;
    float pdm = _sinf(iTime*1.1f+lp*dp.z*29.0f)*0.5f+0.5f;
    pdm *= 0.2f;
    pdm += 0.3f;
    pdm = 0.5f;
    //p += _sinf(p*20.0f+iTime)/24.0f;
    float lss = 2.8f;
    st += _sinf(st*2.0f)*0.25f+_sinf(st*3.0f)*0.25f;
    int sps = 7;
    float3 rp = p;
    //swi2(rp,x,y) *= r2d(deg*90*_floor(iTime*8.0f));
    //swi2(rp,x,z) *= r2d(deg*90*_floor(iTime*20.3f));
    //sps += int(rp.x*15.0f);
    //sps += (_sinf(iTime)*0.5f+0.5f)*10.0f; 
    sps = clamp(sps,1,18);
    //lr = deg*15.0f;
   lr = 0.2f;
   float ad = (_sinf(iTime*0.4f+lp*29.0f)*0.5f+0.5f);
    
    for (int i=0;i<sps;i++) {
    d = _fminf(d,dty(p,to_float3(ls,lr,0.0f)));
    //d -= _sinf(d*80.0f)*0.005f;
    //p += _sinf(d*200.0f)*0.001f;
    
    //d = _fminf(d,dty(swi3(p,y,x,z),to_float3(ls,lr,0.0f)));
    //d = _fminf(d,dty(swi3(p,x,z,y),to_float3(ls,lr,0.0f)));
    //p.x += 0.1f;
    ia += 1.0f;
    //lr *= 2.9f;
    lr *= 0.5f;
    //lr += 0.000002f;
    //lr = _sinf(lr+iTime*0.1f)*0.1f+0.1f;
    //lr += 0.01f;
    //lr *= ia+0.0003f;
    pd *= u;
    
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , r2d((3.14159f/180.0f)*-45.0f)));
    //swi2(p,z,y) = _fabs(swi2(p,z,y));
    p.z = _fabs(p.z);
    p.y = _fabs(p.y);
    
    //p.x = _fabs(p.x)-0.09f;
    
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , r2d((3.14159f/180.0f)*+45.0f)));
    
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , r2d((3.14159f/180.0f)*-45.0f)));

    //swi2(p,x,y) = _fabs(swi2(p,x,y))-pd;
    p.x = _fabs(p.x)-pd;
    p.y = _fabs(p.y)-pd;

    pd *= pdm;
    //pd *= _sinf(iTime*0.1f+dp.z*2.0f)*0.5f+0.5f;

    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , r2d((3.14159f/180.0f)*+45.0f)));
    
    //p = _fabs(p)-0.02f;
    
    ls *= lsm;
    ls += 0.002f;
    
    }
    //d += _sinf(p.z*200.0f)*0.001f;
    //p += iTime*0.1f;
    //swi2(dp,x,y) *= r2d((iTime*0.1f));
    //swi2(dp,x,z) *= r2d((iTime*0.3f));
    //p = fract(p*0.5f)/0.5f;
    d = _fminf(d,dty(p,to_float3(ls,lr,0.0f)));
    //p *= _sinf(p*10.0f)*0.5f;
    d += (_sinf(p.y*90.0f+iTime+dp.x*5.0f)*0.5f+0.5f)*0.1f;
    d += (_sinf(p.y*30.0f+iTime*0.2f+dp.x*2.0f)*0.5f+0.5f)*0.1f;
    //d -= (_sinf(dp.y*7.0f+iTime*0.2f)*0.5f+0.5f)*0.05f;
    d *= 0.5f;
    //d = _fminf(d,dty(swi3(p,y,x,z),to_float3(ls,lr,0.0f)));
    //d = _fminf(d,dty(swi3(p,x,z,y),to_float3(ls,lr,0.0f)));

    return to_float4_aw(p,d*0.5f);
}


__DEVICE__ float4 map(float3 p, float iTime) {
    //p += 1.0f;
    //swi2(p,x,y) *= r2d(1.0f);
    //swi2(p,x,z) *= r2d(1.0f);
    
   
    //swi2(p,x,y) += 2.0f;
    //
    float3 c = p;
    p.z -= 1.0f;

    //vec3 c = p;
    float cd = 12.0f*(_sinf(iTime*0.002f+5.0f)*0.5f+0.5f);
    //c.z = (fract(c.z*cd))/cd;
    float rt = iTime*0.1f;
    //swi2(p,x,y) *= r2d((3.14159f/180.0f)*-45*iTime*9.0f);
    //p = _fabs(p);
    //p = _fabs(p+0.5f);
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , r2d(deg*35.264f)));
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , r2d(-deg*90.0f)));
    //p.z += 0.5f;
    //p = _fabs(p)-0.5f;
    float pfd = 0.9f;
    //p = (fract((p-0.25f)*pfd)-0.5f)/pfd;
    //swi2(p,x,z) *= r2d(0.75f,iTime);
    //p += 0.89f;
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , r2d(_sinf(rt*0.5f)*0.5f)));
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , r2d(rt)));

    
    float d = length(p)-3.0f;
    //float env = d;
    //float env = _mix(d,sdBox(p,to_float3_s(1.966f)),0.001f+_sinf(p.x*13.0f)*0.001f);
    float env = _mix(d,sdBox(p,to_float3_s(1.966f)),0.002f);
    //swi2(p,x,z) *= r2d(-rt);
    //p.z += iTime;
    
    //float pz = p.z;
    float4 frd = fr(p,iTime);
    //d = sdf(p);
    
    //d = _fabs(d)-0.0008f;
    
    
    d = frd.w;
    d = _fminf(d,-env);
    //float dl = (_sinf(iTime)*0.5f+0.5f)*0.1f;
    //d = _fabs(d+0.01f)-0.01f;
    float dl = 0.001f;
    d = _fabs(d+dl)-dl;
    //d = _mix(d,(length(p)-0.28f)*0.7f,0.9f);
   // d = _fminf(d,sdBox(p,to_float3_s(0.3f)));
    c.z *= 2.0f;
    d = _fmaxf(d, -(length(c)-0.5f));
    //d = _fmaxf(d, -(length(c)-01.2f));
    
    return to_float4_aw(p,d);
}

__DEVICE__ float3 calcNorm(float3 p, float iTime) {
    //float eps = 0.01f*(_sinf(p.z*0.1f)*0.5f+0.5f);
    float eps = 0.0008f;
    float2 h = to_float2(eps,0.0f);
    return normalize(to_float3(map(p-swi3(h,x,y,y),iTime).w-map(p+swi3(h,x,y,y),iTime).w,
                               map(p-swi3(h,y,x,y),iTime).w-map(p+swi3(h,y,x,y),iTime).w,
                               map(p-swi3(h,y,y,x),iTime).w-map(p+swi3(h,y,y,x),iTime).w));
}

__DEVICE__ float edges(float3 p, float iTime) {
    float eps = 0.001f;
    //return calcNorm(p)
    
    float3 n1 = calcNorm(p*(1.0f-eps),iTime);
    float3 n2 = calcNorm(p*(1.0f),iTime);
    return clamp(_fabs((n1.x+n1.y+n1.z)-(n2.x+n2.y+n2.z)),0.0f,1.0f);
    //return (map(p+(eps)).w-map(p-(eps)).w)*18.0f;
}


#define render 0
__DEVICE__ float2 RM(float3 ro, float3 rd, float iTime) {
    float dO = 0.0f;
    float ii = 0.0f;
    int steps = 130;
    //steps = int(steps*(_sinf(iTime)*0.5f+0.5f));
    if (render == 1) {
        steps = 150;
    }
    float d = 130.0f;
    for (int i=0;i<steps;i++) {
        float3 p = ro+rd*dO;
        //ro += calcNormL(p)*0.02f;
        //ro += lens(p).xyz*0.2f;
        float dS = map(p,iTime).w;
        dO += dS*(d/float(steps));
        //dO += dS*(100.0f/float(steps))*(dO+2.0f)*0.15f;
        ii += 0.5f*d/(float)(steps);
        if (dO > 1000.0f || dS < 0.00009f) {
            break;
        }
    }
    return to_float2(dO,ii);
}

__DEVICE__ float3 color(float3 p, float2 d) {
    return (p);
}

__KERNEL__ void BalancedEntropyFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord / iResolution;
    float2 tv = fragCoord / iResolution;
    
    float2 R = iResolution;
    float ar = R.x/R.y;
    //uv.x *= ar;
float zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz;        
    uv -= 0.5f;
    uv.x *= ar;
    float c= length(uv);
    float3 col = to_float3_s(0.0f);
    float3 ro = to_float3(0.0f,0.0f,0);
    //ro.z = -2.0f;
    float3 rd = normalize(to_float3_aw(uv,1.0f));
    
    float3 bak = swi3(_tex2DVecN(iChannel0,tv.x,tv.y,15),x,y,z);
    float rn = noise(to_float2_s(noise(fract_f2(swi2(rd,x,y)*220.0f+iTime*0.01f)*90.0f)*100.0f));
    //if (rn > 0.2f) {
    //if (1 == 1) {
    if (rn < clamp(c*0.7f,0.0f,1.0f)+0.3f && render != 0) {
        col = bak;
        fragColor = to_float4_aw(col,1.0f);
        return;
    }
    
    float2 d = RM(ro,rd,iTime);
    float3 p = ro+rd*d.x;
    float4 mp = map(p,iTime);
    float3 n = calcNorm(p,iTime);
    float lp = length(p);
    
    float3 na = n;
    //vec3

    
    float2 dd = d;
    float ga = 1.0f;
    float time = iTime;
    float4 m = mp;
    float4 mr = mp;
    float3 b = to_float3_s(0.0f);
    float e = edges(p,iTime);
    //e = clamp(e,0.0f,0.1f)*8;
    //col += d.x*ga*0.1f;
    //col += n*ga*0.1f+d.y*ga*0.05f;
    //col += d.x*0.1f;
    
    for(int i=0;i<2;i++) {
        if (d.x > 1000.0f) {
            //col *= 0.0f;
            ga = 0.0f;
        }
        //col += d.y*0.02f*ga;
        col += hsv2rgb(to_float3(d.x+iTime*0.01f,d.y*0.015f,d.y*0.02f*ga));
        //col += (_fabs(e)*30.0f-0.3f)*ga;
        //col += n+d.y*0.1f-2.0f;
        //col += (n+d.y*0.1f-2.0f+d.x*0.1f)*ga;
        //col += _fabs((n*1.8f+d.y*0.05f-2.0f+d.x*0.1f))*ga;
        //col += n*d.y*ga*0.01f;
        //col += n*ga;
        //col += n*ga*d.x;
        //col += to_float3_aw(d.x*0.1f)*ga;
        //col += d.x*ga*0.1f;
        //col += n*ga*0.1f+d.y*ga*0.05f;
        ga *= 0.3f;
        //ga -= 0.2f;
        //ga *= 0.2f;
        ro = p-n*0.002f;
        rd = reflect(rd,n);
        d = RM(ro,rd,iTime);
        //b += d.x*0.02f*ga;
        p = ro+rd*d.x;
        n = calcNorm(p,iTime);
        e = edges(p,iTime);
        mr = map(p,iTime);
        //col += d.x*0.1f;
        
        //col += n*ga*0.1f+d.y*ga*0.25f;
    }
    //col += d.y*ga*0.01f;
    //col += e;
    col += hsv2rgb(to_float3(d.x,d.y*0.015f,d.y*0.02f*ga));
    //col *= 0.6f;
    col = rgb2hsv(col);
    col.x += 0.7f;
    //col.x += iTime;
    col.y *= 1.2f;
    col.z -= dd.x*0.15f;
    col = hsv2rgb(col);
    float2 ttv = tv;
    ttv -= 0.5f;
    //ttv *= 0.99f;
    ttv += 0.5f;

    col += slogo(ttv,ar,iTime)*0.6f;
    float2 ux = uv;
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}