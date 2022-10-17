
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel2
// Connect Image 'Texture: Abstract 1' to iChannel0
// Connect Image 'Texture: Abstract 2' to iChannel1
// Connect Image 'Texture: Organic 4' to iChannel3


#define PI 3.141592653589f

__DEVICE__ void addLight(float t,out float3 *col, float2 mouse, float offset1, float2 uv, float3 barCol, float yOffset, float2 size){
    float3 lightCol=_mix(to_float3(1.0f,1.0f,1.0f),barCol,0.3f);
    float lightY = 0.1f+(yOffset - t*0.1f);//1.0f-mod_f(t*0.1f+yOffset, 1.2f)-yOffset+0.1f;
    //float lightY = 0.1f+(yOffset - t*0.1f);//1.0f-mod_f(t*0.1f+yOffset, 1.2f)-yOffset+0.1f;
    
    lightY = 1.0f-mod_f(1.0f-lightY,1.1f);
    
    float lightDst = distance_f2(to_float2(mouse.x-offset1,lightY),uv);
    //float lightDstX = distance_f2(to_float2(mouse.y-offset1,lightY),uv);
    
    
    float r = _fabs(_sinf(t*2.0f+yOffset*t*2.0f))*(0.03f+size.x)+(0.06f+size.y);
    //float r = _fabs(_sinf(t*2.0f+yOffset*t*2.0f))*size.x+size.y;
    float lightF=smoothstep(r,0.0f,lightDst);
    *col=_mix(*col,lightCol,lightF*1.5f);
}

