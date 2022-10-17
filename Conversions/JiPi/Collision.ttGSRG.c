
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define SIZE 122
//#define FLASH_POWER 0.38f
//#define RADIUS 0.01f
//#define SPEED  0.0018f

#define R iResolution

//#define FD(x,y) texelFetch(iChannel0, to_int2(x, y), 0)
#define FD(x,y) texture(iChannel0, (make_float2(to_int2(x, y))+0.5f)/R)

__DEVICE__ float hash12(float2 p)
{
    float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: StartTexture' to iChannel1

#define _reflect(I,N) (I-2.0f*dot(N,I)*N)

__KERNEL__ void CollisionFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(StartTex, 0);
    CONNECT_INTSLIDER0(SIZE, 1, 300, 122);
    
    CONNECT_SLIDER0(FLASH_POWER, -1.0f, 1.0f, 0.38f);
    CONNECT_SLIDER1(RADIUS, -1.0f, 1.0f, 0.01f);
    CONNECT_SLIDER2(SPEED, -1.0f, 1.0f, 0.0018f);
    
    fragCoord+=0.5f;
 
    int x = int(fragCoord.x);
    int y = int(fragCoord.y);
    
    if(y > 2 || x > SIZE){
       SetFragmentShaderComputedColor(fragColor);
       return;
      //discard;
    }     
    
    float2 rt = to_float2(iResolution.x / iResolution.y, 1.0f);
    
    if(iFrame == 1 || Reset){
        float ms = _sqrtf((float)(SIZE));
        float yp = _floor((float)(x) / ms) - ms*0.45f;
        float xp = mod_f((float)(x), ms) - ms*0.45f;
        float2 pos = (to_float2(xp,yp)) * (1.0f/ms)*0.9f;
                
        pos *= rt;                       

        float2 dir = normalize(to_float2(hash12(fragCoord*200.0f)*2.0f-1.0f, hash12(fragCoord * 100.0f)*2.0f-1.0f));   
        
        if(StartTex)
        {
        
          float tex = texture(iChannel1,fragCoord/R).w;
          if (tex>0.0f)
          {
            ms = _sqrtf((float)(SIZE));
            yp = _floor((float)(x) / ms) - ms*0.45f;
            xp = mod_f((float)(x), ms) - ms*0.45f;
            pos = (to_float2(xp,yp)) * (1.0f/ms)*0.9f;
                    
            pos *= rt;                       

            dir = normalize(to_float2(hash12(fragCoord*200.0f)*2.0f-1.0f, hash12(fragCoord * 100.0f)*2.0f-1.0f));               
            
            if(y==0){
                fragColor = to_float4_f2f2(pos, dir);                   
            }
            if(y==1){
                fragColor = to_float4(-100.0f, 0,0,0);                   
            }            
          }
        }
        else
        {
          if(y==0){
              fragColor = to_float4_f2f2(pos, dir);                   
          }
          if(y==1){
              fragColor = to_float4(-100.0f, 0,0,0);                   
          }
        }
        
    } else {
        float4 iPoint = FD(x,0);
        float2 pos = swi2(iPoint,x,y);        
        float2 dir = swi2(iPoint,z,w);
        
        bool col = false;
        
        if(iPoint.x <= (-0.5f*rt.x + RADIUS)){
            dir.x *= -1.0f;      
            col = true;
        }
        if(iPoint.x >= (0.5f*rt.x - RADIUS)){
            dir.x *= -1.0f; 
            col = true;
        }
        if(iPoint.y <= (-0.5f*rt.y + RADIUS)){
            dir.y *= -1.0f;            
            col = true;
        }
        if(iPoint.y >= (0.5f*rt.y - RADIUS)){
            dir.y *= -1.0f;            
            col = true;
        }
        
        for(int i=0; i<SIZE; i+=1){
            if(i!=x){
                float4 nPoint = FD(i,0);
                float2 nPointDir = swi2(nPoint,z,w);
                if(distance_f2(swi2(nPoint,x,y), pos) < (RADIUS*2.0f)){
                    float2 inV = normalize(swi2(nPoint,x,y) - pos);
                    if(dot(dir, inV) > 0.0f){                       
                        dir = _reflect(dir, inV);                        
                    }
                    col = true;                    
                }
            }          
        }
        
        dir = normalize(dir);
        
        pos += dir * SPEED;
        
        pos.x = _fminf(max(pos.x, -0.5f*rt.x + RADIUS), 0.5f*rt.x - RADIUS);
        pos.y = _fminf(max(pos.y, -0.5f*rt.y + RADIUS), 0.5f*rt.y - RADIUS);
        
    
        if(y==0){
            fragColor = to_float4_f2f2(pos, dir);
        }
        if(y==1){
            if(col){
                fragColor = to_float4(iTime, 0, 0, 0);
            } else {
                fragColor = FD(x,1);
            }
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


//#define COL1 to_float3(24, 32, 38) / 255.0f
//#define COL2 to_float3(235, 241, 245) / 255.0f

#define SF 2.0f/_fminf(iResolution.x, iResolution.y)
#define SS(l, s) smoothstep(SF, -SF, l - s)

__KERNEL__ void CollisionFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
    CONNECT_INTSLIDER0(SIZE, 1, 300, 122);
    CONNECT_SLIDER0(FLASH_POWER, -1.0f, 1.0f, 0.38f);
    
    CONNECT_COLOR0(COL1, 0.095f, 0.125f, 0.15f, 1.0f); //Background
    CONNECT_COLOR1(COL2, 0.92f, 0.95f, 0.96f, 1.0f);   //Pixel
    
    fragCoord+=0.5f;
    float2 uv = (fragCoord - iResolution * 0.5f) / iResolution.y;
    
    float m = 0.0f;    
    for (float i = 0.0f; i < (float)(SIZE); i += 1.0f) {
        float4 point = FD(i, 0);
        float4 colData = FD(i, 1);
        
        float colTimeDiff = clamp(iTime - colData.x, 0.0f, 0.5f)*5.0f;        
        
        float2 pos = swi2(point,x,y);        
        
        float d = length(uv - pos)*0.5f;        
        float g = 0.1f/(d*(10.0f + 20.0f*colTimeDiff))*FLASH_POWER;        

        m += g;        
    }
    
    //float3 col = _mix(swi3(COL1,x,y,z), swi3(COL2,x,y,z), m);
    fragColor = _mix(COL1, COL2, m);
    
    //fragColor = to_float4_aw(col, 1.0f);
    
    //fragColor.w = _mix(COL1.w,1.0f,m);

  SetFragmentShaderComputedColor(fragColor);
}