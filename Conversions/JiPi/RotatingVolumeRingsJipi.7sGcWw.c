
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


//cloud marching function from shader "Cloudy Shapes" by kaneta: https://www.shadertoy.com/view/WdXGRj
//soft shadows function by iq in shader "Soft Shadow Variation.": https://www.shadertoy.com/view/lsKcDD
//cosine color palette function courtesy of iq

__DEVICE__ float hash( float n )
{
    return fract(_sinf(n)*43758.5453f);
}
__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}
__DEVICE__ float3 rot_x(in float3 v, in float theta) {
    mat3 rotx = to_mat3(1.0f, 0.0f, 0.0f, 0.0f, _cosf(theta), _sinf(theta), 0.0f, -_sinf(theta), _cosf(theta));
    return mul_mat3_f3(rotx , v);
}
__DEVICE__ float3 rot_y(in float3 v, in float theta) {
    mat3 roty = to_mat3(_cosf(theta), 0.0f, -_sinf(theta), 0.0f, 1.0f, 0.0f, _sinf(theta), 0.0f, _cosf(theta));
    return mul_mat3_f3(roty , v);
}
__DEVICE__ float3 rot_z(in float3 v, in float theta) {
    mat3 roty = to_mat3(_cosf(theta), _sinf(theta), 0.0f, -_sinf(theta), _cosf(theta), 0.0f, 0.0f, 0.0f, 1.0f);
    return mul_mat3_f3(roty , v);
}
__DEVICE__ float3 computeColor( float density, float radius )
{
  // color based on density alone, gives impression of occlusion within
  // the media
  float3 result = _mix( to_float3(1.0f,0.9f,0.8f), to_float3(0.4f,0.15f,0.1f), density );
  
  // color added to the media
  float3 colCenter = 7.0f*to_float3(0.8f,1.0f,1.0f);
  float3 colEdge = 1.5f*to_float3(0.48f,0.53f,0.5f);
  result *= _mix( colCenter, colEdge, _fminf( (radius+0.05f)/0.9f, 1.15f ) );
  
  return result;
}
__DEVICE__ float noise( in float3 x)
{
  float3 p = _floor(x);
  float3 f = fract_f3(x);
  f = f*f*(3.0f - 2.0f*f);
  float n = p.x + p.y*157.0f + 113.0f*p.z;
  return _mix(_mix(_mix( hash(n+  0.0f), hash(n+  1.0f),f.x),
                   _mix( hash(n+157.0f), hash(n+158.0f),f.x),f.y),
              _mix(_mix( hash(n+113.0f), hash(n+114.0f),f.x),
                   _mix( hash(n+270.0f), hash(n+271.0f),f.x),f.y),f.z);
}

__DEVICE__ float fbm (in float3 p, in int o)
{
  const mat3 m3  = to_mat3( 0.00f,  0.80f,  0.60f,
                         -0.80f,  0.36f, -0.48f,
                         -0.60f, -0.48f,  0.64f );
  
    float f = 0.0f;
    float freq = 1.0f;
    for (int i = 0; i < o; i++)
    {
        float n = noise(p * freq) / freq;
        f += n;
        freq *= 2.0f;
        p = mul_mat3_f3(m3 , p);
    }
    return f;
}
__DEVICE__ float rmf(float3 p)
{
    float signal = 0.0f;
    float value  = 0.0f;
    float weight = 1.0f;
    float h = 1.0f;
    float f = 1.0f;

    for (int i=0; i < 4; i++) 
    {
        signal = noise(p)*2.0f-0.4f;
        signal = _powf(1.0f - _fabs(signal), 2.0f) * weight;
        weight = clamp(0.0f, 1.0f, signal * 16.0f);
        value += (signal * _powf(f, -1.0f));
        f *= 2.0f;
        p *= 2.0f;
    }
    
    return (value * 1.25f) - 1.0f;
}

  
__DEVICE__ float sdSphere(in float3 p, in float3 c, in float r, float iTime) {
    p = rot_x(p, iTime * 2.0f);
    return length(p - c) - (r + fbm(p * 1.35f, 4));
}

__DEVICE__ float sdTorus( float3 p, float2 t )
{
  float2 q = to_float2(length(swi2(p,x,z))-t.x,p.y);
  return length(q)-t.y;
}
__DEVICE__ float2 map(in float3 p, float iTime){
    float t = iTime * 0.125f;
    float3 p_rz = rot_z(p, iTime * 0.25f);
    float3 p_rx = rot_x(p, iTime * 0.125f);
    float2 t1 = to_float2(1.0f- sdTorus(p_rz, to_float2(6.0f, 0.5f)) + fbm(t+p_rz+fbm(p, 2)*3.5f, 4)*0.75f, 1.0f);
    float2 t2 = to_float2(1.0f- sdTorus(p_rx, to_float2(12.0f, 1.0f)) +rmf(t+p_rx*1.25f*(fbm(t+p_rx, 4)*0.625f))*0.5f, 2.0f);
    float2 s1 = to_float2(1.0f-sdSphere(p, to_float3_s(0), 0.5f, iTime), 3.0f);
    
    if (t1.x > t2.x && t1.x > s1.x) {
        return t1;
    } else if (t2.x > s1.x) {
        return t2;
    } else {
        return s1;
    }
}

