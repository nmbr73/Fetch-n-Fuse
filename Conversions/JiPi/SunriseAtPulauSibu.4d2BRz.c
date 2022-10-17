
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel0
// Connect Image 'Texture: Pebbles' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Man has demonstrated that he is master of everything except his own nature
//---------------------------------------------------------------------------

// One morning we woke up when the sky was still dark.
// We walked half an hour through the forest,
// to reach the other side of the island,
// where the beach is facing the rising sun.
// The sun was already there, one half over the horizon.
// The sky was on fire.
// We swum in the sea, staring at the rising sun.



__DEVICE__ float3 gamma( float3 col, float g){
    return pow_f3(col,to_float3_s(g));
}
    
    
// clouds layered noise
__DEVICE__ float noiseLayer(float2 uv, float iTime, float4 iMouse, __TEXTURE2D__ iChannel0){    
    float t = (iTime+iMouse.x)/5.0f;
    uv.y -= t/60.0f; // clouds pass by
    float e = 0.0f;
    for(float j=1.0f; j<9.0f; j++){
        // shift each layer in different directions
        float timeOffset = t*mod_f(j,2.989f)*0.02f - t*0.015f;
        e += 1.0f-texture(iChannel0, uv * (j*1.789f) + j*159.45f + timeOffset).x / j ;
    }
    e /= 3.5f;
    return e;
}

// waves layered noise
__DEVICE__ float waterHeight(float2 uv, float iTime, float4 iMouse, __TEXTURE2D__ iChannel1){
    float t = (iTime+iMouse.x);
    float e = 0.0f;
    for(float j=1.0f; j<6.0f; j++){
        // shift each layer in different directions
        float timeOffset = t*mod_f(j,0.789f)*0.1f - t*0.05f;
        e += texture(iChannel1, uv * (j*1.789f) + j*159.45f + timeOffset).x / j ;
    }
    e /= 6.0f;
    return e;
}

__DEVICE__ float3 waterNormals(float2 uv, float iTime, float4 iMouse, __TEXTURE2D__ iChannel1){
    uv.x *= 0.25f;
    float eps = 0.008f;    
    float3 n=to_float3( waterHeight(uv, iTime, iMouse, iChannel1) - waterHeight(uv+to_float2(eps,0.0f), iTime, iMouse, iChannel1),
                        1.0f,
                        waterHeight(uv, iTime, iMouse, iChannel1) - waterHeight(uv+to_float2(0.0f,eps), iTime, iMouse, iChannel1));
   return normalize(n);
}  


__DEVICE__ float3 drawSky( float2 uv, float2 uvInit, float iTime, float4 iMouse, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, float3 ColArray[6]){ 
        
  float clouds = noiseLayer(uv, iTime, iMouse, iChannel0);
    
    // clouds normals
    float eps = 0.1f;
    float3 n = to_float3(  clouds - noiseLayer(uv+to_float2(eps,0.0f), iTime, iMouse, iChannel0),
                -0.3f,
                 clouds - noiseLayer(uv+to_float2(0.0f,eps), iTime, iMouse, iChannel0));
    n = normalize(n);
    
    // fake lighting
    float l = dot(n, normalize(to_float3(uv.x,-0.2f,uv.y+0.5f)));
    l = clamp(l,0.0f,1.0f);
    
    // clouds color  (color gradient from light)
    //float3 cloudColor = _mix(baseSkyColor, darkColor, length(uvInit)*1.7f);
    float3 cloudColor = _mix(ColArray[3], ColArray[2], length(uvInit)*1.7f);
    //cloudColor = _mix( cloudColor,sunColor, l );
    cloudColor = _mix( cloudColor,ColArray[0], l );
    
    // sky color (color gradient on Y)
    //float3 skyColor = _mix(lightColor , baseSkyColor, clamp(uvInit.y*2.0f,0.0f,1.0f) );
    float3 skyColor = _mix(ColArray[1] , ColArray[3], clamp(uvInit.y*2.0f,0.0f,1.0f) );
    //skyColor = _mix ( skyColor, darkColor, clamp(uvInit.y*3.0f-0.8f,0.0f,1.0f) );
    skyColor = _mix ( skyColor, ColArray[2], clamp(uvInit.y*3.0f-0.8f,0.0f,1.0f) );
    //skyColor = _mix ( skyColor, sunColor, clamp(-uvInit.y*10.0f+1.1f,0.0f,1.0f) );
    skyColor = _mix ( skyColor, ColArray[0], clamp(-uvInit.y*10.0f+1.1f,0.0f,1.0f) );
  // draw sun
    if(length(uvInit-to_float2(0.0f,0.04f) )<0.03f){
       skyColor += to_float3(2.0f,1.0f,0.8f);
    }
float zzzzzzzzzzzzzzzzzzz;       
     // mix clouds and sky
    float cloudMix = clamp(0.0f,1.0f,clouds*4.0f-8.0f);
    float3 color = _mix( cloudColor, skyColor, clamp(cloudMix,0.0f,1.0f) );
    
    // draw islands on horizon
    uvInit.y = _fabs(uvInit.y);
    float islandHeight = texture(iChannel1, swi2(uvInit,x,x)/2.0f+0.67f).x/15.0f - uvInit.y + 0.978f;
    islandHeight += texture(iChannel1, swi2(uvInit,x,x)*2.0f).x/60.0f;
    islandHeight = clamp(_floor(islandHeight),0.0f,1.0f);    
    //float3 landColor = _mix(baseSkyColor, darkColor, length(uvInit)*1.5f);
    float3 landColor = _mix(ColArray[3], ColArray[2], length(uvInit)*1.5f);
    color = _mix(color, landColor, islandHeight);

    return color;
}

