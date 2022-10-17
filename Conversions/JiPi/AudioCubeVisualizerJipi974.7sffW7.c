
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/a6a1cf7a09adfed8c362492c88c30d74fb3d2f4f7ba180ba34b98556660fada1.mp3' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*
Audio cube II - Visualiser - 
Variation of AudioCube by: kuvkar - 19th March, 2015 https://www.shadertoy.com/view/llBGR1
Auido Cube II by: uNiversal - 28th May, 2015
Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
*/
 
//Distance field functions from
//http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
/*-------------------------------------------------------------------------*/
__DEVICE__ float sdSphere( float3 p, float s )
{
    return length(p)-s;
}

__DEVICE__ float sdTorus( float3 p, float2 t )
{
    float2 q = to_float2(length(swi2(p,x,z))-t.x,p.y);
    return length(q)-t.y;
}

__DEVICE__ float udBox( float3 p, float3 b )
{
    return length(_fmaxf(abs_f3(p)-b,to_float3_s(0.0f)));
}

__DEVICE__ float sdHexPrism( float3 p, float2 h )
{
    float3 q = abs_f3(p);
    return _fmaxf(q.z-h.y,_fmaxf((q.x*0.866025f+q.y*0.5f),q.y)-h.x);
}

__DEVICE__ float udRoundBox( float3 p, float3 b, float r )
{
    return length(_fmaxf(abs_f3(p)-b,to_float3_s(0.0f)))-r;
}

__DEVICE__ float smin( float a, float b, float k )
{
    float res = _expf( -k*a ) + _expf( -k*b );
    return -_logf( res )/k;
}

__DEVICE__ float sdBox( float3 p, float3 b )
{
    float3 d = abs_f3(p) - b;
    return _fminf(max(d.x,_fmaxf(d.y,d.z)),0.0f) +
                   length(_fmaxf(d,to_float3_s(0.0f)));
}

/*-------------------------------------------------------------------------*/

#define PI 3.14159f
#define TYPE_CUBE 1
#define TYPE_SPHERE 2
#define OBJECT_COUNT 2

__DEVICE__ mat3 rotx(float a) { mat3 rot; rot.r0 = to_float3(1.0f, 0.0f, 0.0f);         rot.r1 = to_float3(0.0f, _cosf(a), -_sinf(a)); rot.r2 = to_float3(0.0f, _sinf(a), _cosf(a)); return rot; }
__DEVICE__ mat3 roty(float a) { mat3 rot; rot.r0 = to_float3(_cosf(a), 0.0f, _sinf(a)); rot.r1 = to_float3(0.0f, 1.0f, 0.0f);          rot.r2 = to_float3(-_sinf(a), 0.0f, _cosf(a)); return rot; }

struct Object
{
    mat3 rot;
    float3 pos;
    float3 size;
    int type;
};



__DEVICE__ float dist(float3 position, Object objects[OBJECT_COUNT], __TEXTURE2D__ iChannel0)
{
    float m = 9999.0f;
    float4 audio = texture(iChannel0, to_float2(0.0f, 0.0f));
    float2 uv = to_float2(position.x, position.y) * 1.0f;
    float4 col = _tex2DVecN(iChannel0,uv.x,uv.y,15);

    Object o = objects[0];
    float3 p = position + o.pos;

    p = mul_f3_mat3(p , o.rot);
    float f = 0.0f;

    float a = sdBox(p, o.size);
    float b = sdSphere(p, o.size.x);
    float au = audio.x;

    f = au * a + (1.0f - au) * b;
    f -= sdSphere(p, o.size.x) * (0.3f + au * 0.4f);

    m = _fminf(f, m);
    return m;
}

__DEVICE__ float toClipSpace(float f)
{
    return f * 2.0f - 1.0f;
}

__DEVICE__ float3 lookAt(float3 from, float3 to, float3 dir)
{
    mat3 m;
  
    float3 fwd = normalize(to - from);
    float3 _up = to_float3(0.0f, 1.0f, 0.0f);
    float3 r = cross(fwd, _up);
    float3 u = cross(r, fwd);
    
    m.r0 = r;
    m.r1 = u;
    m.r2 = fwd;
    float3 d = mul_mat3_f3(m , dir);    
    d.z *= -1.0f;
    return d;
}

