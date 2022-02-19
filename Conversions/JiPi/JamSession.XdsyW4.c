
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/garth_knight/garth-knight-regardez-moi' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define NUM_STEPS 32
#define EPS 0.001f
#define FAR_CLIP 15.0f
#define LEVELS_SCALAR 1.0f

#define time iTime

// reference: https://www.shadertoy.com/view/4lGSzy
// 2017 passion

__DEVICE__ float noise3D(float3 p)
{
  return fract(_sinf(dot(p ,to_float3(12.9898f,78.233f,12.7378f))) * 43758.5453f)*2.0f-1.0f;
}

__DEVICE__ float3 mixc(float3 col1, float3 col2, float v)
{
    v = clamp(v,0.0f,1.0f);
    return col1+v*(col2-col1);
}

// polynomial smooth _fminf (k = 0.1f);
__DEVICE__ float smin( float a, float b, float k )
{
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ mat3 lookAt(float3 origin, float3 target, float roll) {
  float3 rr = to_float3(_sinf(roll), _cosf(roll), 0.0f);
  float3 ww = normalize(target - origin);
  float3 uu = normalize(cross(ww, rr));
  float3 vv = normalize(cross(uu, ww));

  return to_mat3_f3(uu, vv, ww);
}

__DEVICE__ float map(float3 p,float iTime){
    float c = length(p) - 0.5f;
    
    float c1 = length(p) - 0.20f;
    p.x += 0.75f*_sinf(time*1.4f);
    p.y -= 0.75f*_cosf(time/2.0f);
    p.z += 0.75f*_cosf(time+_sinf(time));
    
    float c2 = length(p) - 0.33f;
    p.x -= 0.75f*_sinf(time/0.4f);
    p.y += 0.75f*_cosf(time/2.0f);
    p.z -= 0.75f*_cosf(time+_sinf(time*3.0f));
    
    float c3 = length(p) - 0.30f;
    p.x += 0.75f*_cosf(time/2.4f);
    p.y -= 0.75f*_cosf(time*1.2f);
    p.z += 0.75f*_sinf(time+_sinf(time));
    
    float c4 = length(p) - 0.175f;
    p.x -= 0.75f*_sinf(time*1.8f);
    p.y += 0.75f*_sinf(time/2.0f);
    p.z -= 0.75f*_cosf(time+_sinf(time));
    
    float f = smin(c, c2, 0.3f);
    f = smin(f, c1, 0.2f);
    f = smin(f, c3, 0.33f);
    return smin(f, c4, 0.4f);
}


__DEVICE__ float trace(float3 r, float3 o,float iTime){
    float t = 0.0f;
    for(int i = 0; i < NUM_STEPS; i++){
        float3 p = o+r * t;
        float d = map(p,iTime);
        if(_fabs(d) < EPS || t > FAR_CLIP)
            break;
        t += d;// * 0.75f;
    }
    return t;
}

__DEVICE__ float3 getNormal(float3 p,float iTime){
    float2 e = to_float2(0.0f, EPS);
  return normalize((to_float3(map(p + swi3(e,y,x,x),iTime),
                              map(p + swi3(e,x,y,x),iTime), 
                              map(p + swi3(e,x,x,y),iTime)) - map(p,iTime)) / e.y);
}

__KERNEL__ void JamSessionFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
    CONNECT_CHECKBOX0(Textur, 0);

    float2 uv = fragCoord / iResolution;
    uv = uv * 2.0f - 1.0f;
    uv.x *= iResolution.x / iResolution.y;
    
    fragColor = to_float4_s(0.0f);
    
    //float time = iTime;

    float3 l = normalize(to_float3(0.3f, 0.8f, 0.2f));
    float3 ray = normalize(to_float3_aw(uv, 1.0f - dot(uv, uv) * 0.25f));
    float3 o = to_float3(2.0f*_cosf(time), -0.5f*0.75f+_sinf(time/2.0f)*0.75f, 
                         2.0f*_sinf(time));
    mat3 camMat = lookAt(o, to_float3_s(0.0f), _sinf(time*0.13f)*0.25f);

    ray = mul_mat3_f3(camMat , ray);
    
    float3 col = to_float3_s(0.0f);
    float3 ref = to_float3_s(0.0f);
    
    // https://www.shadertoy.com/view/4lGSzy
    float nBands = 32.0f;
    float i = _floor(ray.x*nBands);
    float f = fract(ray.x*nBands);
    float band = i/nBands;
    band *= band*band;
    band = band*0.995f;
    band += 0.005f;
    float s = texture( iChannel0, to_float2(band,0.25f) ).x;
    
    /* Gradient colors and amount here */
    const int nColors = 4;
    float3 colors[nColors];  
    colors[0] = to_float3(0.0f,0.0f,1.0f);
    colors[1] = to_float3(0.0f,1.0f,1.0f);
    colors[2] = to_float3(1.0f,1.0f,0.0f);
    colors[3] = to_float3(1.0f,0.0f,0.0f);
    
    float3 gradCol = colors[0];
    float nc = (float)(nColors)-1.0f;
    for(int i = 1; i < nColors; i++)
    {
      gradCol = mixc(gradCol,colors[i],(s-float(i-1)/nc)*nc);
    }
      
    col += to_float3_s(1.0f-smoothstep(0.0f,0.01f,ray.y-s*LEVELS_SCALAR));
    col *= gradCol;

    ref += to_float3_s(1.0f-smoothstep(0.0f,-0.01f,ray.y+s*LEVELS_SCALAR));
    ref*= gradCol*smoothstep(-0.5f,0.5f,ray.y);
    
    col = _mix(ref,col,smoothstep(-0.01f,0.01f,ray.y));

    col *= smoothstep(0.125f,0.375f,f);
    col *= smoothstep(0.875f,0.625f,f);

    col = clamp(col, 0.0f, 1.0f);

    float dither = noise3D(to_float3_aw(swi2(ray,z,y),time))*15.0f/256.0f;
    col += dither;
    
    
    float hit = trace(ray, o,iTime);
    float3 sp = o+ray * hit;
    float d = map(sp,iTime);
    float3 n = getNormal(sp,iTime);
    
   
    float3 diff = to_float3_s(clamp(dot(n, l), 0.15f, 1.0f));
    
    if(Textur)
       diff = swi3(texture(iChannel1, swi2(sp,x,z)+0.5f),x,y,z);
    
    
    if(_fabs(d) < 0.05f)
        fragColor = to_float4_aw(diff,Alpha);
    else
        fragColor = to_float4_aw(col, Alpha);


  SetFragmentShaderComputedColor(fragColor);
}