__KERNEL__ void SunriseAtPulauSibuFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    // visual parameters -------------------
    const float3 sunColor = to_float3(1.5f,0.9f,0.7f);      // 0
    const float3 lightColor = to_float3(1.0f,0.8f,0.7f);    // 1
    const float3 darkColor = to_float3(0.2f,0.2f,0.3f);     // 2
    const float3 baseSkyColor = to_float3(0.6f,0.7f,0.8f);  // 3
    const float3 seaColor = to_float3(0.1f,0.3f,0.5f);      // 4
    const float3 seaLight = to_float3(0.1f,0.45f,0.55f);    // 5
    //---------------------------------------

    float3 ColArray[6] = { sunColor, lightColor, darkColor, baseSkyColor, seaColor, seaLight};
 

    // center uv around horizon and manage ratio
    float2 uvInit = fragCoord / iResolution;
    uvInit.x -= 0.5f;
    uvInit.x *= iResolution.x/iResolution.y;  
    uvInit.y -= 0.35f;
    
    // perspective deform 
    float2 uv = uvInit;
    uv.y -=0.01f;
    uv.y = _fabs(uv.y);
    uv.y = _logf(uv.y)/2.0f;
    uv.x *= 1.0f-uv.y;
    uv *= 0.2f;
    
    float3 col = to_float3(1.0f,1.0f,1.0f);
float IIIIIIIIIIIIIIII;    
    // draw water
    if(uvInit.y < 0.0f){       
       
        float3 n = waterNormals(uv, iTime, iMouse, iChannel1);
        
        // draw reflection of sky into water
        float3 waterReflections = drawSky(uv+swi2(n,x,z), uvInit+swi2(n,x,z), iTime, iMouse, iChannel0, iChannel1, ColArray);

        // mask for fore-ground green light effect in water
        float transparency = dot(n, to_float3(0.0f,0.2f,1.5f));        
        transparency -= length ( (uvInit - to_float2(0.0f,-0.35f)) * to_float2(0.2f,1.0f) );
        transparency = (transparency*12.0f+1.5f);
        
        // add foreground water effect
        waterReflections = _mix( waterReflections, seaColor, clamp(transparency,0.0f,1.0f) );
        waterReflections = _mix( waterReflections, seaLight, _fmaxf(0.0f,transparency-1.5f) );

         col = waterReflections;
        
        // darken sea near horizon
         col = _mix(col, col*to_float3(0.6f,0.8f,1.0f), -uv.y);
        
        //sun specular
        col += _fmaxf(0.0f,0.02f-_fabs(uv.x+n.x))* 8000.0f * to_float3(1.0f,0.7f,0.3f) * -uv.y * _fmaxf(0.0f,-n.z);
        
    }else{      
        
        // sky
        col = drawSky(uv, uvInit, iTime, iMouse, iChannel0, iChannel1, ColArray);
    }
    
    // sun flare & vignette
    col += to_float3(1.0f,0.8f,0.6f) * (0.55f-length(uvInit)) ;
    
    // "exposure" adjust
    col *= 0.75f;
    col = gamma(col,1.3f);
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
