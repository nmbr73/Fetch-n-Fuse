
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------

#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define OCTAVES 5

__DEVICE__ float random(in float3 p)
{
    return fract(_sinf(dot(p, to_float3(12.345345f, 18.42545f, 32.454342f))) * 4321488.23432f);
}

__DEVICE__ float simplexNoise(in float3 p)
{
    float3 ip = _floor(p);
    float3 fp = fract_f3(p);
    
    float a1 = random(ip);
    float a2 = random(ip + to_float3(1.0f, 0.0f, 0.0f));
    float a3 = random(ip + to_float3(0.0f, 1.0f, 0.0f));
    float a4 = random(ip + to_float3(1.0f, 1.0f, 0.0f));
    
    float b1 = random(ip + to_float3(0.0f, 0.0f, 1.0f));
    float b2 = random(ip + to_float3(1.0f, 0.0f, 1.0f));
    float b3 = random(ip + to_float3(0.0f, 1.0f, 1.0f));
    float b4 = random(ip + to_float3(1.0f, 1.0f, 1.0f));
    
    
    float3 u = fp * fp * (3.0f - 2.0f * fp);
    
    float x1 = _mix(a1, a2, u.x);
    float x2 = _mix(a3, a4, u.x);
    
    float y1 = _mix(b1, b2, u.x);
    float y2 = _mix(b3, b4, u.x);
    
    float r1 = _mix(x1, x2, u.y);
    float r2 = _mix(y1, y2, u.y);
    
    return _mix(r1, r2, u.z); 
}

__DEVICE__ float fbm(in float3 p)
{
    float result = 0.0f;
    float amp = 0.5f;
    for(int i = 0; i < OCTAVES; ++i)
    {
        float m = _powf(2.0f, (float)(i));
        result += simplexNoise(p * m) * (1.0f / m);
    }
    return _powf(result, 2.0f);
}

__DEVICE__ float3 calcNormalA(in float3 p)
{
    float3 p1 = p + to_float3(1.0f, 0.0f, 0.0f);
    float3 p2 = p + to_float3(-1.0f, 0.0f, 0.0f);
    float3 p3 = p + to_float3(0.0f, 0.0f, 1.0f);
    float3 p4 = p + to_float3(0.0f, 0.0f, -1.0f);
    
    float a = fbm(p1);
    float b = fbm(p2);
    float c = fbm(p3);
    float d = fbm(p4);
    
    return normalize(to_float3(a - b, 1.0f, c - d));
}

__KERNEL__ void Clouds2DshadowsJipi613Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    float2 uv = fragCoord / iResolution;

    uv *= 30.0f;
    float3 p =to_float3_aw(uv + iTime * 0.08f, iTime * 0.1f);
    float3 normal = calcNormalA(p);
   
    fragColor = to_float4_aw(normal, fbm(p));

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rock Tiles' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define MAX_DIST 6000.0f
#define MIN_DIST 0.01f
#define STEPS 255

struct Object
{
    int id;
    float d;
};

__DEVICE__ Object minValue(Object a, Object b)
{
    if(a.d <= b.d)
    {
        return a;
    }
    return b;
}

__DEVICE__ float3 rotateY(in float3 p, in float angle, float3 axis)
{
    float3 result = p;
    result -= axis;

    float rnd = radians(angle);
    result = to_float3(result.x * _cosf(rnd) - result.z * _sinf(rnd), result.y, result.x * _sinf(rnd) + result.z * _cosf(rnd));
    result += axis;
    return result;
}

__DEVICE__ Object sdPlane(in int id, in float3 p, in float3 normal, in float h)
{
    //return Object(id, dot(p, normal) + h);
    Object ret = {id, dot(p, normal) + h};
    return ret;
}

__DEVICE__ Object sdScene(in float3 p)
{
    Object plane1 = sdPlane(0, p, to_float3(0.0f, -1.0f, 0.0f), 3.0f);
    Object plane2 = sdPlane(1, p, to_float3(0.0f, 1.0f, 0.0f), 1.0f);

    Object result = minValue(plane1, plane2);

    return result;
}

__DEVICE__ Object rayMarching(in float3 ro, in float3 rd)
{
    Object result;
    for(int i = 0; i < STEPS; ++i)
    {
        float3 p = ro + rd * result.d;
        Object tmp = sdScene(p);
        result.d += tmp.d;
        result.id = tmp.id;
        
        if(tmp.d <= MIN_DIST || tmp.d >= MAX_DIST)
            break;
    }
    return result;
}

