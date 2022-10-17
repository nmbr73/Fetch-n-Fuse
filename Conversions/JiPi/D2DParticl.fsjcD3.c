
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float2 coordToWorld(float2 fragCoord, float2 iResolution){
    float2 uv = fragCoord/iResolution;
    uv.y /= (iResolution.x/iResolution.y);
    uv.y = 1.0f-uv.y;
    return uv;
}

__KERNEL__ void D2DParticlFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    fragCoord += 0.5f;
  
    //PHYSICS
    const float ballRad = 0.01f;
    const float ballCount = 200.0f;
    const float gravity = 0.001f;

    
    if (fragCoord.y != 0.5f || fragCoord.x >= ballCount+0.5f) { SetFragmentShaderComputedColor(fragColor); return; }
    
    
    if (iFrame == 0) {
      
        fragColor = to_float4(0.25f+fragCoord.x*0.003f,0.5f+fragCoord.x*0.00005f,0.0f,0.0f);
        
    } else {
        
        float4 myData = texture(iChannel0,fragCoord/iResolution);
        myData.w += gravity;
        
        swi2S(myData,x,y, swi2(myData,x,y) + swi2(myData,z,w));
        

        float4 myDataOff = to_float4_s(0.0f);
        float intersections = 0.0f;
        
        for(float i=0.0f; i<ballCount; i+=1.0f){
            
            if (intersections < 4.0f) {//arbitrary

                float2 otherCoord = to_float2(i,0) + to_float2_s(0.5f);
                if (length(otherCoord-fragCoord) == 0.0f) continue;//skip test against self

                float4 otherData = texture(iChannel0,otherCoord/iResolution);
                float2 gap = swi2(myData,x,y)-swi2(otherData,x,y);
                float gapdist = length(gap);
                
                if (gapdist < ballRad*0.5f) {//fix interlock
                    // this DOESN'T move the other particle -- it just lies about my sense of where it is!
                    float ejectAng = otherCoord.x+otherCoord.y;
                    swi2S(otherData,x,y, swi2(otherData,x,y) + to_float2(_cosf(ejectAng),_sinf(ejectAng))*ballRad*0.01f);

                    gap = swi2(myData,x,y)-swi2(otherData,x,y);
                    gapdist = length(gap);
                }
                if (gapdist < ballRad*2.0f) {
                    float2 gapnorm = normalize(gap);
                    float myVelAlongCollisionAxis = myData.z*gapnorm.x + myData.w*gapnorm.y;
                    float otherVelAlongCollisionAxis = otherData.z*gapnorm.x + otherData.w*gapnorm.y;

                    swi2S(myDataOff,z,w, swi2(myDataOff,z,w) + (otherVelAlongCollisionAxis-myVelAlongCollisionAxis)*gapnorm*0.5f);
                    swi2S(myDataOff,x,y, swi2(myDataOff,x,y) + ((ballRad*2.0f-gapdist)*0.5f)*gapnorm);
                    intersections++;
                }

            }
            
        }
        
        swi2S(myData,x,y, swi2(myData,x,y) + swi2(myDataOff,x,y));
        swi2S(myData,z,w, swi2(myData,z,w) + swi2(myDataOff,z,w));

              
        
        if (iMouse.z > 0.0f) {
          float2 worldMouse = coordToWorld(swi2(iMouse,x,y), iResolution);
            float2 mouseGap = worldMouse-swi2(myData,x,y);
            float mouseGapDist = length(mouseGap);
            float2 mouseGapNorm = normalize(mouseGap);
            
            float pullStrength = mouseGapDist/0.9f;
            pullStrength = _fminf(1.0f,_fmaxf(pullStrength,0.0f));
            pullStrength = _powf(pullStrength,0.75f);
            pullStrength = _sinf((pullStrength-0.25f)*6.28f)*0.5f+0.5f;
            pullStrength *= 0.0013f;
            
            swi2S(myData,z,w, swi2(myData,z,w) + mouseGapNorm*pullStrength);
        }
        
        if (myData.y > 1.0f-ballRad) {
            myData.w += -_fabs(myData.w)*1.5f;
            myData.y = 1.0f-ballRad;
        }
        if (myData.y < ballRad) {
            myData.w += _fabs(myData.w)*1.5f;
            myData.y = ballRad;
        }
        if (myData.x > 1.0f-ballRad) {
            myData.z += -_fabs(myData.z)*1.5f;
            myData.x = 1.0f-ballRad;
        }
        if (myData.x < ballRad) {
            myData.z += _fabs(myData.z)*1.5f;
            myData.x = ballRad;
        }
        
        swi2S(myData,z,w, swi2(myData,z,w)*0.95f);
        
        fragColor = myData;
    }
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0