//*****************************************************
__KERNEL__ void ImageHorzVertTransitionFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    CONNECT_COLOR0(barCol, 0.0f, 1.0f, 1.0f, 1.0f);
    CONNECT_SLIDER0(thk, -1.0f, 1.0f, 0.01f);
    
    CONNECT_SLIDER1(EdgeClose, -1.0f, 1.0f, 0.03f);
    CONNECT_SLIDER2(Bar, -1.0f, 10.0f, 7.0f);

    CONNECT_CHECKBOX0(AddLight, 0);
    CONNECT_CHECKBOX1(TextureFit, 0);
    CONNECT_CHECKBOX2(StrangeFit, 0);
    CONNECT_CHECKBOX3(TestFit, 0);
    
    CONNECT_SLIDER3(Mul, -1.0f, 10.0f, 1.0f);
    //CONNECT_SLIDER4(Off, -1.0f, 10.0f, 0.0f);
    CONNECT_POINT0(Off, 0.0f, 0.0f);
    
    CONNECT_POINT1(Lights, 0.0f, 0.0f);
    
    float ratio = iResolution.x/iResolution.y;
    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;

    // Time varying pixel color
    float2 mouse = swi2(iMouse,x,y) / iResolution;
    float3 col=to_float3_s(0);
    //float thk=0.01f;
    
    float3 col1=swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float3 col2=swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z);
    float3 col3=swi3(_tex2DVecN(iChannel2,uv.x,uv.y,15),x,y,z);
    float3 col4=swi3(_tex2DVecN(iChannel3,uv.x,uv.y,15),x,y,z);
    
    
    if(TextureFit||StrangeFit||TestFit)
    {  
      float2 tuv = uv*2.0f;
      
      //if(StrangeFit) tuv = uv/_fmaxf(mouse.x,mouse.y) + to_float2(mouse.x/2.0f+mouse.x, mouse.y/2.0f+mouse.y);//uv*2.0f;
      if(StrangeFit) tuv = (uv + ((_fmaxf(mouse.x,mouse.y) == mouse.x) ? to_float2(0.0f,(mouse.x-mouse.y)/2.0f) : to_float2((mouse.y-mouse.x)/2.0f,0.0f))) / _fmaxf(mouse.x,mouse.y);
      //if (TestFit) tuv = (uv+Off)/Mul;// + Off;
      col1=swi3(_tex2DVecN(iChannel0,tuv.x-0.0f,tuv.y+0.0f,15),x,y,z);
      
      //if(StrangeFit) tuv = (uv + ((_fmaxf(mouse.x,mouse.y) == mouse.x) ? to_float2(-mouse.x,(mouse.x-mouse.y)/2.0f) : to_float2(-(1.0f-mouse.x)*2.0f,(1.0f-mouse.x)/2.0f)))/_fmaxf(1.0f-mouse.x,mouse.y);
      //if(StrangeFit) tuv = (uv + ((_fmaxf(1.0f-mouse.x,mouse.y) == (1.0f-mouse.x)) ? to_float2(-mouse.x,(mouse.x-mouse.y)/2.0f + Off.y ) : to_float2(-((mouse.x-mouse.y)/2.0f+0.5f),0.0f)))/_fmaxf(1.0f-mouse.x,mouse.y);
      if(StrangeFit) tuv = (uv + ((_fmaxf(1.0f-mouse.x,mouse.y) == (1.0f-mouse.x)) ? to_float2(-mouse.x, (1.0f-mouse.x-mouse.y)/2.0f ) : to_float2(-((mouse.x-mouse.y)/2.0f+0.5f),0.0f)))/_fmaxf(1.0f-mouse.x,mouse.y);
      //if (TestFit) tuv = (uv+Off)/Mul;// + Off;
      col2=swi3(_tex2DVecN(iChannel1,tuv.x,tuv.y,15),x,y,z);
      

      if(StrangeFit) tuv = (uv + ((_fmaxf(mouse.x,1.0f-mouse.y) == mouse.x) ? to_float2(0.0f,((mouse.x-mouse.y)/2.0f-0.5f)) : to_float2((1.0f-mouse.y-mouse.x)/2.0f,-mouse.y))) / _fmaxf(mouse.x,1.0f-mouse.y);
      //if (TestFit) tuv = (uv+Off)/Mul;// + Off;
      col3=swi3(_tex2DVecN(iChannel2,tuv.x,tuv.y,15),x,y,z);
      

      if(StrangeFit) tuv = (uv + ((_fmaxf(1.0f-mouse.x,1.0f-mouse.y) == (1.0f-mouse.x)) ? to_float2(-mouse.x, -(mouse.x+mouse.y)/2.0f) : to_float2(-(mouse.x+mouse.y)/2.0f,-mouse.y)))/_fmaxf(1.0f-mouse.x,1.0f-mouse.y);
      if (TestFit) tuv = (uv+Off)/Mul;// + Off;
      col4=swi3(_tex2DVecN(iChannel3,tuv.x,tuv.y,15),x,y,z);
    
    }
    
    if (isnan(col1.x)) col1.x = 0.0f;
    if (isnan(col1.y)) col1.y = 0.0f;
    if (isnan(col1.z)) col1.z = 0.0f;
    
    if (isnan(col2.x)) col2.x = 0.0f;
    if (isnan(col2.y)) col2.y = 0.0f;
    if (isnan(col2.z)) col2.z = 0.0f;
    
    if (isnan(col3.x)) col3.x = 0.0f;
    if (isnan(col3.y)) col3.y = 0.0f;
    if (isnan(col3.z)) col3.z = 0.0f;
    
    if (isnan(col4.x)) col4.x = 0.0f;
    if (isnan(col4.y)) col4.y = 0.0f;
    if (isnan(col4.z)) col4.z = 0.0f;
    
    
    
    //vec3 col1=to_float3_s(0.0f);
    //vec3 col2=to_float3_s(1.0f);
    //float3 barCol=to_float3(0,1.0f,1.0f);
    float t = iTime;
    //t=0.0f;
    float offset1 = _sinf(uv.y*PI*4.0f+t*7.0f)*0.005f;
    offset1 += -_cosf(uv.y*PI*8.0f+t*3.0f)*0.007f;
    offset1 +=  _cosf(uv.y*PI*8.0f+t*3.0f)*0.007f * _cosf(4.0f+uv.y*PI*3.0f+t*6.0f);//
    
    
    float offset2 = _sinf(uv.x*PI*4.0f+t*7.0f)*0.005f;
    offset2 += -_cosf(uv.x*PI*8.0f+t*3.0f)*0.007f;
    offset2 += _cosf(uv.x*PI*8.0f+t*3.0f)*0.007f * _cosf(4.0f+uv.x*PI*3.0f+t*6.0f) * ratio;//
    

      //offset1+=-_sinf(PI+uv.y*PI*13.0f+t*4.0f)*0.002f;//
      //offset1+=-_cosf(7.0f+uv.y*PI*13.0f+t*14.0f)*0.003f*_sinf(3.0f+uv.y*PI*3.0f+t*4.0f);//
      //float offset2=0.0f;//_fabs(_sinf(2.0f+uv.y*PI*14.0f))*0.015f;

    
    float edgeCloseFactorX = smoothstep(mouse.x-thk-offset1-EdgeClose,mouse.x-offset1,uv.x);
    float edgeCloseFactorY = smoothstep(mouse.y-thk*ratio-offset2-EdgeClose,mouse.y-offset2,uv.y);
    
    
    float barFactorX = edgeCloseFactorX* smoothstep(mouse.x+thk   -offset1, mouse.x-offset1, uv.x);
    float barFactorY = edgeCloseFactorY* smoothstep(mouse.y+thk*Bar-offset2, mouse.y-offset2, uv.y);
       
    
    col = _mix(col1,col2,step(mouse.x-offset1, uv.x));
    float3 _col = _mix(col3,col4,step(mouse.x-offset1, uv.x));
        
    col = _mix(col,_col,step(mouse.y-offset2, uv.y)); 
        
    col=_mix(col,swi3(barCol,x,y,z),barFactorX);
    col=_mix(col,swi3(barCol,x,y,z),barFactorY);

    if(AddLight)
    {    
      addLight(t, &col, mouse, offset1, uv, swi3(barCol,x,y,z),1.0f,Lights);
      addLight(t, &col, mouse, offset1, uv, swi3(barCol,x,y,z),0.4f,Lights);
      addLight(t, &col, mouse, offset1, uv, swi3(barCol,x,y,z),0.0f,Lights);
      
      addLight(t, &col, to_float2(mouse.y,mouse.x), offset2, to_float2(uv.y,uv.x), swi3(barCol,x,y,z),1.0f,Lights);
      addLight(t, &col, to_float2(mouse.y,mouse.x), offset2, to_float2(uv.y,uv.x), swi3(barCol,x,y,z),0.4f,Lights);
      addLight(t, &col, to_float2(mouse.y,mouse.x), offset2, to_float2(uv.y,uv.x), swi3(barCol,x,y,z),0.0f,Lights);
     
    }
    
    if (uv.x==0.0f && uv.y == 0.0f) col = to_float3_s(mouse.x/mouse.y);
    
    // Output to screen
    fragColor = to_float4_aw(col,1.0f);

    

  SetFragmentShaderComputedColor(fragColor);
}