
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float calc(float colorSum, int range, float preColor){
    float d = float (range*range - 1);
    float diff = colorSum/d - preColor;
    
    float result=0.0f;
    
    if(diff > 0.0f){
        if(colorSum/d > 4.0f ){
            result += -0.01f; 
        }else{
            result = preColor + 0.01f;
        }
        
    }else{
        result = 0.0f;
    }
    
    return result;
}

__DEVICE__ float3 kernel(int2 fragCoord, float2 R, __TEXTURE2D__ iChannel1){
    float3 colorSum = to_float3_s(0.0f);
    //float3 preColor = texelFetch(iChannel1, fragCoord, 0).rgb;
    float3 preColor = swi3(texture(iChannel1, (make_float2(fragCoord)+0.5f)/R),x,y,z);
    int range = 10;
    for(int i = -range; i < range+1 ; i++ ){
        for(int j = -range; j < range+1 ; j++ ){
            if(i != 0 || j != 0){
                //colorSum += texelFetch(iChannel1, fragCoord + to_int2(i,j), 0).rgb;
                colorSum += swi3(texture(iChannel1, (make_float2(fragCoord + to_int2(i,j))+0.5f)/iResolution),x,y,z);
            }  
        }
    }
    
    float3 c = to_float3(calc(colorSum.x, range, preColor.x),
                         calc(colorSum.y, range, preColor.y),
                         calc(colorSum.z, range, preColor.z));
                        
    return c;
}

__DEVICE__ float3 getColor(int2 fragCoord, float2 R, __TEXTURE2D__ iChannel1){
    float3 num = kernel(fragCoord,R,iChannel1);
    return num;
}


__DEVICE__ float2 N22(float2 uv){
    float3 a = fract_f3(swi3(uv,x,y,x) * to_float3(123.34f,234.34f,345.65f));
    a += dot(a, a+34.45f);
    return fract_f2(to_float2(a.x*a.y, a.y*a.z));
}

__DEVICE__ float3 circle(float2 uv, float2 pos, float size, float2 R) {
    uv.x = uv.x*iResolution.x/iResolution.y;
    uv = uv - pos;
float zzzzzzzzzzzzzzzzz;    
    float3 col = to_float3_s(length(10.0f/size*uv));
    float3 result = to_float3_s(smoothstep(0.98f,1.0f,col.x)); 
    return result;
}

__KERNEL__ void DiffusionTest23JipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
  
    float3 col = to_float3_s(0.0f);
    float2 pos = to_float2_s(1.0f);

float AAAAAAAAAAAAAAAA;    

    /**/
    if(iFrame < 10 || Reset){
        col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
        //col = circle(uv, pos, 0.1f,R);
        for(int i = 0; i < 10; i++ ){
            pos = to_float2_s((float)(i)+1.0f);
            //col += 1.0f* (1.0f - circle(uv, N22(pos), 1.0f,R));
        }
    }else{
        col = getColor(to_int2((int)fragCoord.x,(int)fragCoord.y),R,iChannel1);
    }
    /**/
    
    fragColor = to_float4_aw(col,1);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/3405e48f74815c7baa49133bdc835142948381fbe003ad2f12f5087715731153.ogv' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define S(a,b,t) smoothstep(a,b,t)

__KERNEL__ void DiffusionTest23JipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;

    //uv.x *= iResolution.x/iResolution.y;
    float3 col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    //col = (col - 0.5f)* to_float3(_tex2DVecN(iChannel1,uv.x,uv.y,15)) -0.8f;
    col = col * to_float3(0.5f,0.3f,0.3f);
    fragColor = to_float4_aw(col,1.0f);
float IIIIIIIIIIIIII;
  SetFragmentShaderComputedColor(fragColor);
}