
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------



__DEVICE__ const float renderScale=2.0f;
__DEVICE__ const float loopTime=1.0f;
__DEVICE__ const float population=1.5f;//1.0f<pop<2.0f;
__DEVICE__ const float mouseSize=6.0f;
__DEVICE__ const int freezeFrames=4;

__DEVICE__ int getShift(int iFrame){
    return int((float)(0)+mod_f((float)(iFrame-freezeFrames)/_fabs(loopTime),2.0f));
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel2
// Connect Buffer A 'Texture: Organic 3' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0




__DEVICE__ int2 modIto_float2(int2 a,int2 b){
    return to_int2((int)(mod_f((float)(a.x),(float)(b.x))),(int)(mod_f((float)(a.y),(float)(b.y))));
}
__KERNEL__ void BlockCaBFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  const int   map[] = {15,14,13, 3,11, 5, 6, 1, 7, 9,10, 2,12, 4, 8, 0}; //0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
  const int undomap[]={15,7,11,3,13,5,6,8,14,9,10,4,12,2,1,0};  
  
  int2 seedLen = to_int2(12,12);


 

  const int seed[]={
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,1,
  0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,0, 0,1,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,1, 1,0, 0,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  1,0, 0,1, 0,0, 0,0, 0,0, 0,0,
                               
  0};
  /*
  const int seed_destroys_vertical[]=int[](
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,1,
  0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,0, 0,1,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,1, 1,0, 0,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  1,0, 0,1, 0,0, 0,0, 0,0, 0,0,
                               
  0);
  const int seed_reflect[]=int[](
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
  0,0, 0,0, 0,0, 0,1, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,1, 0,0, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,1, 1,0, 0,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  1,0, 0,1, 0,0, 0,0, 0,0, 0,0,
                               
  0);
  const int seed_moveThrough[]=int[](
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
  0,0, 0,0, 0,0, 0,1, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,1, 0,0, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,1, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,0, 0,1, 1,0, 0,0, 0,0, 0,0,
                               
  0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
  0,0, 1,0, 0,1, 0,0, 0,0, 0,0,
                               
  0);*/

    int isInverted;//"a" to invert time
    if(_tex2DVecN(iChannel2,((float)(65)+0.5)/iResolution.x,((float)(2)+0.5)/iResolution.x,15).x>0.0f)  isInverted=1;
    else                                                                                                isInverted=0;
    
    float2 uv = fragCoord/iResolution;
    int2 worldSize=to_int2_cfloat(iResolution/renderScale);
    float2 edge=(iResolution/renderScale-fragCoord);
    float2 t=uv+to_float2_s(1.0f-1.0f/renderScale);
    int thift = getShift(iFrame);
    if(1.0f>_fminf(min(fragCoord.x,fragCoord.y),_fminf(edge.x,edge.y))){
        fragColor = to_float4(1-thift,0.0f,0.0f,1.0f);//on border
        SetFragmentShaderComputedColor(fragColor);
        return ;
    }if(_fmaxf(t.x,t.y)>1.0f){
        fragColor = to_float4(thift,0.0f,0.0f,1.0f);//outside screen
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    if(iFrame<freezeFrames){//on start
        int state=0;
        int2 pos=to_int2_cfloat(fragCoord)-to_int2(10,10);
        if(pos.x>=0&&pos.y>=0)
        if(pos.x<seedLen.x&&pos.y<seedLen.y)state = seed[pos.y*seedLen.x+pos.x];
        fragColor = to_float4_s((float)(mod_f((float)(1-thift+state),2.0f)));
        //fragColor = texture(iChannel1,uv+iMouse.xy/iResolution);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }if(mod_f((float)(iFrame-freezeFrames),loopTime)!=0.0f){
        fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    if(mouseSize>length(swi2(iMouse,x,y)/renderScale-fragCoord)){
        if(iMouse.z>0.5f) {fragColor = to_float4(thift,0.5f,0.0f,1.0f); SetFragmentShaderComputedColor(fragColor); return;}
        if(iMouse.w>0.5f) {fragColor = to_float4(1-thift,0.5f,0.0f,1.0f); SetFragmentShaderComputedColor(fragColor); return;}
    }
    int shift1 = int(mod_f(float(isInverted+thift),2.0f));
    //int2 pos = (to_int2_cfloat(fragCoord)+shift1)/to_int2(2,2)*2-shift1;
    int2 pos = to_int2(((int)fragCoord.x+shift1)/2*2-shift1, ((int)fragCoord.y+shift1)/2*2-shift1);
    
    int2 dif = (to_int2_cfloat(fragCoord)-pos);
    int n = dif.x+2*dif.y;
    int2 c=to_int2_cfloat(fragCoord);
    int sum;
    if(true){
    sum=int(
        +1*(int)(population*_tex2DVecN(iChannel0, ((float)(pos.x)+0.5f)/iResolution.x,((float)(pos.y)+0.5f)/iResolution.y,15).x)
        +2*(int)(population*_tex2DVecN(iChannel0, ((float)(pos.x+1)+0.5f)/iResolution.x,((float)(pos.y)+0.5f)/iResolution.y,15).x)
        +4*(int)(population*_tex2DVecN(iChannel0, ((float)(pos.x)+0.5f)/iResolution.x,((float)(pos.y+1)+0.5f)/iResolution.y,15).x)
        +8*(int)(population*_tex2DVecN(iChannel0, ((float)(pos.x+1)+0.5f)/iResolution.x,((float)(pos.y+1)+0.5f)/iResolution.y,15).x)
    );
    }else{
   
      /*
        int2 pos1=pos;
        int2 worldSize1=worldSize -to_int2(10);
        int2 worldSize2=worldSize1-to_int2(0);
        sum+=1*int(population*texelFetch(iChannel0,pos+to_int2(0,0),0).x);
        
        pos1=modIto_float2(pos+worldSize1+to_int2(1,0),worldSize1);
        sum+=2*int(population*texelFetch(iChannel0,pos1,0).x);
        
        pos1=modIto_float2(pos+worldSize1+to_int2(0,1),worldSize1);
        sum+=4*int(population*texelFetch(iChannel0,pos1,0).x);
        
        pos1=modIto_float2(pos+worldSize1+to_int2(1,1),worldSize1);
        sum+=8*int(population*texelFetch(iChannel0,pos1,0).x);
    */}
    if((bool)(isInverted))  sum=(map[sum]>>n)&1;
    else                    sum=(undomap[sum]>>n)&1;
    
    fragColor = to_float4_aw(to_float3_s(sum),1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void BlockCaBFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    if(getShift(iFrame)==1)
      fragColor = _tex2DVecN(iChannel0,((float)(int)(fragCoord.x/renderScale)+0.5f)/iResolution.x,((float)(int)(fragCoord.y/renderScale)+0.5f)/iResolution.y,15);
    else
      fragColor = _tex2DVecN(iChannel1,((float)(int)(fragCoord.x)+0.5f)/iResolution.x,((float)(int)(fragCoord.y)+0.5f)/iResolution.y,15);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel1
// Connect Image 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void BlockCaBFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  fragColor = _tex2DVecN(iChannel0,((float)(int)(fragCoord.x)+0.5f)/iResolution.x,((float)(int)(fragCoord.y)+0.5f)/iResolution.y,15);

        //Debugging
        //float2 tuv = fragCoord/iResolution;
        //fragColor  = _tex2DVecN(iChannel4,tuv.x,tuv.y,15);


  SetFragmentShaderComputedColor(fragColor);
}