__DEVICE__ float map_2(in float3 p, float iTime){
    float t = iTime * 0.125f;
    float3 p_rz = rot_z(p, iTime * 0.25f);
    float3 p_rx = rot_x(p, iTime * 0.125f);
    float t1 = sdTorus(p_rz, to_float2(6.0f, 0.5f)) + fbm(t+p_rz+fbm(p, 1)*3.5f, 2)*0.75f;
    float t2 = sdTorus(p_rx, to_float2(12.0f, 1.0f)) +rmf(t+p_rx*1.25f*(fbm(t+p_rx, 2)*0.625f))*0.5f;
    float s1 = sdSphere(p, to_float3_s(0), 0.5f, iTime);
    
    return _fminf(t1, _fminf(t2, s1));
}
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float mint, in float tmax, int technique, float iTime )
{
  float res = 1.0f;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    
    for( int i=0; i<16; i++ )
    {
    float h = map_2( ro + rd*t, iTime );

        // traditional technique
        if( technique==0 )
        {
          res = _fminf( res, 10.0f*h/t );
        }
        // improved technique
        else
        {
            // use this if you are getting artifact on the first iteration, or unroll the
            // first iteration out of the loop
            //float y = (i==0) ? 0.0f : h*h/(2.0f*ph); 

            float y = h*h/(2.0f*ph);
            float d = _sqrtf(h*h-y*y);
            res = _fminf( res, 10.0f*d/_fmaxf(0.0f,t-y) );
            ph = h;
        }
        
        t += h;
        
        if( res<0.0001f || t>tmax ) break;
        
    }
    res = clamp( res, 0.0f, 1.0f );
    return res*res*(3.0f-2.0f*res);
}
    
//fixed value for now
//float jitter;

#define MAX_STEPS 35
#define SHADOW_STEPS 6
#define VOLUME_LENGTH 50.0f
#define SHADOW_LENGTH 2.0f
__DEVICE__ float4 cloudMarch(float3 p, float3 ray, float iTime, float jitter)
{
    float density = 0.0f;

    float stepLength = VOLUME_LENGTH / (float)(MAX_STEPS);
    float shadowStepLength = SHADOW_LENGTH / (float)(SHADOW_STEPS);
    float3 light = normalize(to_float3(-1.0f, 2.0f, 1.0f));

    float4 sum = to_float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    float3 pos = p + ray * jitter * stepLength;
    
    for (int i = 0; i < MAX_STEPS; i++)
    {
        if (_fabs(sum.w) < 0.1f) {
          break;
        }
        float2 res_i = map(pos, iTime);
        float d = res_i.x;
        if(d> 0.001f)
        {
            float mat_i = res_i.y;
            float3 lpos = pos + light * jitter * shadowStepLength;
            float shadow = 0.0f;
    
            for (int s = 0; s < SHADOW_STEPS; s++)
            {
                lpos += light * shadowStepLength;
                float2 res_s = map(lpos, iTime);
                float lsample = res_s.x;
                shadow += lsample;
            }
    
            density = clamp((d / float(MAX_STEPS)) * 20.0f, 0.0f, 1.0f);
            float s = _expf((-shadow / float(SHADOW_STEPS)) * 3.0f);
            
            swi3S(sum,x,y,z, swi3(sum,x,y,z) + to_float3_s(s * density) * to_float3(1.1f, 0.9f, 0.5f) * sum.w);
            
            sum.w *= 1.0f-density;
            float3 pa_col = to_float3_s(0.0f);
            float ss = 1.0f;
            if (mat_i == 1.0f){
                pa_col = pal( density * 0.5f, to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(2.0f,1.0f,0.0f),to_float3(0.5f,0.20f,0.25f) );
                ss = clamp(calcSoftshadow(pos, light, 0.01f, 50.0f, 0, iTime)+0.5f, 0.0f, 1.0f);
            } else if (mat_i == 2.0f) {
                pa_col = pal( density*2.0f, to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,1.0f,1.0f),to_float3(0.0f,0.10f,0.20f) );
            } else {
                pa_col = pal( density *8.0f , to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,0.7f,0.4f),to_float3(0.0f,0.15f,0.20f) ) * 3.0f;
            }
            float2 res_m = map(pos + to_float3(0,0.25f,0.0f), iTime);
            float zzzzzzzzzzzzzzzz;
            swi3S(sum,x,y,z, swi3(sum,x,y,z) + _expf(-res_m.x * 0.2f) * pa_col * sum.w); //!!
            sum *= ss;
        }
        pos += ray * stepLength;
    }

    return sum;
}

__DEVICE__ float3 camera(in float2 uv, in float3 ro, float3 ta, float fd){
    float3 up = to_float3(0,1,0); 
    float3 ww = normalize(ta-ro); 
    float3 uu = normalize(cross(ww, up)); 
    float3 vv = normalize(cross(uu, ww)); 
    
    float3 rd = normalize(uv.x*uu + uv.y*vv + fd*ww);
    return rd;
}
__KERNEL__ void RotatingVolumeRingsJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

float IIIIIIIIIIIIIII;
    float2 uv = fragCoord/iResolution;
    uv -= to_float2_s(0.5f);
    uv.x *= iResolution.x/iResolution.y;
    float jitter = 0.0f;
   // jitter = hash(uv.x + uv.y * 57.0f + iTime) * 0.0125f;
    float3 li = normalize(to_float3(0.5f, 0.8f, 3.0f));
    float a = 10.0f * iMouse.x/iResolution.x;
    float3 ro = 1.35f*to_float3( 14.0f * _sinf(a), 13.0f, 14.0f* _cosf(a));
    float3 ta = to_float3(0,0,0);   
    float3 rd = camera(uv, ro, ta, 1.0f);
    fragColor = pow_f4(cloudMarch(ro, rd, iTime, jitter), to_float4_s(2.15f)) * 0.15f;

  SetFragmentShaderComputedColor(fragColor);
}