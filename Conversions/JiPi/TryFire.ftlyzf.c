// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel0


#define PI 3.1415926f
#define SEMI_PI 1.5707963f
//from iq
__DEVICE__ float3 palete(float h,float s,float v){
   float3 a = to_float3(v,v,v);
   float3 b = to_float3(s,s,s);
   float3 c = to_float3(1,1,1);
   
   float3 d = to_float3(0.0f,0.33f,0.67f);
   return a+b*cos_f3(2.0f*PI*(c*h+d)); 
} 

__DEVICE__ mat2 rot2D(float a){
   float c = _cosf(a);
   float s = _sinf(a);
   return to_mat2(c,s,-s,c); 
}

//from iq https://www.iquilezles.org/www/articles/fbm/fbm.htm
__DEVICE__ float fbm(float2 uv, __TEXTURE2D__ iChannel0) {
    float total = 0.0f, amp = 1.0f,freq = 1.0f;
    float G = _expf(-0.7f);
    for (int i = 0; i <6; i++) {
        //total += noise(uv*freq) * amp;
        total += texture(iChannel0,uv*freq).x * amp;
        freq = freq*2.0f;
        amp *= G;
    }
    return total;
}


__KERNEL__ void TryFireFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float2 uv   = fragCoord / iResolution-to_float2_s(0.5f);
    float2 cuv  = to_float2(uv.x*(iResolution.x/iResolution.y),uv.y);
    float id  = _floor(cuv.x/0.4f);
    cuv.x     = mod_f(cuv.x,0.4f)-0.2f;
    float3 col  = to_float3_s(0); 
    float2 dir  = mul_f2_mat2(to_float2(0,1),rot2D(0.0f)); cuv.y+=0.2f;
    
    float f   = fbm((cuv-dir*(iTime+id)*0.8f)*0.031f, iChannel0);
    float2  suv = normalize(to_float2(cuv.x,cuv.y*0.23f));
    float dp  = clamp(dot(dir,suv),0.0f,1.0f);
      
    float v   = _powf((0.2f*dp)/length(suv)/length(cuv),5.0f)*dp*_powf(f,f*9.0f);
    col = to_float3(1.1f,1.1f,0.8f)*cuv.x; 
    col += _mix(v,_powf(0.025f*f/length(to_float2(cuv.x,cuv.y-0.02f)),8.0f),0.5f);
    col*= palete(0.94f+id*0.28f,1.0f,1.0f);
     
    col = smoothstep(to_float3_s(0.0f),to_float3_s(1.0f),col);
    //col = _powf(col,to_float3_s(1.6f));
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}