
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float hash(float2 p)
{
   return fract(_sinf(dot(p, to_float2(12.9898f, 78.233f))) * 43758.5453f);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// Particles idea taken from https://www.shadertoy.com/view/ll3SWs

#define A 3
#define speed 3.0f
#define size 0.02f
#define fade 0.99f

// check if there is an arriving particle at this pixel in next frame
__DEVICE__ float arrivingParticle(float2 coord, out float4 *partData, float2 iResolution, __TEXTURE2D__ iChannel0) {

    *partData = to_float4_s(0);
    float c=0.0f;

    // scan area from -D to D
    for (int i=-A; i<A; i++) {
        for (int j=-A; j<A; j++) {
            // position to check
            float2 arrCoord = coord + to_float2(i,j);
            float4 data = texture(iChannel0, arrCoord/iResolution);
            
            // no particles here
            if (dot(data,data)<0.1f) continue;

            // get next position of particle
            float2 nextCoord = swi2(data,x,y) + swi2(data,z,w);
            
            // add the particle if it's within range
            float2 offset = abs_f2(nextCoord-coord);
            if (offset.x <0.5f && offset.y <0.5f) {
                *partData += data;
        c++;
            }
        }
    }
    *partData/=c; //average pos and speeds of resulting particle
    return c;
}

__KERNEL__ void MagicPowderJipi190Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    
    CONNECT_CHECKBOX0(Tex, 0);
    CONNECT_SLIDER0(Intens, -1.0f, 3.0f, 1.0f);

    fragCoord+=0.5f;

    float t=(float)(iFrame)*0.1f;
    float2 uv = fragCoord/iResolution;
    float2 m = swi2(iMouse,x,y)/iResolution;
    float2 uvm=uv-m;
    
    //draw particles
    if (t<35.0f) uvm=0.5f-uv-to_float2(_sinf(t)*0.7f,_cosf(t)*0.6f)*0.14f*iTime;    
    
    if ((iMouse.z>0.0f||iTime<5.0f) && step(length(uvm),size)>0.0f) {
      fragColor = to_float4_f2f2(fragCoord, speed*normalize(uvm+to_float2(hash(uv+1.5465f+t), hash(uv+2.5648f+t))-0.5f)*(0.3f+hash(uv+t)*0.7f))
                                 *step(length(uvm),0.2f);
                                   
      SetFragmentShaderComputedColor(fragColor);                                    
      return;
    }


    //Umriss der Textur 
    if (Tex)
    {
      float tex = texture(iChannel1, uv).x;
      
      if (tex > 0.0f)
      {
        //fragColor = to_float4_f2f2(fragCoord, speed*normalize(uvm+to_float2(hash(uv+1.5465f+t), hash(uv+2.5648f+t))-0.5f)*(0.3f+hash(uv+t)*0.7f))
        //                         *step(length(uvm),0.2f);
                                   
        fragColor = to_float4_f2f2(fragCoord,  speed*normalize(uv+to_float2(hash(uv+1.5465f+t), hash(uv+2.5648f+t))-0.5f)*(0.3f+hash(uv+t)*0.7f))
                                               *Intens;
        
        SetFragmentShaderComputedColor(fragColor);                                    
        return;
      }
      
    }


    // get the data of a particle arriving at this pixel 
    float4 partData;
    float p = arrivingParticle(fragCoord, &partData, iResolution, iChannel0);
   
    // no particles, empty pixel
    if (p<1.0f) {
      fragColor = to_float4_s(0.0f);
            
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
    
    //swi2(partData,x,y)+=swi2(partData,z,w);
    partData.x+=partData.z;
    partData.y+=partData.w;
    
    
    //swi2(partData,z,w)*=fade;
    partData.z*=fade;
    partData.w*=fade;

    //set particle data
    fragColor = partData;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define samples 100 
#define size 0.04f
#define brightness 3.0f
#define part_color to_float3(1.0f,0.7f,0.4f)

__KERNEL__ void MagicPowderJipi190Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

float zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz;

    float c=0.0f;
    float2 uv=fragCoord/iResolution;
     for (int i=0; i<samples; i++) {
        float a=hash(uv+(float)(i)*0.123f)*6.28f;
        float l=hash(uv+(float)(i)*0.454f)*size;
        float2 smp = to_float2(_cosf(a),_sinf(a))*l;
        smp.y*=iResolution.x/iResolution.y;
        float4 part=texture(iChannel0, uv+smp);
        c+=length(swi2(part,z,w))*(size-l*0.9f)/size; 
     }
    float4 part = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float3 col=(c/(float)(samples))*part_color*brightness*5.0f;
    float2 uvc = (uv-0.5f)*2.0f;
    col*=1.0f-length(uvc*uvc*uvc);
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}