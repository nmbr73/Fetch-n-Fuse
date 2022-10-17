
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Volumetric screamer - Result of an improvised live coding session on Twitch
// LIVE SHADER CODING, SHADER SHOWDOWN STYLE, EVERY TUESDAYS 20:00 Uk time: 
// https://www.twitch.tv/evvvvil_



__DEVICE__ float smin( float d1,float d2,float k){ float h=_fmaxf(k-_fabs(d1-d2),0.0f);return _fminf(d1,d2)-h*h*0.25f/k;}
__DEVICE__ float smax( float d1,float d2,float k){ float h=_fmaxf(k-_fabs(-d1-d2),0.0f);return _fmaxf(-d1,d2)+h*h*0.25f/k;}
__DEVICE__ mat2 r2(float r){ return to_mat2(_cosf(r),_sinf(r),-_sinf(r),_cosf(r));} 
__DEVICE__ float noi(float3 p){
    float3 f=_floor(p),s=to_float3(7,157,113);
    p-=f;
    float4 h=to_float4(0,s.y,s.z,s.y+s.z)+dot(f,s);
    p=p*p*(3.0f-2.0f*p);
    h=_mix(fract_f4(sin_f4(h)*43758.5f),fract_f4(sin_f4(h+s.x)*43758.5f),p.x);
    swi2S(h,x,y, _mix(swi2(h,x,z),swi2(h,y,w),p.y));
    return _mix(h.x,h.y,p.z);  
}  
__DEVICE__ float ferlin(float3 p, float tt){ 
    float f=0.0f,A=0.5f,I;
    //swi2(p,z,y)+=tt*2.0f; 
    p.z+=tt*2.0f; 
    p.y+=tt*2.0f; 
    
    for(int i=0;i<3;i++) I=(float)(i),f+=A/(I+1.0f)*noi(p+I),p=(2.1f+0.1f*I)*p;
    return f;
} 
__DEVICE__ float face(float3 p, float3 faceP, float b){
    p-=to_float3(0,-12.0f+b*20.0f,0)+_sinf(p.y*2.0f)*0.1f;
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z),r2(1.65f*(1.0f-b))));  
    
    faceP=p*to_float3(1,0.7f,1);   
    float t=length(faceP)-4.0f-_sinf(p.y)*0.66f;
    t=smin(t,length(abs_f3(faceP+to_float3(0,-2.5f,-1))-to_float3(2,0,0))-4.0f,1.0f);
    float3 spikeP=p+to_float3(0,-3.9f,2);
    spikeP.x=_fabs(spikeP.x)-2.0f;
    swi2S(spikeP,x,y, mul_f2_mat2(swi2(spikeP,x,y),r2(-0.785f)));
    swi2S(spikeP,y,z, mul_f2_mat2(swi2(spikeP,y,z),r2(-0.1785f)));
    
    t=smin(t,length(swi2(spikeP,x,z))-2.0f+_fabs(p.x)*0.2f,1.5f);
    float3 eyeP=abs_f3(p-to_float3(0,2,0));
    swi2S(eyeP,x,y, mul_f2_mat2(swi2(eyeP,x,y),r2(0.6f)));
    
    float eyes=_fmaxf(eyeP.y,(length(abs_f3(faceP+to_float3(0,-1.5f,3.0f))-to_float3(1.0f,0,0))-3.0f));   
    t=smax(eyes,t,1.0f);   
    t=_fminf(t,_fmaxf(eyeP.y+4.0f,eyes));
    t=smax(length(faceP+to_float3(0,2,-2.0f+5.0f*b))-2.5f,t,0.5f); 
    swi2S(spikeP,x,y, mul_f2_mat2(swi2(spikeP,x,y),r2(-0.1485f)));
    
    spikeP-=to_float3(8.0f*b,-3,-1);
    t=smin(t,length(swi2(spikeP,x,z))-1.0f+_fabs(spikeP.y+3.0f)*0.25f,1.5f);
    return t;
}
__DEVICE__ float terrain(float3 p, float b, float tt){
    float t=p.y+5.0f+_cosf(length(p*(0.5f))-b*15.0f-tt*4.0f)*b+noi(p*0.07f+1.0f)*5.0f; //WOBBLE: _cosf(length(p*(0.5f))-b*15.0f-tt*4.0f)
    t=smax(length(swi2(p,x,z))-2.0f-b*6.0f,t,3.0f);
    t=smin(t,length(swi2(p,x,z))-1.0f+(p.y+15.0f-b*17.0f)*0.5f,1.5f);
    return t;
} 
__DEVICE__ float cmp( float3 p, float3 faceP, float b, float tt, float *g, float *gg) 
{  

    float t=face(p,faceP,b);  
    t=smin(t,terrain(p,b,tt),2.5f);
    float3 boltP=p;
    boltP=abs_f3(boltP-to_float3(0,0,2))-11.0f+_sinf(p.y*5.0f*p.x*0.1f+tt*25.5f)*0.05f+4.0f*_sinf(p.y*0.3f-3.0f)+p.y*0.2f;//ORIGINAL SHADER IN BONZOMATIC HAD NOISE TEXTURE CALL FOR BETTER LIGHTNING BOLT EFFECT BUT, THIS SHADER BEING GREEDY ENOUGH, I THOUGHT BEST REPLACE WITH BUNCH OF SINS ON SHADERTOY
    float bolt=length(swi2(boltP,x,z))-0.1f; //~Above line on bonzo end should be: _fabs(boltP-to_float3(0,0,2))-11.0f+texture(texNoise,swi2(p,x,y)*0.1f+tt*0.5f).r*2.0f+4.0f*_sinf(p.y*0.3f-3)+p.y*0.2f;      
    bolt=_fmaxf(bolt,p.y+10.0f-b*25.0f);
    float mouthFlash=_fmaxf(p.z,length(swi2(faceP,x,y)-to_float2(0,-2))+2.0f+p.z*0.2f*b);
    *g+=0.1f/(0.1f+bolt*bolt*(1.02f-b)*(40.0f-39.5f*_sinf(p.y*0.2f-b*8.0f)));
    *gg+=0.1f/(0.1f+mouthFlash*mouthFlash*(1.05f-b)*(40.0f-39.5f*_sinf(p.z*0.3f+tt*5.0f)));
    return t;
}  
__KERNEL__ void TwitchVolumetricScreamerFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
    float2 z;float tt,b,g=0.0f,gg=0.0f,cr;float3 faceP,cp;float4 su=to_float4_s(0);

    float2 uv=(fragCoord/iResolution-0.5f)/to_float2(iResolution.y/iResolution.x,1);
    tt=mod_f(iTime,62.82f);
    b=smoothstep(0.0f,1.0f,_sinf(tt)*0.5f+0.5f);
    float3 ro=to_float3(_sinf(tt*0.5f)*10.0f,_mix(15.0f,-3.0f,b),-20.0f+_sinf(tt*0.5f)*5.0f)*_mix(to_float3_s(1),to_float3(2,1,_cosf(tt*0.5f)*1.5f),_cosf(-tt*0.5f+0.5f)*0.5f+0.5f),
           cw=normalize(to_float3(0,b*10.0f,0)-ro), cu=normalize(cross(cw,to_float3(0,1,0))),
           cv=normalize(cross(cu,cw)), 
           rd= mul_mat3_f3(to_mat3_f3(cu,cv,cw),normalize(to_float3_aw(uv,0.5f))),
           co,fo;
    co=fo=to_float3(0.1f,0.12f,0.17f)-length(uv)*0.1f-rd.y*0.2f;    

    cr=cmp(ro-3.0f, faceP,b,tt,&g,&gg)+fract(dot(sin_f2(uv*476.567f+swi2(uv,y,x)*785.951f+tt),to_float2_s(984.156f)));
    for(int i=0;i<128;i++){
        cp=ro+rd*(cr+=1.0f/2.5f);
        if(su.w>0.99f) break; //NOTE TO SELF: cr>t NOT NEEDED AS ONLY VOLUMETRIC GEOM ARE PRESENT
        float de=clamp((-cmp(cp, faceP,b,tt,&g,&gg)*9.0f+8.0f*ferlin(cp,tt))/8.0f,0.0f,1.0f);
        su+=to_float4_aw(to_float3_s(_mix(1.0f,0.0f,de)*de),_mix(0.07f,de,_expf(-0.00001f*cr*cr*cr)))*(1.0f-su.w);//FOG ON CLOUDS! _mix(0.07f,de,_expf(-0.00001f*cr*cr*cr))
    }
    co=_mix(co,swi3(su,x,y,z),su.w);  
    fragColor = to_float4_aw(pow_f3(co+g*0.4f*to_float3(0.5f,0.2f,0.1f)+gg*0.4f*to_float3(0.1f,0.2f,0.5f),to_float3_s(0.55f)),1);


  SetFragmentShaderComputedColor(fragColor);
}