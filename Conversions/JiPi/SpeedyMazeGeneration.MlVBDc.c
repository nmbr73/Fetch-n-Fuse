
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


//https://www.shadertoy.com/view/4djSRW
//  1 out, 2 in...
__DEVICE__ float hash12(float2 p)
{
    float3 p3  = fract_f3(swi3(p,x,y,x) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}
//  1 out, 3 in...
__DEVICE__ float hash13(float3 p3)
{
    p3  = fract_f3(p3 * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float3 hsv2rgb(float3 c){
    float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
    return c.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), c.y);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

__DEVICE__ float activate(float colorSum, int range, float preColor){
    
    float result = -colorSum ;
    
    return result;
}

__DEVICE__ float3 sobel(float2 fragCoord, float2 R, __TEXTURE2D__ iChannel1, float amp){
    float3 colorSum = to_float3_s(0.0f);
    //vec3 preColor = texelFetch(iChannel1, fragCoord, 0).rgb;
    float3 preColor = swi3(texture(iChannel1, fragCoord/R),x,y,z);
    int range = 1;
    float m3[3][3] = {
      {-1.0f, -1.0f,-1.0f},
      { 0.0f,  1.0f,-1.0f},
      { 1.0f,  1.0f, 1.0f}
      };
    
    float3 c = to_float3(0.0f,1.0f,0.0f);


    for(int i = -range; i < range+1 ; i++ ){
        for(int j = -range; j < range+1 ; j++ ){
            //colorSum += texelFetch(iChannel1, fragCoord + ivec2(i,j), 0).rgb * m3[j+1][i+1];
            colorSum += swi3(texture(iChannel1, (fragCoord + make_float2(i,j)) / R),x,y,z) * amp * m3[j+1][i+1];
        }
    }
    c = to_float3_s(activate(colorSum.x, range, preColor.x)
    //activate(colorSum.g, range, preColor.g),
    //activate(colorSum.b, range, preColor.b)
    );

    return c;
}



//by copying this code into multiple buffers it is possible to generate mazes 4x as fast
#define g(x,y) texture(iChannel0,(make_float2(x,y)+U)/R)

__KERNEL__ void SpeedyMazeGenerationFuse__Buffer_A(float4 O, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, float4 iDate, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Textur, 0);
    CONNECT_CHECKBOX2(Sobel, 0);
    CONNECT_SLIDER1(TexMul, -10.0f, 20.0f, 5.0f);
    CONNECT_SLIDER2(SobelAmp, -10.0f, 20.0f, 1.0f);
    CONNECT_SLIDER3(SobelThres, -10.0f, 20.0f, 0.0f);
    
    
    U+=0.5f;
    if( iFrame == 0 || Reset)
    { 
        if(Textur)
        {
          //O = to_float4_s(texture(iChannel1,U/R).w);//*TexMul - (TexMul/2.0f);//*5.0f - 2.5f;//
          //O = texture(iChannel1,U/R).w > 0.0f ? to_float4_s(1.0f) : to_float4_s(0.0f);//*TexMul - (TexMul/2.0f);//*5.0f - 2.5f;//
          //O = to_float4_s(sobel(U,R,iChannel1, SobelAmp).x);
          O = sobel(U,R,iChannel1, SobelAmp).x > SobelThres ? to_float4_s(1.0f) : to_float4_s(0.0f);//*TexMul - (TexMul/2.0f);//*5.0f - 2.5f;//
        }
        else
          O = to_float4_s( U.x<=1.0f && U.y<=2.0f || U.x<=1.0f && U.y>=R.y-2.0f || U.x>=R.x-1.0f && U.y<=2.0f || U.x>=R.x-1.0f && U.y>=R.y-2.0f);
    }     
    else {
        //float2 R  = iResolution;
        float4 a  = g(0,0),
             b  = g(1,0),
             c  = g(0,1),
             d  = g(-1,0),
             e  = g(0,-1),
             h0 = g(-1,-1),
             h1 = g(1,-1),
             h2 = g(-1,1),
             h3 = g(1,1);
        int r = (int)( 4.0f* hash13(to_float3_aw(U,iTime+iTime)) ), f;
        float2 p;
        if (r==0) { p = swi2(b,x,y); f = (int)(h0.x+h2.x); }
        if (r==1) { p = swi2(c,x,y); f = (int)(h0.x+h1.x); }
        if (r==2) { p = swi2(d,x,y); f = (int)(h1.x+h3.x); }
        if (r==3) { p = swi2(e,x,y); f = (int)(h2.x+h3.x); }
        
        O = (2.0f*a+b+c+d+e).x==1.0f && f==0 && p.x==1.0f
            ? to_float4(1,1.0f+p.y,0,0)
            : a;

        U = abs_f2( U-swi2(iMouse,x,y) ); 
        U = _fminf( U, abs_f2(U-R) );
       if( length(U)<50.0f && iMouse.z>0.0f) O = to_float4_s(0);
    }


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


//Click and drag to damage the maze and watch it heal

__KERNEL__ void SpeedyMazeGenerationFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 1.0f, 1.0f, 1.0f, 1.0f); 
    CONNECT_SLIDER0(ColHSV,  -2000.0f, 3000.0f, 1000.0f);
  
    fragCoord+=0.5f; 

    float2 a=swi2(texture(iChannel0,fragCoord/iResolution),x,y);
    //fragColor=a.x*to_float4_aw(hsv2rgb(to_float3(a.y/1000.0f,1.0f,1.0f)),1.0f);
    fragColor=a.x*to_float4_aw(hsv2rgb(to_float3(a.y/ColHSV*Color.x,Color.y,Color.z)),Color.w);


  SetFragmentShaderComputedColor(fragColor);
}