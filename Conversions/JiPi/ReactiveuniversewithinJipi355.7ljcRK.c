
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/a6a1cf7a09adfed8c362492c88c30d74fb3d2f4f7ba180ba34b98556660fada1.mp3' to iChannel0

#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define S(a, b, t) smoothstep(a, b, t)
__DEVICE__ float DistLine(float2 p, float2 a, float2 b)
{
    float2 pa = p-a;
    float2 ba = b-a;
    float t = clamp(dot(pa, ba)/dot(ba, ba), 0.0f, 1.0f);
    return length(pa-ba*t);
}

__DEVICE__ float N21(float2 p)//Random number generation
{
    p = fract_f2(p* to_float2(233.34f, 851.73f));
    p += dot(p, p + 23.45f);
    return fract(p.x*p.y);
}

__DEVICE__ float2 N22(float2 p)// Random float2 generation
{
    float n = N21(p);
    return to_float2(n, N21(p+n));
}

__DEVICE__ float2 GetPos(float2 id, float2 oofs, float iTime)
{
    float2 n = N22(id+oofs)*iTime;
    return oofs + sin_f2(n)*0.4f;
    //N22(id)-.5
}
__DEVICE__ float Line(float2 p, float2 a, float2 b)
{
    float d = DistLine(p, a, b);
    float m = S(0.03f, 0.01f, d);
    float d2 = length(a-b);
    m*= S(1.2f, 0.8f, d2) + S(0.05f, 0.03f, _fabs(d2-0.75f));
    return m;
}
__DEVICE__ float Layer(float2 uv, float music, float iTime)
{
    float2 gv = fract_f2(uv) - 0.5f;
    float2 id = _floor(uv);
    float m = 0.0f;
    float2 p[9];
    
    int i =0;
    for(float y = -1.0f; y<=1.0f; y++)
    {
        for(float x = -1.0f; x<=1.0f; x++)
        {
            p[i++] = GetPos(id, to_float2(x,y),iTime);
        }
    }
    
    for(int i =0; i< 9; i++)
    {
        m+= Line(gv, p[4], p[i]);
        
        float2 j = (p[i] - gv)*20.0f;
        float sparkle = 1.0f/dot(j, j);
        m+=sparkle *(music + (_sinf(iTime *p[i].x*0.01f)*0.5f+0.5f));
    }
    m+= Line(gv, p[1], p[3]);
    m+= Line(gv, p[1], p[5]);
    m+= Line(gv, p[3], p[7]);
    m+= Line(gv, p[5], p[7]);
    return m;
}
__DEVICE__ mat2 rotationMatrix(float angle)
{
    angle *= 3.14f / 180.0f;
    float s=_sinf(angle), c=_cosf(angle);
    return to_mat2( c, -s, s, c );
}
__KERNEL__ void ReactiveuniversewithinJipi355Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.345f, 0.456f, 0.657f, 1.0f); 

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float gradient = uv.y;
    float2 mouse = (swi2(iMouse,x,y)/iResolution)-0.5f;
    
    //float fft = texelFetch(iChannel0, to_int2(0.7f, 0), 0).x;
    float2 fftpos = to_float2(0.7f, 0.0f);
    float fft = texture(iChannel0, (make_float2(to_int2_cfloat(fftpos))+0.5f)/iResolution).x;

    
    float m = 0.0f;
    float t = iTime*0.1f;
    float s = _sinf(t);
    float c = _cosf(t);
    mat2 rot = rotationMatrix((t*100.0f)+(fft*10.0f));
    
    uv = mul_f2_mat2(uv,rot);
    mouse = mul_f2_mat2(mouse,rot);
    
    for(float i = 0.0f; i<1.0f; i+=1.0f/4.0f)
    {
        float z = fract(i+t);
        float size = _mix(10.0f, 0.5f, z);
        float fade = S(0.0f, 0.5f, z)*S(1.0f,0.8f, z);
        m +=Layer(uv*size+i*20.0f-mouse, fft,iTime)*fade;
    }
    //float3 base = sin_f3(t*to_float3(0.345f, 0.456f, 0.657f)*fft)*0.4f +0.6f;
    float3 base = sin_f3(t*swi3(Color,x,y,z)*fft)*0.4f +0.6f;
    float3 col = m * base;
    
    
    gradient *= fft*2.0f;
    
    col -= gradient*base;
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}