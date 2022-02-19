
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// https://www.shadertoy.com/view/ssBczR
// based on https://www.shadertoy.com/view/sdscDs insulate by jt
// wrap 2d SDF around a torus - less exact than plane version

#define EPSILON 0.001
#define DIST_MAX 50.0

// https://iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
__DEVICE__ float halfspace(float3 p, float d)
{
    return p.z - d;
}

__DEVICE__ float torus(float3 p, float2 t)
{
    float2 q = to_float2(length(swi2(p,x,z)) - t.x, p.y);
    return length(q) - t.y;
}

__DEVICE__ float box2d(float2 p)
{
    float2 d = abs_f2(p) - 1.0f;
    return _fminf(max(d.x, d.y),0.0f) + length(_fmaxf(d, to_float2_s(0.0f)));
}

__DEVICE__ float circle2d(float2 p, float r)
{
    return length(p) - r;
}

__DEVICE__ float segment2d(float2 p, float2 a, float2 b)
{
    float2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0f, 1.0f);
    return length(pa - ba * h);
}

__DEVICE__ float insulate_box(float3 p)
{
    float dp = torus(p, to_float2(1,0.5f)); // distance to torus
    float d = box2d(swi2(p,x,y)); // distance to 2d SDF
    return _sqrtf(dp*dp+d*d); // 3dify 2d SDF
}

__DEVICE__ float insulate_boxes(float3 p)
{
    float dp = torus(p, to_float2(1,0.5f)); // distance to torus
    float d = _fabs(_fabs(box2d(swi2(p,x,y))) - 0.5f) - 0.25f; // distance to 2d SDF
    return _sqrtf(dp*dp+d*d); // 3dify 2d SDF
}

__DEVICE__ float insulate_circle(float3 p)
{
    float dp = torus(p, to_float2(1,0.5f)); // distance to torus
    float d = circle2d(swi2(p,x,y), 1.0f); // distance to 2d SDF
    return _sqrtf(dp*dp+d*d); // 3dify 2d SDF
}

__DEVICE__ float insulate_circles(float3 p)
{
    float dp = torus(p, to_float2(1,0.5f)); // distance to torus
    float d = _fabs(_fabs(circle2d(swi2(p,x,y), 1.0f)) - 0.5f) - 0.25f; // distance to 2d SDF
    return _sqrtf(dp*dp+d*d); // 3dify 2d SDF
}

__DEVICE__ float insulate_segment(float3 p)
{
    float dp = torus(p, to_float2(1,0.5f)); // distance to torus
    float d = segment2d(swi2(p,x,y), to_float2_s(-1.5f), to_float2_s(+1.5f)); // distance to 2d SDF
    return _sqrtf(dp*dp+d*d); // 3dify 2d SDF
}

__DEVICE__ float map(float3 p, float iTime)
{
  
    float d = _mix(0.01f, 0.1f, 0.5f + 0.5f * _cosf(iTime));
    return
        _fminf
        (
            _fminf
            (
                _fminf
                (
                    insulate_circles(p) - d,
                    insulate_boxes(p) - d
                ),
                insulate_segment(p) - d
            ),
            halfspace(p, -1.2f)
        );
}

// https://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm tetrahedron normals
__DEVICE__ float3 normal(float3 p, float iTime)
{
    const float h = EPSILON;
    const float2 k = to_float2(1,-1);
    return
        normalize
        (
            swi3(k,x,y,y) * map(p + swi3(k,x,y,y)*h,iTime)
            +
            swi3(k,y,y,x) * map(p + swi3(k,y,y,x)*h,iTime)
            +
            swi3(k,y,x,y) * map(p + swi3(k,y,x,y)*h,iTime)
            +
            swi3(k,x,x,x) * map(p + swi3(k,x,x,x)*h,iTime)
        );
}

__DEVICE__ float trace(float3 ro, float3 rd, float iTime)
{
    for(float t = 0.0f; t < DIST_MAX;)
    {
        float h = map(ro + rd * t,iTime);
        if(h < EPSILON)
            return t;
        t += h * 0.5f; // NOTE: due to inexact SDF step must be reduced
    }
    return DIST_MAX;
}

