
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



// hash without sine
// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float2 hash22(float2 p) {

    float3 MOD3 = to_float3(443.8975f, 397.2973f, 491.1871f);
    float3 p3 = fract_f3((swi3(p,x,y,x)) * MOD3);
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract_f2(to_float2((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y));
}

__KERNEL__ void P3DriftsJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel3)
{

    CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f;


    const float MaxParticleSpeed = 0.2f;

    float2 res = iResolution;
    float2 ps = 1.0f / res;
    float2 uv = fragCoord / res;
    
    float4 buf[9];
    buf[0] = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    buf[1] = texture(iChannel0, fract_f2(uv-to_float2(ps.x, 0.0f)));
    buf[2] = texture(iChannel0, fract_f2(uv-to_float2(-ps.x, 0.0f)));
    buf[3] = texture(iChannel0, fract_f2(uv-to_float2(0.0f, ps.y)));
    buf[4] = texture(iChannel0, fract_f2(uv-to_float2(0.0f, -ps.y)));
    buf[5] = texture(iChannel0, fract_f2(uv-to_float2(ps.x, ps.y)));
    buf[6] = texture(iChannel0, fract_f2(uv-to_float2(-ps.x, ps.y)));
    buf[7] = texture(iChannel0, fract_f2(uv-to_float2(ps.x, -ps.y)));
    buf[8] = texture(iChannel0, fract_f2(uv-to_float2(-ps.x, -ps.y)));
    
    // this cell's particle direction & position, if any
    float2 pDir = swi2(buf[0],x,y);//.rg;
    float2 pPos = swi2(buf[0],z,w);//.ba;
    
    // update this cell's particle position
    pPos = mod_f2f2(pPos+pDir, res);

    float ct = 0.0f;
    float2 pDirAdd = to_float2_s(0.0f);
    float2 pPosAdd = to_float2_s(0.0f);
    float2 pOffs = to_float2_s(0.0f);
    if(true){//length(pDir)==0.0f) {
        for(int i=1; i<9; i++) {
            float2 pPosI = mod_f2f2(swi2(buf[i],z,w)+swi2(buf[i],x,y), res);
            
            // add up incoming particles
            //if(_floor(pPosI)==_floor(fragCoord)){// || (length(buf[i].rg)>0.&&hash12(mod_f(uv+iTime/100.0f-8.0f, 100.0f))>0.9125f)) {
            if(_floor(pPosI.x)==_floor(fragCoord.x) && _floor(pPosI.y)==_floor(fragCoord.y)){// || (length(buf[i].rg)>0.&&hash12(mod_f(uv+iTime/100.0f-8.0f, 100.0f))>0.9125f)) {
                pDirAdd += swi2(buf[i],x,y);//.rg;
                pPosAdd += pPosI;
                ct ++;
            }
            
            // slow down & 'bounce' particle when near a neighbor
            if(distance_f2(pPos, pPosI)<1.5f) {
                pDir *= 0.5f;
                pOffs -= 0.05f*(pPosI-pPos)/(1.0f+distance_f2(pPos, pPosI));
                //pOffs -= 0.5f*(pPosI-pPos)/(1.0f+distance(pPos, pPosI))*(length(buf[i].rg));
                pOffs += 0.5f*swi2(buf[i],x,y);//.rg;
            }   
        }

        // if particles were added up, average and transfer them to the current cell
        if(ct>0.0f) {
            pDir = (pDirAdd / ct);
            pPos = pPosAdd / ct;
        }
        
        // apply 'bounce'
        pDir += pOffs;
        
        // clear cell of data when particle leaves it
        //if(_floor(pPos)!=_floor(fragCoord)) {
        if(_floor(pPos.x)!=_floor(fragCoord.x) || _floor(pPos.y)!=_floor(fragCoord.y)) {
            pDir = to_float2_s(0.0f);
            pPos = to_float2_s(0.0f);
        }
        
        // make sure particle doesn't travel too fast
        if(length(pDir)>MaxParticleSpeed)
            pDir = MaxParticleSpeed*normalize(pDir);
    }
    
    // fill field with noise
    //if(iFrame==0 || texture(iChannel3, to_float2(82.5f/256.0f, 0.0f)).x>0.0f) {
    if(iFrame==0 || Reset) {
        pDir = 0.02f * (0.5f-hash22(mod_f2f2(uv-iTime/100.0f, to_float2_s(100.0f))));
        pPos = fragCoord;
  }   
    
  fragColor = to_float4_f2f2(pDir, pPos);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0




__KERNEL__ void P3DriftsJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    float magAmt = 8.0f;  // zoom level

    float2 res = iResolution;
    //vec2 uv = fragCoord / res;
    //float2 uv = (fragCoord + to_float2_s(0.01f)) / res;  // führt zu fehlerhaftem Bildaufbau
    float2 uv = (fragCoord + to_float2_s(0.0f)) / res;
    float2 p = fragCoord / res.y;
    float2 ps = 1.0f / res;
    
    // mouse center
    float mc = length(p-swi2(iMouse,x,y)/res.y);

    // apply zoom
    if(iMouse.z>0.0f)
        uv = uv/magAmt-swi2(iMouse,x,y)/res*(1.0f-magAmt)/magAmt;
    
    // corrective measure
    //uv += 0.5f/res;
    
    // combine circles from neighboring particles
    float f = 1.0f;
    for(float _y=-1.0f; _y<=1.0f; _y+=1.0f) {
        for(float _x=-1.0f; _x<=1.0f; _x+=1.0f) {
            float2 pos = swi2(texture(iChannel0, (uv-to_float2(_x, _y)/res)),z,w);//.ba;
            float c = _fmaxf(0.0f, 0.5f*res.x*length((uv-pos/res)/to_float2(ps.x/ps.y, 1.0f)));
            f = _fminf(f, c);
        }
    }
    
    fragColor = to_float4_aw(_mix(to_float3(0.0f, 0.25f, 0.0f), to_float3_s(1.0f), f), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}