__DEVICE__ bool trace(float3 from, float3 dir, out float3 *hitPosition, out float *m, Object objects[OBJECT_COUNT], __TEXTURE2D__ iChannel0)
{
    const int steps = 300;
    float step = 0.01f;    
    float3 rp = from;
    *m = 99999.0f;
float zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz;      
    for (int i = 0; i < steps; ++i)
    {
        rp += dir * step;
        float sp = dist(rp,objects,iChannel0);
        step  = sp;
        *m = _fminf(*m, _fabs(sp));
        if (_fabs(sp) <= 0.001f)
        {
          *hitPosition = rp;
          return true;
        }
    }
    return false;
}

__DEVICE__ Object getObject(float3 position, int type)
{
    Object c;
    c.pos = position;
    c.rot = to_mat3_f(1.0f);
    c.type = type;
    c.size = to_float3(0.1f, 0.1f, 0.1f);
    return c;
}

__KERNEL__ void AudioCubeVisualizerJipi974Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    Object objects[OBJECT_COUNT];

    float2 uv = to_float2(fragCoord.x / iResolution.x, fragCoord.y / iResolution.x);
    uv.y += (iResolution.y / iResolution.x) * 0.4f;

    float4 audio = texture(iChannel0, to_float2(1.0f, 0.0f));

    objects[0] = getObject(to_float3_s(0.0f), TYPE_CUBE);
    objects[0].size = to_float3(0.1f, 0.1f, 0.1f);

    objects[0].rot = mul_mat3_mat3(objects[0].rot, rotx(iTime * 1.5f));
    objects[0].rot = mul_mat3_mat3(objects[0].rot, roty(_sinf(iTime * 1.5f)));

    float3 camPos = to_float3(0.0f, 0.0f, 1.0f);    
    float3 lk = to_float3(-toClipSpace(uv.x), -toClipSpace(uv.y), -2.0f);
    float3 mclip = to_float3_s(0.0f);
    float3 dir = lookAt(camPos, mclip, normalize(lk));
    float3 hit = to_float3_s(0.0f);
    float4 color = to_float4_s(0.0f);

    float m;
                   
    bool h = trace(camPos, dir, &hit, &m, objects,iChannel0);
    float4 aColor = to_float4_s(1.0f);

    float p =  _powf(length(uv - to_float2(0.5f, 0.5f)), 3.0f);
    aColor.x = 0.5f + (_sinf((iTime + 100.0f) * 0.25f) * 0.5f);
    aColor.y = 0.5f + (_cosf((iTime + 300.0f) * 0.15f) * 0.5f);

    if(h)
    {
        float2 offset = to_float2(0.001f, 0.0f);
        float3 grad = normalize(to_float3(dist(hit + swi3(offset,x,y,y),objects,iChannel0) - dist(hit - swi3(offset,x,y,y),objects,iChannel0), 
                                          dist(hit + swi3(offset,y,x,y),objects,iChannel0) - dist(hit - swi3(offset,y,x,y),objects,iChannel0),
                                          dist(hit + swi3(offset,y,y,x),objects,iChannel0) - dist(hit - swi3(offset,y,y,x),objects,iChannel0)));
        
        float d = clamp(dot(grad, to_float3(0.0f, 0.0f, 1.0f)), 0.0f, 1.0f);
        float rim = (1.0f - d) * 1.4f;        
        color += aColor * _powf(rim, 2.0f);
    }
    else
    {
        color = aColor * _powf(length(uv - to_float2(0.5f, 0.5f)) * (1.0f + p * audio.x * 5.0f), 1.4f) * 1.5f;
    }

    float d = (1.0f - m * 75.0f) * 15.0f * p;
    color += aColor * _powf(clamp(d, 0.0f, 0.0f), 1.2f);

    fragColor = color;


  SetFragmentShaderComputedColor(fragColor);
}