// https://iquilezles.org/www/articles/rmshadows/rmshadows.htm
__DEVICE__ float shadow( in float3 ro, in float3 rd, float mint, float maxt, float iTime )
{
    for( float t=mint; t<maxt; )
    {
        float h = map(ro + rd*t,iTime);
        if( h<EPSILON )
            return 0.0f;
        t += h * 0.5f; // NOTE: due to inexact SDF step must be reduced
    }
    return 1.0f;
}

// https://www.shadertoy.com/view/Xds3zN raymarching primitives
__DEVICE__ float calcAO( in float3 pos, in float3 nor, float iTime )
{
  
    float occ = 0.0f;
    float sca = 1.0f;
    for( int i=0; i<5; i++ )
    {
        float h = 0.01f + 0.12f*(float)(i)/4.0f;
        float d = map( pos + h*nor, iTime );
        occ += (h-d)*sca;
        sca *= 0.95f;
        if( occ>0.35f ) break;
    }
    return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f ) ;
}

#define pi 3.1415926

__KERNEL__ void ApplyInsuFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    float2 uv = fragCoord / iResolution;
    float2 ndc = 2.0f * uv - 1.0f;
    ndc.x *= (iResolution.x) / (iResolution.y);
    float mx = 2.0f * pi * (iMouse.x) / (iResolution.x);
    float my = pi / 2.0f + pi / 2.0f * (iMouse.y) / (iResolution.y);
    mx = (iMouse.x != 0.0f) ? mx : 2.0f * pi * fract(iTime * 0.1f);
    my = (iMouse.y != 0.0f) ? my : pi * 3.0f / 4.0f;;

    mat2 R = to_mat2_f2_f2(to_float2(_cosf(mx), _sinf(mx)), to_float2(-_sinf(mx), _cosf(mx)));
    float3 ro = to_float3(0.0f, 0.0f, -5.0f );//to_float3(0.0f, -10.0f * my, 0.0f);
    //mat2 S = mat2(to_float2(0.0f, 1.0f), to_float2(-1.0f, 0.0f));
    mat2 S = to_mat2_f2_f2(to_float2(_cosf(my), _sinf(my)), to_float2(-_sinf(my), _cosf(my)));
    //ro.yz=S*swi2(ro,y,z);
    swi2S(ro,y,z, mul_mat2_f2(S,swi2(ro,y,z)));
    
    //swi2(ro,x,y) = R * swi2(ro,x,y);
    swi2S(ro,x,y, mul_mat2_f2(R,swi2(ro,x,y)));

    float3 rd = normalize(to_float3_aw(0.5f * swi2(ndc,x,y), 1.0f)); // NOTE: omitting normalization results in clipped edges artifact
    swi2S(rd,y,z, mul_mat2_f2(S,swi2(rd,y,z)));
    swi2S(rd,x,y, mul_mat2_f2(R,swi2(rd,x,y)));

    float dist = trace(ro, rd, iTime);
    float3 dst = ro + rd * dist;
    float3 n = normal(dst, iTime);

    float3 lightdir = normalize(to_float3(1.0f, 1.0f, 1.0f));
    float3 ambient = to_float3_s(0.4f);
    float brightness = _fmaxf(dot(lightdir, n), 0.0f);
    brightness *= shadow(ro+rd*dist,lightdir, 0.01f, DIST_MAX, iTime); // XXX artifacts on cylinder XXX
    float3 color = to_float3_s(1.0f);
    color *= (n * 0.5f + 0.5f);
    color = (ambient * calcAO(dst, n, iTime) + brightness) * color;

    fragColor = _mix(to_float4_aw(color, 1.0f), to_float4_s(0.0f), step(DIST_MAX, dist));
    fragColor = sqrt_f4(fragColor); // approximate gamma

  SetFragmentShaderComputedColor(fragColor);
}