/*
__DEVICE__ float2 coordToWorld(float2 fragCoord){
    float2 uv = fragCoord/iResolution;
    uv.y /= (iResolution.x/iResolution.y);
    uv.y = 1.0f-uv.y;
    return uv;
}
*/

//#define DEBUGVIEW

__DEVICE__ float mySmooth( float a, float b, float k ){
    float h = clamp(0.5f+0.5f*(b-a)/k,0.0f,1.0f);
    return _mix(b,a,h) - k*h*(1.0f-h);
}

__DEVICE__ float blobDist(float2 uv, float ballCount, float2 iResolution, __TEXTURE2D__ iChannel0){
    
    float dist = 9999.9f;
    
    for(float i=0.0f; i<ballCount; i++){
        float4 curData = texture(iChannel0,to_float2(i+0.5f,0.5f)/iResolution);
        float curDist = length(swi2(curData,x,y)-uv);
      #ifdef DEBUGVIEW
        dist = _fminf(dist,curDist);
      #else
        dist = mySmooth(dist,curDist,0.07f);
      #endif
    }
    
    return dist;
    
}

__KERNEL__ void D2DParticlFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord += 0.5f;

    //CURRENT FRAME'S BLOB
    const float ballRad = 0.01f;
    const float ballCount = 200.0f;
        
    float2 uv = coordToWorld(fragCoord, iResolution);
    float dist = blobDist(uv, ballCount, iResolution, iChannel0);
    

  #ifdef DEBUGVIEW
    fragColor = to_float4( (dist < ballRad) ? 1.0f : 0.0f );
  #else
     
    float height = 1.0f-dist/ballRad*0.3f;
    if (height >= 0.0f) {
        height = _fminf(1.0f,_fmaxf(0.0f,height));
        height = height*1.57f;
        
        const float2 e = to_float2(0.00001f,0.0f);
        float2 normal2d = normalize(to_float2(dist-blobDist(uv-swi2(e,x,y),ballCount, iResolution, iChannel0),
                                              dist-blobDist(uv-swi2(e,y,x),ballCount, iResolution, iChannel0)));
        
        float3 normal3d = to_float3(normal2d.x*_cosf(height),normal2d.y*_cosf(height),_sinf(height));

        fragColor = to_float4_aw(normal3d,1.0f);
    } else {
      fragColor = to_float4_s(0.0f);   
    }
    
  #endif

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


//TRAIL
__KERNEL__ void D2DParticlFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord += 0.5f;
    
    float4 curFrame = texture(iChannel0,fragCoord/iResolution);
    float4 oldFrame = texture(iChannel1,fragCoord/iResolution);
    
    fragColor = to_float4_s(oldFrame.w*0.9f+curFrame.w*curFrame.z);
    


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Wood' to iChannel2
// Connect Image 'Previsualization: Buffer B' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1




__KERNEL__ void D2DParticlFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    fragCoord += 0.5f;
    //COMPOSITE

    const float3 lightPt = to_float3(0.5f,0.75f,0.0f);
    const float diffuseCheat = 0.85f;
    const float3 baseColor = to_float3(0.0f,1.0f,0.0f);
    const float specP = 8.0f;
    const float specA = 0.75f;
    
    float4 normalData = texture(iChannel0,fragCoord/iResolution);

    float2 tuv = fragCoord/iResolution;
    //tuv.x *= iResolution.y/iResolution.x;

    float3 color = swi3(texture(iChannel2,tuv),x,y,z);
/*
float4 Color = texture(iChannel2,tuv);
SetFragmentShaderComputedColor(Color);
return;
*/    
    if (normalData.w > 0.0f) {
        
        float3 normal = -1.0f*swi3(normalData,x,y,z);
        float3 intersectPt = to_float3_aw(fragCoord/iResolution.x,1.0f-normal.z*0.1f);
        float3 curCameraRayUnit = normalize(intersectPt);//not quite correct but whatever
        
        float3 lightGap = lightPt-intersectPt;
        float3 lightGapNorm = normalize(lightGap);
        float litAmt = dot(normal,lightGapNorm);
        litAmt = litAmt*(1.0f-diffuseCheat)+diffuseCheat;

        float lightDist = length(lightGap);
        lightDist /= 16.0f;
        lightDist = _fmaxf(lightDist,0.0f);
        lightDist = _fminf(lightDist,1.0f);
        lightDist = _powf(1.0f-lightDist,2.0f);

        float specular = _fmaxf(0.0f,dot(normalize(lightGapNorm-curCameraRayUnit),normal));

        color *= (-normal.z)*0.75f;
        color += baseColor*litAmt*lightDist + _powf(specular,specP)*specA;
        
    } else {
      color.y += (texture(iChannel1,fragCoord/iResolution).x > 0.1f) ? 0.5f : 0.0f;
    }
    
    fragColor = to_float4_aw(_fminf(to_float3_s(1.0f),color),1.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}