
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



#define Pi _acosf(-1.0f) 

__DEVICE__ float orb(float2 center, float brightness, float2 uv){
    float dist = length(uv-center);
    return _fminf(1.0f,brightness/(dist*dist)); 
}

__DEVICE__ float orb_path(float2 ixy,float brightness,float2 v,float time,float2 uv, float g){
    float2 loc = ixy + time*v + time*time*to_float2(0.0f,g);
    return orb(loc,brightness,uv); 
    
}

//From hash1 in https://www.shadertoy.com/view/4ttSWf
__DEVICE__ float rand(float n){
    return fract( n*17.0f*fract( n*0.3183099f ) );

}
__DEVICE__ float rand2d( float2 p )
{
    p  = 50.0f*fract_f2( p*0.3183099f );
    return fract( p.x*p.y*(p.x+p.y) );
}
__DEVICE__ float2 randto_float2( float seed){
    const float2 k =  to_float2( 0.3183099f, 0.3678794f);
    return fract_f2(seed*fract_f2(k*seed)); 
}

__DEVICE__ float3 randto_float3( float seed){
    const float3 k = to_float3( 0.3183099f, 0.3678794f, 0.446234f );
    return fract_f3(seed*fract_f3(k*seed)); 
}

__DEVICE__ float3 scaleTo1(float3 v){
    float ma = _fmaxf(max(v.x,v.y),v.z);
    return v/ma;
}

__DEVICE__ float rand_range(float l, float r, float seed){
    return l+rand(seed)*(r-l); 
}

__DEVICE__ float flicker(float time,float delay){
    float time_after_delay = _fmaxf(0.0f,time-delay);
    return (_cosf(time*time_after_delay)+1.0f)/2.0f;
}

__DEVICE__ float2 find_velocity(float2 start,float2 end,float time, float g){
    end-=start;
    return to_float2(end.x/time,(end.y-g*time*time)/time); 
}

__DEVICE__ float get_orb(float2 start,float2 end,float time_to_take,float time,float2 uv, float g){
    float2 v = find_velocity(start,end,time_to_take,g);
    return orb_path(start, 0.0001f, v, time,uv,g);  
}
__DEVICE__ float firework_flicker(float2 center, float time, float seed, float2 uv, float g){
    uv-=center;
    uv.x = _fabs(uv.x); 
    float brightness = (1.0f - smoothstep(0.0f,5.0f,time))*0.0002f;
    float sdf = 0.0f;
    
    for(int i=1;i<=54;i++){
        float fi = (float)(i); 
        float flickeri = flicker(time,3.0f*rand(fi*seed))*brightness;
        float theta = rand_range(0.0f,2.0f*Pi,fi*16.1f*seed);
        float r = 0.05f+0.03f*(fi/9.0f);
        float2 v = r*to_float2(_cosf(theta),_sinf(theta));  
        sdf += orb_path(to_float2_s(0.0f),flickeri,v,time,uv,g);
        
    }
    
    return sdf; 
}

__DEVICE__ float3 firework_flicker_total(float delay,float time_to_start, float time, float seed,float2 uv, float g){
    float3 col = scaleTo1(randto_float3(seed));
    float2 center = randto_float2(seed);
    center.x = 2.0f*center.x-1.0f;
    if(time<delay){
        return to_float3_s(0.0f); 
    }
    time-=delay;
    if(time<time_to_start){
        return col*get_orb(to_float2(0.0f,-1.0f),center,time_to_start,time,uv,g);
    }
    return col*firework_flicker(center, time-time_to_start,seed,uv,g); 
}



__DEVICE__ float firework_standard(float2 center, float time, float seed, float2 uv, float g){
    uv-=center;
    float brightness = (1.0f - smoothstep(0.0f,5.0f,time))*0.0002f;
    float sdf = 0.0f;
     for(int i=1;i<=54;i++){
        float fi = (float)(i)/54.0f; 
        float theta = rand_range(0.0f,2.0f*Pi,rand(seed*20.0f*fi));
        float r = 0.05f+0.03f*(2.0f*fi);
        float2 v = r*to_float2(_cosf(theta),_sinf(theta));  
        sdf += orb_path(to_float2_s(0.0f),brightness,v,time,uv,g);
        
    }
    return sdf;
}

__DEVICE__ float3 firework_standard_total(float delay,float time_to_start, float time, float seed,float2 uv, float g){
    float3 col = randto_float3(seed);
    float2 center = randto_float2(seed); 
    center.x=2.0f*center.x-1.0f;
    if(time<delay){
        return to_float3_s(0.0f); 
    }
    time-=delay;
    if(time<time_to_start){
        return col*get_orb(to_float2(0.0f,-1.0f),center,time_to_start,time,uv,g);
    }
    return col*firework_standard(center, time-time_to_start,seed,uv,g); 
}


__DEVICE__ float3 background(float2 uv, float3 White, float3 SkyBlue, float3 Black){
    float2 star_loc = _floor(100.0f*uv);
    float star; 
    if(uv.y > -0.3f && rand2d(star_loc)>0.99f){
        star = orb(star_loc,0.5f,100.0f*uv);
    }else{
        star = 0.0f;
    }
    float3 star3 = star*White; 
    return _mix(SkyBlue,Black,(uv.y+1.0f))/2.0f+star3;
}

__DEVICE__ float3 scene(float2 uv, float3 White, float3 SkyBlue, float3 Black, float g, float iTime){
    float3 col;
    float time = iTime*2.5f; 
    time = mod_f(time,110.0f); 
    col = background(uv, White, SkyBlue,Black);
    for(int i=0;i<=3;i++){
        float fi = (float)(i);
        float delay = fi;
        col += firework_standard_total(delay,3.0f, mod_f(time,11.0f), 1.0f+(float)((int)((time+11.0f)/11.0f)*(7+i)), uv, g );
    }
    for(int i=0;i<=3;i++){
      float fi = (float)(i);
      float delay = fi;
      col += firework_flicker_total(delay,3.0f,mod_f(time,11.0f),2.0f+(float)((int)((time+11.0f)/11.0f)*(11+2*i)),uv,g);
    }
    //col = get_orb(to_float2_s(0.0f),to_float2(-1.0f,0.0f),5.0f,iTime,uv,g)*White; 
    return col;
}

__KERNEL__ void HappyJulyJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
	CONNECT_COLOR0(White, 1.0f, 1.0f, 1.0f, 1.0f);
	CONNECT_COLOR1(SkyBlue, 0.5294f, 0.8078f, 0.9216f, 1.0f);
	CONNECT_COLOR2(Black, 0.0f, 0.0f, 0.0f, 1.0f);
	CONNECT_SLIDER0(g, -1.0f, 1.0f, -0.01f);

#ifdef ORG
	const float3 White = to_float3_s(1.0f); 
	const float3 Orange = to_float3(1.0f,1.0f,0.0f); 
	const float3 SkyBlue = to_float3(0.5294f,0.8078f,0.9216f);
	const float3 Black = to_float3_s(0.0f); 
	const float g = -0.01f;
	
#endif
	
    // Normalized pixel coordinates (from -1 to 1)
    float2 uv = fragCoord/iResolution.y;
    uv-=to_float2(0.5f*iResolution.x/iResolution.y,0.5f);
    uv*=2.0f;
    
    float3 col = scene(uv,swi3(White,x,y,z),swi3(SkyBlue,x,y,z),swi3(Black,x,y,z),g, iTime);

    // Output to screen
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}