
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define AA 1
#define EPS 0.001f
#define MAX 50.0f

__DEVICE__ float2 hash2(float2 p)
{
    return fract_f2(sin_f2(mul_f2_mat2(p,to_mat2(98,-64,-73,69)))*397.0f)*0.8f;
}
__DEVICE__ float height(float2 p, float iTime)
{
    return p.x+_sinf(p.y*0.3f)*3.0f-iTime;
}
__DEVICE__ float cell(float3 p, float iTime)
{
    float2 f = _floor(swi2(p,x,z));
    float d = 4.0f;
    
    for(int X = -1; X<=1; X++)
    for(int Y = -1; Y<=1; Y++)
    {
        float2 h = f+to_float2(X,Y);
        h += hash2(h)-swi2(p,x,z);
        
        float3 c = to_float3_aw(h,p.y+1.0f);
        float2 R = normalize(sin_f2(swi2(c,x,y)+swi2(p,x,z)));
        mat2 r = to_mat2(R.x,R.y,-R.y,R.x);

        float off = height(swi2(p,x,z)+swi2(c,x,y),iTime);
        c.z -= _sqrtf(_fabs(off))-1.0f;
        c.z = _fmaxf(c.z,0.0f);

        float s = 0.13f*smoothstep(-0.2f,0.2f,off);
        swi2S(c,x,y, mul_f2_mat2(swi2(c,x,y) , r));
        
        float w = 0.15f;
        d = _fminf(d, length(_fmaxf(abs_f3(c)-s, to_float3_s(0.0f)))+s-w);
    }
    
    return d;
}
__DEVICE__ float dist(float3 p, float iTime)
{
    return _fminf(p.y+1.0f,cell(p,iTime));
}
__DEVICE__ float3 normal(float3 p, float iTime)
{
    float2 e = to_float2(-2,2)*EPS;
    return normalize(dist(p+swi3(e,x,x,y),iTime)*swi3(e,x,x,y)+dist(p+swi3(e,x,y,x),iTime)*swi3(e,x,y,x)+
                     dist(p+swi3(e,y,x,x),iTime)*swi3(e,y,x,x)+dist(p+e.y,iTime)*e.y);
}
__DEVICE__ float3 color(float3 p,float3 r, float iTime)
{
    float off = height(swi2(p,x,z),iTime);
    float s = smoothstep(-0.2f,0.2f,off);
    
    float l = cell(to_float3(p.x,-2,p.z),iTime);
    float e = smoothstep(0.02f,0.0f,l);


    float3 n = normal(p,iTime);
    float ao = clamp(dist(p+n*0.2f,iTime)/0.2f,0.1f,1.0f);
    float3 sd = normalize(to_float3(3,2,-1));
    float dl = _fmaxf(0.3f+0.7f*dot(n,sd),0.0f);
    float sl = _fmaxf(dot(reflect(r,n),sd)*1.2f-1.0f,0.0f);
    
    for(float i = 0.02f;i<0.5f; i*=1.3f)
    {
        dl *= clamp(1.5f-i/(i+dist(p+sd*i*2.0f,iTime)),0.0f,1.0f);
    }
    float3 sh = _mix(to_float3(0.1f,0.15f,0.2f),to_float3_s(1),dl);
    
    float3 col = _mix(to_float3(0.7f,1,0.2f),to_float3(1,0.4f,0.1f),s);
    return _mix(to_float3(0.5f,0.7f,0.8f),col*_fminf((p.y+1.1f)/0.4f,1.0f),e)*sh*_sqrtf(ao)+sl;
}
__DEVICE__ float4 march(float3 p,float3 r, float iTime)
{
    float4 m = to_float4_aw(p+r,1);
    for(int i = 0;i<200;i++)
    {
        float s = dist(swi3(m,x,y,z),iTime);
        m += to_float4_aw(r,1)*s;
        
        if (s<EPS || m.w>MAX) return m;
    }
    return m;
}
__KERNEL__ void TerraformJipi570Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    float3 p = to_float3(iTime-2.0f,0.5f+0.5f*_cosf(iTime*0.2f),1);
    float3 col = to_float3_s(0);
    for(int X = 0;X<AA;X++)
    for(int Y = 0;Y<AA;Y++)
    {
        float2 c = fragCoord+to_float2(X,Y)/(float)(AA)-0.5f;
        float3 r = normalize(to_float3_aw(c-to_float2(0.5f,0.6f)*iResolution,iResolution.y));
    
        float4 m = march(p,r,iTime);
        float fog = smoothstep(MAX*0.4f,MAX,m.w);
        
        col += _mix(color(swi3(m,x,y,z),r,iTime),exp_f3(-1.0f*to_float3(13,7,4)*r.y*r.y-0.2f),fog);
    }
    col /= (float)(AA*AA);
    fragColor = to_float4_aw(pow_f3(col,to_float3_s(0.45f)),1);

  SetFragmentShaderComputedColor(fragColor);
}