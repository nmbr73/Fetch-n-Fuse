
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0


#define ei(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))

__DEVICE__ float ln (float3 p, float3 a, float3 b) {
    float l = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
    return _mix(0.75f,1.0f,l)*length(p-a-(b-a)*l);
}
__DEVICE__ float4 map (float3 u, float T) {
    float2 uxz = mul_f2_mat2(swi2(u,x,z) , ei(1.1f+T));
    u.x=uxz.x;u.z=uxz.y;
    float2 uxy = mul_f2_mat2(swi2(u,x,y) , ei(1.1f));
    u.x=uxy.x;u.y=uxy.y;
    
    float d = 1e9;
    float4 c=to_float4_s(0);
    float sg = 1e9;
    float l = 0.08f;
    u.y = _fabs(u.y);
    u.y+=0.1f;
    mat2 M1 = ei(1.0f);
    float w = 0.02f;
    mat2 M2 = ei(0.6f);
    mat2 M3 = ei(0.4f+0.2f*_sinf(T));
    for (float i = 1.0f; i < 20.0f; i++)
    {
        sg = ln(u,to_float3_s(0),to_float3(0,l,0))/l;
        d = _fminf(d,sg*l-w);
        w *= 0.7f;
        u.y -= l;
        float2 uxz = mul_f2_mat2(swi2(u,x,z) , M1);
        //swi2(u,x,z) = _fabs(swi2(u,x,z));
        uxz = abs_f2(uxz);
        u.x=uxz.x;u.z=uxz.y;
        
        float2 uzy = mul_f2_mat2(swi2(u,z,y) , M3);
        u.z=uzy.x;u.y=uzy.y;
        
        l *= 0.75f;
        c += _expf(-sg*sg)*(0.7f+0.5f*sin_f4(2.0f+3.0f*i/16.0f+to_float4(1,2,3,4)));
    }
    return to_float4_aw(swi3(c,x,y,z),d);
}


__KERNEL__ void NightGardenFuse(float4 Q, float2 U, float iTime, float2 iResolution)
{

    float2 R = iResolution; float T = iTime;
    Q = to_float4_s(0);
    float3 p = to_float3_aw( 0.3f*(U-0.5f*R)/R.y, 2);
    float3 d = normalize( to_float3(0,0,0.74f) - p );
    for (float i = 0.0f; i < 30.0f; i+=1.0f){
        float4 x = map(p,T);
        float f = 1.0f/(1.0f+1e3*_fabs(p.z-0.2f));
        p += d* _fmaxf( _fabs(x.w), 2e-5/f );
        Q += 0.01f*x*_expf(-1e3*(x.w*x.w)*f);
       
    }


  SetFragmentShaderComputedColor(Q);
}