
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float3 kernel(int2 fragCoord, float2 R, __TEXTURE2D__ iChannel1, float PreNumPos, float PreNumNeg, int Range, float Diff){
    float3 num = to_float3_s(0.0f);
    float3 preNum = swi3(texture(iChannel1, (make_float2(fragCoord)+0.5f)/R),x,y,z);
    for(int i = -Range; i < Range+1 ; i++ ){
        for(int j = -Range; j < Range+1 ; j++ ){
            if(i != 0 || j != 0){
                num += swi3(texture(iChannel1, (make_float2(fragCoord + to_int2(i,j))+0.5f)/R),x,y,z);
            }  
        }
    }
    
    float3 diff = num/Diff - preNum; //8.0f
    if(diff.x > 0.0f){
        preNum = preNum + diff*1.0f * PreNumPos;
    }else{
        preNum = preNum + PreNumNeg;
    }
    return preNum;
}

__DEVICE__ float3 getColor(int2 fragCoord, float2 R, __TEXTURE2D__ iChannel1, float PreNumPos, float PreNumNeg, int Range, float Diff){
    float3 num = kernel(fragCoord, R, iChannel1, PreNumPos,PreNumNeg, Range, Diff);
    return num;
}

__KERNEL__ void Diffusion10Jipi649Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);

    CONNECT_SLIDER0(PreNumPos, -1.0f, 1.0f, 1.0f);
    CONNECT_SLIDER1(PreNumNeg, -1.0f, 1.0f, 0.0f);
    CONNECT_SLIDER2(Diff, 0.0f, 32.0f, 8.0f);
    CONNECT_INTSLIDER0(Range, 1, 5, 1);
    CONNECT_BUTTON0(Modus, 0, Elf,  Zwoelf, DreiZehn, NN, Special);

    if (Modus == 1) PreNumPos = -0.001f, PreNumNeg = 0.01f , Range = 1;
    if (Modus == 2) PreNumPos = -0.001f, PreNumNeg = 0.01f , Range = 10;
    

    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
  
    float3 col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
float AAAAAAAAAAAAAAAAA;    
    /**/
    if(iFrame < 10 || Reset){
        col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    }else{
        col = getColor(to_int2_cfloat(fragCoord),iResolution,iChannel1, PreNumPos, PreNumNeg, Range, Diff);
    }
    /**/
    fragColor = to_float4_aw(col,1);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define S(a,b,t) smoothstep(a,b,t)

__KERNEL__ void Diffusion10Jipi649Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;

    //uv.x *= iResolution.x/iResolution.y;
    float3 col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}