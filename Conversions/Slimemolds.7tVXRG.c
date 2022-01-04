
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

//__DEVICE__ float2 sin_f2(float2 i) {float2 r; r.x = _sinf(i.x); r.y = _sinf(i.y); return r;}


#define NB      10000.

#define SENSE_ANGLE    15.0f
#define SENSE_DIST      5.0f
#define SENSE_SIZE      3.0f

#define SPEED 2.5

#define SCENT_INTENSITY  0.9f
#define SCENT_DECAY      0.001f
#define SCENT_DESPERSION 0.1f

#define SCREEN_X      450.0f
#define SCREEN_Y      450.0f

#define EXPLORE 0.001

__DEVICE__ float getId(float2 resolution, float2 fragCoord) { 
    return _floor(fragCoord.x*resolution.y + fragCoord.y);
    //return _floor(fragCoord.x);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Buffer A' to iChannel0
// Connect 'Buffer B' to iChannel1


//Cell management

__DEVICE__ float2 hash(float n) { 
    return fract_f2(sin_f2(to_float2(n,n*7.0f))* 43758.5453f); 
}

__DEVICE__ float4 ParticleA(float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0) { 
    //return texelFetch(iChannel0, to_int2(fragCoord),0);
    return _tex2DVecN(iChannel0, (fragCoord.x)/iResolution.x,(fragCoord.y)/iResolution.y,15); //interessante Vaiante mit 0.5f
}

__DEVICE__ float sense(float2 pos, float size, float2 iResolution, __TEXTURE2D__ iChannel1){
    float total = 0.0f;
    for(float x = -size/2.0f; x <= size/2.0f; x++){
        for(float y = -size/2.0f; y <= size/2.0f; y++) {
            float2 sense_loc = pos + to_float2(x , y);
            
            //total += length(texelFetch(iChannel1, to_int2(sense_loc),0));
            total += length(_tex2DVecN(iChannel1, (sense_loc.x+0.0f)/iResolution.x, (sense_loc.y+0.0f)/iResolution.y,15)); //interessante Vaiante
            
            //total += length(texture(iChannel1, (to_float2(to_int2(sense_loc))+0.5f)/iResolution));
            
        }
    }

    return total;
}

__DEVICE__ float4 changeDirection(float4 fish, float2 iResolution, __TEXTURE2D__ iChannel1) {
    float sense_angle = radians(SENSE_ANGLE);
    float dist = SENSE_DIST;
    float size = SENSE_SIZE;
    
    float angle = _atan2f(fish.w, fish.z);
    float new_angle = 0.0f;
    
    float sense_angle1 = angle + sense_angle;
    float2 sense_pixel1 = swi2(fish,x,y) + to_float2(_cosf(sense_angle1)*dist,_sinf(sense_angle1)*dist);
    float sense_value1 = sense(sense_pixel1,size,iResolution,iChannel1);
    
    float sense_angle2 = angle;
    float2 sense_pixel2 = swi2(fish,x,y) + to_float2(_cosf(sense_angle2)*dist,_sinf(sense_angle2)*dist);
    float sense_value2 = sense(sense_pixel2,size,iResolution,iChannel1);
    
    float sense_angle3 = angle - sense_angle;
    float2 sense_pixel3 = swi2(fish,x,y) + to_float2(_cosf(sense_angle3)*dist,_sinf(sense_angle3)*dist);
    float sense_value3 = sense(sense_pixel3,size,iResolution,iChannel1);
    
    float explore = fract(_sinf(dot(swi2(fish,z,w), swi2(fish,x,y))) * 43758.43758f);
    if(explore < EXPLORE){
        float rand = fract(_sinf(dot(swi2(fish,z,w), swi2(fish,x,y))) * 5453.5453f);
        if(rand > 0.5f) 
            new_angle = sense_angle1;
        else
            new_angle = sense_angle3;
    } 
    else if(sense_value1 > sense_value2 && sense_value1 > sense_value3) {
        new_angle = sense_angle1;
    } 
    
    //else if(sense_value2 > sense_value1 && sense_value2 > sense_value3){
    //    new_angle = sense_angle2  ;   
    //}
    
    else if(sense_value3 > sense_value2 && sense_value3 > sense_value1) {
        new_angle = sense_angle3 ;    
    }
    
    else {
        float rand = fract(_sinf(dot(swi2(fish,z,w), swi2(fish,x,y))) * 43758.5453f);
        if(rand > 0.5f) 
            new_angle = sense_angle1;
        else
            new_angle = sense_angle3;
    }

    //swi2(fish,z,w) = to_float2(_cosf(new_angle)*SPEED, _sinf(new_angle)*SPEED);
    fish.w = _cosf(new_angle)*SPEED;
    fish.w = _sinf(new_angle)*SPEED;
    
    return fish;
    
}


__KERNEL__ void SlimemoldsFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    
    float2 screensize = iResolution;
    //vec2 screensize = to_float2(SCREEN_X,SCREEN_Y);
    
    float4 col = to_float4_s(0);
    
    if(iFrame < 10){
        float id = getId(screensize,fragCoord);
        float2 random_pos = hash(id)*screensize;
        float angle = radians(hash(id).x*hash(id).y*360.0f);
        
        float2 random_direction = to_float2(_cosf(angle)*SPEED,_sinf(angle)*SPEED);
        fragColor = to_float4(fragCoord.x,fragCoord.y,random_direction.x,random_direction.y);
    }
    else {
        //col = to_float4(uv.x,0.0f,0.0f,0.1f);
        //vec4 fish1 = Fish(0)
        float4 fish = ParticleA(fragCoord,iResolution,iChannel0);
        fish = changeDirection(fish,iResolution,iChannel1);

        
        //swi2(fish,x,y) += swi2(fish,z,w);
        fish.x += fish.z;
        fish.y += fish.w;
        
        //Edge detection
        if(fish.x < 0.0f){
            fish.x = 0.0f;
            fish.z *= -1.0f;
        }
        if(fish.y < 0.0f){
            fish.y = 0.0f;
            fish.w *= -1.0f;
        }
        if(fish.x > screensize.x){
            fish.x = screensize.x;
            fish.z *= -1.0f;
        }
        if(fish.y > screensize.y){
            fish.y = screensize.y;
            fish.w *= -1.0f;
        }
        
        fragColor = fish;
    }
    


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Buffer A' to iChannel0
// Connect 'Buffer B' to iChannel1


//Scent
__DEVICE__ float4 ParticleB(float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0) { 
    //return texelFetch(iChannel0, to_int2(fragCoord),0);
    //return texture(iChannel0, (fragCoord+0.5f)/iResolution);
    return _tex2DVecN(iChannel0, (fragCoord.x+0.5f)/iResolution.x,(fragCoord.y+0.5f)/iResolution.y,15); //interessante Vaiante mit 0.5f
}
__DEVICE__ float drawScent(float2 uv, float2 coord) {
    float d = length(to_float2(uv.x - coord.x, uv.y - coord.y));
    //return 0.03f/d;
    return 0.3f/d < 0.2f ? 0.0f : SCENT_INTENSITY;
}

__DEVICE__ float removeScent(float2 uv, float2 coord) {
    float d = length(to_float2(uv.x - coord.x, uv.y - coord.y));
    //return 0.03f/d;
    return 0.3f/d < 0.5f ? 0.0f : SCENT_INTENSITY;
}


__KERNEL__ void SlimemoldsFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1)
{

    float2 screensize = iResolution;

    float4 col = to_float4_s(0.0f);
    float2 uv = fragCoord;
    float id = getId(screensize,fragCoord);
    
    // = Initialization ===================================
    if(iFrame > 10){

        //col = _tex2DVecN(iChannel1,fragCoord.x,fragCoord.y,15);
        //col = texelFetch(iChannel1, to_int2(fragCoord),0)*0.995f;
        //col = texelFetch(iChannel1, to_int2(fragCoord),0);
        
        float blur = SCENT_DESPERSION;

        for(int i = -1;i<2;i++)
            for(int j = -1;j<2;j++)
                //col += _tex2DVecN(iChannel1,to_float2(fragCoord/iResolution)+to_float2(i,j)/iChannelResolution[1].xy*blur*1.0f,blur);
                col += _tex2DVecN(iChannel1,(fragCoord.x/iResolution.x)+(i)/iResolution.x*blur*1.0f,(fragCoord.y/iResolution.y)+(j)/iResolution.y*blur*1.0f,15);
              
        col/=9.05f;
        
        col-=SCENT_DECAY;
        
        
 
        for(float i = 0.0f;i<=400.0f;i+=1.0f){
            for(float t = 0.0f;t<=15.0f;t+=1.0f){
                float4 particle = ParticleB(to_float2(i,t),iResolution,iChannel0);
                col += drawScent(fragCoord, swi2(particle,x,y));
            }
        }
    
    }
    
    fragColor = _fminf(col,to_float4_s(1.0f));
    
    //fragColor = 0.3f/length(to_float2(fragCoord-swi2(iMouse,x,y))) > 0.01f ? to_float4_s(0.0f) : col ;  
    
    


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Buffer A' to iChannel0
// Connect 'Buffer B' to iChannel1


__DEVICE__ float4 ParticleI(float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0) { 
    //return texelFetch(iChannel0, to_int2(fragCoord),0);
    //return texture(iChannel0, (fragCoord+0.5f)/iResolution);
    return _tex2DVecN(iChannel0, (fragCoord.x)/iResolution.x,(fragCoord.y)/iResolution.y,15); //interessante Vaiante mit 0.5f
}

__DEVICE__ float drawParticle(float2 uv, float2 coord) {
    float d = length(to_float2(uv.x - coord.x, uv.y - coord.y));
    return 0.5f/d < 0.5f ? 0.0f : 1.0f;
}

__KERNEL__ void SlimemoldsFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    
    //vec2 screensize = to_float2(SCREEN_X,SCREEN_Y);
    float2 screensize = iResolution;
    float2 uv = fragCoord;
   
    float4 col = to_float4_s(0);
    
    for(float i = 0.0f;i<=400.0f;i+=1.0f){
        for(float t = 0.0f;t<=15.0f;t+=1.0f){
            float4 particle = ParticleI(to_float2(i,t),iResolution,iChannel0);
           //vec4 fish1 = Fish(fragCoord);
            col += drawParticle(uv, swi2(particle,x,y));
        }
    }
    
    //col += texelFetch(iChannel1, to_int2(uv),0);
    col += _tex2DVecN(iChannel1, (uv.x+0.5f)/iResolution.x,(uv.y+0.5f)/iResolution.y,15); //Interessante Variante ohne 0.5
    // Output to screen
    fragColor = col;


  SetFragmentShaderComputedColor(fragColor);
}