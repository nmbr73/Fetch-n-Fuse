
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define PI 3.1415926535f
#define T iTime*3.0f
#define PELLET_SIZE 1.0f/16.0f
#define PELLET_NUM 2
#define THICKNESS 0.13f
#define RADIUS 0.7f
//#define COUNTER_ROT // counter rotation that keeps the arc at the same place
#define SMOOTH_MOT // smooth motion of pellets
#define SMOOTH_SHP // smooth shapes

__DEVICE__ float sdArc( in float2 p, in float a, in float ra, float rb ) // By iq: https://www.shadertoy.com/view/wl23RK
{
    a *= PI;
    float2 sc = to_float2( _sinf(a),_cosf(a) );
    p.x = _fabs(p.x);
    return ((sc.y*p.x>sc.x*p.y) ? length(p-sc*ra) : 
                                  _fabs(length(p)-ra)) - rb;
}

__DEVICE__ mat2 rot( float a )
{
    a *= PI;
    float s = _sinf(a), c = _cosf(a);
    return to_mat2( c,-s,s,c );
}

__DEVICE__ float s( float x )
{
#ifdef SMOOTH_MOT
    return smoothstep( 0.0f,1.0f,x );
#endif
    return x;
}

__DEVICE__ float sminCubic( float a, float b, float k ) // By iq: https://iquilezles.org/articles/smin/
{
    float h = _fmaxf( k-_fabs(a-b), 0.0f )/k;
    return _fminf( a, b ) - h*h*h*k*(1.0f/6.0f);
}

__DEVICE__ float3 pal( float x, float3 Color1, float3 Color2 )
{
    //return _mix( to_float3(0.988f,0.569f,0.086f), to_float3(1,0.082f,0.537f), x );
    return _mix( Color1, Color2, x );
}

__DEVICE__ float f(float x) {
    return -2.0f*PELLET_SIZE*x;
}

__DEVICE__ float dist(float2 p, float iTime, bool CounterRot){

if(CounterRot)
    p = mul_f2_mat2(p , rot(-f(T-0.5f)));

    int n = PELLET_NUM;
    float N = float(n);

    float d1 = sdArc( mul_f2_mat2(p,rot( f(_floor(T)) + 1.0f )), 
                      0.5f - PELLET_SIZE, 
                      RADIUS, 
                      THICKNESS);
    float d2 = 9e9;
    for (int i = 0; i < n; i++) {
        float j = float(i);
        float t = s( fract((T + j)/N) );
        float a = _mix( -0.5f, 0.5f - f(1.0f), t) + f(T);
        d2 = _fminf( sdArc( mul_f2_mat2(p , rot(a)),
                            PELLET_SIZE,
                            RADIUS,
                            THICKNESS), d2);
    }
#ifdef SMOOTH_SHP
    float r = _fabs( length(p) - RADIUS ) - THICKNESS; // sdf of ring containing the arcs 
    float d = sminCubic( d1, d2, 0.2f );
    return _fmaxf( d, r );
#endif
    return _fminf(d1, d2);
}

__KERNEL__ void LoadingFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
  
    CONNECT_COLOR0(Color1, 0.988f, 0.569f, 0.086f, 1.0f);
    CONNECT_COLOR1(Color2, 1.0f, 0.082f, 0.537f, 1.0f);
    CONNECT_SLIDER0(Vignette, -1.0f, 5.0f, 1.5f);
    
    CONNECT_CHECKBOX0(Background, 0);
    CONNECT_CHECKBOX1(CounterRot, 0);
    
    float2 uv = fragCoord/iResolution - 0.5f;
    float2 p = (2.0f*fragCoord - iResolution)/iResolution.y;
    

    float d = dist( p, iTime, CounterRot ); // shape
    float m = smoothstep( 0.01f,0.0f,d );
    
    float d1 = dist( p + to_float2(0.0f,0.15f), iTime, CounterRot ); // shadow
    float s = smoothstep( 0.2f,-0.4f,d1 );
    
    m = _fmaxf(s,m); // combine shadow and shape
    
    float3 col = m*pal( p.x - p.y + 0.5f, swi3(Color1,x,y,z), swi3(Color2,x,y,z) ); // color shape and shadow
    col += 1.0f - m; // white background
    col *= 1.0f - Vignette * dot(uv,uv); // vignette
    fragColor = to_float4_aw(col,1.0f);
    
    if(Background) fragColor *= m;

  SetFragmentShaderComputedColor(fragColor);
}