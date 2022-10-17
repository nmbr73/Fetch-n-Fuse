
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Volumen' to iChannel0


#define R iResolution

#define PI  3.1415926f
__DEVICE__ mat2 rot(float c){float s=_sinf(c);return to_mat2(c=_cosf(c),s,-s,c);}

#define normal(FUNC,P,E)\
(\
    to_float3(\
        FUNC(P+to_float3(E,0,0),iTime,iChannel0)-FUNC(P-to_float3(E,0,0),iTime,iChannel0),\
        FUNC(P+to_float3(0,E,0),iTime,iChannel0)-FUNC(P-to_float3(0,E,0),iTime,iChannel0),\
        FUNC(P+to_float3(0,0,E),iTime,iChannel0)-FUNC(P-to_float3(0,0,E),iTime,iChannel0)\
     )/(E*2.0f)\
)

__DEVICE__ float map(float3 p, float iTime, __TEXTURE2D__ iChannel0){
    float d = 1e9;
    
    p -= (swi3(decube_f3(iChannel0,p),x,y,z)-0.5f)*0.1f;
    
    d = _fminf(d,length(fract_f3(p)-0.5f)+1.0f);
    
    swi2S(p,x,y, (swi2(p,x,y)+swi2(p,y,x)*to_float2(-1,1))/_sqrtf(2.0f));
    
    swi2S(p,x,z, (swi2(p,x,z)+swi2(p,z,x)*to_float2(-1,1))/_sqrtf(2.0f));
    p*=0.4f;
    p-=iTime*0.3f;
    d = _fminf(d,(length(fract_f3(p)-0.5f))/0.4f);
        
    //d = _fminf(d,(sdBox(fract(p-0.5f)-0.5f,to_float3(0.4f,0.5f,0.6f))+0.7f)/0.4f);
    
    return d;
}

__DEVICE__ float trace(float3 ro,float3 rd, float iTime, __TEXTURE2D__ iChannel0){
    float3 p = ro;
    float t = 0.0f;
    float h = -0.4f;
    for(int i=0;i<40;i++){
        t += (map(p,iTime,iChannel0)+t*h)/(1.0f-h);
        p = ro+rd*t;
    }
    return t;
}

__KERNEL__ void FluffballsFuse(float4 O, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    float2 uv = (2.0f*U-R)/R.y*0.8f;
    float2 m = (2.0f*swi2(iMouse,x,y)-R)/R.y*2.0f;

    float3 ro = to_float3(_sinf(iTime*0.2f)*4.0f,_sinf(0.1f*iTime*1.23f)*4.0f,-0)+iTime;
    float3 rd = normalize(to_float3_aw(uv,1));
    
    if(iMouse.z>0.0f){
        swi2S(rd,y,z, mul_f2_mat2(swi2(rd,y,z),rot(-m.y)));
        swi2S(rd,x,z, mul_f2_mat2(swi2(rd,x,z),rot(-m.x)));
    }
    
    swi2S(rd,y,z, mul_f2_mat2(swi2(rd,y,z),rot(iTime*0.37f)));
    swi2S(rd,x,y, mul_f2_mat2(swi2(rd,x,y),rot(iTime*0.4f)));
     
    
    //swi3S(O,x,y,z, (rd*0.5f+0.5f)*2.0f);
    float3 Oxyz = (rd*0.5f+0.5f)*2.0f;
    
    float t = trace(ro,rd, iTime, iChannel0);
    
    float3 p = ro+rd*t;
    float3 n = normal(map,p,0.15f);
    
    Oxyz += to_float3(1,2,3)*_fmaxf(dot(n,normalize(to_float3(0,1,0)))*0.5f+0.5f,0.0f)*0.2f;//n*0.5f+0.5f;
    Oxyz += to_float3(4,2,1)*_fmaxf(dot(n,normalize(to_float3(3,1,0))),0.0f);//n*0.5f+0.5f;


    Oxyz += to_float3(0.1f,0.2f,0.3f)*_expf(t*0.4f);
    Oxyz *= 0.6f;Oxyz-=0.4f;
    
    Oxyz = 1.0f - exp_f3(-Oxyz);
    Oxyz = pow_f3(Oxyz,to_float3_s(1.0f/2.2f));
    
    O = to_float4_aw(Oxyz,1.0f);


  SetFragmentShaderComputedColor(O);
}