__DEVICE__ float _saturatef(in float a)
{
    return clamp(a, 0.0f, 1.0f);
}

__DEVICE__ float diffuseCloud(in float3 p, in float3 lightPos, in float3 normal)
{
    float3 lightDir = normalize(lightPos - p);
    
    return _saturatef(dot(lightDir, normal));
}

__DEVICE__ float diffuse(in float3 p, in float3 lightPos, in float3 normal)
{
    float3 lightDir = normalize(lightPos - p);
    
    float3 p1 = p - normal * MIN_DIST;
    
    float result = _saturatef(dot(lightDir, normal));
    if(rayMarching(p1, lightDir).d <= length(lightPos - p))
    {
        result *= 0.2f;
    }
    
    return result;
}

__DEVICE__ float3 calcCloudsShadow(in float3 col, in float clouds)
{
    return _mix(col, col * to_float3_s(0.4f), clouds);
}

__DEVICE__ float3 calcNormal(float3 p)
{
    float e = 0.0005f;
    return normalize(
                    to_float3(
                        sdScene(to_float3(p.x + e, p.y, p.z)).d - sdScene(to_float3(p.x - e, p.y, p.z)).d,
                        sdScene(to_float3(p.x, p.y + e, p.z)).d - sdScene(to_float3(p.x, p.y - e, p.z)).d,
                        sdScene(to_float3(p.x, p.y, p.z + e)).d - sdScene(to_float3(p.x, p.y, p.z - e)).d
                    ));
}

__KERNEL__ void Clouds2DshadowsJipi613Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_POINT0(TexXY, 0.0f, 0.0f);
    CONNECT_SLIDER0(TexSize, 0.0f, 20.0f, 1.0f);
    CONNECT_SLIDER1(TexSize2, 0.0f, 20.0f, 1.0f);

    CONNECT_POINT1(ViewXY, 0.0f, 0.0f);
    CONNECT_SLIDER2(ViewZ, -10.0f, 20.0f, 0.0f);

    CONNECT_COLOR0(Color, 0.74296875f, 0.86953125f, 0.98234375f, 1.0f);
    CONNECT_SLIDER3(Bright, -1.0f, 1.0f, 0.01f);

    float3 lightPos = to_float3(100.0f, 1000.0f, 0.0f);
    float2 uv = (fragCoord - 0.5f * iResolution)/iResolution.y;

    float3 ro = to_float3(0.0f, 1.0f, -3.0f);
    
    ro.x += ViewXY.x;
    ro.y += ViewXY.y;
    ro.z += ViewZ;
    
    float3 rd = normalize(to_float3_aw(uv, 1.0f));

    Object result = rayMarching(ro, rd);
    float3 clouds;
    float3 col = to_float3_s(0.2f);
    
    if(result.d <= MAX_DIST)
    {
        float3 p = ro + rd * result.d;
        float4 clouds = texture(iChannel0, swi2(p,x,z) / 60.0f * 0.5f + 0.5f);
        if(result.id == 0)
        {        
            col = _mix(to_float3_s(1.0f), to_float3(0.52734375f, 0.8046875f, 0.91796875f), clouds.w) *  _mix(diffuseCloud(p, lightPos, swi3(clouds,x,y,z)), 1.0f, clouds.w);
        }
        else if(result.id == 1)
        {
            float3 normal = calcNormal(p);
            p.x /= R.x/R.y;
            col = swi3(texture(iChannel1, ((swi2(p,x,z)*TexSize2 / 2.0f * 0.5f + 0.5f)+TexXY)*TexSize),x,y,z) * _mix(diffuse(p, lightPos, normal), 1.0f, clouds.w);
        }
    }

    //col = _mix(to_float3(0.74296875f, 0.86953125f, 0.98234375f), col, _expf(-result.d * 0.01f));
    col = _mix(swi3(Color,x,y,z), col, _expf(-result.d * Bright));//0.01f));
    // Output to screen
    fragColor = to_float4_aw(col, Color.w);
    //fragColor = to_float4_aw(texture(iChannel0, uv * 0.5f + 0.5f).aaa,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}