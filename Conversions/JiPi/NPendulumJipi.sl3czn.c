
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ int AMT = 8; // Number of pendulums. Each one being longer than the previous. Currently uses x of a buffer to store each item. 
__DEVICE__ int show = 0; // if it is 1, then only show the pendulum with the most links
__DEVICE__ float gravity = -0.02f;
__DEVICE__ float trails = 1.5f;
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


// Thanks iq for line segment sdf.
__DEVICE__ float line_segment(in float2 p, in float2 a, in float2 b) {
  float2 ba = b - a;
  float2 pa = p - a;
  float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0f, 1.0f);
  return length(pa - h * ba);
}


__DEVICE__ float4 tf(int2 p, int i, int j, float2 iResolution, __TEXTURE2D__ iChannel0){
    //return texelFetch(iChannel0,p+to_int2(i,j),0);
    return texture(iChannel0,(make_float2(p+to_int2(i,j))+0.5f)/iResolution);
}
__DEVICE__ float4 get(int i, float2 iResolution, __TEXTURE2D__ Channel){
    //return texelFetch(iChannel1,to_int2(i,0),0);
    return texture(Channel,(make_float2(to_int2(i,0))+0.5f)/iResolution);
}


__DEVICE__ float4 state(int2 p, float2 iResolution, __TEXTURE2D__ iChannel0){
    float zzzzzzzzzzzzzzz;
    float4 colNow = to_float4(0,0,0,0);
    float4 r = tf(p,0,0, iResolution, iChannel0) * 0.7f;
    for(int i = -1; i < 2; i++){
        for(int j = -1; j < 2; j++){
            float4 u = tf(p,i,j, iResolution, iChannel0);
            r+= u*0.03f;
        }
    }
    return r;
}

__KERNEL__ void NPendulumJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;
float AAAAAAAAAAAAAAAAA;
    fragColor = to_float4(0.0f,0.0f,1.0f,1.0f);
    float4 col = state(to_int2_cfloat(fragCoord), iResolution, iChannel0);

    if(iFrame < 1 || Reset){
        float2 uv = fragCoord/iResolution;
        col = texture(iChannel1, uv*0.5f)*0.5f-0.25f;
    }

    for(int i = 0; i < AMT * AMT; i ++){
        if(i%AMT <= i/AMT){
            float4 col2 = get(i, iResolution, iChannel1);
            if(show == 0 || i >= AMT * (AMT-1)){
                if(length(to_float2(col2.x,col2.y + iResolution.y*0.4f) - fragCoord) < 2.0f){
                    col += 0.1f*to_float4(1.0f*_sinf(iTime*5.0f + (float)(i))+1.0f, 1.0f*_sinf(iTime*5.0f + 2.1f + (float)(i))+1.0f, 1.0f*_sinf(iTime*5.0f + 4.2f + (float)(i))+1.0f, 0);
                    if(i%AMT == i/AMT){
                        col += trails*to_float4(1.0f*_sinf(iTime*5.0f + (float)(i))+1.0f, 1.0f*_sinf(iTime*5.0f + 2.1f + (float)(i))+1.0f, 1.0f*_sinf(iTime*5.0f + 4.2f + (float)(i))+1.0f, 0);
                    }
                }
                
                float4 col3 = get(i-1, iResolution, iChannel1);
                if(i%AMT == 0){
                    col3.x = iResolution.x*0.5f;
                    col3.y = iResolution.y*0.5f;
                }
                if(line_segment(fragCoord-to_float2(0,iResolution.y*0.4f),swi2(col2,x,y),swi2(col3,x,y)) <0.5f){
                    col += 0.04f*to_float4(1.0f*_sinf(iTime*5.0f + (float)(i))+1.0f, 1.0f*_sinf(iTime*5.0f + 2.1f + (float)(i))+1.0f, 1.0f*_sinf(iTime*5.0f + 4.2f + (float)(i))+1.0f, 0);
                }
            }
        }
    }
    
    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


//vec4 get(int i){
//    return texelFetch(iChannel0,to_int2(i,0),0);
//}

__KERNEL__ void NPendulumJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord+=0.5f;
float BBBBBBBBBBBBBB;
    int t = (int)(fragCoord.x);
    fragColor = to_float4(0.0f,0.0f,1.0f,1.0f);
    if(t < AMT * AMT && t%AMT <= t/AMT){
        
        // Physics
        float4 r = get((int)(fragCoord.x), iResolution, iChannel0);

        float wlen = iResolution.y*0.8f/(1.0f + (float)(t/AMT));
        // Center Length
        float2 center = iResolution*0.5f;
        if(t%AMT >=1){
            center = swi2(get(t-1, iResolution, iChannel0),x,y);
            wlen += 0.01f * (float)(t%AMT);
        }
        float2 delt = swi2(r,x,y) - center;
        float len = wlen - length(delt);
        
        r.z += len * normalize(delt).x * 0.9f;
        r.w += len * normalize(delt).y * 0.9f; 
        
        r.x = (center.x + wlen * normalize(delt).x) * 0.1f + r.x * 0.9f; // force position towards 
        r.y = (center.y + wlen * normalize(delt).y) * 0.1f + r.y * 0.9f;
        
        if(t%AMT <= t/AMT - 1){
            center = swi2(get(t+1, iResolution, iChannel0),x,y);
        
            delt = swi2(r,x,y) - center;
            len = wlen - length(delt);
        
            r.z += len * normalize(delt).x * 0.9f;
            r.w += len * normalize(delt).y * 0.9f; 
        
        }
        r.w+= gravity;
        
        if(iMouse.z > 0.0f && t%AMT == t/AMT){// mouse drag
            center = swi2(iMouse,x,y) - to_float2(0,iResolution.y*0.4f);
            r.x = (center.x ) * 0.03f + r.x * 0.97f;  
            r.y = (center.y ) * 0.03f + r.y * 0.97f;
            r.z = 0.0f;
            r.w = 0.0f; 
        }
        
        r.x += r.z;
        r.y += r.w;
        
        //r.z *= 0.9999f;
        //r.w *= 0.9999f;
        
        fragColor = r;
        
        if(iFrame < 1 || Reset){            
            fragColor = to_float4(iResolution.x * 0.5f + wlen*(float)(t%AMT+1),iResolution.y*0.5f,0.0f,0.0f);
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void NPendulumJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    float4 raw = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    // Time varying pixel color
    float4 col = raw*1.0f+ 0.0f;
   
    fragColor = to_float4_aw(swi3(col,x,y,z), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}